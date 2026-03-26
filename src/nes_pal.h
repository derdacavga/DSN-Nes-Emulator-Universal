#ifndef _NESPAL_H_
#define _NESPAL_H_

extern rgb_t nes_palette[];
extern rgb_t shady_palette[];

extern void pal_generate(void); 
extern void pal_dechue(void);
extern void pal_inchue(void);
extern void pal_dectint(void);
extern void pal_inctint(void);

#endif