#include "noftypes.h"
#include "nes_mmc.h"
#include "nes.h"
#include "libsnss.h"
#include "log.h"

#define MAP40_IRQ_PERIOD (4096 / 113.666666)

static struct
{
   int enabled, counter;
} irq;
 
static void map40_init(void)
{
   mmc_bankrom(8, 0x6000, 6);
   mmc_bankrom(8, 0x8000, 4);
   mmc_bankrom(8, 0xA000, 5);
   mmc_bankrom(8, 0xE000, 7);

   irq.enabled = false;
   irq.counter = (int)MAP40_IRQ_PERIOD;
}

static void map40_hblank(int vblank)
{
   UNUSED(vblank);

   if (irq.enabled && irq.counter)
   {
      irq.counter--;
      if (0 == irq.counter)
      {
         nes_irq();
         irq.enabled = false;
      }
   }
}

static void map40_write(uint32 address, uint8 value)
{
   int range = (address >> 13) - 4;

   switch (range)
   {
   case 0: 
      irq.enabled = false;
      irq.counter = (int)MAP40_IRQ_PERIOD;
      break;

   case 1: 
      irq.enabled = true;
      break;

   case 3:  
      mmc_bankrom(8, 0xC000, value & 7);
      break;

   default:
      break;
   }
}

static void map40_getstate(SnssMapperBlock *state)
{
   state->extraData.mapper40.irqCounter = irq.counter;
   state->extraData.mapper40.irqCounterEnabled = irq.enabled;
}

static void map40_setstate(SnssMapperBlock *state)
{
   irq.counter = state->extraData.mapper40.irqCounter;
   irq.enabled = state->extraData.mapper40.irqCounterEnabled;
}

static map_memwrite map40_memwrite[] =
    {
        {0x8000, 0xFFFF, map40_write},
        {-1, -1, NULL}};

mapintf_t map40_intf =
    {
        40,                /* mapper number */
        "SMB 2j (pirate)", /* mapper name */
        map40_init,        /* init routine */
        NULL,              /* vblank callback */
        map40_hblank,      /* hblank callback */
        map40_getstate,    /* get state (snss) */
        map40_setstate,    /* set state (snss) */
        NULL,              /* memory read structure */
        map40_memwrite,    /* memory write structure */
        NULL               /* external sound device */
};