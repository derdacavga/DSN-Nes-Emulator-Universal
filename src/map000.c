#include "noftypes.h"
#include "nes_mmc.h"

mapintf_t map0_intf =
    {
        0,      /* mapper number */
        "None", /* mapper name */
        NULL,   /* init routine */
        NULL,   /* vblank callback */
        NULL,   /* hblank callback */
        NULL,   /* get state (snss) */
        NULL,   /* set state (snss) */
        NULL,   /* memory read structure */
        NULL,   /* memory write structure */
        NULL    /* external sound device */
}; 