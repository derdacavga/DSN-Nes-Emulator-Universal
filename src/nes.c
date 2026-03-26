#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <noftypes.h>
#include "nes6502.h"
#include <log.h>
#include <osd.h>
#include <gui.h>
#include <nes.h>
#include <nes_apu.h>
#include <nes_ppu.h>
#include <nes_rom.h>
#include <nes_mmc.h>
#include <vid_drv.h>
#include <nofrendo.h>

#pragma GCC optimize("O3")
#pragma GCC optimize("unroll-loops")


#define  NES_CLOCK_DIVIDER    12
//#define  NES_MASTER_CLOCK     21477272.727272727272
#define  NES_MASTER_CLOCK     (236250000 / 11)
#define  NES_SCANLINE_CYCLES  (1364.0 / NES_CLOCK_DIVIDER)
#define  NES_FIQ_PERIOD       (NES_MASTER_CLOCK / NES_CLOCK_DIVIDER / 60)

#define  NES_RAMSIZE          0x800

#define  NES_SKIP_LIMIT       (NES_REFRESH_RATE / 5)  

static nes_t nes;
 
int nes_isourfile(const char *filename)
{
   return rom_checkmagic(filename);
}
 
nes_t *nes_getcontextptr(void)
{
   return &nes;
}

void nes_getcontext(nes_t *machine)
{
   apu_getcontext(nes.apu);
   ppu_getcontext(nes.ppu);
   nes6502_getcontext(nes.cpu);
   mmc_getcontext(nes.mmc);

   *machine = nes;
}

void nes_setcontext(nes_t *machine)
{
   ASSERT(machine);

   apu_setcontext(machine->apu);
   ppu_setcontext(machine->ppu);
   nes6502_setcontext(machine->cpu);
   mmc_setcontext(machine->mmc);

   nes = *machine;
}

static uint8 ram_read(uint32 address)
{
   return nes.cpu->mem_page[0][address & (NES_RAMSIZE - 1)];
}

static void ram_write(uint32 address, uint8 value)
{
   nes.cpu->mem_page[0][address & (NES_RAMSIZE - 1)] = value;
}

static void write_protect(uint32 address, uint8 value)
{ 
   UNUSED(address);
   UNUSED(value);
}

static uint8 read_protect(uint32 address)
{ 
   UNUSED(address); 
   return 0xFF;
}

#define  LAST_MEMORY_HANDLER  { -1, -1, NULL } 
static nes6502_memread default_readhandler[] =
{
   { 0x0800, 0x1FFF, ram_read },
   { 0x2000, 0x3FFF, ppu_read },
   { 0x4000, 0x4015, apu_read },
   { 0x4016, 0x4017, ppu_readhigh },
   LAST_MEMORY_HANDLER
};

static nes6502_memwrite default_writehandler[] =
{
   { 0x0800, 0x1FFF, ram_write },
   { 0x2000, 0x3FFF, ppu_write },
   { 0x4000, 0x4013, apu_write },
   { 0x4015, 0x4015, apu_write },
   { 0x4014, 0x4017, ppu_writehigh },
   LAST_MEMORY_HANDLER
};
 
