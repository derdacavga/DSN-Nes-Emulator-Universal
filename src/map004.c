#include "noftypes.h"
#include "nes_mmc.h"
#include "nes.h"
#include "libsnss.h"

static struct
{
   int counter, latch;
   bool enabled, reset;
} irq;

static uint8 reg;
static uint8 command;
static uint16 vrombase;
 
static void map4_write(uint32 address, uint8 value)
{
   switch (address & 0xE001)
   {
   case 0x8000:
      command = value;
      vrombase = (command & 0x80) ? 0x1000 : 0x0000;

      if (reg != (value & 0x40))
      {
         if (value & 0x40)
            mmc_bankrom(8, 0x8000, (mmc_getinfo()->rom_banks * 2) - 2);
         else
            mmc_bankrom(8, 0xC000, (mmc_getinfo()->rom_banks * 2) - 2);
      }
      reg = value & 0x40;
      break;

   case 0x8001:
      switch (command & 0x07)
      {
      case 0:
         value &= 0xFE;
         mmc_bankvrom(1, vrombase ^ 0x0000, value);
         mmc_bankvrom(1, vrombase ^ 0x0400, value + 1);
         break;

      case 1:
         value &= 0xFE;
         mmc_bankvrom(1, vrombase ^ 0x0800, value);
         mmc_bankvrom(1, vrombase ^ 0x0C00, value + 1);
         break;

      case 2:
         mmc_bankvrom(1, vrombase ^ 0x1000, value);
         break;

      case 3:
         mmc_bankvrom(1, vrombase ^ 0x1400, value);
         break;

      case 4:
         mmc_bankvrom(1, vrombase ^ 0x1800, value);
         break;

      case 5:
         mmc_bankvrom(1, vrombase ^ 0x1C00, value);
         break;

      case 6:
         mmc_bankrom(8, (command & 0x40) ? 0xC000 : 0x8000, value);
         break;

      case 7:
         mmc_bankrom(8, 0xA000, value);
         break;
      }
      break;

   case 0xA000: 
      if (0 == (mmc_getinfo()->flags & ROM_FLAG_FOURSCREEN))
      {
         if (value & 1)
            ppu_mirror(0, 0, 1, 1);  
         else
            ppu_mirror(0, 1, 0, 1);  
      }
      break;

   case 0xA001: 
      break;

   case 0xC000:
      irq.latch = value;
      //      if (irq.reset)
      //         irq.counter = irq.latch;
      break;

   case 0xC001:
      irq.reset = true;
      irq.counter = irq.latch;
      break;

   case 0xE000:
      irq.enabled = false;
      //      if (irq.reset)
      //         irq.counter = irq.latch;
      break;

   case 0xE001:
      irq.enabled = true;
      //      if (irq.reset)
      //         irq.counter = irq.latch;
      break;

   default:
      __asm__("nop");
      __asm__("nop");
      __asm__("nop");
      __asm__("nop");
      break;
   }

   if (true == irq.reset)
      irq.counter = irq.latch;
}

static void map4_hblank(int vblank)
{
   if (vblank)
      return;

   if (ppu_enabled())
   {
      if (irq.counter >= 0)
      {
         irq.reset = false;
         irq.counter--;

         if (irq.counter < 0)
         {
            if (irq.enabled)
            {
               irq.reset = true;
               nes_irq();
            }
         }
      }
   }
}

static void map4_getstate(SnssMapperBlock *state)
{
   state->extraData.mapper4.irqCounter = irq.counter;
   state->extraData.mapper4.irqLatchCounter = irq.latch;
   state->extraData.mapper4.irqCounterEnabled = irq.enabled;
   state->extraData.mapper4.last8000Write = command;
}

static void map4_setstate(SnssMapperBlock *state)
{
   irq.counter = state->extraData.mapper4.irqCounter;
   irq.latch = state->extraData.mapper4.irqLatchCounter;
   irq.enabled = state->extraData.mapper4.irqCounterEnabled;
   command = state->extraData.mapper4.last8000Write;
}

static void map4_init(void)
{
   irq.counter = irq.latch = 0;
   irq.enabled = irq.reset = false;
   reg = command = 0;
   vrombase = 0x0000;
}

static map_memwrite map4_memwrite[] =
    {
        {0x8000, 0xFFFF, map4_write},
        {-1, -1, NULL}};

mapintf_t map4_intf =
    {
        4,             /* mapper number */
        "MMC3",        /* mapper name */
        map4_init,     /* init routine */
        NULL,          /* vblank callback */
        map4_hblank,   /* hblank callback */
        map4_getstate, /* get state (snss) */
        map4_setstate, /* set state (snss) */
        NULL,          /* memory read structure */
        map4_memwrite, /* memory write structure */
        NULL           /* external sound device */
};