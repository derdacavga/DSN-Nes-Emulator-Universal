#ifndef _NES_MMC_H_
#define _NES_MMC_H_

#include <libsnss.h>
#include <nes_apu.h>

#define  MMC_LASTBANK      -1

typedef struct
{
   uint32 min_range, max_range;
   uint8 (*read_func)(uint32 address);
} map_memread;

typedef struct
{
   uint32 min_range, max_range;
   void (*write_func)(uint32 address, uint8 value);
} map_memwrite;


typedef struct mapintf_s
{
   int number;
   char *name;
   void (*init)(void);
   void (*vblank)(void);
   void (*hblank)(int vblank);
   void (*get_state)(SnssMapperBlock *state);
   void (*set_state)(SnssMapperBlock *state);
   map_memread *mem_read;
   map_memwrite *mem_write;
   apuext_t *sound_ext;
} mapintf_t;


#include <nes_rom.h>
typedef struct mmc_s
{
   mapintf_t *intf;
   rominfo_t *cart;  
} mmc_t;

extern rominfo_t *mmc_getinfo(void);

extern void mmc_bankvrom(int size, uint32 address, int bank);
extern void mmc_bankrom(int size, uint32 address, int bank);
 
extern mmc_t *mmc_create(rominfo_t *rominfo);
extern void mmc_destroy(mmc_t **nes_mmc);

extern void mmc_getcontext(mmc_t *dest_mmc);
extern void mmc_setcontext(mmc_t *src_mmc);

extern bool mmc_peek(int map_num);

extern void mmc_reset(void);

#endif