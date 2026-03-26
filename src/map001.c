#include <string.h>
#include "noftypes.h"
#include "nes_mmc.h"
#include "nes_ppu.h"

 
static int bitcount = 0;
static uint8 latch = 0;
static uint8 regs[4];
static int bank_select;
static uint8 lastreg;

static void map1_write(uint32 address, uint8 value)
{
   int regnum = (address >> 13) - 4;

   if (value & 0x80)
   {
      regs[0] |= 0x0C;
      bitcount = 0;
      latch = 0;
      return;
   }

   if (lastreg != regnum)
   {
      bitcount = 0;
      latch = 0;
      lastreg = regnum;
   }
   //lastreg = regnum;

   latch |= ((value & 1) << bitcount++);
 
   if (5 != bitcount)
      return;

   regs[regnum] = latch;
   value = latch;
   bitcount = 0;
   latch = 0;

   switch (regnum)
   {
   case 0:
   {
      if (0 == (value & 2))
      {
         int mirror = value & 1;
         ppu_mirror(mirror, mirror, mirror, mirror);
      }
      else
      {
         if (value & 1)
            ppu_mirror(0, 0, 1, 1);
         else
            ppu_mirror(0, 1, 0, 1);
      }
   }
   break;

   case 1:
      if (regs[0] & 0x10)
         mmc_bankvrom(4, 0x0000, value);
      else
         mmc_bankvrom(8, 0x0000, value >> 1);
      break;

   case 2:
      if (regs[0] & 0x10)
         mmc_bankvrom(4, 0x1000, value);
      break;

   case 3:
      if (mmc_getinfo()->rom_banks == 0x20)
      {
         bank_select = (regs[1] & 0x10) ? 0 : 0x10;
      }
      else if (mmc_getinfo()->rom_banks == 0x40)
      {
         if (regs[0] & 0x10)
            bank_select = (regs[1] & 0x10) | ((regs[2] & 0x10) << 1);
         else
            bank_select = (regs[1] & 0x10) << 1;
      }
      else
      {
         bank_select = 0;
      }

      if (0 == (regs[0] & 0x08))
         mmc_bankrom(32, 0x8000, ((regs[3] >> 1) + (bank_select >> 1)));
      else if (regs[0] & 0x04)
         mmc_bankrom(16, 0x8000, ((regs[3] & 0xF) + bank_select));
      else
         mmc_bankrom(16, 0xC000, ((regs[3] & 0xF) + bank_select));

   default:
      break;
   }
}

static void map1_init(void)
{
   bitcount = 0;
   latch = 0;

   memset(regs, 0, sizeof(regs));

   if (mmc_getinfo()->rom_banks == 0x20)
      mmc_bankrom(16, 0xC000, 0x0F);

   map1_write(0x8000, 0x80);
}

static void map1_getstate(SnssMapperBlock *state)
{
   state->extraData.mapper1.registers[0] = regs[0];
   state->extraData.mapper1.registers[1] = regs[1];
   state->extraData.mapper1.registers[2] = regs[2];
   state->extraData.mapper1.registers[3] = regs[3];
   state->extraData.mapper1.latch = latch;
   state->extraData.mapper1.numberOfBits = bitcount;
}

static void map1_setstate(SnssMapperBlock *state)
{
   regs[1] = state->extraData.mapper1.registers[0];
   regs[1] = state->extraData.mapper1.registers[1];
   regs[2] = state->extraData.mapper1.registers[2];
   regs[3] = state->extraData.mapper1.registers[3];
   latch = state->extraData.mapper1.latch;
   bitcount = state->extraData.mapper1.numberOfBits;
}

static map_memwrite map1_memwrite[] =
    {
        {0x8000, 0xFFFF, map1_write},
        {-1, -1, NULL}};

mapintf_t map1_intf =
    {
        1,             /* mapper number */
        "MMC1",        /* mapper name */
        map1_init,     /* init routine */
        NULL,          /* vblank callback */
        NULL,          /* hblank callback */
        map1_getstate, /* get state (snss) */
        map1_setstate, /* set state (snss) */
        NULL,          /* memory read structure */
        map1_memwrite, /* memory write structure */
        NULL           /* external sound device */
};