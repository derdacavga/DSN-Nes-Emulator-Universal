#include <string.h>
#include "noftypes.h"
#include "nes_mmc.h"
#include "nes_ppu.h"
#include "libsnss.h"

static uint8 latch[2];
static uint8 regs[4];
 
static void mmc9_latchfunc(uint32 address, uint8 value)
{
   if (0xFD == value || 0xFE == value)
   {
      int reg;

      if (address)
      {
         latch[1] = value;
         reg = 2 + (value - 0xFD);
      }
      else
      {
         latch[0] = value;
         reg = value - 0xFD;
      }

      mmc_bankvrom(4, address, regs[reg]);
   }
}
 
static void map9_write(uint32 address, uint8 value)
{
   switch ((address & 0xF000) >> 12)
   {
   case 0xA:
      mmc_bankrom(8, 0x8000, value);
      break;

   case 0xB:
      regs[0] = value;
      if (0xFD == latch[0])
         mmc_bankvrom(4, 0x0000, value);
      break;

   case 0xC:
      regs[1] = value;
      if (0xFE == latch[0])
         mmc_bankvrom(4, 0x0000, value);
      break;

   case 0xD:
      regs[2] = value;
      if (0xFD == latch[1])
         mmc_bankvrom(4, 0x1000, value);
      break;

   case 0xE:
      regs[3] = value;
      if (0xFE == latch[1])
         mmc_bankvrom(4, 0x1000, value);
      break;

   case 0xF:
      if (value & 1)
         ppu_mirror(0, 0, 1, 1); 
      else
         ppu_mirror(0, 1, 0, 1);  
      break;

   default:
      break;
   }
}

static void map9_init(void)
{
   memset(regs, 0, sizeof(regs));

   mmc_bankrom(8, 0x8000, 0);
   mmc_bankrom(8, 0xA000, (mmc_getinfo()->rom_banks * 2) - 3);
   mmc_bankrom(8, 0xC000, (mmc_getinfo()->rom_banks * 2) - 2);
   mmc_bankrom(8, 0xE000, (mmc_getinfo()->rom_banks * 2) - 1);

   latch[0] = 0xFE;
   latch[1] = 0xFE;

   ppu_setlatchfunc(mmc9_latchfunc);
}

static void map9_getstate(SnssMapperBlock *state)
{
   state->extraData.mapper9.latch[0] = latch[0];
   state->extraData.mapper9.latch[1] = latch[1];
   state->extraData.mapper9.lastB000Write = regs[0];
   state->extraData.mapper9.lastC000Write = regs[1];
   state->extraData.mapper9.lastD000Write = regs[2];
   state->extraData.mapper9.lastE000Write = regs[3];
}

static void map9_setstate(SnssMapperBlock *state)
{
   latch[0] = state->extraData.mapper9.latch[0];
   latch[1] = state->extraData.mapper9.latch[1];
   regs[0] = state->extraData.mapper9.lastB000Write;
   regs[1] = state->extraData.mapper9.lastC000Write;
   regs[2] = state->extraData.mapper9.lastD000Write;
   regs[3] = state->extraData.mapper9.lastE000Write;
}

static map_memwrite map9_memwrite[] =
    {
        {0x8000, 0xFFFF, map9_write},
        {-1, -1, NULL}};

mapintf_t map9_intf =
    {
        9,             /* mapper number */
        "MMC2",        /* mapper name */
        map9_init,     /* init routine */
        NULL,          /* vblank callback */
        NULL,          /* hblank callback */
        map9_getstate, /* get state (snss) */
        map9_setstate, /* set state (snss) */
        NULL,          /* memory read structure */
        map9_memwrite, /* memory write structure */
        NULL           /* external sound device */
};