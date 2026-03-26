#include "noftypes.h"
#include "nes_mmc.h"
#include "nes_ppu.h"
 
static void map15_write(uint32 address, uint8 value)
{
   int bank = value & 0x3F;
   uint8 swap = (value & 0x80) >> 7;

   switch (address & 0x3)
   {
   case 0:
      mmc_bankrom(8, 0x8000, (bank << 1) + swap);
      mmc_bankrom(8, 0xA000, (bank << 1) + (swap ^ 1));
      mmc_bankrom(8, 0xC000, ((bank + 1) << 1) + swap);
      mmc_bankrom(8, 0xE000, ((bank + 1) << 1) + (swap ^ 1));

      if (value & 0x40)
         ppu_mirror(0, 0, 1, 1);  
      else
         ppu_mirror(0, 1, 0, 1);  
      break;

   case 1:
      mmc_bankrom(8, 0xC000, (bank << 1) + swap);
      mmc_bankrom(8, 0xE000, (bank << 1) + (swap ^ 1));
      break;

   case 2:
      if (swap)
      {
         mmc_bankrom(8, 0x8000, (bank << 1) + 1);
         mmc_bankrom(8, 0xA000, (bank << 1) + 1);
         mmc_bankrom(8, 0xC000, (bank << 1) + 1);
         mmc_bankrom(8, 0xE000, (bank << 1) + 1);
      }
      else
      {
         mmc_bankrom(8, 0x8000, (bank << 1));
         mmc_bankrom(8, 0xA000, (bank << 1));
         mmc_bankrom(8, 0xC000, (bank << 1));
         mmc_bankrom(8, 0xE000, (bank << 1));
      }
      break;

   case 3:
      mmc_bankrom(8, 0xC000, (bank << 1) + swap);
      mmc_bankrom(8, 0xE000, (bank << 1) + (swap ^ 1));

      if (value & 0x40)
         ppu_mirror(0, 0, 1, 1);  
      else
         ppu_mirror(0, 1, 0, 1);  
      break;

   default:
      break;
   }
}

static void map15_init(void)
{
   mmc_bankrom(32, 0x8000, 0);
}

static map_memwrite map15_memwrite[] =
    {
        {0x8000, 0xFFFF, map15_write},
        {-1, -1, NULL}};

mapintf_t map15_intf =
    {
        15,                /* mapper number */
        "Contra 100-in-1", /* mapper name */
        map15_init,        /* init routine */
        NULL,              /* vblank callback */
        NULL,              /* hblank callback */
        NULL,              /* get state (snss) */
        NULL,              /* set state (snss) */
        NULL,              /* memory read structure */
        map15_memwrite,    /* memory write structure */
        NULL               /* external sound device */
};