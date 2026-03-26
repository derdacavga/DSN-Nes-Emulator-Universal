#include <stdio.h>
#include <string.h>
#include <noftypes.h>
#include <nesstate.h>
#include <gui.h>
#include <nes.h>
#include <log.h>
#include <osd.h>
#include <libsnss.h>
#include "nes6502.h"

#define  FIRST_STATE_SLOT  0
#define  LAST_STATE_SLOT   9

static int state_slot = FIRST_STATE_SLOT;
 
void state_setslot(int slot)
{ 
   if (state_slot != slot && slot >= FIRST_STATE_SLOT
       && slot <= LAST_STATE_SLOT)
   {
      state_slot = slot;
      gui_sendmsg(GUI_WHITE, "State slot set to %d", slot);
   }
}

static int save_baseblock(nes_t *state, SNSS_FILE *snssFile)
{
   int i;

   ASSERT(state);

   nes6502_getcontext(state->cpu);
   ppu_getcontext(state->ppu);

   snssFile->baseBlock.regA = state->cpu->a_reg;
   snssFile->baseBlock.regX = state->cpu->x_reg;
   snssFile->baseBlock.regY = state->cpu->y_reg;
   snssFile->baseBlock.regFlags = state->cpu->p_reg;
   snssFile->baseBlock.regStack = state->cpu->s_reg;
   snssFile->baseBlock.regPc = state->cpu->pc_reg;

   snssFile->baseBlock.reg2000 = state->ppu->ctrl0;
   snssFile->baseBlock.reg2001 = state->ppu->ctrl1;

   memcpy(snssFile->baseBlock.cpuRam, state->cpu->mem_page[0], 0x800);
   memcpy(snssFile->baseBlock.spriteRam, state->ppu->oam, 0x100);
   memcpy(snssFile->baseBlock.ppuRam, state->ppu->nametab, 0x1000);
 
   for (i = 0; i < 32; i++)
      snssFile->baseBlock.palette[i] = state->ppu->palette[i] & 0x3F;

   snssFile->baseBlock.mirrorState[0] = (state->ppu->page[8] + 0x2000 - state->ppu->nametab) / 0x400;
   snssFile->baseBlock.mirrorState[1] = (state->ppu->page[9] + 0x2400 - state->ppu->nametab) / 0x400;
   snssFile->baseBlock.mirrorState[2] = (state->ppu->page[10] + 0x2800 - state->ppu->nametab) / 0x400;
   snssFile->baseBlock.mirrorState[3] = (state->ppu->page[11] + 0x2C00 - state->ppu->nametab) / 0x400;

   snssFile->baseBlock.vramAddress = state->ppu->vaddr;
   snssFile->baseBlock.spriteRamAddress = state->ppu->oam_addr;
   snssFile->baseBlock.tileXOffset = state->ppu->tile_xofs;

   return 0;
}

static bool save_vramblock(nes_t *state, SNSS_FILE *snssFile)
{
   ASSERT(state);

   if (NULL == state->rominfo->vram)
      return -1;

   if (state->rominfo->vram_banks > 2)
   {
      log_printf("too many VRAM banks: %d\n", state->rominfo->vram_banks);
      return -1;
   }

   snssFile->vramBlock.vramSize = VRAM_8K * state->rominfo->vram_banks;

   memcpy(snssFile->vramBlock.vram, state->rominfo->vram, snssFile->vramBlock.vramSize);
   return 0;
}

static int save_sramblock(nes_t *state, SNSS_FILE *snssFile)
{
   int i;
   bool written = false;
   int sram_length;

   ASSERT(state);

   sram_length = state->rominfo->sram_banks * SRAM_1K;
 
   for (i = 0; i < sram_length; i++)
   {
      if (state->rominfo->sram[i])
      {
         written = true;
         break;
      }
   }

   if (false == written)
      return -1;

   if (state->rominfo->sram_banks > 8)
   {
      log_printf("Unsupported number of SRAM banks: %d\n", state->rominfo->sram_banks);
      return -1;
   }

   snssFile->sramBlock.sramSize = SRAM_1K * state->rominfo->sram_banks; 
   snssFile->sramBlock.sramEnabled = true;

   memcpy(snssFile->sramBlock.sram, state->rominfo->sram, snssFile->sramBlock.sramSize);

   return 0;
}

