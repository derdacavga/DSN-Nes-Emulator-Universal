#include <string.h>

#include "noftypes.h"
#include "log.h"
#include "nes_apu.h"
#include "nes6502.h"

#pragma GCC optimize("O3")
#pragma GCC optimize("inline-functions")

//#define APU_OVERSAMPLE
#define APU_VOLUME_DECAY(x) ((x) -= ((x) >> 7))
 
#define APU_RECTANGLE_OUTPUT(channel) (apu.rectangle[channel].output_vol)
#define APU_TRIANGLE_OUTPUT (apu.triangle.output_vol + (apu.triangle.output_vol >> 2))
#define APU_NOISE_OUTPUT ((apu.noise.output_vol + apu.noise.output_vol + apu.noise.output_vol) >> 2)
#define APU_DMC_OUTPUT ((apu.dmc.output_vol + apu.dmc.output_vol + apu.dmc.output_vol) >> 2)
 
static apu_t apu; 
static int32 decay_lut[16];
static int vbl_lut[32];
static int trilength_lut[128];
 
#ifndef REALTIME_NOISE
static int8 noise_long_lut[APU_NOISE_32K];
static int8 noise_short_lut[APU_NOISE_93];
#endif  
 
static const uint8 vbl_length[32] =
    {
        5, 127,
        10, 1,
        19, 2,
        40, 3,
        80, 4,
        30, 5,
        7, 6,
        13, 7,
        6, 8,
        12, 9,
        24, 10,
        48, 11,
        96, 12,
        36, 13,
        8, 14,
        16, 15};
 
static const int freq_limit[8] =
    {
        0x3FF, 0x555, 0x666, 0x71C, 0x787, 0x7C1, 0x7E0, 0x7F0};
 
static const int noise_freq[16] =
    {
        4, 8, 16, 32, 64, 96, 128, 160,
        202, 254, 380, 508, 762, 1016, 2034, 4068};
 
const int dmc_clocks[16] =
    {
        428, 380, 340, 320, 286, 254, 226, 214,
        190, 160, 142, 128, 106, 85, 72, 54};
 
static const int duty_flip[4] = {2, 4, 8, 12};

void apu_setcontext(apu_t *src_apu)
{
   apu = *src_apu;
}

void apu_getcontext(apu_t *dest_apu)
{
   *dest_apu = apu;
}

void apu_setchan(int chan, bool enabled)
{
   if (enabled)
      apu.mix_enable |= (1 << chan);
   else
      apu.mix_enable &= ~(1 << chan);
}
 
#ifdef REALTIME_NOISE
INLINE int8 shift_register15(uint8 xor_tap)
{
   static int sreg = 0x4000;
   int bit0, tap, bit14;

   bit0 = sreg & 1;
   tap = (sreg & xor_tap) ? 1 : 0;
   bit14 = (bit0 ^ tap);
   sreg >>= 1;
   sreg |= (bit14 << 14);
   return (bit0 ^ 1);
}
#else  /* !REALTIME_NOISE */
static void shift_register15(int8 *buf, int count)
{
   static int sreg = 0x4000;
   int bit0, bit1, bit6, bit14;

   if (count == APU_NOISE_93)
   {
      while (count--)
      {
         bit0 = sreg & 1;
         bit6 = (sreg & 0x40) >> 6;
         bit14 = (bit0 ^ bit6);
         sreg >>= 1;
         sreg |= (bit14 << 14);
         *buf++ = bit0 ^ 1;
      }
   }
   else /* 32K noise */
   {
      while (count--)
      {
         bit0 = sreg & 1;
         bit1 = (sreg & 2) >> 1;
         bit14 = (bit0 ^ bit1);
         sreg >>= 1;
         sreg |= (bit14 << 14);
         *buf++ = bit0 ^ 1;
      }
   }
}
#endif  
 
#ifdef APU_OVERSAMPLE

