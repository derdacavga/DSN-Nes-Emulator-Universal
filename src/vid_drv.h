#ifndef _VID_DRV_H_
#define _VID_DRV_H_

#include "bitmap.h"

typedef struct viddriver_s
{ 
   const char *name; 
   int (*init)(int width, int height); 
   void (*shutdown)(void); 
   int (*set_mode)(int width, int height); 
   void (*set_palette)(rgb_t *palette); 
   void (*clear)(uint8 color); 
   bitmap_t *(*lock_write)(void); 
   void (*free_write)(int num_dirties, rect_t *dirty_rects); 
   void (*custom_blit)(bitmap_t *primary, int num_dirties,
                       rect_t *dirty_rects); 
   bool invalidate;
} viddriver_t;
 
extern bitmap_t *vid_getbuffer(void);

extern int vid_init(int width, int height, viddriver_t *osd_driver);
extern void vid_shutdown(void);

extern int vid_setmode(int width, int height);
extern void vid_setpalette(rgb_t *pal);

extern void vid_blit(bitmap_t *bitmap, int src_x, int src_y, int dest_x,
                     int dest_y, int blit_width, int blit_height);
extern void vid_flush(void);

#endif