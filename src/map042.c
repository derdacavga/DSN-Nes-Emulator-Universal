#include "noftypes.h"
#include "nes_mmc.h"
#include "nes.h"
#include "libsnss.h"
#include "log.h"

static struct
{
  bool enabled;
  uint32 counter;
} irq;
 
static void map42_irq_reset(void)
{ 
  irq.enabled = false;
  irq.counter = 0x0000;
 
  return;
}
 
static void map42_init(void)
{ 
  mmc_bankrom(8, 0x8000, 0x0C);
  mmc_bankrom(8, 0xA000, 0x0D);
  mmc_bankrom(8, 0xC000, 0x0E);
  mmc_bankrom(8, 0xE000, 0x0F);
 
  map42_irq_reset();
 
  return;
}
 
static void map42_hblank(int vblank)
{ 
  UNUSED(vblank);
 
  if (irq.enabled)
  { 
    irq.counter = irq.counter + 114;
 
    if (irq.counter >= 0x6000)
    { 
      nes_irq(); 
      map42_irq_reset();
    }
  }
}
 
static void map42_write(uint32 address, uint8 value)
{
  switch (address & 0x03)
  { 
  case 0x00:
    mmc_bankrom(8, 0x6000, value & 0x0F);
    break;
  case 0x01:
    if (value & 0x08)
      ppu_mirror(0, 0, 1, 1);  
    else
      ppu_mirror(0, 1, 0, 1);  
    break; 
  case 0x02:
    if (value & 0x02)
      irq.enabled = true;
    else
      map42_irq_reset();
    break;
 
  default:
    break;
  } 
  return;
}
 
static void map42_setstate(SnssMapperBlock *state)
{ 
  UNUSED(state); 
  return;
}
 
static void map42_getstate(SnssMapperBlock *state)
{ 
  UNUSED(state); 
  return;
}

static map_memwrite map42_memwrite[] =
    {
        {0xE000, 0xFFFF, map42_write},
        {-1, -1, NULL}};

mapintf_t map42_intf =
    {
        42,                     /* Mapper number */
        "Baby Mario (bootleg)", /* Mapper name */
        map42_init,             /* Initialization routine */
        NULL,                   /* VBlank callback */
        map42_hblank,           /* HBlank callback */
        map42_getstate,         /* Get state (SNSS) */
        map42_setstate,         /* Set state (SNSS) */
        NULL,                   /* Memory read structure */
        map42_memwrite,         /* Memory write structure */
        NULL                    /* External sound device */
};