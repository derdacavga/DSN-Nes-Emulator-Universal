#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "noftypes.h"
#include "nes_ppu.h"
#include "nes_apu.h"
#include "nesinput.h"
#include "nes.h"
#include "log.h"
#include "osd.h"

#include "bitmap.h"

#include "gui.h"
#include "gui_elem.h"
#include "vid_drv.h"
  
rgb_t gui_pal[GUI_TOTALCOLORS] =
    {
        {0x00, 0x00, 0x00}, /* black      */
        {0x3F, 0x3F, 0x3F}, /* dark gray  */
        {0x7F, 0x7F, 0x7F}, /* gray       */
        {0xBF, 0xBF, 0xBF}, /* light gray */
        {0xFF, 0xFF, 0xFF}, /* white      */
        {0xFF, 0x00, 0x00}, /* red        */
        {0x00, 0xFF, 0x00}, /* green      */
        {0x00, 0x00, 0xFF}, /* blue       */
        {0xFF, 0xFF, 0x00}, /* yellow     */
        {0xFF, 0xAF, 0x00}, /* orange     */
        {0xFF, 0x00, 0xFF}, /* purple     */
        {0x3F, 0x7F, 0x7F}, /* teal       */
        {0x00, 0x2A, 0x00}, /* dk. green  */
        {0x00, 0x00, 0x3F}  /* dark blue  */
};
 
#include "pcx.h"
#include "nesstate.h"
static bool option_drawsprites = true;
 
void gui_savesnap(void)
{
   char filename[PATH_MAX];
   nes_t *nes = nes_getcontextptr();

   if (osd_makesnapname(filename, PATH_MAX) < 0)
      return;

#ifdef NOFRENDO_DOUBLE_FRAMEBUFFER
   if (pcx_write(filename, nes->vidbuf, nes->ppu->curpal))
#else  
   if (pcx_write(filename, vid_getbuffer(), nes->ppu->curpal))
#endif  

      return;

   gui_sendmsg(GUI_GREEN, "Screen saved to %s", filename);
}
 
void gui_togglesprites(void)
{
   option_drawsprites ^= true;
   ppu_displaysprites(option_drawsprites);
   gui_sendmsg(GUI_GREEN, "Sprites %s", option_drawsprites ? "displayed" : "hidden");
}
 
void gui_togglefs(void)
{
   nes_t *machine = nes_getcontextptr();

   machine->autoframeskip ^= true;
   if (machine->autoframeskip)
      gui_sendmsg(GUI_YELLOW, "automatic frameskip");
   else
      gui_sendmsg(GUI_YELLOW, "unthrottled emulation");
}
 
void gui_displayinfo()
{
   gui_sendmsg(GUI_ORANGE, (char *)rom_getinfo(nes_getcontextptr()->rominfo));
}

void gui_toggle_chan(int chan)
{
#define FILL_CHAR 0x7C   
#define BLANK_CHAR 0x7F  
   static bool chan_enabled[6] = {true, true, true, true, true, true};

   chan_enabled[chan] ^= true;
   apu_setchan(chan, chan_enabled[chan]);

   gui_sendmsg(GUI_ORANGE, "%ca %cb %cc %cd %ce %cext",
               chan_enabled[0] ? FILL_CHAR : BLANK_CHAR,
               chan_enabled[1] ? FILL_CHAR : BLANK_CHAR,
               chan_enabled[2] ? FILL_CHAR : BLANK_CHAR,
               chan_enabled[3] ? FILL_CHAR : BLANK_CHAR,
               chan_enabled[4] ? FILL_CHAR : BLANK_CHAR,
               chan_enabled[5] ? FILL_CHAR : BLANK_CHAR);
}

void gui_setfilter(int filter_type)
{
   char *types[3] = {"no", "lowpass", "weighted"};
   static int last_filter = 2;

   if (last_filter == filter_type || filter_type < 0 || filter_type > 2)
      return;

   apu_setfilter(filter_type);
   gui_sendmsg(GUI_ORANGE, "%s filter", types[filter_type]);
   last_filter = filter_type;
} 

