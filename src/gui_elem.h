#ifndef _GUI_ELEM_H_
#define _GUI_ELEM_H_

typedef struct fontchar_s
{
   uint8 lines[6];
   uint8 spacing;
} fontchar_t;

typedef struct font_s
{
   fontchar_t *character;
   uint8 height;
} font_t;

extern font_t small;

#define CURSOR_WIDTH 11
#define CURSOR_HEIGHT 19

extern const uint8 cursor_color[];
extern const uint8 cursor[];

#endif