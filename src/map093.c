#include "noftypes.h"
#include "nes_mmc.h"
#include "nes_ppu.h"

static void map93_write(uint32 address, uint8 value)
{
   UNUSED(address);
 
   mmc_bankrom(16, 0x8000, value >> 4);

   if (value & 1)
      ppu_mirror(0, 1, 0, 1); 
   else
      ppu_mirror(0, 0, 1, 1);  
}

static map_memwrite map93_memwrite[] =
    {
        {0x8000, 0xFFFF, map93_write},
        {-1, -1, NULL}};

mapintf_t map93_intf =
    {
        93,             /* mapper number */
        "Mapper 93",    /* mapper name */
        NULL,           /* init routine */
        NULL,           /* vblank callback */
        NULL,           /* hblank callback */
        NULL,           /* get state (snss) */
        NULL,           /* set state (snss) */
        NULL,           /* memory read structure */
        map93_memwrite, /* memory write structure */
        NULL            /* external sound device */
};