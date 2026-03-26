#include "noftypes.h"
#include "nes_mmc.h"
#include "nes_ppu.h"
#include "nes.h"

static struct
{
   bool enabled, expired;
   int counter;
   int latch_c005, latch_c003;
} irq;

static void map160_write(uint32 address, uint8 value)
{
   if (address >= 0x8000 && address <= 0x8003)
   {
      mmc_bankrom(8, 0x8000 + 0x2000 * (address & 3), value);
   }
   else if (address >= 0x9000 && address <= 0x9007)
   {
      mmc_bankvrom(1, 0x400 * (address & 7), value);
   }
   else if (0xC002 == address)
   {
      irq.enabled = false;
      irq.latch_c005 = irq.latch_c003;
   }
   else if (0xC003 == address)
   {
      if (false == irq.expired)
      {
         irq.counter = value;
      }
      else
      {
         irq.expired = false;
         irq.enabled = true;
         irq.counter = irq.latch_c005;
      }
   }
   else if (0xC005 == address)
   {
      irq.latch_c005 = value;
      irq.counter = value;
   }
#ifdef NOFRENDO_DEBUG
   else
   {
      nofrendo_log_printf("mapper 160: untrapped write $%02X to $%04X\n", value, address);
   }
#endif 
}

static void map160_hblank(int vblank)
{
   if (!vblank)
   {
      if (ppu_enabled() && irq.enabled)
      {
         if (0 == irq.counter && false == irq.expired)
         {
            irq.expired = true;
            nes_irq();
         }
         else
         {
            irq.counter--;
         }
      }
   }
}

static void map160_init(void)
{
   irq.enabled = false;
   irq.expired = false;
   irq.counter = 0;
   irq.latch_c003 = irq.latch_c005 = 0;
}

static map_memwrite map160_memwrite[] =
    {
        {0x8000, 0xFFFF, map160_write},
        {-1, -1, NULL}};

mapintf_t map160_intf =
    {
        160,                /* mapper number */
        "Aladdin (pirate)", /* mapper name */
        map160_init,        /* init routine */
        NULL,               /* vblank callback */
        map160_hblank,      /* hblank callback */
        NULL,               /* get state (snss) */
        NULL,               /* set state (snss) */
        NULL,               /* memory read structure */
        map160_memwrite,    /* memory write structure */
        NULL                /* external sound device */
};