#define APU_MAKE_RECTANGLE(ch)                                                                                                           \
   static int32 apu_rectangle_##ch(void)                                                                                                 \
   {                                                                                                                                     \
      int32 output, total;                                                                                                               \
      int num_times;                                                                                                                     \
                                                                                                                                         \
      APU_VOLUME_DECAY(apu.rectangle[ch].output_vol);                                                                                    \
                                                                                                                                         \
      if (false == apu.rectangle[ch].enabled || 0 == apu.rectangle[ch].vbl_length)                                                       \
         return APU_RECTANGLE_OUTPUT(ch);                                                                                                \
                                                                                                         \
      if (false == apu.rectangle[ch].holdnote)                                                                                           \
         apu.rectangle[ch].vbl_length--;                                                                                                 \
 
      apu.rectangle[ch].env_phase -= 4;                                                                                     \
      while (apu.rectangle[ch].env_phase < 0)                                                                                            \
      {                                                                                                                                  \
         apu.rectangle[ch].env_phase += apu.rectangle[ch].env_delay;                                                                     \
                                                                                                                                         \
         if (apu.rectangle[ch].holdnote)                                                                                                 \
            apu.rectangle[ch].env_vol = (apu.rectangle[ch].env_vol + 1) & 0x0F;                                                          \
         else if (apu.rectangle[ch].env_vol < 0x0F)                                                                                      \
            apu.rectangle[ch].env_vol++;                                                                                                 \
      }                                                                                                                                  \
 
      if (apu.rectangle[ch].freq < 8 || (false == apu.rectangle[ch].sweep_inc && apu.rectangle[ch].freq > apu.rectangle[ch].freq_limit)) \
         return APU_RECTANGLE_OUTPUT(ch);                                                                                                \
 
      if (apu.rectangle[ch].sweep_on && apu.rectangle[ch].sweep_shifts)                                                                  \
      {                                                                                                                                  \
         apu.rectangle[ch].sweep_phase -= 2;                                                                               \
         while (apu.rectangle[ch].sweep_phase < 0)                                                                                       \
         {                                                                                                                               \
            apu.rectangle[ch].sweep_phase += apu.rectangle[ch].sweep_delay;                                                              \
                                                                                                                                         \
            if (apu.rectangle[ch].sweep_inc)                                                                             \
            {                                                                                                                            \
               if (0 == ch)                                                                                                              \
                  apu.rectangle[ch].freq += ~(apu.rectangle[ch].freq >> apu.rectangle[ch].sweep_shifts);                                 \
               else                                                                                                                      \
                  apu.rectangle[ch].freq -= (apu.rectangle[ch].freq >> apu.rectangle[ch].sweep_shifts);                                  \
            }                                                                                                                            \
            else                                                                                                      \
            {                                                                                                                            \
               apu.rectangle[ch].freq += (apu.rectangle[ch].freq >> apu.rectangle[ch].sweep_shifts);                                     \
            }                                                                                                                            \
         }                                                                                                                               \
      }                                                                                                                                  \
                                                                                                                                         \
      apu.rectangle[ch].accum -= apu.cycle_rate;                                                                                         \
      if (apu.rectangle[ch].accum >= 0)                                                                                                  \
         return APU_RECTANGLE_OUTPUT(ch);                                                                                                \
                                                                                                                                         \
      if (apu.rectangle[ch].fixed_envelope)                                                                                              \
         output = apu.rectangle[ch].volume << 8;                                                                    \
      else                                                                                                                               \
         output = (apu.rectangle[ch].env_vol ^ 0x0F) << 8;                                                                               \
                                                                                                                                         \
      num_times = total = 0;                                                                                                             \
                                                                                                                                         \
      while (apu.rectangle[ch].accum < 0)                                                                                                \
      {                                                                                                                                  \
         apu.rectangle[ch].accum += apu.rectangle[ch].freq + 1;                                                                          \
         apu.rectangle[ch].adder = (apu.rectangle[ch].adder + 1) & 0x0F;                                                                 \
                                                                                                                                         \
         if (apu.rectangle[ch].adder < apu.rectangle[ch].duty_flip)                                                                      \
            total += output;                                                                                                             \
         else                                                                                                                            \
            total -= output;                                                                                                             \
                                                                                                                                         \
         num_times++;                                                                                                                    \
      }                                                                                                                                  \
                                                                                                                                         \
      apu.rectangle[ch].output_vol = total / num_times;                                                                                  \
      return APU_RECTANGLE_OUTPUT(ch);                                                                                                   \
   }

