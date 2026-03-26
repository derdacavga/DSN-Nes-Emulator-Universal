#include "noftypes.h"
#include "nes_mmc.h"
#include "nes_ppu.h"
 
static void map99_vromswitch(uint8 value)
{
   int bank = (value & 0x04) >> 2;
   mmc_bankvrom(8, 0x0000, bank);
}
 
static void map99_init(void)
{
   ppu_mirror(0, 1, 2, 3);
   ppu_setvromswitch(map99_vromswitch);
}

mapintf_t map99_intf =
    {
        99,           /* mapper number */
        "VS. System", /* mapper name */
        map99_init,   /* init routine */
        NULL,         /* vblank callback */
        NULL,         /* hblank callback */
        NULL,         /* get state (snss) */
        NULL,         /* set state (snss) */
        NULL,         /* memory read structure */
        NULL,         /* memory write structure */
        NULL          /* external sound device */
};