#include "noftypes.h"
#include "nes_mmc.h"
 
static void map66_write(uint32 address, uint8 value)
{
   UNUSED(address);

   mmc_bankrom(32, 0x8000, (value >> 4) & 3);
   mmc_bankvrom(8, 0x0000, value & 3);
}

static void map66_init(void)
{
   mmc_bankrom(32, 0x8000, 0);
   mmc_bankvrom(8, 0x0000, 0);
}

static map_memwrite map66_memwrite[] =
    {
        {0x8000, 0xFFFF, map66_write},
        {-1, -1, NULL}};

mapintf_t map66_intf =
    {
        66,             /* mapper number */
        "GNROM",        /* mapper name */
        map66_init,     /* init routine */
        NULL,           /* vblank callback */
        NULL,           /* hblank callback */
        NULL,           /* get state (snss) */
        NULL,           /* set state (snss) */
        NULL,           /* memory read structure */
        map66_memwrite, /* memory write structure */
        NULL            /* external sound device */
};