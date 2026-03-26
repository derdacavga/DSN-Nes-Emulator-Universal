#ifndef _NES_ROM_H_
#define _NES_ROM_H_

#include <unistd.h>
#include <osd.h>

typedef enum
{
   MIRROR_HORIZ   = 0,
   MIRROR_VERT    = 1
} mirror_t;

#define  ROM_FLAG_BATTERY     0x01
#define  ROM_FLAG_TRAINER     0x02
#define  ROM_FLAG_FOURSCREEN  0x04
#define  ROM_FLAG_VERSUS      0x08

typedef struct rominfo_s
{ 
   uint8 *rom, *vrom; 
   uint8 *sram, *vram;
 
   int rom_banks, vrom_banks;
   int sram_banks, vram_banks;

   int mapper_number;
   mirror_t mirror;

   uint8 flags;

   char filename[PATH_MAX + 1];
} rominfo_t;


extern int rom_checkmagic(const char *filename);
extern rominfo_t *rom_load(const char *filename);
extern void rom_free(rominfo_t **rominfo);
extern char *rom_getinfo(rominfo_t *rominfo);


#endif 