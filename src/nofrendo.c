#include <stdio.h>
#include <stdlib.h>

#include "noftypes.h"
#include "nofrendo.h"
#include "event.h"
#include "nofconfig.h"
#include "log.h"
#include "osd.h"
#include "gui.h"
#include "vid_drv.h"

#include "nes.h"

static struct
{
   char *filename, *nextfilename;
   system_t type, nexttype;

   union
   {
      nes_t *nes;
   } machine;

   int refresh_rate;

   bool quit;
} console;
 
volatile int nofrendo_ticks = 0;
static void timer_isr(void)
{
   nofrendo_ticks++;
}
static void timer_isr_end(void) {}  

static void shutdown_everything(void)
{
   if (console.filename)
   {
      NOFRENDO_FREE(console.filename);
      console.filename = NULL;
   }
   if (console.nextfilename)
   {
      NOFRENDO_FREE(console.nextfilename);
      console.nextfilename = NULL;
   }

   config.close();
   osd_shutdown();
   gui_shutdown();
   vid_shutdown();
   nofrendo_log_shutdown();
}
 
void main_eject(void)
{
   switch (console.type)
   {
   case system_nes:
      nes_poweroff();
      nes_destroy(&(console.machine.nes));
      break;

   default:
      break;
   }

   if (NULL != console.filename)
   {
      NOFRENDO_FREE(console.filename);
      console.filename = NULL;
   }
   console.type = system_unknown;
}
 
void main_quit(void)
{
   console.quit = true;

   main_eject();
 
   if (NULL != console.nextfilename)
   {
      NOFRENDO_FREE(console.nextfilename);
      console.nextfilename = NULL;
   }
   console.nexttype = system_unknown;
}
 
static system_t detect_systemtype(const char *filename)
{
   if (NULL == filename)
      return system_unknown;

   if (0 == nes_isourfile(filename))
      return system_nes;
 
   return system_unknown;
}

static int install_timer(int hertz)
{
   return osd_installtimer(hertz, (void *)timer_isr,
                           (int)timer_isr_end - (int)timer_isr,
                           (void *)&nofrendo_ticks,
                           sizeof(nofrendo_ticks));
}
 
static int internal_insert(const char *filename, system_t type)
{ 
   if (system_autodetect == type)
      type = detect_systemtype(filename);

   console.filename = NOFRENDO_STRDUP(filename);
   console.type = type;
 
   event_set_system(type);

   switch (console.type)
   {
   case system_nes:
      gui_setrefresh(NES_REFRESH_RATE);

      console.machine.nes = nes_create();
      if (NULL == console.machine.nes)
      {
         nofrendo_log_printf("Failed to create NES instance.\n");
         return -1;
      }

      if (nes_insertcart(console.filename, console.machine.nes))
         return -1;

      vid_setmode(NES_SCREEN_WIDTH, NES_SCREEN_HEIGHT);

      if (install_timer(NES_REFRESH_RATE))
         return -1;

      nes_emulate();
      break;

   case system_unknown:
   default:
      nofrendo_log_printf("system type unknown, playing nofrendo NES intro.\n");
      if (NULL != console.filename)
         NOFRENDO_FREE(console.filename);
 
      return internal_insert(filename, system_nes);
   }

   return 0;
}
 
void main_insert(const char *filename, system_t type)
{
   console.nextfilename = NOFRENDO_STRDUP(filename);
   console.nexttype = type;

   main_eject();
}

int nofrendo_main(int argc, char *argv[])
{ 
   console.filename = NULL;
   console.nextfilename = NULL;
   console.type = system_unknown;
   console.nexttype = system_unknown;
   console.refresh_rate = 0;
   console.quit = false;

   if (nofrendo_log_init())
      return -1;

   event_init();

   return osd_main(argc, argv);
}
 
int main_loop(const char *filename, system_t type)
{
   vidinfo_t video;
 
   atexit(shutdown_everything);
   nofrendo_log_printf("[BOOT] main_loop start\n");

   nofrendo_log_printf("[BOOT] config.open...\n");
   if (config.open())
   {
      nofrendo_log_printf("[BOOT] config.open FAILED\n");
      return -1;
   }
   nofrendo_log_printf("[BOOT] config.open OK\n");

   nofrendo_log_printf("[BOOT] osd_init...\n");
   if (osd_init())
   {
      nofrendo_log_printf("[BOOT] osd_init FAILED\n");
      return -1;
   }
   nofrendo_log_printf("[BOOT] osd_init OK\n");

   nofrendo_log_printf("[BOOT] gui_init...\n");
   if (gui_init())
   {
      nofrendo_log_printf("[BOOT] gui_init FAILED\n");
      return -1;
   }
   nofrendo_log_printf("[BOOT] gui_init OK\n");

   nofrendo_log_printf("[BOOT] osd_getvideoinfo...\n");
   osd_getvideoinfo(&video);
   nofrendo_log_printf("[BOOT] vid_init...\n");
   if (vid_init(video.default_width, video.default_height, video.driver))
   {
      nofrendo_log_printf("[BOOT] vid_init FAILED\n");
      return -1;
   }
   nofrendo_log_printf("[BOOT] vid_init OK\n");

   console.nextfilename = NOFRENDO_STRDUP(filename);
   console.nexttype = type;
   nofrendo_log_printf("[BOOT] entering main loop with %s\n", console.nextfilename ? console.nextfilename : "(null)");

   while (false == console.quit)
   {
      if (internal_insert(console.nextfilename, console.nexttype))
         return 1;
   }

   return 0;
}