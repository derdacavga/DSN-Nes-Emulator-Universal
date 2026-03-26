#include <string.h>
#include <stdlib.h>
#include <noftypes.h>
#include <nes_ppu.h>
#include <nes.h>
#include <gui.h>
#include "nes6502.h"
#include <log.h>
#include <nes_mmc.h>
#include "nes6502.h"
#include <bitmap.h>
#include <vid_drv.h>
#include <nes_pal.h>
#include <nesinput.h>

#pragma GCC optimize("O3")
#pragma GCC optimize("unroll-loops")
#pragma GCC optimize("tree-vectorize")

 
#define  PPU_MEM(x)           ppu.page[(x) >> 10][(x)] 
#define  BG_TRANS             0x80
#define  SP_PIXEL             0x40
#define  BG_CLEAR(V)          ((V) & BG_TRANS)
#define  BG_SOLID(V)          (0 == BG_CLEAR(V))
#define  SP_CLEAR(V)          (0 == ((V) & SP_PIXEL))
 
#define  FULLBG               (ppu.palette[0] | BG_TRANS)
 
static ppu_t ppu;


void ppu_displaysprites(bool display)
{
   ppu.drawsprites = display;
}

void ppu_setcontext(ppu_t *src_ppu)
{
   int nametab[4];
   ASSERT(src_ppu);
   ppu = *src_ppu;
 
   nametab[0] = (src_ppu->page[8] - src_ppu->nametab + 0x2000) >> 10;
   nametab[1] = (src_ppu->page[9] - src_ppu->nametab + 0x2400) >> 10;
   nametab[2] = (src_ppu->page[10] - src_ppu->nametab + 0x2800) >> 10;
   nametab[3] = (src_ppu->page[11] - src_ppu->nametab + 0x2C00) >> 10;

   ppu.page[8] = ppu.nametab + (nametab[0] << 10) - 0x2000;
   ppu.page[9] = ppu.nametab + (nametab[1] << 10) - 0x2400;
   ppu.page[10] = ppu.nametab + (nametab[2] << 10) - 0x2800;
   ppu.page[11] = ppu.nametab + (nametab[3] << 10) - 0x2C00;
   ppu.page[12] = ppu.page[8] - 0x1000;
   ppu.page[13] = ppu.page[9] - 0x1000;
   ppu.page[14] = ppu.page[10] - 0x1000;
   ppu.page[15] = ppu.page[11] - 0x1000;
}

void ppu_getcontext(ppu_t *dest_ppu)
{
   int nametab[4];
   
   ASSERT(dest_ppu);
   *dest_ppu = ppu;
 
   nametab[0] = (ppu.page[8] - ppu.nametab + 0x2000) >> 10;
   nametab[1] = (ppu.page[9] - ppu.nametab + 0x2400) >> 10;
   nametab[2] = (ppu.page[10] - ppu.nametab + 0x2800) >> 10;
   nametab[3] = (ppu.page[11] - ppu.nametab + 0x2C00) >> 10;

   dest_ppu->page[8] = dest_ppu->nametab + (nametab[0] << 10) - 0x2000;
   dest_ppu->page[9] = dest_ppu->nametab + (nametab[1] << 10) - 0x2400;
   dest_ppu->page[10] = dest_ppu->nametab + (nametab[2] << 10) - 0x2800;
   dest_ppu->page[11] = dest_ppu->nametab + (nametab[3] << 10) - 0x2C00;
   dest_ppu->page[12] = dest_ppu->page[8] - 0x1000;
   dest_ppu->page[13] = dest_ppu->page[9] - 0x1000;
   dest_ppu->page[14] = dest_ppu->page[10] - 0x1000;
   dest_ppu->page[15] = dest_ppu->page[11] - 0x1000;
}

ppu_t *ppu_create(void)
{
   static bool pal_generated = false;
   ppu_t *temp;

   temp = malloc(sizeof(ppu_t));
   if (NULL == temp)
      return NULL;

   memset(temp, 0, sizeof(ppu_t));

   temp->latchfunc = NULL;
   temp->vromswitch = NULL;
   temp->vram_present = false;
   temp->drawsprites = true;
 
   if (false == pal_generated)
   {
      pal_generate();
      pal_generated = true;
   }

   ppu_setdefaultpal(temp);

   return temp;
}

void ppu_destroy(ppu_t **src_ppu)
{
   if (*src_ppu)
   {
      free(*src_ppu);
      *src_ppu = NULL;
   }
}