#else  
#define APU_MAKE_RECTANGLE(ch)                                                                                                           \
   static int32 apu_rectangle_##ch(void)                                                                                                 \
   {                                                                                                                                     \
      int32 output;                                                                                                                      \
                                                                                                                                         \
      APU_VOLUME_DECAY(apu.rectangle[ch].output_vol);                                                                                    \
                                                                                                                                         \
      if (false == apu.rectangle[ch].enabled || 0 == apu.rectangle[ch].vbl_length)                                                       \
         return APU_RECTANGLE_OUTPUT(ch);                                                                                                \
                                                                                                                                         \
      if (false == apu.rectangle[ch].holdnote)                                                                                           \
         apu.rectangle[ch].vbl_length--;                                                                                                 \
                                                                                                                                         \
      apu.rectangle[ch].env_phase -= 4;                                                                                                  \
      while (apu.rectangle[ch].env_phase < 0)                                                                                            \
      {                                                                                                                                  \
         apu.rectangle[ch].env_phase += apu.rectangle[ch].env_delay;                                                                     \
                                                                                                                                         \
         if (apu.rectangle[ch].holdnote)                                                                                                 \
            apu.rectangle[ch].env_vol = (apu.rectangle[ch].env_vol + 1) & 0x0F;                                                          \
         else if (apu.rectangle[ch].env_vol < 0x0F)                                                                                      \
            apu.rectangle[ch].env_vol++;                                                                                                 \
      }                                                                                                                                  \
                                                                                                                                         \
      if (apu.rectangle[ch].freq < 8 || (false == apu.rectangle[ch].sweep_inc && apu.rectangle[ch].freq > apu.rectangle[ch].freq_limit)) \
         return APU_RECTANGLE_OUTPUT(ch);                                                                                                \
                                                                                                                                         \
      /* frequency sweeping at a rate of (sweep_delay + 1) / 120 secs */                                                                 \
      if (apu.rectangle[ch].sweep_on && apu.rectangle[ch].sweep_shifts)                                                                  \
      {                                                                                                                                  \
         apu.rectangle[ch].sweep_phase -= 2;                                                                                             \
         while (apu.rectangle[ch].sweep_phase < 0)                                                                                       \
         {                                                                                                                               \
            apu.rectangle[ch].sweep_phase += apu.rectangle[ch].sweep_delay;                                                              \
                                                                                                                                         \
            if (apu.rectangle[ch].sweep_inc)                                                                                             \
            {                                                                                                                            \
               if (0 == ch)                                                                                                              \
                  apu.rectangle[ch].freq += ~(apu.rectangle[ch].freq >> apu.rectangle[ch].sweep_shifts);                                 \
               else                                                                                                                      \
                  apu.rectangle[ch].freq -= (apu.rectangle[ch].freq >> apu.rectangle[ch].sweep_shifts);                                  \
            }                                                                                                                            \
            else                                                                                                                         \
            {                                                                                                                            \
               apu.rectangle[ch].freq += (apu.rectangle[ch].freq >> apu.rectangle[ch].sweep_shifts);                                     \
            }                                                                                                                            \
         }                                                                                                                               \
      }                                                                                                                                  \
                                                                                                                                         \
      apu.rectangle[ch].accum -= apu.cycle_rate;                                                                                         \
      if (apu.rectangle[ch].accum >= 0)                                                                                                  \
         return APU_RECTANGLE_OUTPUT(ch);                                                                                                \
                                                                                                                                         \
      while (apu.rectangle[ch].accum < 0)                                                                                                \
      {                                                                                                                                  \
         apu.rectangle[ch].accum += (apu.rectangle[ch].freq + 1);                                                                        \
         apu.rectangle[ch].adder = (apu.rectangle[ch].adder + 1) & 0x0F;                                                                 \
      }                                                                                                                                  \
                                                                                                                                         \
      if (apu.rectangle[ch].fixed_envelope)                                                                                              \
         output = apu.rectangle[ch].volume << 8; /* fixed volume */                                                                      \
      else                                                                                                                               \
         output = (apu.rectangle[ch].env_vol ^ 0x0F) << 8;                                                                               \
                                                                                                                                         \
      if (0 == apu.rectangle[ch].adder)                                                                                                  \
         apu.rectangle[ch].output_vol = output;                                                                                          \
      else if (apu.rectangle[ch].adder == apu.rectangle[ch].duty_flip)                                                                   \
         apu.rectangle[ch].output_vol = -output;                                                                                         \
                                                                                                                                         \
      return APU_RECTANGLE_OUTPUT(ch);                                                                                                   \
   }

