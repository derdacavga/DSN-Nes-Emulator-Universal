#include "noftypes.h"
#include "nes_mmc.h"
#include "nes_ppu.h"
#include "log.h"
 
static void map7_write(uint32 address, uint8 value)
{
   int mirror;
   UNUSED(address);

   mmc_bankrom(32, 0x8000, value);
   mirror = (value & 0x10) >> 4;
   ppu_mirror(mirror, mirror, mirror, mirror);
}

static void map7_init(void)
{
   mmc_bankrom(32, 0x8000, 0);
}

static map_memwrite map7_memwrite[] =
    {
        {0x8000, 0xFFFF, map7_write},
        {-1, -1, NULL}};

mapintf_t map7_intf =
    {
        7,             /* mapper number */
        "AOROM",       /* mapper name */
        map7_init,     /* init routine */
        NULL,          /* vblank callback */
        NULL,          /* hblank callback */
        NULL,          /* get state (snss) */
        NULL,          /* set state (snss) */
        NULL,          /* memory read structure */
        map7_memwrite, /* memory write structure */
        NULL           /* external sound device */
};