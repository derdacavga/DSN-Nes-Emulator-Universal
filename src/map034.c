#include "noftypes.h"
#include "nes_mmc.h"

static void map34_init(void)
{
   mmc_bankrom(32, 0x8000, MMC_LASTBANK);
}

static void map34_write(uint32 address, uint8 value)
{
   if ((address & 0x8000) || (0x7FFD == address))
   {
      mmc_bankrom(32, 0x8000, value);
   }
   else if (0x7FFE == address)
   {
      mmc_bankvrom(4, 0x0000, value);
   }
   else if (0x7FFF == address)
   {
      mmc_bankvrom(4, 0x1000, value);
   }
}

static map_memwrite map34_memwrite[] =
    {
        {0x7FFD, 0xFFFF, map34_write},
        {-1, -1, NULL}};

mapintf_t map34_intf =
    {
        34,             /* mapper number */
        "Nina-1",       /* mapper name */
        map34_init,     /* init routine */
        NULL,           /* vblank callback */
        NULL,           /* hblank callback */
        NULL,           /* get state (snss) */
        NULL,           /* set state (snss) */
        NULL,           /* memory read structure */
        map34_memwrite, /* memory write structure */
        NULL            /* external sound device */
};