enum
{
   GUI_WAVENONE,
   GUI_WAVELINE,
   GUI_WAVESOLID,
   GUI_NUMWAVESTYLES
};

enum
{
   BUTTON_UP,
   BUTTON_DOWN
};
 
static message_t msg;
static bool option_showfps = false;
static bool option_showgui = false;
static int option_wavetype = GUI_WAVENONE;
static bool option_showpattern = false;
static bool option_showoam = false;
static int pattern_col = 0;
 
static bool gui_fpsupdate = false;
static int gui_ticks = 0;
static int gui_fps = 0;
static int gui_refresh = 60;  

static int mouse_x, mouse_y, mouse_button;

static bitmap_t *gui_surface;
 
INLINE void gui_putpixel(int x_pos, int y_pos, uint8 color)
{
   gui_surface->line[y_pos][x_pos] = color;
}
 
static void gui_hline(int x_pos, int y_pos, int length, uint8 color)
{
   while (length--)
      gui_putpixel(x_pos++, y_pos, color);
}

static void gui_vline(int x_pos, int y_pos, int height, uint8 color)
{
   while (height--)
      gui_putpixel(x_pos, y_pos++, color);
}
 
static void gui_rect(int x_pos, int y_pos, int width, int height, uint8 color)
{
   gui_hline(x_pos, y_pos, width, color);
   gui_hline(x_pos, y_pos + height - 1, width, color);
   gui_vline(x_pos, y_pos + 1, height - 2, color);
   gui_vline(x_pos + width - 1, y_pos + 1, height - 2, color);
}

static void gui_rectfill(int x_pos, int y_pos, int width, int height, uint8 color)
{
   while (height--)
      gui_hline(x_pos, y_pos++, width, color);
}
 
static void gui_buttonrect(int x_pos, int y_pos, int width, int height, bool down)
{
   uint8 color1, color2;

   if (down)
   {
      color1 = GUI_GRAY;
      color2 = GUI_WHITE;
   }
   else
   {
      color1 = GUI_WHITE;
      color2 = GUI_GRAY;
   }

   gui_hline(x_pos, y_pos, width - 1, color1);
   gui_vline(x_pos, y_pos + 1, height - 2, color1);
   gui_hline(x_pos, y_pos + height - 1, width, color2);
   gui_vline(x_pos + width - 1, y_pos, height - 1, color2);
}
 
INLINE void gui_charline(char ch, int x_pos, int y_pos, uint8 color)
{
   int count = 8;
   while (count--)
   {
      if (ch & (1 << count))
         gui_putpixel(x_pos, y_pos, color);
      x_pos++;
   }
}

static void gui_putchar(uint8 *dat, int height, int x_pos, int y_pos, uint8 color)
{
   while (height--)
      gui_charline(*dat++, x_pos, y_pos++, color);
}
 
static int gui_textlen(char *str, font_t *font)
{
   int pixels = 0;
   int num_chars = strlen(str);

   while (num_chars--)
      pixels += font->character[(*str++ - 32)].spacing;

   return pixels;
}
 
static int gui_textout(char *str, int x_pos, int y_pos, font_t *font, uint8 color)
{
   int x_new;
   int num_chars = strlen(str);
   int code;

   x_new = x_pos;

   while (num_chars--)
   { 
      code = *str++;
      if (code > 0x7F)
         code = 0x7F;
      code -= 32;  
      gui_putchar(font->character[code].lines, font->height, x_new, y_pos, color);
      x_new += font->character[code].spacing;
   }
 
   return (x_new - x_pos);
}
 
static int gui_textbar(char *str, int x_pos, int y_pos, font_t *font,
                       uint8 color, uint8 bgcolor, bool buttonstate)
{
   int width = gui_textlen(str, &small);
 
   gui_buttonrect(x_pos, y_pos, width + 3, font->height + 3, buttonstate);
   gui_rectfill(x_pos + 1, y_pos + 1, width + 1, font->height + 1, bgcolor);
 
   return gui_textout(str, x_pos + 2, y_pos + 2, font, color);
}
 
