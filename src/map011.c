#include "noftypes.h"
#include "nes_mmc.h"
 
static void map11_write(uint32 address, uint8 value)
{
   UNUSED(address);

   mmc_bankrom(32, 0x8000, value & 0x0F);
   mmc_bankvrom(8, 0x0000, value >> 4);
}

static void map11_init(void)
{
   mmc_bankrom(32, 0x8000, 0);
   mmc_bankvrom(8, 0x0000, 0);
}

static map_memwrite map11_memwrite[] =
    {
        {0x8000, 0xFFFF, map11_write},
        {-1, -1, NULL}};

mapintf_t map11_intf =
    {
        11,             /* mapper number */
        "Color Dreams", /* mapper name */
        map11_init,     /* init routine */
        NULL,           /* vblank callback */
        NULL,           /* hblank callback */
        NULL,           /* get state (snss) */
        NULL,           /* set state (snss) */
        NULL,           /* memory read structure */
        map11_memwrite, /* memory write structure */
        NULL            /* external sound device */
};