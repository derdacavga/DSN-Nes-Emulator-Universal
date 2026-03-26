#include "noftypes.h"
#include "nes_mmc.h"
#include "nes.h"
#include "libsnss.h"
#include "log.h"

static uint8 prg_low_bank;
static uint8 chr_low_bank;
static uint8 prg_high_bank;
static uint8 chr_high_bank;
 
static void map46_set_banks(void)
{ 
  mmc_bankrom(32, 0x8000, (prg_high_bank << 1) | (prg_low_bank));
  mmc_bankvrom(8, 0x0000, (chr_high_bank << 3) | (chr_low_bank));
 
  return;
}
 
static void map46_init(void)
{ 
  prg_high_bank = 0x00;
  chr_high_bank = 0x00;
  map46_set_banks();
 
  return;
}
 
static void map46_write(uint32 address, uint8 value)
{ 
  if (address & 0x8000)
  {
    prg_low_bank = value & 0x01;
    chr_low_bank = (value >> 4) & 0x07;
    map46_set_banks();
  }
  else
  {
    prg_high_bank = value & 0x0F;
    chr_high_bank = (value >> 4) & 0x0F;
    map46_set_banks();
  } 
  return;
}
 
static void map46_setstate(SnssMapperBlock *state)
{ 
  UNUSED(state); 
  return;
}
 
static void map46_getstate(SnssMapperBlock *state)
{ 
  UNUSED(state); 
  return;
}

static map_memwrite map46_memwrite[] =
    {
        {0x6000, 0xFFFF, map46_write},
        {-1, -1, NULL}};

mapintf_t map46_intf =
    {
        46,                     /* Mapper number */
        "Pelican Game Station", /* Mapper name */
        map46_init,             /* Initialization routine */
        NULL,                   /* VBlank callback */
        NULL,                   /* HBlank callback */
        map46_getstate,         /* Get state (SNSS) */
        map46_setstate,         /* Set state (SNSS) */
        NULL,                   /* Memory read structure */
        map46_memwrite,         /* Memory write structure */
        NULL                    /* External sound device */
};