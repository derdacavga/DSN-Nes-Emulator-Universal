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
 
static void map73_init(void)
{ 
  irq.enabled = false;
  irq.counter = 0x0000; 
  return;
}
 
static void map73_hblank(int vblank)
{ 
  UNUSED(vblank); 
  if (irq.enabled)
  { 
    irq.counter = irq.counter + 114; 
    if (irq.counter & 0x10000)
    { 
      irq.counter &= 0xFFFF; 
      nes_irq(); 
      irq.enabled = false;
    }
  }
}
 
static void map73_write(uint32 address, uint8 value)
{
  switch (address & 0xF000)
  {
  case 0x8000:
    irq.counter &= 0xFFF0;
    irq.counter |= (uint32)(value);
    break;
  case 0x9000:
    irq.counter &= 0xFF0F;
    irq.counter |= (uint32)(value << 4);
    break;
  case 0xA000:
    irq.counter &= 0xF0FF;
    irq.counter |= (uint32)(value << 8);
    break;
  case 0xB000:
    irq.counter &= 0x0FFF;
    irq.counter |= (uint32)(value << 12);
    break;
  case 0xC000:
    if (value & 0x02)
      irq.enabled = true;
    else
      irq.enabled = false;
    break;
  case 0xF000:
    mmc_bankrom(16, 0x8000, value);
  default:
    break;
  } 
  return;
}
 
static void map73_setstate(SnssMapperBlock *state)
{ 
  UNUSED(state); 
  return;
}
 
static void map73_getstate(SnssMapperBlock *state)
{ 
  UNUSED(state); 
  return;
}

static map_memwrite map73_memwrite[] =
    {
        {0x8000, 0xFFFF, map73_write},
        {-1, -1, NULL}};

mapintf_t map73_intf =
    {
        73,             /* Mapper number */
        "Konami VRC3",  /* Mapper name */
        map73_init,     /* Initialization routine */
        NULL,           /* VBlank callback */
        map73_hblank,   /* HBlank callback */
        map73_getstate, /* Get state (SNSS) */
        map73_setstate, /* Set state (SNSS) */
        NULL,           /* Memory read structure */
        map73_memwrite, /* Memory write structure */
        NULL            /* External sound device */
};