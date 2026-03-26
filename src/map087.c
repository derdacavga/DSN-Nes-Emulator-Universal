#include "noftypes.h"
#include "nes_mmc.h"
#include "nes.h"
#include "libsnss.h"
#include "log.h"
 
static void map87_write(uint32 address, uint8 value)
{ 
   UNUSED(address); 
   if (value & 0x02)
      mmc_bankvrom(8, 0x0000, 0x01);
   else
      mmc_bankvrom(8, 0x0000, 0x00); 
   return;
}

static map_memwrite map87_memwrite[] =
    {
        {0x6000, 0x7FFF, map87_write},
        {-1, -1, NULL}};

mapintf_t map87_intf =
    {
        87,                /* Mapper number */
        "16K VROM switch", /* Mapper name */
        NULL,              /* Initialization routine */
        NULL,              /* VBlank callback */
        NULL,              /* HBlank callback */
        NULL,              /* Get state (SNSS) */
        NULL,              /* Set state (SNSS) */
        NULL,              /* Memory read structure */
        map87_memwrite,    /* Memory write structure */
        NULL               /* External sound device */
};