#include "noftypes.h"
#include "nes_mmc.h"
#include "nes.h"
#include "libsnss.h"
#include "log.h"
 
static void map229_init(void)
{ 
  mmc_bankrom(32, 0x8000, 0x00);
  mmc_bankvrom(8, 0x0000, 0x00); 
  return;
}
 
static void map229_write(uint32 address, uint8 value)
{ 
  UNUSED(value);
 
  mmc_bankvrom(8, 0x0000, (uint8)(address & 0x1F));
 
  if ((address & 0x1E) == 0x00)
  {
    mmc_bankrom(32, 0x8000, 0x00);
  }
  else
  {
    mmc_bankrom(16, 0x8000, (uint8)(address & 0x1F));
    mmc_bankrom(16, 0xC000, (uint8)(address & 0x1F));
  } 
  if (address & 0x20)
    ppu_mirror(0, 0, 1, 1);
  else
    ppu_mirror(0, 1, 0, 1); 
  return;
}

static map_memwrite map229_memwrite[] =
    {
        {0x8000, 0xFFFF, map229_write},
        {-1, -1, NULL}};

mapintf_t map229_intf =
    {
        229,                 /* Mapper number */
        "31 in 1 (bootleg)", /* Mapper name */
        map229_init,         /* Initialization routine */
        NULL,                /* VBlank callback */
        NULL,                /* HBlank callback */
        NULL,                /* Get state (SNSS) */
        NULL,                /* Set state (SNSS) */
        NULL,                /* Memory read structure */
        map229_memwrite,     /* Memory write structure */
        NULL                 /* External sound device */
};