#endif  
 
APU_MAKE_RECTANGLE(0)
APU_MAKE_RECTANGLE(1)
 
static int32 apu_triangle(void)
{
   APU_VOLUME_DECAY(apu.triangle.output_vol);

   if (false == apu.triangle.enabled || 0 == apu.triangle.vbl_length)
      return APU_TRIANGLE_OUTPUT;

   if (apu.triangle.counter_started)
   {
      if (apu.triangle.linear_length > 0)
         apu.triangle.linear_length--;
      if (apu.triangle.vbl_length && false == apu.triangle.holdnote)
         apu.triangle.vbl_length--;
   }
   else if (false == apu.triangle.holdnote && apu.triangle.write_latency)
   {
      if (--apu.triangle.write_latency == 0)
         apu.triangle.counter_started = true;
   }

   if (0 == apu.triangle.linear_length || apu.triangle.freq < 4) 
      return APU_TRIANGLE_OUTPUT;

   apu.triangle.accum -= apu.cycle_rate;
   while (apu.triangle.accum < 0)
   {
      apu.triangle.accum += apu.triangle.freq;
      apu.triangle.adder = (apu.triangle.adder + 1) & 0x1F;

      if (apu.triangle.adder & 0x10)
         apu.triangle.output_vol -= (2 << 8);
      else
         apu.triangle.output_vol += (2 << 8);
   }

   return APU_TRIANGLE_OUTPUT;
}
 
