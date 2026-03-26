#include "noftypes.h"
#include "nes_mmc.h"
 
static void map79_write(uint32 address, uint8 value)
{
   if ((address & 0x5100) == 0x4100)
   {
      mmc_bankrom(32, 0x8000, (value >> 3) & 1);
      mmc_bankvrom(8, 0x0000, value & 7);
   }
}

static void map79_init(void)
{
   mmc_bankrom(32, 0x8000, 0);
   mmc_bankvrom(8, 0x0000, 0);
}

static map_memwrite map79_memwrite[] =
    {
        {0x4100, 0x5FFF, map79_write}, 
        {-1, -1, NULL}};

mapintf_t map79_intf =
    {
        79,             /* mapper number */
        "NINA-03/06",   /* mapper name */
        map79_init,     /* init routine */
        NULL,           /* vblank callback */
        NULL,           /* hblank callback */
        NULL,           /* get state (snss) */
        NULL,           /* set state (snss) */
        NULL,           /* memory read structure */
        map79_memwrite, /* memory write structure */
        NULL            /* external sound device */
};