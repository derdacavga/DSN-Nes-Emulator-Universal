#include "noftypes.h"
#include "nes_mmc.h"
#include "nes.h"
#include "log.h"

static struct
{
   int counter, latch;
   bool enabled, reset;
} irq;

static uint8 command = 0;
static uint16 vrombase = 0x0000;

static void map64_hblank(int vblank)
{
   if (vblank)
      return;

   irq.reset = false;

   if (ppu_enabled())
   {
      if (0 == irq.counter--)
      {
         irq.counter = irq.latch;

         if (true == irq.enabled)
            nes_irq();

         irq.reset = true;
      }
   }
}
 
static void map64_write(uint32 address, uint8 value)
{
   switch (address & 0xE001)
   {
   case 0x8000:
      command = value;
      vrombase = (value & 0x80) ? 0x1000 : 0x0000;
      break;

   case 0x8001:
      switch (command & 0xF)
      {
      case 0:
         mmc_bankvrom(1, 0x0000 ^ vrombase, value);
         mmc_bankvrom(1, 0x0400 ^ vrombase, value);
         break;

      case 1:
         mmc_bankvrom(1, 0x0800 ^ vrombase, value);
         mmc_bankvrom(1, 0x0C00 ^ vrombase, value);
         break;

      case 2:
         mmc_bankvrom(1, 0x1000 ^ vrombase, value);
         break;

      case 3:
         mmc_bankvrom(1, 0x1400 ^ vrombase, value);
         break;

      case 4:
         mmc_bankvrom(1, 0x1800 ^ vrombase, value);
         break;

      case 5:
         mmc_bankvrom(1, 0x1C00 ^ vrombase, value);
         break;

      case 6:
         mmc_bankrom(8, (command & 0x40) ? 0xA000 : 0x8000, value);
         break;

      case 7:
         mmc_bankrom(8, (command & 0x40) ? 0xC000 : 0xA000, value);
         break;

      case 8:
         mmc_bankvrom(1, 0x0400, value);
         break;

      case 9:
         mmc_bankvrom(1, 0x0C00, value);
         break;

      case 15:
         mmc_bankrom(8, (command & 0x40) ? 0x8000 : 0xC000, value);
         break;

      default:
#ifdef NOFRENDO_DEBUG
         nofrendo_log_printf("mapper 64: unknown command #%d", command & 0xF);
#endif  
         break;
      }
      break;

   case 0xA000:
      if (value & 1)
         ppu_mirror(0, 0, 1, 1);
      else
         ppu_mirror(0, 1, 0, 1);
      break;

   case 0xC000:
      //irq.counter = value;
      irq.latch = value;
      break;

   case 0xC001:
      //irq.latch = value;
      irq.reset = true;
      break;

   case 0xE000:
      //irq.counter = irq.latch;
      irq.enabled = false;
      break;

   case 0xE001:
      irq.enabled = true;
      break;

   default:
#ifdef NOFRENDO_DEBUG
      nofrendo_log_printf("mapper 64: Wrote $%02X to $%04X", value, address);
#endif  
      break;
   }

   if (true == irq.reset)
      irq.counter = irq.latch;
}

static void map64_init(void)
{
   mmc_bankrom(8, 0x8000, MMC_LASTBANK);
   mmc_bankrom(8, 0xA000, MMC_LASTBANK);
   mmc_bankrom(8, 0xC000, MMC_LASTBANK);
   mmc_bankrom(8, 0xE000, MMC_LASTBANK);

   irq.counter = irq.latch = 0;
   irq.reset = irq.enabled = false;
}

static map_memwrite map64_memwrite[] =
    {
        {0x8000, 0xFFFF, map64_write},
        {-1, -1, NULL}};

mapintf_t map64_intf =
    {
        64,               /* mapper number */
        "Tengen RAMBO-1", /* mapper name */
        map64_init,       /* init routine */
        NULL,             /* vblank callback */
        map64_hblank,     /* hblank callback */
        NULL,             /* get state (snss) */
        NULL,             /* set state (snss) */
        NULL,             /* memory read structure */
        map64_memwrite,   /* memory write structure */
        NULL              /* external sound device */
};