void ppu_setpage(int size, int page_num, uint8 *location)
{ 
   switch (size)
   {
   case 8:  
      ppu.page[page_num++] = location;
      ppu.page[page_num++] = location;
      ppu.page[page_num++] = location;
      ppu.page[page_num++] = location;
   case 4:  
      ppu.page[page_num++] = location;
      ppu.page[page_num++] = location;
   case 2:
      ppu.page[page_num++] = location;
   case 1:
      ppu.page[page_num++] = location;
      break;
   }
}
 
void ppu_mirrorhipages(void)
{
   ppu.page[12] = ppu.page[8] - 0x1000;
   ppu.page[13] = ppu.page[9] - 0x1000;
   ppu.page[14] = ppu.page[10] - 0x1000;
   ppu.page[15] = ppu.page[11] - 0x1000;
}

void ppu_mirror(int nt1, int nt2, int nt3, int nt4)
{
   ppu.page[8] = ppu.nametab + (nt1 << 10) - 0x2000;
   ppu.page[9] = ppu.nametab + (nt2 << 10) - 0x2400;
   ppu.page[10] = ppu.nametab + (nt3 << 10) - 0x2800;
   ppu.page[11] = ppu.nametab + (nt4 << 10) - 0x2C00;
   ppu.page[12] = ppu.page[8] - 0x1000;
   ppu.page[13] = ppu.page[9] - 0x1000;
   ppu.page[14] = ppu.page[10] - 0x1000;
   ppu.page[15] = ppu.page[11] - 0x1000;
}
 
uint8 *ppu_getpage(int page)
{
   return ppu.page[page];
}

static void mem_trash(uint8 *buffer, int length)
{
   int i;

   for (i = 0; i < length; i++)
      buffer[i] = (uint8) rand();
}
 
void ppu_reset(int reset_type)
{
   if (HARD_RESET == reset_type)
      mem_trash(ppu.oam, 256);

   ppu.ctrl0 = 0;
   ppu.ctrl1 = PPU_CTRL1F_OBJON | PPU_CTRL1F_BGON;
   ppu.stat = 0;
   ppu.flipflop = 0;
   ppu.vaddr = ppu.vaddr_latch = 0x2000;
   ppu.oam_addr = 0;
   ppu.tile_xofs = 0;

   ppu.latch = 0;
   ppu.vram_accessible = true;
}
 
static void ppu_setstrike(int x_loc)
{
   if (false == ppu.strikeflag)
   {
      ppu.strikeflag = true; 
      ppu.strike_cycle = nes6502_getcycles(false) + (x_loc / 3);
   }
}

static void ppu_oamdma(uint8 value)
{
   uint32 cpu_address;
   uint8 oam_loc;

   cpu_address = (uint32) (value << 8); 
   oam_loc = ppu.oam_addr;
   do
   {
      ppu.oam[oam_loc++] = nes6502_getbyte(cpu_address++);
   }
   while (oam_loc != ppu.oam_addr);
 
   cpu_address -= 256; 
   if ((ppu.oam_addr >> 2) & 1)
   {
      for (oam_loc = 4; oam_loc < 8; oam_loc++)
         ppu.oam[oam_loc] = nes6502_getbyte(cpu_address++);
      cpu_address += 248;
      for (oam_loc = 0; oam_loc < 4; oam_loc++)
         ppu.oam[oam_loc] = nes6502_getbyte(cpu_address++);
   } 
   else
   {
      for (oam_loc = 0; oam_loc < 8; oam_loc++)
         ppu.oam[oam_loc] = nes6502_getbyte(cpu_address++);
   } 
   nes6502_burn(513);
   nes6502_release();
}
 
void ppu_writehigh(uint32 address, uint8 value)
{
   switch (address)
   {
   case PPU_OAMDMA:
      ppu_oamdma(value);
      break;

   case PPU_JOY0: 
      if (ppu.vromswitch)
         ppu.vromswitch(value);
 
      value &= 1;
      
      if (0 == value && ppu.strobe)
         input_strobe();

      ppu.strobe = value;
      break;

   case PPU_JOY1:  
      nes_setfiq(value);
      break;

   default:
      break;
   }
}
 
uint8 ppu_readhigh(uint32 address)
{
   uint8 value;

   switch (address)
   {
   case PPU_JOY0:
      value = input_get(INP_JOYPAD0);
      break;

   case PPU_JOY1: 
      value = input_get(INP_ZAPPER | INP_JOYPAD1 
                        /*| INP_ARKANOID*/ 
                        /*| INP_POWERPAD*/);
      break;

   default:
      value = 0xFF;
      break;
   }

   return value;
}
 
