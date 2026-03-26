#include "noftypes.h"
#include "nes_mmc.h"
 
static void map94_write(uint32 address, uint8 value)
{
   UNUSED(address); 
   mmc_bankrom(16, 0x8000, value >> 2);
}

static map_memwrite map94_memwrite[] =
    {
        {0x8000, 0xFFFF, map94_write},
        {-1, -1, NULL}};

mapintf_t map94_intf =
    {
        94,             /* mapper number */
        "Mapper 94",    /* mapper name */
        NULL,           /* init routine */
        NULL,           /* vblank callback */
        NULL,           /* hblank callback */
        NULL,           /* get state (snss) */
        NULL,           /* set state (snss) */
        NULL,           /* memory read structure */
        map94_memwrite, /* memory write structure */
        NULL            /* external sound device */
};