#include "noftypes.h"
#include "nes_mmc.h"
 
static void map8_write(uint32 address, uint8 value)
{
   UNUSED(address);

   mmc_bankrom(16, 0x8000, value >> 3);
   mmc_bankvrom(8, 0x0000, value & 7);
}

static void map8_init(void)
{
   mmc_bankrom(16, 0x8000, 0);
   mmc_bankrom(16, 0xC000, 1);
   mmc_bankvrom(8, 0x0000, 0);
}

static map_memwrite map8_memwrite[] =
    {
        {0x8000, 0xFFFF, map8_write},
        {-1, -1, NULL}};

mapintf_t map8_intf =
    {
        8,             /* mapper number */
        "FFE F3xxx",   /* mapper name */
        map8_init,     /* init routine */
        NULL,          /* vblank callback */
        NULL,          /* hblank callback */
        NULL,          /* get state (snss) */
        NULL,          /* set state (snss) */
        NULL,          /* memory read structure */
        map8_memwrite, /* memory write structure */
        NULL           /* external sound device */
};