uint8 ppu_read(uint32 address)
{
   uint8 value;
    
   switch (address & 0x2007)
   {
   case PPU_STAT:
      value = (ppu.stat & 0xE0) | (ppu.latch & 0x1F);

      if (ppu.strikeflag)
      {
         if (nes6502_getcycles(false) >= ppu.strike_cycle)
            value |= PPU_STATF_STRIKE;
      }
 
      ppu.stat &= ~PPU_STATF_VBLANK;
      ppu.flipflop = 0;
      break;

   case PPU_VDATA: 
      value = ppu.latch = ppu.vdata_latch; 
      if ((ppu.bg_on || ppu.obj_on) && !ppu.vram_accessible)
      {
         ppu.vdata_latch = 0xFF;
         log_printf("VRAM read at $%04X, scanline %d\n", 
                    ppu.vaddr, nes_getcontextptr()->scanline);
      }
      else
      {
         uint32 addr = ppu.vaddr;
         if (addr >= 0x3000)
            addr -= 0x1000;
         ppu.vdata_latch = PPU_MEM(addr);
      }

      ppu.vaddr += ppu.vaddr_inc;
      ppu.vaddr &= 0x3FFF;
      break;

   case PPU_OAMDATA:
   case PPU_CTRL0:
   case PPU_CTRL1:
   case PPU_OAMADDR:
   case PPU_SCROLL:
   case PPU_VADDR:
   default:
      value = ppu.latch;
      break;
   }

   return value;
}
 
void ppu_write(uint32 address, uint8 value)
{ 
   ppu.latch = value;
   
   switch (address & 0x2007)
   {
   case PPU_CTRL0:
      ppu.ctrl0 = value;

      ppu.obj_height = (value & PPU_CTRL0F_OBJ16) ? 16 : 8;
      ppu.bg_base = (value & PPU_CTRL0F_BGADDR) ? 0x1000 : 0;
      ppu.obj_base = (value & PPU_CTRL0F_OBJADDR) ? 0x1000 : 0;
      ppu.vaddr_inc = (value & PPU_CTRL0F_ADDRINC) ? 32 : 1;
      ppu.tile_nametab = value & PPU_CTRL0F_NAMETAB; 
      ppu.vaddr_latch &= ~0x0C00;
      ppu.vaddr_latch |= ((value & 3) << 10);
      break;

   case PPU_CTRL1:
      ppu.ctrl1 = value;

      ppu.obj_on = (value & PPU_CTRL1F_OBJON) ? true : false;
      ppu.bg_on = (value & PPU_CTRL1F_BGON) ? true : false;
      ppu.obj_mask = (value & PPU_CTRL1F_OBJMASK) ? false : true;
      ppu.bg_mask = (value & PPU_CTRL1F_BGMASK) ? false : true;
      break;

   case PPU_OAMADDR:
      ppu.oam_addr = value;
      break;

   case PPU_OAMDATA:
      ppu.oam[ppu.oam_addr++] = value;
      break;

   case PPU_SCROLL:
      if (0 == ppu.flipflop)
      { 
         ppu.vaddr_latch &= ~0x001F;
         ppu.vaddr_latch |= (value >> 3); 
         ppu.tile_xofs = (value & 7); 
      }
      else
      { 
         ppu.vaddr_latch &= ~0x73E0;
         ppu.vaddr_latch |= ((value & 0xF8) << 2); 
         ppu.vaddr_latch |= ((value & 7) << 12); 
      }

      ppu.flipflop ^= 1;

      break;

   case PPU_VADDR:
      if (0 == ppu.flipflop)
      { 
         ppu.vaddr_latch &= ~0xFF00;
         ppu.vaddr_latch |= ((value & 0x3F) << 8);
      }
      else
      { 
         ppu.vaddr_latch &= ~0x00FF;
         ppu.vaddr_latch |= value;
         ppu.vaddr = ppu.vaddr_latch;
      }
      
      ppu.flipflop ^= 1;

      break;

   case PPU_VDATA:
      if (ppu.vaddr < 0x3F00)
      { 
         if ((ppu.bg_on || ppu.obj_on) && !ppu.vram_accessible)
         {
            log_printf("VRAM write to $%04X, scanline %d\n", 
                       ppu.vaddr, nes_getcontextptr()->scanline);
            PPU_MEM(ppu.vaddr) = 0xFF; 
         }
         else 
         {
            uint32 addr = ppu.vaddr;

            if (false == ppu.vram_present && addr >= 0x3000)
               ppu.vaddr -= 0x1000;

            PPU_MEM(addr) = value;
         }
      }
      else
      {
         if (0 == (ppu.vaddr & 0x0F))
         {
            int i;

            for (i = 0; i < 8; i ++)
               ppu.palette[i << 2] = (value & 0x3F) | BG_TRANS;
         }
         else if (ppu.vaddr & 3)
         {
            ppu.palette[ppu.vaddr & 0x1F] = value & 0x3F;
         }
      }

      ppu.vaddr += ppu.vaddr_inc;
      ppu.vaddr &= 0x3FFF;
      break;

   default:
      break;
   }
}
 
