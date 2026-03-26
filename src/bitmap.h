#ifndef _BITMAP_H_
#define _BITMAP_H_

#include <stdint.h>   
#include <stdbool.h> 
#include "noftypes.h"  
 
typedef struct rect_s
{
   int16 x, y;
   uint16 w, h;
} rect_t;
 
typedef struct rgb_s
{
   int r, g, b;
} rgb_t;
 
typedef struct bitmap_s
{
   int width;   
   int height;  
   int pitch; 
   bool hardware;     
   uint8 *data;   
 
   uint8 *line[ZERO_LENGTH]; 
} bitmap_t;
 
extern void bmp_clear(const bitmap_t *bitmap, uint8 color);
extern bitmap_t *bmp_create(int width, int height, int overdraw);
extern bitmap_t *bmp_createhw(uint8 *addr, int width, int height, int pitch);
extern void bmp_destroy(bitmap_t **bitmap);

#endif