static int32 apu_noise(void)
{
   int32 outvol;

#if defined(APU_OVERSAMPLE) && defined(REALTIME_NOISE)
#else   
   int32 noise_bit;
#endif  
#ifdef APU_OVERSAMPLE
   int num_times;
   int32 total;
#endif  

   APU_VOLUME_DECAY(apu.noise.output_vol);

   if (false == apu.noise.enabled || 0 == apu.noise.vbl_length)
      return APU_NOISE_OUTPUT;
 
   if (false == apu.noise.holdnote)
      apu.noise.vbl_length--;
 
   apu.noise.env_phase -= 4;  
   while (apu.noise.env_phase < 0)
   {
      apu.noise.env_phase += apu.noise.env_delay;

      if (apu.noise.holdnote)
         apu.noise.env_vol = (apu.noise.env_vol + 1) & 0x0F;
      else if (apu.noise.env_vol < 0x0F)
         apu.noise.env_vol++;
   }

   apu.noise.accum -= apu.cycle_rate;
   if (apu.noise.accum >= 0)
      return APU_NOISE_OUTPUT;

#ifdef APU_OVERSAMPLE
   if (apu.noise.fixed_envelope)
      outvol = apu.noise.volume << 8;  
   else
      outvol = (apu.noise.env_vol ^ 0x0F) << 8;

   num_times = total = 0;
#endif  

   while (apu.noise.accum < 0)
   {
      apu.noise.accum += apu.noise.freq;

#ifdef REALTIME_NOISE

#ifdef APU_OVERSAMPLE
      if (shift_register15(apu.noise.xor_tap))
         total += outvol;
      else
         total -= outvol;

      num_times++;
#else  
      noise_bit = shift_register15(apu.noise.xor_tap);
#endif 

#else 
      apu.noise.cur_pos++;

      if (apu.noise.short_sample)
      {
         if (APU_NOISE_93 == apu.noise.cur_pos)
            apu.noise.cur_pos = 0;
      }
      else
      {
         if (APU_NOISE_32K == apu.noise.cur_pos)
            apu.noise.cur_pos = 0;
      }

#ifdef APU_OVERSAMPLE
      if (apu.noise.short_sample)
         noise_bit = noise_short_lut[apu.noise.cur_pos];
      else
         noise_bit = noise_long_lut[apu.noise.cur_pos];

      if (noise_bit)
         total += outvol;
      else
         total -= outvol;

      num_times++;
#endif  
#endif  
   }

#ifdef APU_OVERSAMPLE
   apu.noise.output_vol = total / num_times;
#else  
   if (apu.noise.fixed_envelope)
      outvol = apu.noise.volume << 8; 
   else
      outvol = (apu.noise.env_vol ^ 0x0F) << 8;

#ifndef REALTIME_NOISE
   if (apu.noise.short_sample)
      noise_bit = noise_short_lut[apu.noise.cur_pos];
   else
      noise_bit = noise_long_lut[apu.noise.cur_pos];
#endif  

   if (noise_bit)
      apu.noise.output_vol = outvol;
   else
      apu.noise.output_vol = -outvol;
#endif  

   return APU_NOISE_OUTPUT;
}

INLINE void apu_dmcreload(void)
{
   apu.dmc.address = apu.dmc.cached_addr;
   apu.dmc.dma_length = apu.dmc.cached_dmalength;
   apu.dmc.irq_occurred = false;
}
 
static int32 apu_dmc(void)
{
   int delta_bit;

   APU_VOLUME_DECAY(apu.dmc.output_vol);
 
   if (apu.dmc.dma_length)
   {
      apu.dmc.accum -= apu.cycle_rate;

      while (apu.dmc.accum < 0)
      {
         apu.dmc.accum += apu.dmc.freq;

         delta_bit = (apu.dmc.dma_length & 7) ^ 7;

         if (7 == delta_bit)
         {
            apu.dmc.cur_byte = nes6502_getbyte(apu.dmc.address);
 
            nes6502_burn(1);
 
            if (0xFFFF == apu.dmc.address)
               apu.dmc.address = 0x8000;
            else
               apu.dmc.address++;
         }

         if (--apu.dmc.dma_length == 0)
         { 
            if (apu.dmc.looping)
            {
               apu_dmcreload();
            }
            else
            { 
               if (apu.dmc.irq_gen)
               {
                  apu.dmc.irq_occurred = true;
                  if (apu.irq_callback)
                     apu.irq_callback();
               }
 
               apu.dmc.enabled = false;
               break;
            }
         }
 
         if (apu.dmc.cur_byte & (1 << delta_bit))
         {
            if (apu.dmc.regs[1] < 0x7D)
            {
               apu.dmc.regs[1] += 2;
               apu.dmc.output_vol += (2 << 8);
            }
         } 
         else
         {
            if (apu.dmc.regs[1] > 1)
            {
               apu.dmc.regs[1] -= 2;
               apu.dmc.output_vol -= (2 << 8);
            }
         }
      }
   }

   return APU_DMC_OUTPUT;
}

