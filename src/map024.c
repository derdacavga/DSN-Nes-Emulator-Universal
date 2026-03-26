#include "noftypes.h"
#include "nes_mmc.h"
#include "nes.h"
#include "log.h"
#include "vrcvisnd.h"

static struct
{
   int counter, enabled;
   int latch, wait_state;
} irq;

static void map24_init(void)
{
   irq.counter = irq.enabled = 0;
   irq.latch = irq.wait_state = 0;
}

static void map24_hblank(int vblank)
{
   UNUSED(vblank);

   if (irq.enabled)
   {
      if (256 == ++irq.counter)
      {
         irq.counter = irq.latch;
         nes_irq();
         //irq.enabled = false;
         irq.enabled = irq.wait_state;
      }
   }
}

static void map24_write(uint32 address, uint8 value)
{
   switch (address & 0xF003)
   {
   case 0x8000:
      mmc_bankrom(16, 0x8000, value);
      break;

   case 0x9003: 
      break;

   case 0xB003:
      switch (value & 0x0C)
      {
      case 0x00:
         ppu_mirror(0, 1, 0, 1); 
         break;

      case 0x04:
         ppu_mirror(0, 0, 1, 1);  
         break;

      case 0x08:
         ppu_mirror(0, 0, 0, 0);
         break;

      case 0x0C:
         ppu_mirror(1, 1, 1, 1);
         break;

      default:
         break;
      }
      break;

   case 0xC000:
      mmc_bankrom(8, 0xC000, value);
      break;

   case 0xD000:
      mmc_bankvrom(1, 0x0000, value);
      break;

   case 0xD001:
      mmc_bankvrom(1, 0x0400, value);
      break;

   case 0xD002:
      mmc_bankvrom(1, 0x0800, value);
      break;

   case 0xD003:
      mmc_bankvrom(1, 0x0C00, value);
      break;

   case 0xE000:
      mmc_bankvrom(1, 0x1000, value);
      break;

   case 0xE001:
      mmc_bankvrom(1, 0x1400, value);
      break;

   case 0xE002:
      mmc_bankvrom(1, 0x1800, value);
      break;

   case 0xE003:
      mmc_bankvrom(1, 0x1C00, value);
      break;

   case 0xF000:
      irq.latch = value;
      break;

   case 0xF001:
      irq.enabled = (value >> 1) & 0x01;
      irq.wait_state = value & 0x01;
      if (irq.enabled)
         irq.counter = irq.latch;
      break;

   case 0xF002:
      irq.enabled = irq.wait_state;
      break;

   default:
#ifdef NOFRENDO_DEBUG
      nofrendo_log_printf("invalid VRC6 write: $%02X to $%04X", value, address);
#endif  
      break;
   }
}

static void map24_getstate(SnssMapperBlock *state)
{
   state->extraData.mapper24.irqCounter = irq.counter;
   state->extraData.mapper24.irqCounterEnabled = irq.enabled;
}

static void map24_setstate(SnssMapperBlock *state)
{
   irq.counter = state->extraData.mapper24.irqCounter;
   irq.enabled = state->extraData.mapper24.irqCounterEnabled;
}

static map_memwrite map24_memwrite[] =
    {
        {0x8000, 0xF002, map24_write},
        {-1, -1, NULL}};

mapintf_t map24_intf =
    {
        24,             /* mapper number */
        "Konami VRC6",  /* mapper name */
        map24_init,     /* init routine */
        NULL,           /* vblank callback */
        map24_hblank,   /* hblank callback */
        map24_getstate, /* get state (snss) */
        map24_setstate, /* set state (snss) */
        NULL,           /* memory read structure */
        map24_memwrite, /* memory write structure */
        &vrcvi_ext      /* external sound device */
};