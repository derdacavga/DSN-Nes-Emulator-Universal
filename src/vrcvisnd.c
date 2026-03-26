#include "noftypes.h"
#include "vrcvisnd.h"
#include "nes_apu.h"

typedef struct vrcvirectangle_s
{
   bool enabled;

   uint8 reg[3];

   float accum;
   uint8 adder;

   int32 freq;
   int32 volume;
   uint8 duty_flip;
} vrcvirectangle_t;

typedef struct vrcvisawtooth_s
{
   bool enabled;

   uint8 reg[3];

   float accum;
   uint8 adder;
   uint8 output_acc;

   int32 freq;
   uint8 volume;
} vrcvisawtooth_t;

typedef struct vrcvisnd_s
{
   vrcvirectangle_t rectangle[2];
   vrcvisawtooth_t saw;
   float incsize;
} vrcvisnd_t;

static vrcvisnd_t vrcvi;
 
static int32 vrcvi_rectangle(vrcvirectangle_t *chan)
{ 
   chan->accum -= vrcvi.incsize;  
   while (chan->accum < 0)
   {
      chan->accum += chan->freq;
      chan->adder = (chan->adder + 1) & 0x0F;
   }
 
   if (false == chan->enabled)
      return 0;

   if (chan->adder < chan->duty_flip)
      return -(chan->volume);
   else
      return chan->volume;
}
 
static int32 vrcvi_sawtooth(vrcvisawtooth_t *chan)
{ 
   chan->accum -= vrcvi.incsize; 
   while (chan->accum < 0)
   {
      chan->accum += chan->freq;
      chan->output_acc += chan->volume;

      chan->adder++;
      if (7 == chan->adder)
      {
         chan->adder = 0;
         chan->output_acc = 0;
      }
   }
 
   if (false == chan->enabled)
      return 0;

   return (chan->output_acc >> 3) << 9;
}
 
static int32 vrcvi_process(void)
{
   int32 output;

   output = vrcvi_rectangle(&vrcvi.rectangle[0]);
   output += vrcvi_rectangle(&vrcvi.rectangle[1]);
   output += vrcvi_sawtooth(&vrcvi.saw);

   return output;
}
 
static void vrcvi_write(uint32 address, uint8 value)
{
   int chan = (address >> 12) - 9;

   switch (address & 0xB003)
   {
   case 0x9000:
   case 0xA000:
      vrcvi.rectangle[chan].reg[0] = value;
      vrcvi.rectangle[chan].volume = (value & 0x0F) << 8;
      vrcvi.rectangle[chan].duty_flip = (value >> 4) + 1;
      break;

   case 0x9001:
   case 0xA001:
      vrcvi.rectangle[chan].reg[1] = value;
      vrcvi.rectangle[chan].freq = ((vrcvi.rectangle[chan].reg[2] & 0x0F) << 8) + value + 1;
      break;

   case 0x9002:
   case 0xA002:
      vrcvi.rectangle[chan].reg[2] = value;
      vrcvi.rectangle[chan].freq = ((value & 0x0F) << 8) + vrcvi.rectangle[chan].reg[1] + 1;
      vrcvi.rectangle[chan].enabled = (value & 0x80) ? true : false;
      break;

   case 0xB000:
      vrcvi.saw.reg[0] = value;
      vrcvi.saw.volume = value & 0x3F;
      break;

   case 0xB001:
      vrcvi.saw.reg[1] = value;
      vrcvi.saw.freq = (((vrcvi.saw.reg[2] & 0x0F) << 8) + value + 1) << 1;
      break;

   case 0xB002:
      vrcvi.saw.reg[2] = value;
      vrcvi.saw.freq = (((value & 0x0F) << 8) + vrcvi.saw.reg[1] + 1) << 1;
      vrcvi.saw.enabled = (value & 0x80) ? true : false;
      break;

   default:
      break;
   }
}
 
static void vrcvi_reset(void)
{
   int i;
   apu_t apu;
 
   apu_getcontext(&apu);
   vrcvi.incsize = apu.cycle_rate;
 
   for (i = 0; i < 3; i++)
   {
      vrcvi_write(0x9000 + i, 0);
      vrcvi_write(0xA000 + i, 0);
      vrcvi_write(0xB000 + i, 0);
   }
}

static apu_memwrite vrcvi_memwrite[] =
    {
        {0x9000, 0x9002, vrcvi_write}, 
        {0xA000, 0xA002, vrcvi_write},
        {0xB000, 0xB002, vrcvi_write},
        {-1, -1, NULL}};

apuext_t vrcvi_ext =
    {
        NULL, 
        NULL, 
        vrcvi_reset,
        vrcvi_process,
        NULL, 
        vrcvi_memwrite};