static void build_address_handlers(nes_t *machine)
{
   int count, num_handlers = 0;
   mapintf_t *intf;
   
   ASSERT(machine);
   intf = machine->mmc->intf;

   memset(machine->readhandler, 0, sizeof(machine->readhandler));
   memset(machine->writehandler, 0, sizeof(machine->writehandler));

   for (count = 0; num_handlers < MAX_MEM_HANDLERS; count++, num_handlers++)
   {
      if (NULL == default_readhandler[count].read_func)
         break;

      memcpy(&machine->readhandler[num_handlers], &default_readhandler[count],
             sizeof(nes6502_memread));
   }

   if (intf->sound_ext)
   {
      if (NULL != intf->sound_ext->mem_read)
      {
         for (count = 0; num_handlers < MAX_MEM_HANDLERS; count++, num_handlers++)
         {
            if (NULL == intf->sound_ext->mem_read[count].read_func)
               break;

            memcpy(&machine->readhandler[num_handlers], &intf->sound_ext->mem_read[count],
                   sizeof(nes6502_memread));
         }
      }
   }

   if (NULL != intf->mem_read)
   {
      for (count = 0; num_handlers < MAX_MEM_HANDLERS; count++, num_handlers++)
      {
         if (NULL == intf->mem_read[count].read_func)
            break;

         memcpy(&machine->readhandler[num_handlers], &intf->mem_read[count],
                sizeof(nes6502_memread));
      }
   }
 
   machine->readhandler[num_handlers].min_range = 0x4018;
   machine->readhandler[num_handlers].max_range = 0x5FFF;
   machine->readhandler[num_handlers].read_func = read_protect;
   num_handlers++;
   machine->readhandler[num_handlers].min_range = -1;
   machine->readhandler[num_handlers].max_range = -1;
   machine->readhandler[num_handlers].read_func = NULL;
   num_handlers++;
   ASSERT(num_handlers <= MAX_MEM_HANDLERS);

   num_handlers = 0;

   for (count = 0; num_handlers < MAX_MEM_HANDLERS; count++, num_handlers++)
   {
      if (NULL == default_writehandler[count].write_func)
         break;

      memcpy(&machine->writehandler[num_handlers], &default_writehandler[count],
             sizeof(nes6502_memwrite));
   }

   if (intf->sound_ext)
   {
      if (NULL != intf->sound_ext->mem_write)
      {
         for (count = 0; num_handlers < MAX_MEM_HANDLERS; count++, num_handlers++)
         {
            if (NULL == intf->sound_ext->mem_write[count].write_func)
               break;

            memcpy(&machine->writehandler[num_handlers], &intf->sound_ext->mem_write[count],
                   sizeof(nes6502_memwrite));
         }
      }
   }

   if (NULL != intf->mem_write)
   {
      for (count = 0; num_handlers < MAX_MEM_HANDLERS; count++, num_handlers++)
      {
         if (NULL == intf->mem_write[count].write_func)
            break;

         memcpy(&machine->writehandler[num_handlers], &intf->mem_write[count],
                sizeof(nes6502_memwrite));
      }
   }
 
   machine->writehandler[num_handlers].min_range = 0x4018;
   machine->writehandler[num_handlers].max_range = 0x5FFF;
   machine->writehandler[num_handlers].write_func = write_protect;
   num_handlers++;
   machine->writehandler[num_handlers].min_range = 0x8000;
   machine->writehandler[num_handlers].max_range = 0xFFFF;
   machine->writehandler[num_handlers].write_func = write_protect;
   num_handlers++;
   machine->writehandler[num_handlers].min_range = -1;
   machine->writehandler[num_handlers].max_range = -1;
   machine->writehandler[num_handlers].write_func = NULL;
   num_handlers++;
   ASSERT(num_handlers <= MAX_MEM_HANDLERS);
}
 
void nes_irq(void)
{
#ifdef NOFRENDO_DEBUG
//   if (nes.scanline <= NES_SCREEN_HEIGHT)
//      memset(nes.vidbuf->line[nes.scanline - 1], GUI_RED, NES_SCREEN_WIDTH);
#endif 

   nes6502_irq();
}

static uint8 nes_clearfiq(void)
{
   if (nes.fiq_occurred)
   {
      nes.fiq_occurred = false;
      return 0x40;
   }

   return 0;
}

void nes_setfiq(uint8 value)
{
   nes.fiq_state = value;
   nes.fiq_cycles = (int) NES_FIQ_PERIOD;
}

static void nes_checkfiq(int cycles)
{
   nes.fiq_cycles -= cycles;
   if (nes.fiq_cycles <= 0)
   {
      nes.fiq_cycles += (int) NES_FIQ_PERIOD;
      if (0 == (nes.fiq_state & 0xC0))
      {
         nes.fiq_occurred = true;
         nes6502_irq();
      }
   }
}

void nes_nmi(void)
{
   nes6502_nmi();
}

static void nes_renderframe(bool draw_flag)
{
   int elapsed_cycles;
   mapintf_t *mapintf = nes.mmc->intf;
   int in_vblank = 0;

   while (262 != nes.scanline)
   {
//      ppu_scanline(nes.vidbuf, nes.scanline, draw_flag);
		ppu_scanline(vid_getbuffer(), nes.scanline, draw_flag);

      if (241 == nes.scanline)
      { 
         elapsed_cycles = nes6502_execute(7);
         nes.scanline_cycles -= elapsed_cycles;
         nes_checkfiq(elapsed_cycles);

         ppu_checknmi();

         if (mapintf->vblank)
            mapintf->vblank();
         in_vblank = 1;
      } 

      if (mapintf->hblank)
         mapintf->hblank(in_vblank);

      nes.scanline_cycles += (float) NES_SCANLINE_CYCLES;
      elapsed_cycles = nes6502_execute((int) nes.scanline_cycles);
      nes.scanline_cycles -= (float) elapsed_cycles;
      nes_checkfiq(elapsed_cycles);

      ppu_endscanline(nes.scanline);
      nes.scanline++;
   }

   nes.scanline = 0;
}

static void system_video(bool draw)
{ 
   if (false == draw)
   {
      gui_frame(false);
      return;
   }
 
//   vid_blit(nes.vidbuf, 0, (NES_SCREEN_HEIGHT - NES_VISIBLE_HEIGHT) / 2,
//            0, 0, NES_SCREEN_WIDTH, NES_VISIBLE_HEIGHT);
 
   gui_frame(true); 
   vid_flush(); 
   osd_getinput();
}
 
