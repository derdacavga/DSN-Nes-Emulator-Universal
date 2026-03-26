#include "noftypes.h"
#include "nes_mmc.h"
#include "nes.h"
#include "libsnss.h"
#include "log.h"

static uint8 register_low;
static uint8 register_high;
 
static void map41_set_chr(void)
{ 
  mmc_bankvrom(8, 0x0000, ((register_low >> 1) & 0x0C) | (register_high));
 
  return;
}
 
static void map41_init(void)
{  
  register_low = 0x00;
  register_high = 0x00;
  mmc_bankrom(32, 0x8000, 0x00);
  map41_set_chr();
 
  return;
}
 
static void map41_low_write(uint32 address, uint8 value)
{ 
  UNUSED(value);
 
  register_low = (uint8)(address & 0x3F);
  mmc_bankrom(32, 0x8000, register_low & 0x07);
  map41_set_chr();
  if (register_low & 0x20)
    ppu_mirror(0, 0, 1, 1);  
  else
    ppu_mirror(0, 1, 0, 1);  
  return;
}
 
static void map41_high_write(uint32 address, uint8 value)
{ 
  UNUSED(address);
 
  if (register_low & 0x04)
  {
    register_high = value & 0x03;
    map41_set_chr();
  }
 
  return;
}
 
static void map41_setstate(SnssMapperBlock *state)
{ 
  UNUSED(state);
 
  return;
}
 
static void map41_getstate(SnssMapperBlock *state)
{ 
  UNUSED(state); 
  return;
}

static map_memwrite map41_memwrite[] =
    {
        {0x6000, 0x67FF, map41_low_write},
        {0x8000, 0xFFFF, map41_high_write},
        {-1, -1, NULL}};

mapintf_t map41_intf =
    {
        41,               /* Mapper number */
        "Caltron 6 in 1", /* Mapper name */
        map41_init,       /* Initialization routine */
        NULL,             /* VBlank callback */
        NULL,             /* HBlank callback */
        map41_getstate,   /* Get state (SNSS) */
        map41_setstate,   /* Set state (SNSS) */
        NULL,             /* Memory read structure */
        map41_memwrite,   /* Memory write structure */
        NULL              /* External sound device */
};