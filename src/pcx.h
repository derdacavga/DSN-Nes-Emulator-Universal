#ifndef _PCX_H_
#define _PCX_H_

#include "osd.h"
#include "bitmap.h"
 
typedef struct pcxheader_s
{
   uint8 Manufacturer __PACKED__;
   uint8 Version __PACKED__;
   uint8 Encoding __PACKED__;
   uint8 BitsPerPixel __PACKED__;
   uint16 Xmin __PACKED__;
   uint16 Ymin __PACKED__;
   uint16 Xmax __PACKED__;
   uint16 Ymax __PACKED__;
   uint16 HDpi __PACKED__;
   uint16 VDpi __PACKED__;
   uint8 Colormap[48] __PACKED__;
   uint8 Reserved __PACKED__;
   uint8 NPlanes __PACKED__;
   uint16 BytesPerLine __PACKED__;
   uint16 PaletteInfo __PACKED__;
   uint16 HscreenSize __PACKED__;
   uint16 VscreenSize __PACKED__;
   uint8 Filler[54] __PACKED__;
} pcxheader_t;

extern int pcx_write(char *filename, bitmap_t *bmp, rgb_t *pal);

#endif