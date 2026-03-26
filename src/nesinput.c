#include <noftypes.h>
#include <nesinput.h>
#include <log.h>

static nesinput_t *nes_input[MAX_CONTROLLERS];
static int active_entries = 0; 
static int pad0_readcount, pad1_readcount, ppad_readcount, ark_readcount;


static int retrieve_type(int type)
{
   int i, value = 0;

   for (i = 0; i < active_entries; i++)
   {
      ASSERT(nes_input[i]);

      if (type == nes_input[i]->type)
         value |= nes_input[i]->data;
   }

   return value;
}

static uint8 get_pad0(void)
{
   uint8 value;

   value = (uint8) retrieve_type(INP_JOYPAD0);
 
   if ((value & INP_PAD_UP) && (value & INP_PAD_DOWN))
      value &= ~(INP_PAD_UP | INP_PAD_DOWN);

   if ((value & INP_PAD_LEFT) && (value & INP_PAD_RIGHT))
      value &= ~(INP_PAD_LEFT | INP_PAD_RIGHT);
 
   return (0x40 | ((value >> pad0_readcount++) & 1));
}

static uint8 get_pad1(void)
{
   uint8 value;

   value = (uint8) retrieve_type(INP_JOYPAD1);
 
   if ((value & INP_PAD_UP) && (value & INP_PAD_DOWN))
      value &= ~(INP_PAD_UP | INP_PAD_DOWN);

   if ((value & INP_PAD_LEFT) && (value & INP_PAD_RIGHT))
      value &= ~(INP_PAD_LEFT | INP_PAD_RIGHT);
 
   return (0x40 | ((value >> pad1_readcount++) & 1));
}

static uint8 get_zapper(void)
{
   return (uint8) (retrieve_type(INP_ZAPPER));
}

static uint8 get_powerpad(void)
{
   int value;
   uint8 ret_val = 0;
   
   value = retrieve_type(INP_POWERPAD);

   if (((value >> 8) >> ppad_readcount) & 1)
      ret_val |= 0x10;
   if (((value & 0xFF) >> ppad_readcount) & 1)
      ret_val |= 0x08;

   ppad_readcount++;

   return ret_val;
}

static uint8 get_vsdips0(void)
{
   return (retrieve_type(INP_VSDIPSW0));
}

static uint8 get_vsdips1(void)
{
   return (retrieve_type(INP_VSDIPSW1));
}

static uint8 get_arkanoid(void)
{
   uint8 value = retrieve_type(INP_ARKANOID);

   if ((value >> (7 - ark_readcount++)) & 1)
      return 0x02;
   else
      return 0;
}
 
uint8 input_get(int types)
{
   uint8 value = 0;

   if (types & INP_JOYPAD0)
      value |= get_pad0();
   if (types & INP_JOYPAD1)
      value |= get_pad1();
   if (types & INP_ZAPPER)
      value |= get_zapper();
   if (types & INP_POWERPAD)
      value |= get_powerpad();
   if (types & INP_VSDIPSW0)
      value |= get_vsdips0();
   if (types & INP_VSDIPSW1)
      value |= get_vsdips1();
   if (types & INP_ARKANOID)
      value |= get_arkanoid();

   return value;
}
 
void input_register(nesinput_t *input)
{
   if (NULL == input)
      return;

   nes_input[active_entries] = input;
   active_entries++;
}

void input_event(nesinput_t *input, int state, int value)
{
   ASSERT(input);

   if (state == INP_STATE_MAKE)
      input->data |= value;  
   else  
      input->data &= ~value;  
}

void input_strobe(void)
{
   pad0_readcount = 0;
   pad1_readcount = 0;
   ppad_readcount = 0;
   ark_readcount = 0;
}