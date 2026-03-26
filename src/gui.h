#ifndef _GUI_H_
#define _GUI_H_

#define GUI_FIRSTENTRY 192

enum
{
   GUI_BLACK = GUI_FIRSTENTRY,
   GUI_DKGRAY,
   GUI_GRAY,
   GUI_LTGRAY,
   GUI_WHITE,
   GUI_RED,
   GUI_GREEN,
   GUI_BLUE,
   GUI_YELLOW,
   GUI_ORANGE,
   GUI_PURPLE,
   GUI_TEAL,
   GUI_DKGREEN,
   GUI_DKBLUE,
   GUI_LASTENTRY
};

#define GUI_TOTALCOLORS (GUI_LASTENTRY - GUI_FIRSTENTRY)
 
#include "bitmap.h"
extern rgb_t gui_pal[GUI_TOTALCOLORS];

#define MAX_MSG_LENGTH 256

typedef struct message_s
{
   int ttl;
   char text[MAX_MSG_LENGTH];
   uint8 color;
} message_t;

extern void gui_tick(int ticks);
extern void gui_setrefresh(int frequency);

extern void gui_sendmsg(int color, char *format, ...);

extern int gui_init(void);
extern void gui_shutdown(void);

extern void gui_frame(bool draw);

extern void gui_togglefps(void);
extern void gui_togglegui(void);
extern void gui_togglewave(void);
extern void gui_togglepattern(void);
extern void gui_toggleoam(void);

extern void gui_decpatterncol(void);
extern void gui_incpatterncol(void);

extern void gui_savesnap(void);
extern void gui_togglesprites(void);
extern void gui_togglefs(void);
extern void gui_displayinfo();
extern void gui_toggle_chan(int chan);
extern void gui_setfilter(int filter_type);

#endif