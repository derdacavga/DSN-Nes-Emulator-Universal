#include <string.h>
#include <noftypes.h>
#include "nes6502.h"
#include <nes_mmc.h>
#include <nes_ppu.h>
#include <libsnss.h>
#include <log.h>
#include <mmclist.h>
#include <nes_rom.h>

#pragma GCC optimize("O3")
#pragma GCC optimize("inline-functions")

#define  MMC_8KROM         (mmc.cart->rom_banks * 2)
#define  MMC_16KROM        (mmc.cart->rom_banks)
#define  MMC_32KROM        (mmc.cart->rom_banks / 2)
#define  MMC_8KVROM        (mmc.cart->vrom_banks)
#define  MMC_4KVROM        (mmc.cart->vrom_banks * 2)
#define  MMC_2KVROM        (mmc.cart->vrom_banks * 4)
#define  MMC_1KVROM        (mmc.cart->vrom_banks * 8)

#define  MMC_LAST8KROM     (MMC_8KROM - 1)
#define  MMC_LAST16KROM    (MMC_16KROM - 1)
#define  MMC_LAST32KROM    (MMC_32KROM - 1)
#define  MMC_LAST8KVROM    (MMC_8KVROM - 1)
#define  MMC_LAST4KVROM    (MMC_4KVROM - 1)
#define  MMC_LAST2KVROM    (MMC_2KVROM - 1)
#define  MMC_LAST1KVROM    (MMC_1KVROM - 1)

static mmc_t mmc;

rominfo_t *mmc_getinfo(void)
{
   return mmc.cart;
}

void mmc_setcontext(mmc_t *src_mmc)
{
   ASSERT(src_mmc);

   mmc = *src_mmc;
}

void mmc_getcontext(mmc_t *dest_mmc)
{
   *dest_mmc = mmc;
}
 
void mmc_bankvrom(int size, uint32 address, int bank)
{
   if (0 == mmc.cart->vrom_banks)
      return;

   switch (size)
   {
   case 1:
      if (bank == MMC_LASTBANK)
         bank = MMC_LAST1KVROM;
      ppu_setpage(1, address >> 10, &mmc.cart->vrom[(bank % MMC_1KVROM) << 10] - address);
      break;

   case 2:
      if (bank == MMC_LASTBANK)
         bank = MMC_LAST2KVROM;
      ppu_setpage(2, address >> 10, &mmc.cart->vrom[(bank % MMC_2KVROM) << 11] - address);
      break;

   case 4:
      if (bank == MMC_LASTBANK)
         bank = MMC_LAST4KVROM;
      ppu_setpage(4, address >> 10, &mmc.cart->vrom[(bank % MMC_4KVROM) << 12] - address);
      break;

   case 8:
      if (bank == MMC_LASTBANK)
         bank = MMC_LAST8KVROM;
      ppu_setpage(8, 0, &mmc.cart->vrom[(bank % MMC_8KVROM) << 13]);
      break;

   default:
      log_printf("invalid VROM bank size %d\n", size);
   }
}
 
void mmc_bankrom(int size, uint32 address, int bank)
{
   nes6502_context mmc_cpu;

   nes6502_getcontext(&mmc_cpu); 

   switch (size)
   {
   case 8:
      if (bank == MMC_LASTBANK)
         bank = MMC_LAST8KROM;
      {
         int page = address >> NES6502_BANKSHIFT;
         mmc_cpu.mem_page[page] = &mmc.cart->rom[(bank % MMC_8KROM) << 13];
         mmc_cpu.mem_page[page + 1] = mmc_cpu.mem_page[page] + 0x1000;
      }

      break;

   case 16:
      if (bank == MMC_LASTBANK)
         bank = MMC_LAST16KROM;
      {
         int page = address >> NES6502_BANKSHIFT;
         mmc_cpu.mem_page[page] = &mmc.cart->rom[(bank % MMC_16KROM) << 14];
         mmc_cpu.mem_page[page + 1] = mmc_cpu.mem_page[page] + 0x1000;
         mmc_cpu.mem_page[page + 2] = mmc_cpu.mem_page[page] + 0x2000;
         mmc_cpu.mem_page[page + 3] = mmc_cpu.mem_page[page] + 0x3000;
      }
      break;

   case 32:
      if (bank == MMC_LASTBANK)
         bank = MMC_LAST32KROM;

      mmc_cpu.mem_page[8] = &mmc.cart->rom[(bank % MMC_32KROM) << 15];
      mmc_cpu.mem_page[9] = mmc_cpu.mem_page[8] + 0x1000;
      mmc_cpu.mem_page[10] = mmc_cpu.mem_page[8] + 0x2000;
      mmc_cpu.mem_page[11] = mmc_cpu.mem_page[8] + 0x3000;
      mmc_cpu.mem_page[12] = mmc_cpu.mem_page[8] + 0x4000;
      mmc_cpu.mem_page[13] = mmc_cpu.mem_page[8] + 0x5000;
      mmc_cpu.mem_page[14] = mmc_cpu.mem_page[8] + 0x6000;
      mmc_cpu.mem_page[15] = mmc_cpu.mem_page[8] + 0x7000;
      break;

   default:
      log_printf("invalid ROM bank size %d\n", size);
      break;
   }

   nes6502_setcontext(&mmc_cpu);
}
 
bool mmc_peek(int map_num)
{
   mapintf_t **map_ptr = mappers;

   while (NULL != *map_ptr)
   {
      if ((*map_ptr)->number == map_num)
         return true;
      map_ptr++;
   }

   return false;
}

static void mmc_setpages(void)
{
   log_printf("setting up mapper %d\n", mmc.intf->number);
 
   mmc_bankrom(16, 0x8000, 0);
   mmc_bankrom(16, 0xC000, MMC_LASTBANK);
   mmc_bankvrom(8, 0x0000, 0);

   if (mmc.cart->flags & ROM_FLAG_FOURSCREEN)
   {
      ppu_mirror(0, 1, 2, 3);
   }
   else
   {
      if (MIRROR_VERT == mmc.cart->mirror)
         ppu_mirror(0, 1, 0, 1);
      else
         ppu_mirror(0, 0, 1, 1);
   }
 
   if (0 == mmc.cart->vrom_banks)
   {
      ASSERT(mmc.cart->vram);

      ppu_setpage(8, 0, mmc.cart->vram);
      ppu_mirrorhipages();
   }
}
 
void mmc_reset(void)
{
   mmc_setpages();

   ppu_setlatchfunc(NULL);
   ppu_setvromswitch(NULL);

   if (mmc.intf->init)
      mmc.intf->init();

   log_printf("reset memory mapper\n");
}


void mmc_destroy(mmc_t **nes_mmc)
{
   if (*nes_mmc)
      free(*nes_mmc);
}

mmc_t *mmc_create(rominfo_t *rominfo)
{
   mmc_t *temp;
   mapintf_t **map_ptr;
  
   for (map_ptr = mappers; (*map_ptr)->number != rominfo->mapper_number; map_ptr++)
   {
      if (NULL == *map_ptr)
         return NULL; /* Should *never* happen */
   }

   temp = malloc(sizeof(mmc_t));
   if (NULL == temp)
      return NULL;

   memset(temp, 0, sizeof(mmc_t));

   temp->intf = *map_ptr;
   temp->cart = rominfo;

   mmc_setcontext(temp);

   log_printf("created memory mapper: %s\n", (*map_ptr)->name);

   return temp;
}