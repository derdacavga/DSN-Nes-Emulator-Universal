#include "noftypes.h"
#include "nes_mmc.h"
 
static void map2_init()
{
   int last_bank = mmc_getinfo()->rom_banks - 1;
   printf("map2_init. last_bank=%d\n", last_bank);
   mmc_bankrom(16, 0xc000, last_bank);
   mmc_bankrom(16, 0x8000, 0);
}

static void map2_write(uint32 address, uint8 value)
{
   UNUSED(address);

   mmc_bankrom(16, 0x8000, value);
}

static map_memwrite map2_memwrite[] =
    {
        {0x8000, 0xFFFF, map2_write},
        {-1, -1, NULL}};

mapintf_t map2_intf =
    {
        2,             /* mapper number */
        "UNROM",       /* mapper name */
        map2_init,     /* init routine */
        NULL,          /* vblank callback */
        NULL,          /* hblank callback */
        NULL,          /* get state (snss) */
        NULL,          /* set state (snss) */
        NULL,          /* memory read structure */
        map2_memwrite, /* memory write structure */
        NULL           /* external sound device */
};