static void ppu_buildpalette(ppu_t *src_ppu, rgb_t *pal)
{
   int i; 
   for (i = 0; i < 64; i++)
   {
      src_ppu->curpal[i].r = src_ppu->curpal[i + 64].r 
                           = src_ppu->curpal[i + 128].r = pal[i].r;
      src_ppu->curpal[i].g = src_ppu->curpal[i + 64].g
                           = src_ppu->curpal[i + 128].g = pal[i].g;
      src_ppu->curpal[i].b = src_ppu->curpal[i + 64].b
                           = src_ppu->curpal[i + 128].b = pal[i].b;
   }

   for (i = 0; i < GUI_TOTALCOLORS; i++)
   {
      src_ppu->curpal[i + 192].r = gui_pal[i].r;
      src_ppu->curpal[i + 192].g = gui_pal[i].g;
      src_ppu->curpal[i + 192].b = gui_pal[i].b;
   }
}
 
void ppu_setpal(ppu_t *src_ppu, rgb_t *pal)
{
   ppu_buildpalette(src_ppu, pal);
   vid_setpalette(src_ppu->curpal);
}

void ppu_setdefaultpal(ppu_t *src_ppu)
{
   ppu_setpal(src_ppu, nes_palette);
}

void ppu_setlatchfunc(ppulatchfunc_t func)
{
   ppu.latchfunc = func;
}

void ppu_setvromswitch(ppuvromswitch_t func)
{
   ppu.vromswitch = func;
}
 
INLINE void draw_bgtile(uint8 *surface, uint8 pat1, uint8 pat2, 
                        const uint8 *colors)
{
   uint32 pattern = ((pat2 & 0xAA) << 8) | ((pat2 & 0x55) << 1)
                    | ((pat1 & 0xAA) << 7) | (pat1 & 0x55);
   
   *surface++ = colors[(pattern >> 14) & 3];
   *surface++ = colors[(pattern >> 6) & 3];
   *surface++ = colors[(pattern >> 12) & 3];
   *surface++ = colors[(pattern >> 4) & 3];
   *surface++ = colors[(pattern >> 10) & 3];
   *surface++ = colors[(pattern >> 2) & 3];
   *surface++ = colors[(pattern >> 8) & 3];
   *surface = colors[pattern & 3];
}

