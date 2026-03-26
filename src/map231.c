#include "noftypes.h"
#include "nes_mmc.h"
 
static void map231_init(void)
{
   mmc_bankrom(32, 0x8000, MMC_LASTBANK);
}

static void map231_write(uint32 address, uint8 value)
{
   int bank, vbank;
   UNUSED(address);

   bank = ((value & 0x80) >> 5) | (value & 0x03);
   vbank = (value >> 4) & 0x07;

   mmc_bankrom(32, 0x8000, bank);
   mmc_bankvrom(8, 0x0000, vbank);
}

static map_memwrite map231_memwrite[] =
    {
        {0x8000, 0xFFFF, map231_write},
        {-1, -1, NULL}};

mapintf_t map231_intf =
    {
        231,             /* mapper number */
        "NINA-07",       /* mapper name */
        map231_init,     /* init routine */
        NULL,            /* vblank callback */
        NULL,            /* hblank callback */
        NULL,            /* get state (snss) */
        NULL,            /* set state (snss) */
        NULL,            /* memory read structure */
        map231_memwrite, /* memory write structure */
        NULL             /* external sound device */
};