#include "noftypes.h"
#include "nes_mmc.h"
#include "nes_ppu.h"

static int select_c000 = 0;
 
static void map32_write(uint32 address, uint8 value)
{
   switch (address >> 12)
   {
   case 0x08:
      if (select_c000)
         mmc_bankrom(8, 0xC000, value);
      else
         mmc_bankrom(8, 0x8000, value);
      break;

   case 0x09:
      if (value & 1)
         ppu_mirror(0, 0, 1, 1);  
      else
         ppu_mirror(0, 1, 0, 1);  

      select_c000 = (value & 0x02);
      break;

   case 0x0A:
      mmc_bankrom(8, 0xA000, value);
      break;

   case 0x0B:
   {
      int loc = (address & 0x07) << 10;
      mmc_bankvrom(1, loc, value);
   }
   break;

   default:
      break;
   }
}

static map_memwrite map32_memwrite[] =
    {
        {0x8000, 0xFFFF, map32_write},
        {-1, -1, NULL}};

mapintf_t map32_intf =
    {
        32,             /* mapper number */
        "Irem G-101",   /* mapper name */
        NULL,           /* init routine */
        NULL,           /* vblank callback */
        NULL,           /* hblank callback */
        NULL,           /* get state (snss) */
        NULL,           /* set state (snss) */
        NULL,           /* memory read structure */
        map32_memwrite, /* memory write structure */
        NULL            /* external sound device */
};