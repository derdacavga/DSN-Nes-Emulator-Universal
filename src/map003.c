#include "noftypes.h"
#include "nes_mmc.h"
 
static void map3_write(uint32 address, uint8 value)
{
   UNUSED(address);

   mmc_bankvrom(8, 0x0000, value);
}

static map_memwrite map3_memwrite[] =
    {
        {0x8000, 0xFFFF, map3_write},
        {-1, -1, NULL}};

mapintf_t map3_intf =
    {
        3,             /* mapper number */
        "CNROM",       /* mapper name */
        NULL,          /* init routine */
        NULL,          /* vblank callback */
        NULL,          /* hblank callback */
        NULL,          /* get state (snss) */
        NULL,          /* set state (snss) */
        NULL,          /* memory read structure */
        map3_memwrite, /* memory write structure */
        NULL           /* external sound device */
};