void apu_write(uint32 address, uint8 value)
{
   int chan;

   switch (address)
   { 
   case APU_WRA0:
   case APU_WRB0:
      chan = (address & 4) >> 2;
      apu.rectangle[chan].regs[0] = value;
      apu.rectangle[chan].volume = value & 0x0F;
      apu.rectangle[chan].env_delay = decay_lut[value & 0x0F];
      apu.rectangle[chan].holdnote = (value & 0x20) ? true : false;
      apu.rectangle[chan].fixed_envelope = (value & 0x10) ? true : false;
      apu.rectangle[chan].duty_flip = duty_flip[value >> 6];
      break;

   case APU_WRA1:
   case APU_WRB1:
      chan = (address & 4) >> 2;
      apu.rectangle[chan].regs[1] = value;
      apu.rectangle[chan].sweep_on = (value & 0x80) ? true : false;
      apu.rectangle[chan].sweep_shifts = value & 7;
      apu.rectangle[chan].sweep_delay = decay_lut[(value >> 4) & 7];
      apu.rectangle[chan].sweep_inc = (value & 0x08) ? true : false;
      apu.rectangle[chan].freq_limit = freq_limit[value & 7];
      break;

   case APU_WRA2:
   case APU_WRB2:
      chan = (address & 4) >> 2;
      apu.rectangle[chan].regs[2] = value;
      apu.rectangle[chan].freq = (apu.rectangle[chan].freq & ~0xFF) | value;
      break;

   case APU_WRA3:
   case APU_WRB3:
      chan = (address & 4) >> 2;
      apu.rectangle[chan].regs[3] = value;
      apu.rectangle[chan].vbl_length = vbl_lut[value >> 3];
      apu.rectangle[chan].env_vol = 0;
      apu.rectangle[chan].freq = ((value & 7) << 8) | (apu.rectangle[chan].freq & 0xFF);
      apu.rectangle[chan].adder = 0;
      break;

   /* triangle */
   case APU_WRC0:
      apu.triangle.regs[0] = value;
      apu.triangle.holdnote = (value & 0x80) ? true : false;

      if (false == apu.triangle.counter_started && apu.triangle.vbl_length)
         apu.triangle.linear_length = trilength_lut[value & 0x7F];

      break;

   case APU_WRC2:
      apu.triangle.regs[1] = value;
      apu.triangle.freq = (((apu.triangle.regs[2] & 7) << 8) + value) + 1;
      break;

   case APU_WRC3:

      apu.triangle.regs[2] = value; 
      apu.triangle.write_latency = (int)(228 / apu.cycle_rate);
      apu.triangle.freq = (((value & 7) << 8) + apu.triangle.regs[1]) + 1;
      apu.triangle.vbl_length = vbl_lut[value >> 3];
      apu.triangle.counter_started = false;
      apu.triangle.linear_length = trilength_lut[apu.triangle.regs[0] & 0x7F];
      break;
 
   case APU_WRD0:
      apu.noise.regs[0] = value;
      apu.noise.env_delay = decay_lut[value & 0x0F];
      apu.noise.holdnote = (value & 0x20) ? true : false;
      apu.noise.fixed_envelope = (value & 0x10) ? true : false;
      apu.noise.volume = value & 0x0F;
      break;

   case APU_WRD2:
      apu.noise.regs[1] = value;
      apu.noise.freq = noise_freq[value & 0x0F];

#ifdef REALTIME_NOISE
      apu.noise.xor_tap = (value & 0x80) ? 0x40 : 0x02;
#else 

      if ((value & 0x80) && false == apu.noise.short_sample)
      { 
         shift_register15(noise_short_lut, APU_NOISE_93);
         apu.noise.cur_pos = 0;
      }
      apu.noise.short_sample = (value & 0x80) ? true : false;
#endif  
      break;

   case APU_WRD3:
      apu.noise.regs[2] = value;
      apu.noise.vbl_length = vbl_lut[value >> 3];
      apu.noise.env_vol = 0;  
      break;
 
   case APU_WRE0:
      apu.dmc.regs[0] = value;
      apu.dmc.freq = dmc_clocks[value & 0x0F];
      apu.dmc.looping = (value & 0x40) ? true : false;

      if (value & 0x80)
      {
         apu.dmc.irq_gen = true;
      }
      else
      {
         apu.dmc.irq_gen = false;
         apu.dmc.irq_occurred = false;
      }
      break;

   case APU_WRE1:  
      value &= 0x7F;  
      apu.dmc.output_vol += ((value - apu.dmc.regs[1]) << 8);
      apu.dmc.regs[1] = value;
      break;

   case APU_WRE2:
      apu.dmc.regs[2] = value;
      apu.dmc.cached_addr = 0xC000 + (uint16)(value << 6);
      break;

   case APU_WRE3:
      apu.dmc.regs[3] = value;
      apu.dmc.cached_dmalength = ((value << 4) + 1) << 3;
      break;

   case APU_SMASK: 
      apu.dmc.enabled = (value & 0x10) ? true : false;
      apu.enable_reg = value;

      for (chan = 0; chan < 2; chan++)
      {
         if (value & (1 << chan))
         {
            apu.rectangle[chan].enabled = true;
         }
         else
         {
            apu.rectangle[chan].enabled = false;
            apu.rectangle[chan].vbl_length = 0;
         }
      }

      if (value & 0x04)
      {
         apu.triangle.enabled = true;
      }
      else
      {
         apu.triangle.enabled = false;
         apu.triangle.vbl_length = 0;
         apu.triangle.linear_length = 0;
         apu.triangle.counter_started = false;
         apu.triangle.write_latency = 0;
      }

      if (value & 0x08)
      {
         apu.noise.enabled = true;
      }
      else
      {
         apu.noise.enabled = false;
         apu.noise.vbl_length = 0;
      }

      if (value & 0x10)
      {
         if (0 == apu.dmc.dma_length)
            apu_dmcreload();
      }
      else
      {
         apu.dmc.dma_length = 0;
      }

      apu.dmc.irq_occurred = false;
      break;
 
   case 0x4009:
   case 0x400D:
      break;

   default:
      break;
   }
}
 