INLINE int draw_oamtile(uint8 *surface, uint8 attrib, uint8 pat1, 
                        uint8 pat2, const uint8 *col_tbl, bool check_strike)
{
   int strike_pixel = -1;
   uint32 color = ((pat2 & 0xAA) << 8) | ((pat2 & 0x55) << 1)
                  | ((pat1 & 0xAA) << 7) | (pat1 & 0x55);
 
   if (color)
   {
      uint8 colors[8]; 
      if (0 == (attrib & OAMF_HFLIP))
      {
         colors[0] = (color >> 14) & 3;
         colors[1] = (color >> 6) & 3;
         colors[2] = (color >> 12) & 3;
         colors[3] = (color >> 4) & 3;
         colors[4] = (color >> 10) & 3;
         colors[5] = (color >> 2) & 3;
         colors[6] = (color >> 8) & 3;
         colors[7] = color & 3;
      }
      else
      {
         colors[7] = (color >> 14) & 3;
         colors[6] = (color >> 6) & 3;
         colors[5] = (color >> 12) & 3;
         colors[4] = (color >> 4) & 3;
         colors[3] = (color >> 10) & 3;
         colors[2] = (color >> 2) & 3;
         colors[1] = (color >> 8) & 3;
         colors[0] = color & 3;
      }
 
      if (check_strike)
      {
         if (colors[0] && BG_SOLID(surface[0]))
            strike_pixel = 0;
         else if (colors[1] && BG_SOLID(surface[1]))
            strike_pixel = 1;
         else if (colors[2] && BG_SOLID(surface[2]))
            strike_pixel = 2;
         else if (colors[3] && BG_SOLID(surface[3]))
            strike_pixel = 3;
         else if (colors[4] && BG_SOLID(surface[4]))
            strike_pixel = 4;
         else if (colors[5] && BG_SOLID(surface[5]))
            strike_pixel = 5;
         else if (colors[6] && BG_SOLID(surface[6]))
            strike_pixel = 6;
         else if (colors[7] && BG_SOLID(surface[7]))
            strike_pixel = 7;
      }
 
      if (attrib & OAMF_BEHIND)
      {
         if (colors[0])
            surface[0] = SP_PIXEL | (BG_CLEAR(surface[0]) ? col_tbl[colors[0]] : surface[0]);
         if (colors[1])
            surface[1] = SP_PIXEL | (BG_CLEAR(surface[1]) ? col_tbl[colors[1]] : surface[1]);
         if (colors[2])
            surface[2] = SP_PIXEL | (BG_CLEAR(surface[2]) ? col_tbl[colors[2]] : surface[2]);
         if (colors[3])
            surface[3] = SP_PIXEL | (BG_CLEAR(surface[3]) ? col_tbl[colors[3]] : surface[3]);
         if (colors[4])
            surface[4] = SP_PIXEL | (BG_CLEAR(surface[4]) ? col_tbl[colors[4]] : surface[4]);
         if (colors[5])
            surface[5] = SP_PIXEL | (BG_CLEAR(surface[5]) ? col_tbl[colors[5]] : surface[5]);
         if (colors[6])
            surface[6] = SP_PIXEL | (BG_CLEAR(surface[6]) ? col_tbl[colors[6]] : surface[6]);
         if (colors[7])
            surface[7] = SP_PIXEL | (BG_CLEAR(surface[7]) ? col_tbl[colors[7]] : surface[7]);
      }
      else
      {
         if (colors[0] && SP_CLEAR(surface[0]))
            surface[0] = SP_PIXEL | col_tbl[colors[0]];
         if (colors[1] && SP_CLEAR(surface[1]))
            surface[1] = SP_PIXEL | col_tbl[colors[1]];
         if (colors[2] && SP_CLEAR(surface[2]))
            surface[2] = SP_PIXEL | col_tbl[colors[2]];
         if (colors[3] && SP_CLEAR(surface[3]))
            surface[3] = SP_PIXEL | col_tbl[colors[3]];
         if (colors[4] && SP_CLEAR(surface[4]))
            surface[4] = SP_PIXEL | col_tbl[colors[4]];
         if (colors[5] && SP_CLEAR(surface[5]))
            surface[5] = SP_PIXEL | col_tbl[colors[5]];
         if (colors[6] && SP_CLEAR(surface[6]))
            surface[6] = SP_PIXEL | col_tbl[colors[6]];
         if (colors[7] && SP_CLEAR(surface[7]))
            surface[7] = SP_PIXEL | col_tbl[colors[7]];
      }
   }

   return strike_pixel;
}

static void ppu_renderbg(uint8 *vidbuf)
{
   uint8 *bmp_ptr, *data_ptr, *tile_ptr, *attrib_ptr;
   uint32 refresh_vaddr, bg_offset, attrib_base;
   int tile_count;
   uint8 tile_index, x_tile, y_tile;
   uint8 col_high, attrib, attrib_shift;
 
   if (false == ppu.bg_on)
   {
      memset(vidbuf, FULLBG, NES_SCREEN_WIDTH);
      return;
   }

   bmp_ptr = vidbuf - ppu.tile_xofs;  
   refresh_vaddr = 0x2000 + (ppu.vaddr & 0x0FE0);  
   x_tile = ppu.vaddr & 0x1F;
   y_tile = (ppu.vaddr >> 5) & 0x1F; 
   bg_offset = ((ppu.vaddr >> 12) & 7) + ppu.bg_base;  
 
   tile_ptr = &PPU_MEM(refresh_vaddr + x_tile);  
   attrib_base = (refresh_vaddr & 0x2C00) + 0x3C0 + ((y_tile & 0x1C) << 1);
   attrib_ptr = &PPU_MEM(attrib_base + (x_tile >> 2));
   attrib = *attrib_ptr++;
   attrib_shift = (x_tile & 2) + ((y_tile & 2) << 1);
   col_high = ((attrib >> attrib_shift) & 3) << 2;
 
   tile_count = 33;
   while (tile_count--)
   { 
      tile_index = *tile_ptr++;
      data_ptr = &PPU_MEM(bg_offset + (tile_index << 4));
 
      if (ppu.latchfunc)
         ppu.latchfunc(ppu.bg_base, tile_index);

      draw_bgtile(bmp_ptr, data_ptr[0], data_ptr[8], ppu.palette + col_high);
      bmp_ptr += 8;

      x_tile++;

      if (0 == (x_tile & 1))  
      {
         if (0 == (x_tile & 3))   
         {
            if (32 == x_tile)  
            {
               x_tile = 0;
               refresh_vaddr ^= (1 << 10);  
               attrib_base ^= (1 << 10);
 
               tile_ptr = &PPU_MEM(refresh_vaddr);
               attrib_ptr = &PPU_MEM(attrib_base);
            } 
            attrib = *attrib_ptr++;
         }

         attrib_shift ^= 2;
         col_high = ((attrib >> attrib_shift) & 3) << 2;
      }
   }
 
   if (ppu.bg_mask)
   {
      uint32 *buf_ptr = (uint32 *) vidbuf;
      uint32 bg_clear = FULLBG | FULLBG << 8 | FULLBG << 16 | FULLBG << 24;

      ((uint32 *) buf_ptr)[0] = bg_clear;
      ((uint32 *) buf_ptr)[1] = bg_clear;
   }
}
 
