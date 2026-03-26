#ifndef _NES_PPU_H_
#define _NES_PPU_H_

#include <bitmap.h>
 
#define  PPU_CTRL0            0x2000
#define  PPU_CTRL1            0x2001
#define  PPU_STAT             0x2002
#define  PPU_OAMADDR          0x2003
#define  PPU_OAMDATA          0x2004
#define  PPU_SCROLL           0x2005
#define  PPU_VADDR            0x2006
#define  PPU_VDATA            0x2007

#define  PPU_OAMDMA           0x4014
#define  PPU_JOY0             0x4016
#define  PPU_JOY1             0x4017
 
#define  PPU_CTRL0F_NMI       0x80
#define  PPU_CTRL0F_OBJ16     0x20
#define  PPU_CTRL0F_BGADDR    0x10
#define  PPU_CTRL0F_OBJADDR   0x08
#define  PPU_CTRL0F_ADDRINC   0x04
#define  PPU_CTRL0F_NAMETAB   0x03
 
#define  PPU_CTRL1F_OBJON     0x10
#define  PPU_CTRL1F_BGON      0x08
#define  PPU_CTRL1F_OBJMASK   0x04
#define  PPU_CTRL1F_BGMASK    0x02
 
#define  PPU_STATF_VBLANK     0x80
#define  PPU_STATF_STRIKE     0x40
#define  PPU_STATF_MAXSPRITE  0x20
 
#define  OAMF_VFLIP           0x80
#define  OAMF_HFLIP           0x40
#define  OAMF_BEHIND          0x20
 
#define  PPU_MAXSPRITE        8
 
typedef void (*ppulatchfunc_t)(uint32 address, uint8 value);
typedef void (*ppuvromswitch_t)(uint8 value);

typedef struct ppu_s
{ 
   uint8 nametab[0x1000];
   uint8 oam[256];
   uint8 palette[32];
   uint8 *page[16];
 
   uint8 ctrl0, ctrl1, stat, oam_addr;
   uint32 vaddr, vaddr_latch;
   int tile_xofs, flipflop;
   int vaddr_inc;
   uint32 tile_nametab;

   uint8 obj_height;
   uint32 obj_base, bg_base;

   bool bg_on, obj_on;
   bool obj_mask, bg_mask;
   
   uint8 latch, vdata_latch;
   uint8 strobe;

   bool strikeflag;
   uint32 strike_cycle;
 
   ppulatchfunc_t latchfunc;
   ppuvromswitch_t vromswitch;
 
   rgb_t curpal[256];

   bool vram_accessible;

   bool vram_present;
   bool drawsprites;
} ppu_t;

 
extern void ppu_setlatchfunc(ppulatchfunc_t func);
extern void ppu_setvromswitch(ppuvromswitch_t func);

extern void ppu_getcontext(ppu_t *dest_ppu);
extern void ppu_setcontext(ppu_t *src_ppu);
 
extern void ppu_mirrorhipages(void);

extern void ppu_mirror(int nt1, int nt2, int nt3, int nt4);

extern void ppu_setpage(int size, int page_num, uint8 *location);
extern uint8 *ppu_getpage(int page);
 
extern void ppu_reset(int reset_type);
extern bool ppu_enabled(void);
extern void ppu_scanline(bitmap_t *bmp, int scanline, bool draw_flag);
extern void ppu_endscanline(int scanline);
extern void ppu_checknmi();

extern ppu_t *ppu_create(void);
extern void ppu_destroy(ppu_t **ppu);
 
extern uint8 ppu_read(uint32 address);
extern void ppu_write(uint32 address, uint8 value);
extern uint8 ppu_readhigh(uint32 address);
extern void ppu_writehigh(uint32 address, uint8 value);
 
extern void ppu_setpal(ppu_t *src_ppu, rgb_t *pal);
extern void ppu_setdefaultpal(ppu_t *src_ppu);
 
extern void ppu_dumppattern(bitmap_t *bmp, int table_num, int x_loc, int y_loc, int col);
extern void ppu_dumpoam(bitmap_t *bmp, int x_loc, int y_loc);
extern void ppu_displaysprites(bool display);

#endif 