uint8 apu_read(uint32 address)
{
   uint8 value;

   switch (address)
   {
   case APU_SMASK:
      value = 0; 
      if (apu.rectangle[0].enabled && apu.rectangle[0].vbl_length)
         value |= 0x01;
      if (apu.rectangle[1].enabled && apu.rectangle[1].vbl_length)
         value |= 0x02;
      if (apu.triangle.enabled && apu.triangle.vbl_length)
         value |= 0x04;
      if (apu.noise.enabled && apu.noise.vbl_length)
         value |= 0x08; 
      if (apu.dmc.enabled)
         value |= 0x10;

      if (apu.dmc.irq_occurred)
         value |= 0x80;

      if (apu.irqclear_callback)
         value |= apu.irqclear_callback();

      break;

   default:
      value = (address >> 8);  
      break;
   }

   return value;
}

#define CLIP_OUTPUT16(out)    \
   {                          \
      if (out > 0x7FFF)       \
         out = 0x7FFF;        \
      else if (out < -0x8000) \
         out = -0x8000;       \
   }                          \

void apu_process(void *buffer, int num_samples)
{
   static int32 prev_sample = 0;

   int16 *buf16;
   uint8 *buf8;

   if (NULL != buffer)
   { 
      apu.buffer = buffer;

      buf16 = (int16 *)buffer;
      buf8 = (uint8 *)buffer;

      while (num_samples--)
      {
         int32 next_sample, accum = 0;

         if (apu.mix_enable & 0x01)
            accum += apu_rectangle_0();
         if (apu.mix_enable & 0x02)
            accum += apu_rectangle_1();
         if (apu.mix_enable & 0x04)
            accum += apu_triangle();
         if (apu.mix_enable & 0x08)
            accum += apu_noise();
         if (apu.mix_enable & 0x10)
            accum += apu_dmc();
         if (apu.ext && (apu.mix_enable & 0x20))
            accum += apu.ext->process();
 
         if (APU_FILTER_NONE != apu.filter_type)
         {
            next_sample = accum;

            if (APU_FILTER_LOWPASS == apu.filter_type)
            {
               accum += prev_sample;
               accum >>= 1;
            }
            else
               accum = (accum + accum + accum + prev_sample) >> 2;

            prev_sample = next_sample;
         }
 
         CLIP_OUTPUT16(accum);
 
         if (16 == apu.sample_bits)
            *buf16++ = (int16)accum;
         else
            *buf8++ = (accum >> 8) ^ 0x80;
      }
   }
}
 