typedef struct obj_s
{
   uint8 y_loc;
   uint8 tile;
   uint8 atr;
   uint8 x_loc;
} obj_t;
 
static void ppu_renderoam(uint8 *vidbuf, int scanline)
{
   uint8 *buf_ptr;
   uint32 vram_offset, savecol[2];
   int sprite_num, spritecount;
   obj_t *sprite_ptr;
   uint8 sprite_height;

   if (false == ppu.obj_on)
      return;
 
   buf_ptr = vidbuf;
 
   if (ppu.obj_mask)
   {
      savecol[0] = ((uint32 *) buf_ptr)[0];
      savecol[1] = ((uint32 *) buf_ptr)[1];
   }

   sprite_height = ppu.obj_height;
   vram_offset = ppu.obj_base;
   spritecount = 0;

   sprite_ptr = (obj_t *) ppu.oam;

   for (sprite_num = 0; sprite_num < 64; sprite_num++, sprite_ptr++)
   {
      uint8 *data_ptr, *bmp_ptr;
      uint32 vram_adr;
      int y_offset;
      uint8 tile_index, attrib, col_high;
      uint8 sprite_y, sprite_x;
      bool check_strike;
      int strike_pixel;

      sprite_y = sprite_ptr->y_loc + 1;
 
      if ((sprite_y > scanline) || (sprite_y <= (scanline - sprite_height))
          || (0 == sprite_y) || (sprite_y >= 240))
         continue;

      sprite_x = sprite_ptr->x_loc;
      tile_index = sprite_ptr->tile;
      attrib = sprite_ptr->atr;

      bmp_ptr = buf_ptr + sprite_x;
 
      if (ppu.latchfunc)
         ppu.latchfunc(vram_offset, tile_index);
 
      col_high = ((attrib & 3) << 2);
 
      if (16 == ppu.obj_height)
         vram_adr = ((tile_index & 1) << 12) | ((tile_index & 0xFE) << 4);
      else
         vram_adr = vram_offset + (tile_index << 4);
 
      data_ptr = &PPU_MEM(vram_adr);
 
      y_offset = scanline - sprite_y;
      if (y_offset > 7)
         y_offset += 8;
 
      if (attrib & OAMF_VFLIP)
      {
         if (16 == ppu.obj_height)
            y_offset -= 23;
         else
            y_offset -= 7;

         data_ptr -= y_offset;
      }
      else
      {
         data_ptr += y_offset;
      }
 
      check_strike = (0 == sprite_num) && (false == ppu.strikeflag);
      strike_pixel = draw_oamtile(bmp_ptr, attrib, data_ptr[0], data_ptr[8], ppu.palette + 16 + col_high, check_strike);
      if (strike_pixel >= 0)
         ppu_setstrike(strike_pixel);
 
      if (++spritecount == PPU_MAXSPRITE)
      {
         ppu.stat |= PPU_STATF_MAXSPRITE;
         break;
      }
   } 
   if (ppu.obj_mask)
   {
      ((uint32 *) buf_ptr)[0] = savecol[0];
      ((uint32 *) buf_ptr)[1] = savecol[1];
   }
}
 