static int save_soundblock(nes_t *state, SNSS_FILE *snssFile)
{
   ASSERT(state);

   apu_getcontext(state->apu);

   /* rect 0 */
   snssFile->soundBlock.soundRegisters[0x00] = state->apu->rectangle[0].regs[0];
   snssFile->soundBlock.soundRegisters[0x01] = state->apu->rectangle[0].regs[1];
   snssFile->soundBlock.soundRegisters[0x02] = state->apu->rectangle[0].regs[2];
   snssFile->soundBlock.soundRegisters[0x03] = state->apu->rectangle[0].regs[3];
   /* rect 1 */
   snssFile->soundBlock.soundRegisters[0x04] = state->apu->rectangle[1].regs[0];
   snssFile->soundBlock.soundRegisters[0x05] = state->apu->rectangle[1].regs[1];
   snssFile->soundBlock.soundRegisters[0x06] = state->apu->rectangle[1].regs[2];
   snssFile->soundBlock.soundRegisters[0x07] = state->apu->rectangle[1].regs[3];
   /* triangle */
   snssFile->soundBlock.soundRegisters[0x08] = state->apu->triangle.regs[0];
   snssFile->soundBlock.soundRegisters[0x0A] = state->apu->triangle.regs[1];
   snssFile->soundBlock.soundRegisters[0x0B] = state->apu->triangle.regs[2];
   /* noise */
   snssFile->soundBlock.soundRegisters[0X0C] = state->apu->noise.regs[0];
   snssFile->soundBlock.soundRegisters[0X0E] = state->apu->noise.regs[1];
   snssFile->soundBlock.soundRegisters[0x0F] = state->apu->noise.regs[2];
   /* dmc */
   snssFile->soundBlock.soundRegisters[0x10] = state->apu->dmc.regs[0];
   snssFile->soundBlock.soundRegisters[0x11] = state->apu->dmc.regs[1];
   snssFile->soundBlock.soundRegisters[0x12] = state->apu->dmc.regs[2];
   snssFile->soundBlock.soundRegisters[0x13] = state->apu->dmc.regs[3];
   /* control */
   snssFile->soundBlock.soundRegisters[0x15] = state->apu->enable_reg;

   return 0;
}

static int save_mapperblock(nes_t *state, SNSS_FILE *snssFile)
{
   int i;
   ASSERT(state);

   mmc_getcontext(state->mmc);
 
   if (0 == state->mmc->intf->number)
      return -1;

   nes6502_getcontext(state->cpu);
 
   for (i = 0; i < 4; i++)
      snssFile->mapperBlock.prgPages[i] = (state->cpu->mem_page[(i + 4) * 2] - state->rominfo->rom) >> 13;

   if (state->rominfo->vrom_banks)
   {
      for (i = 0; i < 8; i++)
         snssFile->mapperBlock.chrPages[i] = (ppu_getpage(i) - state->rominfo->vrom + (i * 0x400)) >> 10;
   }
   else
   { 
      for (i = 0; i < 8; i++)
         snssFile->mapperBlock.chrPages[i] = i;
   }

   if (state->mmc->intf->get_state)
      state->mmc->intf->get_state(&snssFile->mapperBlock);

   return 0;
}

