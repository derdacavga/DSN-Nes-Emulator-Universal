#ifndef _OSD_H_
#define _OSD_H_

#include <limits.h>
#ifndef PATH_MAX
#define PATH_MAX 512
#endif  

#ifdef __GNUC__
#define __PACKED__ __attribute__((packed))

#ifdef __DJGPP__
#define PATH_SEP '\\'
#else  
#define PATH_SEP '/'
#endif  

#elif defined(WIN32)
#define __PACKED__
#define PATH_SEP '\\'
#else  
#define __PACKED__
#define PATH_SEP ':'
#endif  

#if !defined(WIN32) && !defined(__DJGPP__)
#define stricmp strcasecmp
#endif  
 
extern void *mem_alloc(int size, bool prefer_fast_memory);
 
extern void osd_setsound(void (*playfunc)(void *buffer, int size));

#ifndef NSF_PLAYER
#include "noftypes.h"
#include "vid_drv.h"

typedef struct vidinfo_s
{
   int default_width, default_height;
   viddriver_t *driver;
} vidinfo_t;

typedef struct sndinfo_s
{
   int sample_rate;
   int bps;
} sndinfo_t;
 
extern void osd_getvideoinfo(vidinfo_t *info);
extern void osd_getsoundinfo(sndinfo_t *info);
 
extern int osd_init(void);
extern void osd_shutdown(void);
extern int osd_main(int argc, char *argv[]);

extern int osd_installtimer(int frequency, void *func, int funcsize,
                            void *counter, int countersize);
 
extern void osd_getinput(void);
extern void osd_getmouse(int *x, int *y, int *button);
 
extern void osd_fullname(char *fullname, const char *shortname);
extern char *osd_newextension(char *string, char *ext);
 
extern int osd_makesnapname(char *filename, int len);

#endif  

#endif