static void ppu_fakeoam(int scanline)
{
   uint8 *data_ptr;
   obj_t *sprite_ptr;
   uint32 vram_adr, color;
   int y_offset;
   uint8 pat1, pat2;
   uint8 tile_index, attrib;
   uint8 sprite_height, sprite_y, sprite_x; 

   if (false == ppu.obj_on || ppu.strikeflag)
      return;

   sprite_height = ppu.obj_height;
   sprite_ptr = (obj_t *) ppu.oam;
   sprite_y = sprite_ptr->y_loc + 1;
 
   if ((sprite_y > scanline) || (sprite_y <= (scanline - sprite_height)) 
       || (0 == sprite_y) || (sprite_y > 240))
      return;

   sprite_x = sprite_ptr->x_loc;
   tile_index = sprite_ptr->tile;
   attrib = sprite_ptr->atr;
 
   if (16 == ppu.obj_height)
      vram_adr = ((tile_index & 1) << 12) | ((tile_index & 0xFE) << 4);
   else
      vram_adr = ppu.obj_base + (tile_index << 4);

   data_ptr = &PPU_MEM(vram_adr);
 
   y_offset = scanline - sprite_y;
   if (y_offset > 7)
      y_offset += 8;
 
   if (attrib & OAMF_VFLIP)
   {
      if (16 == ppu.obj_height)
         y_offset -= 23;
      else
         y_offset -= 7;
      data_ptr -= y_offset;
   }
   else
   {
      data_ptr += y_offset;
   } 
   pat1 = data_ptr[0];
   pat2 = data_ptr[8];
   color = ((pat2 & 0xAA) << 8) | ((pat2 & 0x55) << 1)
                  | ((pat1 & 0xAA) << 7) | (pat1 & 0x55);

   if (color)
   {
      uint8 colors[8];
 
      if (0 == (attrib & OAMF_HFLIP))
      {
         colors[0] = (color >> 14) & 3;
         colors[1] = (color >> 6) & 3;
         colors[2] = (color >> 12) & 3;
         colors[3] = (color >> 4) & 3;
         colors[4] = (color >> 10) & 3;
         colors[5] = (color >> 2) & 3;
         colors[6] = (color >> 8) & 3;
         colors[7] = color & 3;
      }
      else
      {
         colors[7] = (color >> 14) & 3;
         colors[6] = (color >> 6) & 3;
         colors[5] = (color >> 12) & 3;
         colors[4] = (color >> 4) & 3;
         colors[3] = (color >> 10) & 3;
         colors[2] = (color >> 2) & 3;
         colors[1] = (color >> 8) & 3;
         colors[0] = color & 3;
      }

      if (colors[0])
         ppu_setstrike(sprite_x + 0);
      else if (colors[1])
         ppu_setstrike(sprite_x + 1);
      else if (colors[2])
         ppu_setstrike(sprite_x + 2);
      else if (colors[3])
         ppu_setstrike(sprite_x + 3);
      else if (colors[4])
         ppu_setstrike(sprite_x + 4);
      else if (colors[5])
         ppu_setstrike(sprite_x + 5);
      else if (colors[6])
         ppu_setstrike(sprite_x + 6);
      else if (colors[7])
         ppu_setstrike(sprite_x + 7);
   }
}

bool ppu_enabled(void)
{
   return (ppu.bg_on || ppu.obj_on);
}

static void ppu_renderscanline(bitmap_t *bmp, int scanline, bool draw_flag)
{
   uint8 *buf = bmp->line[scanline];
 
   if (ppu.bg_on || ppu.obj_on)
   {
      if (0 == scanline)
      {
         ppu.vaddr = ppu.vaddr_latch;
      }
      else
      {
         ppu.vaddr &= ~0x041F;
         ppu.vaddr |= (ppu.vaddr_latch & 0x041F);
      }
   }

   if (draw_flag)
      ppu_renderbg(buf);
 
   if (true == ppu.drawsprites && true == draw_flag)
      ppu_renderoam(buf, scanline);
   else
      ppu_fakeoam(scanline);
}


void ppu_endscanline(int scanline)
{ 
   if (scanline < 240 && (ppu.bg_on || ppu.obj_on))
   {
      int ytile;
 
      if (7 == (ppu.vaddr >> 12))
      {
         ppu.vaddr &= ~0x7000;    
         ytile = (ppu.vaddr >> 5) & 0x1F;

         if (29 == ytile)
         {
            ppu.vaddr &= ~0x03E0;   
            ppu.vaddr ^= 0x0800;   
         }
         else if (31 == ytile)
         {
            ppu.vaddr &= ~0x03E0;   
         }
         else
         {
            ppu.vaddr += 0x20;     
         }
      }
      else
      {
         ppu.vaddr += 0x1000;   
      }
   }
}

void ppu_checknmi(void)
{
   if (ppu.ctrl0 & PPU_CTRL0F_NMI)
      nes_nmi();
}

