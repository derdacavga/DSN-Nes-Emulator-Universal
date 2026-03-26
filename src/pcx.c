#include <stdio.h>
#include <string.h>

#include "noftypes.h"
#include "bitmap.h"
#include "pcx.h"
 
int pcx_write(char *filename, bitmap_t *bmp, rgb_t *pal)
{
   FILE *fp;
   pcxheader_t header;
   int i, line;
   int width, height, x_min, y_min;

   ASSERT(bmp);

   width = bmp->width;
   height = bmp->height;
   x_min = 0;
   y_min = 0;

   fp = fopen(filename, "wb");
   if (NULL == fp)
      return -1;
 
   memset(&header, 0, sizeof(header));

   header.Manufacturer = 10;
   header.Version = 5;
   header.Encoding = 1;
   header.BitsPerPixel = 8;
   header.Xmin = x_min;
   header.Ymin = y_min;
   header.Xmax = width - 1;
   header.Ymax = height - 1;
   header.NPlanes = 1;
   header.BytesPerLine = width;
   header.PaletteInfo = 1;
   header.HscreenSize = width - 1;
   header.VscreenSize = height - 1;

   fwrite(&header, 1, sizeof(header), fp);
 
   for (line = 0; line < height; line++)
   {
      uint8 last, *mem;
      int xpos = 0;

      mem = bmp->line[line + y_min] + x_min;

      while (xpos < width)
      {
         int rle_count = 0;

         do
         {
            last = *mem++;
            xpos++;
            rle_count++;
         } while (*mem == last && xpos < width && rle_count < 0x3F);

         if (rle_count > 1 || 0xC0 == (last & 0xC0))
         {
            fputc(0xC0 | rle_count, fp);
            fputc(last, fp);
         }
         else
         {
            fputc(last, fp);
         }
      }
   }
 
   fputc(0x0C, fp);  
   for (i = 0; i < 256; i++)
   {
      fputc(pal[i].r, fp);
      fputc(pal[i].g, fp);
      fputc(pal[i].b, fp);
   }
 
   fclose(fp);
   return 0;
}