static void load_baseblock(nes_t *state, SNSS_FILE *snssFile)
{
   int i;
   
   ASSERT(state);

   nes6502_getcontext(state->cpu);
   ppu_getcontext(state->ppu);

   state->cpu->a_reg = snssFile->baseBlock.regA;
   state->cpu->x_reg = snssFile->baseBlock.regX;
   state->cpu->y_reg = snssFile->baseBlock.regY;
   state->cpu->p_reg = snssFile->baseBlock.regFlags;
   state->cpu->s_reg = snssFile->baseBlock.regStack;
   state->cpu->pc_reg = snssFile->baseBlock.regPc;

   state->ppu->ctrl0 = snssFile->baseBlock.reg2000;
   state->ppu->ctrl1 = snssFile->baseBlock.reg2001;

   memcpy(state->cpu->mem_page[0], snssFile->baseBlock.cpuRam, 0x800);
   memcpy(state->ppu->oam, snssFile->baseBlock.spriteRam, 0x100);
   memcpy(state->ppu->nametab, snssFile->baseBlock.ppuRam, 0x1000);
   memcpy(state->ppu->palette, snssFile->baseBlock.palette, 0x20);
 
   for (i = 0; i < 8; i++)
      state->ppu->palette[i << 2] = state->ppu->palette[0] | 0x80;//BG_TRANS_MASK;

   for (i = 0; i < 4; i++)
   {
      state->ppu->page[i + 8] = state->ppu->page[i + 12] =
         state->ppu->nametab + (snssFile->baseBlock.mirrorState[i] * 0x400) - (0x2000 + (i * 0x400));
   }

   state->ppu->vaddr = snssFile->baseBlock.vramAddress;
   state->ppu->oam_addr = snssFile->baseBlock.spriteRamAddress;
   state->ppu->tile_xofs = snssFile->baseBlock.tileXOffset; 
   state->ppu->flipflop = 0;
   state->ppu->strikeflag = false;

   nes6502_setcontext(state->cpu);
   ppu_setcontext(state->ppu);

   ppu_write(PPU_CTRL0, state->ppu->ctrl0);
   ppu_write(PPU_CTRL1, state->ppu->ctrl1);
   ppu_write(PPU_VADDR, (uint8) (state->ppu->vaddr >> 8));
   ppu_write(PPU_VADDR, (uint8) (state->ppu->vaddr & 0xFF));
}

static void load_vramblock(nes_t *state, SNSS_FILE *snssFile)
{
   ASSERT(state);

   ASSERT(snssFile->vramBlock.vramSize <= VRAM_8K);  
   memcpy(state->rominfo->vram, snssFile->vramBlock.vram, snssFile->vramBlock.vramSize);
}

static void load_sramblock(nes_t *state, SNSS_FILE *snssFile)
{
   ASSERT(state);

   ASSERT(snssFile->sramBlock.sramSize <= SRAM_8K);  
   memcpy(state->rominfo->sram, snssFile->sramBlock.sram, snssFile->sramBlock.sramSize);
}

static void load_controllerblock(nes_t *state, SNSS_FILE *snssFile)
{
   UNUSED(state);
   UNUSED(snssFile);
}

static void load_soundblock(nes_t *state, SNSS_FILE *snssFile)
{
   int i;

   ASSERT(state);

   for (i = 0; i < 0x15; i++)
   {
      if (i != 0x13)  
         apu_write(0x4000 + i, snssFile->soundBlock.soundRegisters[i]);
   }
}
 
static void load_mapperblock(nes_t *state, SNSS_FILE *snssFile)
{
   int i;
   
   ASSERT(state);

   mmc_getcontext(state->mmc);

   for (i = 0; i < 4; i++)
      mmc_bankrom(8, 0x8000 + (i * 0x2000), snssFile->mapperBlock.prgPages[i]);

   if (state->rominfo->vrom_banks)
   {
      for (i = 0; i < 8; i++)
         mmc_bankvrom(1, i * 0x400, snssFile->mapperBlock.chrPages[i]);
   }
   else
   {
      ASSERT(state->rominfo->vram);

      for (i = 0; i < 8; i++)
         ppu_setpage(1, i, state->rominfo->vram);
   }

   if (state->mmc->intf->set_state)
      state->mmc->intf->set_state(&snssFile->mapperBlock);

   mmc_setcontext(state->mmc);
}