void ppu_scanline(bitmap_t *bmp, int scanline, bool draw_flag)
{
   if (scanline < 240)
   { 
      ppu.stat &= ~PPU_STATF_MAXSPRITE;
      ppu_renderscanline(bmp, scanline, draw_flag);
   }
   else if (241 == scanline)
   {
      ppu.stat |= PPU_STATF_VBLANK;
      ppu.vram_accessible = true;
   }
   else if (261 == scanline)
   {
      ppu.stat &= ~PPU_STATF_VBLANK;
      ppu.strikeflag = false;
      ppu.strike_cycle = (uint32) -1;

      ppu.vram_accessible = false;
   }
}
 
INLINE void draw_box(bitmap_t *bmp, int x, int y, int height)
{
   int i;
   uint8 *vid;

   vid = bmp->line[y] + x;

   for (i = 0; i < 10; i++)
      *vid++ = GUI_GRAY;
   vid += (bmp->pitch - 10);
   for (i = 0; i < height; i++)
   {
      vid[0] = vid[9] = GUI_GRAY;
      vid += bmp->pitch;
   }
   for (i = 0; i < 10; i++)
      *vid++ = GUI_GRAY;
}

INLINE void draw_deadsprite(bitmap_t *bmp, int x, int y, int height)
{
   int i, j, index;
   uint8 *vid;
   uint8 colbuf[8] = { GUI_BLACK, GUI_BLACK, GUI_BLACK, GUI_BLACK,
                       GUI_BLACK, GUI_BLACK, GUI_BLACK, GUI_DKGRAY };

   vid = bmp->line[y] + x;

   for (i = 0; i < height; i++)
   {
      index = i;

      if (height == 16)
         index >>= 1;

      for (j = 0; j < 8; j++)
      {
         *(vid + j) = colbuf[index++];
         index &= 7;
      }

      vid += bmp->pitch;
   }
}
 
static void draw_sprite(bitmap_t *bmp, int x, int y, uint8 tile_num, uint8 attrib)
{
   int line, height;
   int col_high, vram_adr;
   uint8 *vid, *data_ptr;

   vid = bmp->line[y] + x;
 
   col_high = ((attrib & 3) << 2);
 
   height = ppu.obj_height;
   if (16 == height)
      vram_adr = ((tile_num & 1) << 12) | ((tile_num & 0xFE) << 4); 
   else
      vram_adr = ppu.obj_base + (tile_num << 4);

   data_ptr = &PPU_MEM(vram_adr);

   for (line = 0; line < height; line++)
   {
      if (line == 8)
         data_ptr += 8;

      draw_bgtile(vid, data_ptr[0], data_ptr[8], ppu.palette + 16 + col_high); 
      data_ptr++;
      vid += bmp->pitch;
   }
}

void ppu_dumpoam(bitmap_t *bmp, int x_loc, int y_loc)
{
   int sprite, x_pos, y_pos, height;
   obj_t *spr_ptr;

   spr_ptr = (obj_t *) ppu.oam;
   height = ppu.obj_height;

   for (sprite = 0; sprite < 64; sprite++)
   {
      x_pos = ((sprite & 0x0F) << 3) + (sprite & 0x0F) + x_loc;
      if (height == 16)
         y_pos = (sprite & 0xF0) + (sprite >> 4) + y_loc;
      else
         y_pos = ((sprite & 0xF0) >> 1) + (sprite >> 4) + y_loc;

      draw_box(bmp, x_pos, y_pos, height);

      if (spr_ptr->y_loc && spr_ptr->y_loc < 240)
         draw_sprite(bmp, x_pos + 1, y_pos + 1, spr_ptr->tile, spr_ptr->atr);
      else
         draw_deadsprite(bmp, x_pos + 1, y_pos + 1, height);

      spr_ptr++;
   }
}
 
void ppu_dumppattern(bitmap_t *bmp, int table_num, int x_loc, int y_loc, int col)
{
   int x_tile, y_tile;
   uint8 *bmp_ptr, *data_ptr, *ptr;
   int tile_num, line;
   uint8 col_high;

   tile_num = 0;
   col_high = col << 2;

   for (y_tile = 0; y_tile < 16; y_tile++)
   { 
      bmp_ptr = bmp->line[y_loc] + x_loc;

      for (x_tile = 0; x_tile < 16; x_tile++)
      {
         data_ptr = &PPU_MEM((table_num << 12) + (tile_num << 4));
         ptr = bmp_ptr;

         for (line = 0; line < 8; line ++)
         {
            draw_bgtile(ptr, data_ptr[0], data_ptr[8], ppu.palette + col_high);
            data_ptr++;
            ptr += bmp->pitch;
         }

         bmp_ptr += 8;
         tile_num++;
      }
      y_loc += 8;
   }
} 