void apu_setfilter(int filter_type)
{
   apu.filter_type = filter_type;
}

void apu_reset(void)
{
   uint32 address; 
   for (address = 0x4000; address <= 0x4013; address++)
      apu_write(address, 0);

   apu_write(0x4015, 0);

   if (apu.ext && NULL != apu.ext->reset)
      apu.ext->reset();
}

void apu_build_luts(int num_samples)
{
   int i;
 
   for (i = 0; i < 16; i++)
      decay_lut[i] = num_samples * (i + 1);
 
   for (i = 0; i < 32; i++)
      vbl_lut[i] = vbl_length[i] * num_samples;
 
   for (i = 0; i < 128; i++)
      trilength_lut[i] = (int)(0.25 * i * num_samples);

#ifndef REALTIME_NOISE
 
   shift_register15(noise_long_lut, APU_NOISE_32K);
   shift_register15(noise_short_lut, APU_NOISE_93);
#endif  
}

void apu_setparams(double base_freq, int sample_rate, int refresh_rate, int sample_bits)
{
   apu.sample_rate = sample_rate;
   apu.refresh_rate = refresh_rate;
   apu.sample_bits = sample_bits;
   apu.num_samples = sample_rate / refresh_rate;
   if (0 == base_freq)
      apu.base_freq = APU_BASEFREQ;
   else
      apu.base_freq = base_freq;
   apu.cycle_rate = (float)(apu.base_freq / sample_rate);
 
   apu_build_luts(apu.num_samples);

   apu_reset();
}
 
apu_t *apu_create(double base_freq, int sample_rate, int refresh_rate, int sample_bits)
{
   apu_t *temp_apu;
   int channel;

   temp_apu = NOFRENDO_MALLOC(sizeof(apu_t));
   if (NULL == temp_apu)
      return NULL;

   memset(temp_apu, 0, sizeof(apu_t));
 
   temp_apu->process = apu_process;
   temp_apu->ext = NULL; 
   temp_apu->irq_callback = NULL;
   temp_apu->irqclear_callback = NULL;

   apu_setcontext(temp_apu);

   apu_setparams(base_freq, sample_rate, refresh_rate, sample_bits);

   for (channel = 0; channel < 6; channel++)
      apu_setchan(channel, true);

   apu_setfilter(APU_FILTER_WEIGHTED);

   apu_getcontext(temp_apu);

   return temp_apu;
}

void apu_destroy(apu_t **src_apu)
{
   if (*src_apu)
   {
      if ((*src_apu)->ext && NULL != (*src_apu)->ext->shutdown)
         (*src_apu)->ext->shutdown();
      NOFRENDO_FREE(*src_apu);
      *src_apu = NULL;
   }
}

void apu_setext(apu_t *src_apu, apuext_t *ext)
{
   ASSERT(src_apu);

   src_apu->ext = ext;
 
   if (src_apu->ext && NULL != src_apu->ext->init)
      src_apu->ext->init();
}