int state_save(void)
{
   SNSS_FILE *snssFile;
   SNSS_RETURN_CODE status;
   char fn[PATH_MAX + 1], ext[5];
   nes_t *machine;
 
   machine = nes_getcontextptr();
   ASSERT(machine);
    
   strncpy(fn, machine->rominfo->filename, PATH_MAX - 4);
   
   ASSERT(state_slot >= FIRST_STATE_SLOT && state_slot <= LAST_STATE_SLOT);
   sprintf(ext, ".ss%d", state_slot);
   osd_newextension(fn, ext);
 
   status = SNSS_OpenFile(&snssFile, fn, SNSS_OPEN_WRITE);
   if (SNSS_OK != status)
      goto _error;
 
   if (0 == save_baseblock(machine, snssFile))
   {
      status = SNSS_WriteBlock(snssFile, SNSS_BASR);
      if (SNSS_OK != status)
         goto _error;
   }

   if (0 == save_vramblock(machine, snssFile))
   {
      status = SNSS_WriteBlock(snssFile, SNSS_VRAM);
      if (SNSS_OK != status)
         goto _error;
   }

   if (0 == save_sramblock(machine, snssFile))
   {
      status = SNSS_WriteBlock(snssFile, SNSS_SRAM);
      if (SNSS_OK != status)
         goto _error;
   }

   if (0 == save_soundblock(machine, snssFile))
   {
      status = SNSS_WriteBlock(snssFile, SNSS_SOUN);
      if (SNSS_OK != status)
         goto _error;
   }

   if (0 == save_mapperblock(machine, snssFile))
   {
      status = SNSS_WriteBlock(snssFile, SNSS_MPRD);
      if (SNSS_OK != status)
         goto _error;
   }
 
   status = SNSS_CloseFile(&snssFile);
   if (SNSS_OK != status)
      goto _error;

   gui_sendmsg(GUI_GREEN, "State %d saved", state_slot);
   return 0;

_error:
   gui_sendmsg(GUI_RED, "error: %s", SNSS_GetErrorString(status));
   SNSS_CloseFile(&snssFile);
   return -1;
}

int state_load(void)
{
   SNSS_FILE *snssFile;
   SNSS_RETURN_CODE status;
   SNSS_BLOCK_TYPE block_type;
   char fn[PATH_MAX + 1], ext[5];
   unsigned int i;
   nes_t *machine;
 
   machine = nes_getcontextptr();
   ASSERT(machine);
 
   strncpy(fn, machine->rominfo->filename, PATH_MAX - 4);

   ASSERT(state_slot >= FIRST_STATE_SLOT && state_slot <= LAST_STATE_SLOT);
   sprintf(ext, ".ss%d", state_slot);
   osd_newextension(fn, ext);
    
   status = SNSS_OpenFile(&snssFile, fn, SNSS_OPEN_READ);
   if (SNSS_OK != status)
      goto _error;
 
   for (i = 0; i < snssFile->headerBlock.numberOfBlocks; i++)
   {
      status = SNSS_GetNextBlockType(&block_type, snssFile);
      if (SNSS_OK != status)
         goto _error;

      status = SNSS_ReadBlock(snssFile, block_type);
      if (SNSS_OK != status)
         goto _error;

      switch (block_type)
      {
      case SNSS_BASR:
         load_baseblock(machine, snssFile);
         break;

      case SNSS_VRAM:
         load_vramblock(machine, snssFile);
         break;

      case SNSS_SRAM:
         load_sramblock(machine, snssFile);
         break;
      
      case SNSS_MPRD:
         load_mapperblock(machine, snssFile);
         break;
      
      case SNSS_CNTR:
         load_controllerblock(machine, snssFile);
         break;
      
      case SNSS_SOUN:
         load_soundblock(machine, snssFile);
         break;
      
      case SNSS_UNKNOWN_BLOCK:
      default:
         log_printf("unknown SNSS block type\n");
         break;
      }
   }
 
   status = SNSS_CloseFile(&snssFile);
   if (SNSS_OK != status)
      goto _error;

   gui_sendmsg(GUI_GREEN, "State %d restored", state_slot);

   return 0;

_error:
   gui_sendmsg(GUI_RED, "error: %s", SNSS_GetErrorString(status));
   SNSS_CloseFile(&snssFile);
   return -1;
} 