void nes_emulate(void)
{
   int last_ticks, frames_to_render;

   osd_setsound(nes.apu->process);

   last_ticks = nofrendo_ticks;
   frames_to_render = 0;
   nes.scanline_cycles = 0;
   nes.fiq_cycles = (int) NES_FIQ_PERIOD;

   while (false == nes.poweroff)
   {
      if (nofrendo_ticks != last_ticks)
      {
         int tick_diff = nofrendo_ticks - last_ticks;

         frames_to_render += tick_diff;
         gui_tick(tick_diff);
         last_ticks = nofrendo_ticks;
      }

      if (true == nes.pause)
      { 
         system_video(true);
         frames_to_render = 0;
      }
      else if (frames_to_render > 1)
      {
         frames_to_render--;
         nes_renderframe(false);
         system_video(false);
      }
      else if ((1 == frames_to_render && true == nes.autoframeskip)
               || false == nes.autoframeskip)
      {
         frames_to_render = 0;
         nes_renderframe(true);
         system_video(true);
      }
   }
}

static void mem_trash(uint8 *buffer, int length)
{
   int i;

   for (i = 0; i < length; i++)
      buffer[i] = (uint8) rand();
}
 
void nes_reset(int reset_type)
{
   if (HARD_RESET == reset_type)
   {
      memset(nes.cpu->mem_page[0], 0, NES_RAMSIZE);
      if (nes.rominfo->vram)
         mem_trash(nes.rominfo->vram, 0x2000 * nes.rominfo->vram_banks);
   }

   apu_reset();
   ppu_reset(reset_type);
   mmc_reset();
   nes6502_reset();

   nes.scanline = 241;

   gui_sendmsg(GUI_GREEN, "NES %s", 
               (HARD_RESET == reset_type) ? "powered on" : "reset");
}

void nes_destroy(nes_t **machine)
{
   if (*machine)
   {
      rom_free(&(*machine)->rominfo);
      mmc_destroy(&(*machine)->mmc);
      ppu_destroy(&(*machine)->ppu);
      apu_destroy(&(*machine)->apu);
//      bmp_destroy(&(*machine)->vidbuf);
      if ((*machine)->cpu)
      {
         if ((*machine)->cpu->mem_page[0])
            free((*machine)->cpu->mem_page[0]);
         free((*machine)->cpu);
      }

      free(*machine);
      *machine = NULL;
   }
}

void nes_poweroff(void)
{
   nes.poweroff = true;
}

void nes_togglepause(void)
{
   nes.pause ^= true;
}
 
int nes_insertcart(const char *filename, nes_t *machine)
{
   nes6502_setcontext(machine->cpu);
 
   machine->rominfo = rom_load(filename);
   if (NULL == machine->rominfo)
      goto _fail;
 
   if (machine->rominfo->sram)
   {
      machine->cpu->mem_page[6] = machine->rominfo->sram;
      machine->cpu->mem_page[7] = machine->rominfo->sram + 0x1000;
   }
 
   machine->mmc = mmc_create(machine->rominfo);
   if (NULL == machine->mmc)
      goto _fail;
 
   if (NULL != machine->rominfo->vram)
      machine->ppu->vram_present = true;
   
   apu_setext(machine->apu, machine->mmc->intf->sound_ext);
   
   build_address_handlers(machine);

   nes_setcontext(machine);

   nes_reset(HARD_RESET);
   return 0;

_fail:
   nes_destroy(&machine);
   return -1;
}
 
nes_t *nes_create(void)
{
   nes_t *machine;
   sndinfo_t osd_sound;
   int i;

   machine = malloc(sizeof(nes_t));
   if (NULL == machine)
      return NULL;

   memset(machine, 0, sizeof(nes_t));
 
//   machine->vidbuf = bmp_create(NES_SCREEN_WIDTH, NES_SCREEN_HEIGHT, 8);
//   if (NULL == machine->vidbuf)
//      goto _fail;

   machine->autoframeskip = true; 
   machine->cpu = malloc(sizeof(nes6502_context));
   if (NULL == machine->cpu)
      goto _fail;

   memset(machine->cpu, 0, sizeof(nes6502_context));
    
   machine->cpu->mem_page[0] = malloc(NES_RAMSIZE);
   if (NULL == machine->cpu->mem_page[0])
      goto _fail; 
   for (i = 1; i < NES6502_NUMBANKS; i++)
      machine->cpu->mem_page[i] = NULL;

   machine->cpu->read_handler = machine->readhandler;
   machine->cpu->write_handler = machine->writehandler;
 
   osd_getsoundinfo(&osd_sound);
   machine->apu = apu_create(0, osd_sound.sample_rate, NES_REFRESH_RATE, osd_sound.bps);

   if (NULL == machine->apu)
      goto _fail;
 
   machine->apu->irq_callback = nes_irq;
   machine->apu->irqclear_callback = nes_clearfiq;
 
   machine->ppu = ppu_create();
   if (NULL == machine->ppu)
      goto _fail;

   machine->poweroff = false;
   machine->pause = false;

   return machine;

_fail:
   nes_destroy(&machine);
   return NULL;
}