static void gui_drawmouse(void)
{
   int ythresh, xthresh;
   int i, j, color;

   ythresh = gui_surface->height - mouse_y - 1;
   for (j = 0; j < CURSOR_HEIGHT; j++)
   {
      if (ythresh < 0)
         continue;

      xthresh = gui_surface->width - mouse_x - 1;
      for (i = 0; i < CURSOR_WIDTH; i++)
      {
         if (xthresh < 0)
            continue;

         color = cursor[(j * CURSOR_WIDTH) + i];

         if (color)
            gui_putpixel(mouse_x + i, mouse_y + j, cursor_color[color]);
         xthresh--;
      }
      ythresh--;
   }
}

void gui_tick(int ticks)
{

   static int fps_counter = 0;

   gui_ticks += ticks;
   fps_counter += ticks;

   if (fps_counter >= gui_refresh)
   {
      fps_counter -= gui_refresh;
      gui_fpsupdate = true;
   }
}
 
static void gui_tickdec(void)
{
#ifdef NOFRENDO_DEBUG
   static int hertz_ticks = 0;
#endif  
   int ticks = gui_ticks;

   if (0 == ticks)
      return;

   gui_ticks = 0;

#ifdef NOFRENDO_DEBUG 
   hertz_ticks += ticks;
   if (hertz_ticks >= (10 * gui_refresh))
   {
      hertz_ticks -= (10 * gui_refresh);
      mem_checkblocks();
   }
#endif  
 
   if (msg.ttl > 0)
   {
      msg.ttl -= ticks;
      if (msg.ttl < 0)
         msg.ttl = 0;
   }
}
 
static void gui_updatefps(void)
{
   static char fpsbuf[20];
 
   if (true == gui_fpsupdate)
   {
      sprintf(fpsbuf, "%4d FPS /%4d%%", gui_fps, (gui_fps * 100) / gui_refresh);
      gui_fps = 0;
      gui_fpsupdate = false;
   }

   gui_textout(fpsbuf, gui_surface->width - 1 - 90, 1, &small, GUI_GREEN);
}
 
void gui_togglefps(void)
{
   option_showfps ^= true;
}
 
void gui_togglegui(void)
{
   option_showgui ^= true;
}

void gui_togglewave(void)
{
   option_wavetype = (option_wavetype + 1) % GUI_NUMWAVESTYLES;
}

void gui_toggleoam(void)
{
   option_showoam ^= true;
}
 
void gui_togglepattern(void)
{
   option_showpattern ^= true;
}
 
void gui_decpatterncol(void)
{
   if (pattern_col && option_showpattern)
      pattern_col--;
}
 
void gui_incpatterncol(void)
{
   if ((pattern_col < 7) && option_showpattern)
      pattern_col++;
}
 
static void gui_updatemsg(void)
{
   if (msg.ttl)
      gui_textbar(msg.text, 2, gui_surface->height - 10, &small, msg.color, GUI_DKGRAY, BUTTON_UP);
}
 
