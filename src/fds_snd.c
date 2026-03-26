#include "noftypes.h"
#include "nes_apu.h"
#include "fds_snd.h"

static int32 fds_incsize = 0;
 
static int32 fds_process(void)
{
   int32 output;
   output = 0;

   return output;
}
 
static void fds_write(uint32 address, uint8 value)
{
   UNUSED(address);
   UNUSED(value);
}
 
static void fds_reset(void)
{
   apu_t apu;

   apu_getcontext(&apu);
   //   fds_incsize = apu.cycle_rate;
   fds_incsize = (int32)(APU_BASEFREQ * 65536.0 / (float)apu.sample_rate);
}

static apu_memwrite fds_memwrite[] =
    {
        {0x4040, 0x4092, fds_write},
        {-1, -1, NULL}};

apuext_t fds_ext =
    {
        NULL,  
        NULL, 
        fds_reset,
        fds_process,
        NULL, 
        fds_memwrite};