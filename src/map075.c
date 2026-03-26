#include "noftypes.h"
#include "nes_mmc.h"
#include "nes_ppu.h"

static uint8 latch[2];
static uint8 hibits;
 
static void map75_write(uint32 address, uint8 value)
{
   switch ((address & 0xF000) >> 12)
   {
   case 0x8:
      mmc_bankrom(8, 0x8000, value);
      break;

   case 0x9:
      hibits = (value & 0x06);

      mmc_bankvrom(4, 0x0000, ((hibits & 0x02) << 3) | latch[0]);
      mmc_bankvrom(4, 0x1000, ((hibits & 0x04) << 2) | latch[1]);

      if (value & 1)
         ppu_mirror(0, 1, 0, 1);  
      else
         ppu_mirror(0, 0, 1, 1);  

      break;

   case 0xA:
      mmc_bankrom(8, 0xA000, value);
      break;

   case 0xC:
      mmc_bankrom(8, 0xC000, value);
      break;

   case 0xE:
      latch[0] = (value & 0x0F);
      mmc_bankvrom(4, 0x0000, ((hibits & 0x02) << 3) | latch[0]);
      break;

   case 0xF:
      latch[1] = (value & 0x0F);
      mmc_bankvrom(4, 0x1000, ((hibits & 0x04) << 2) | latch[1]);
      break;

   default:
      break;
   }
}

static map_memwrite map75_memwrite[] =
    {
        {0x8000, 0xFFFF, map75_write},
        {-1, -1, NULL}};

mapintf_t map75_intf =
    {
        75,             /* mapper number */
        "Konami VRC1",  /* mapper name */
        NULL,           /* init routine */
        NULL,           /* vblank callback */
        NULL,           /* hblank callback */
        NULL,           /* get state (snss) */
        NULL,           /* set state (snss) */
        NULL,           /* memory read structure */
        map75_memwrite, /* memory write structure */
        NULL            /* external sound device */
};