static void gui_updatewave(int wave_type)
{
#define WAVEDISP_WIDTH 128
   int loop, xofs, yofs;
   int difference, offset;
   float scale;
   uint8 val, oldval;
   int vis_length = 0;
   void *vis_buffer = NULL;
   int vis_bps;
   apu_t apu;

   apu_getcontext(&apu);
   vis_buffer = apu.buffer;
   vis_length = apu.num_samples;
   vis_bps = apu.sample_bits;

   xofs = (NES_SCREEN_WIDTH - WAVEDISP_WIDTH);
   yofs = 1;
   scale = (float)(vis_length / (float)WAVEDISP_WIDTH);

   if (NULL == vis_buffer)
   { 
      gui_hline(xofs, yofs + 0x20, WAVEDISP_WIDTH, GUI_GRAY);
      gui_textbar("no sound", xofs + 40, yofs + 0x20 - 4, &small, GUI_RED, GUI_DKGRAY, BUTTON_UP);
   }
   else if (GUI_WAVELINE == wave_type)
   { 
      gui_hline(xofs, yofs + 0x20, WAVEDISP_WIDTH, GUI_GRAY);
 
      if (16 == vis_bps)
         oldval = 0x40 - (((((uint16 *)vis_buffer)[0] >> 8) ^ 0x80) >> 2);
      else
         oldval = 0x40 - (((uint8 *)vis_buffer)[0] >> 2);

      for (loop = 1; loop < WAVEDISP_WIDTH; loop++)
      {
         //val = 0x40 - (vis_buffer[(uint32) (loop * scale)] >> 2);
         if (16 == vis_bps)
            val = 0x40 - (((((uint16 *)vis_buffer)[(uint32)(loop * scale)] >> 8) ^ 0x80) >> 2);
         else
            val = 0x40 - (((uint8 *)vis_buffer)[(uint32)(loop * scale)] >> 2);
         if (oldval < val)
         {
            offset = oldval;
            difference = (val - oldval) + 1;
         }
         else
         {
            offset = val;
            difference = (oldval - val) + 1;
         }

         gui_vline(xofs + loop, yofs + offset, difference, GUI_GREEN);
         oldval = val;
      }
   } 
   else if (GUI_WAVESOLID == wave_type)
   {
      for (loop = 0; loop < WAVEDISP_WIDTH; loop++)
      {
         //val = vis_buffer[(uint32) (loop * scale)] >> 2;
         if (16 == vis_bps)
            val = ((((uint16 *)vis_buffer)[(uint32)(loop * scale)] >> 8) ^ 0x80) >> 2;
         else
            val = ((uint8 *)vis_buffer)[(uint32)(loop * scale)] >> 2;
         if (val == 0x20)
            gui_putpixel(xofs + loop, yofs + 0x20, GUI_GREEN);
         else if (val < 0x20)
            gui_vline(xofs + loop, yofs + 0x20, 0x20 - val, GUI_GREEN);
         else
            gui_vline(xofs + loop, yofs + 0x20 - (val - 0x20), val - 0x20,
                      GUI_GREEN);
      }
   }

   gui_rect(xofs, yofs - 1, WAVEDISP_WIDTH, 66, GUI_DKGRAY);
}

static void gui_updatepattern(void)
{ 
   gui_textbar("Pattern Table 0", 0, 0, &small, GUI_GREEN, GUI_DKGRAY, BUTTON_UP);
   gui_textbar("Pattern Table 1", 128, 0, &small, GUI_GREEN, GUI_DKGRAY, BUTTON_UP);
   gui_hline(0, 9, 256, GUI_DKGRAY);
   gui_hline(0, 138, 256, GUI_DKGRAY);
 
   ppu_dumppattern(gui_surface, 0, 0, 10, pattern_col);
   ppu_dumppattern(gui_surface, 1, 128, 10, pattern_col);
}

static void gui_updateoam(void)
{
   int y;

   y = option_showpattern ? 140 : 0;
   gui_textbar("Current OAM", 0, y, &small, GUI_GREEN, GUI_DKGRAY, BUTTON_UP);
   ppu_dumpoam(gui_surface, 0, y + 9);
}
 
void gui_frame(bool draw)
{
   gui_fps++;
   if (false == draw)
      return;

   gui_surface = vid_getbuffer();

   ASSERT(gui_surface);

   gui_tickdec();

   if (option_showfps)
      gui_updatefps();

   if (option_wavetype != GUI_WAVENONE)
      gui_updatewave(option_wavetype);

   if (option_showpattern)
      gui_updatepattern();

   if (option_showoam)
      gui_updateoam();

   if (msg.ttl)
      gui_updatemsg();

   if (option_showgui)
   {
      osd_getmouse(&mouse_x, &mouse_y, &mouse_button);
      gui_drawmouse();
   }
}

void gui_sendmsg(int color, char *format, ...)
{
   va_list arg;
   va_start(arg, format);
   vsprintf(msg.text, format, arg);

#ifdef NOFRENDO_DEBUG
   nofrendo_log_print("GUI: ");
   nofrendo_log_print(msg.text);
   nofrendo_log_print("\n");
#endif  

   va_end(arg);

   msg.ttl = gui_refresh * 2;  
   msg.color = color;
}

void gui_setrefresh(int frequency)
{
   gui_refresh = frequency;
}

int gui_init(void)
{
   gui_refresh = 60;
   memset(&msg, 0, sizeof(message_t));

   return 0; 
}

void gui_shutdown(void)
{
}