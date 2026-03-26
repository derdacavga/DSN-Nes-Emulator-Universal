#include <stdio.h>
#include <string.h>
#include <noftypes.h>
#include <nes_rom.h>
#include <intro.h>
#include <nes_mmc.h>
#include <nes_ppu.h>
#include <nes.h>
#include <gui.h>
#include <log.h>
#include <osd.h>
#include <esp_heap_caps.h>

#pragma GCC optimize("O3")

extern int osd_rom_open(const char *path);
extern int osd_rom_read(void *dst, int len);
extern void osd_rom_close(void);
 
#define  ROM_DISP_MAXLEN   20


#ifdef ZLIB
#include <zlib.h>
#define  _fopen            gzopen
#define  _fclose           gzclose
#define  _fread(B,N,L,F)   gzread((F),(B),(L)*(N))
#else
#define  _fopen            fopen
#define  _fclose           fclose
#define  _fread(B,N,L,F)   fread((B),(N),(L),(F))
#endif

#define  ROM_FOURSCREEN    0x08
#define  ROM_TRAINER       0x04
#define  ROM_BATTERY       0x02
#define  ROM_MIRRORTYPE    0x01
#define  ROM_INES_MAGIC    "NES\x1A"
 
typedef struct inesheader_s
{
   uint8 ines_magic[4]    ;
   uint8 rom_banks        ;
   uint8 vrom_banks       ;
   uint8 rom_type         ;
   uint8 mapper_hinybble  ;
   uint8 reserved[8]      ;
} inesheader_t;


#define  TRAINER_OFFSET    0x1000
#define  TRAINER_LENGTH    0x200
#define  VRAM_LENGTH       0x2000

#define  ROM_BANK_LENGTH   0x4000
#define  VROM_BANK_LENGTH  0x2000

#define  SRAM_BANK_LENGTH  0x0400
#define  VRAM_BANK_LENGTH  0x2000
 
static void rom_savesram(rominfo_t *rominfo)
{
   FILE *fp;
   char fn[PATH_MAX + 1];

   ASSERT(rominfo);

   if (rominfo->flags & ROM_FLAG_BATTERY)
   {
      strncpy(fn, rominfo->filename, PATH_MAX);
      osd_newextension(fn, ".sav");

      fp = fopen(fn, "wb");
      if (NULL != fp)
      {
         fwrite(rominfo->sram, SRAM_BANK_LENGTH, rominfo->sram_banks, fp);
         fclose(fp);
         log_printf("Wrote battery RAM to %s.\n", fn);
      }
   }
}
 
static void rom_loadsram(rominfo_t *rominfo)
{
   FILE *fp;
   char fn[PATH_MAX + 1];

   ASSERT(rominfo);

   if (rominfo->flags & ROM_FLAG_BATTERY)
   {
      strncpy(fn, rominfo->filename, PATH_MAX);
      osd_newextension(fn, ".sav");

      fp = fopen(fn, "rb");
      if (NULL != fp)
      {
         fread(rominfo->sram, SRAM_BANK_LENGTH, rominfo->sram_banks, fp);
         fclose(fp);
         log_printf("Read battery RAM from %s.\n", fn);
      }
   }
}
 
static int rom_allocsram(rominfo_t *rominfo)
{ 
   rominfo->sram = malloc(SRAM_BANK_LENGTH * rominfo->sram_banks);
   if (NULL == rominfo->sram)
   {
      gui_sendmsg(GUI_RED, "Could not allocate space for battery RAM");
      return -1;
   }
 
   memset(rominfo->sram, 0, SRAM_BANK_LENGTH * rominfo->sram_banks);
   return 0;
}

static int rom_read_exact(void *dst, int len)
{
   int n = osd_rom_read(dst, len);
   return (n == len) ? 0 : -1;
}
 
static int rom_loadtrainer(rominfo_t *rominfo)
{
   ASSERT(rominfo);

   if (rominfo->flags & ROM_FLAG_TRAINER)
   {
      if (rom_read_exact(rominfo->sram + TRAINER_OFFSET, TRAINER_LENGTH))
      {
         gui_sendmsg(GUI_RED, "Could not read trainer");
         return -1;
      }
      log_printf("Read in trainer at $7000\n");
   }
   return 0;
}

static int rom_loadrom(rominfo_t *rominfo)
{
   ASSERT(rominfo);
 
   int rom_len = rominfo->rom_banks * ROM_BANK_LENGTH;
   rominfo->rom = (uint8 *)heap_caps_malloc(rom_len, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
   if (NULL == rominfo->rom)
      rominfo->rom = malloc(rom_len);
   if (NULL == rominfo->rom)
   {
      gui_sendmsg(GUI_RED, "Could not allocate space for ROM image");
      return -1;
   }
   if (rom_read_exact(rominfo->rom, rom_len))
   {
      gui_sendmsg(GUI_RED, "Could not read PRG ROM");
      return -1;
   }
 
   if (rominfo->vrom_banks)
   {
      int vrom_len = rominfo->vrom_banks * VROM_BANK_LENGTH;
      rominfo->vrom = (uint8 *)heap_caps_malloc(vrom_len, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
      if (NULL == rominfo->vrom)
         rominfo->vrom = malloc(vrom_len);
      if (NULL == rominfo->vrom)
      {
         gui_sendmsg(GUI_RED, "Could not allocate space for VROM");
         return -1;
      }
      if (rom_read_exact(rominfo->vrom, vrom_len))
      {
         gui_sendmsg(GUI_RED, "Could not read CHR ROM");
         return -1;
      }

   }
   else
   {
      rominfo->vram = (uint8 *)heap_caps_malloc(VRAM_LENGTH, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
      if (NULL == rominfo->vram)
         rominfo->vram = malloc(VRAM_LENGTH);
      if (NULL == rominfo->vram)
      {
         gui_sendmsg(GUI_RED, "Could not allocate space for VRAM");
         return -1;
      }
      memset(rominfo->vram, 0, VRAM_LENGTH);
   }

   return 0;
}
 
static void rom_checkforpal(rominfo_t *rominfo)
{
   FILE *fp;
   rgb_t vs_pal[64];
   char filename[PATH_MAX + 1];
   int i;

   ASSERT(rominfo);

   strncpy(filename, rominfo->filename, PATH_MAX);
   osd_newextension(filename, ".pal");

   fp = fopen(filename, "rb");
   if (NULL == fp)
      return; /* no palette found  */

   for (i = 0; i < 64; i++)
   {
      vs_pal[i].r = fgetc(fp);
      vs_pal[i].g = fgetc(fp);
      vs_pal[i].b = fgetc(fp);
   }

   fclose(fp); 
   rominfo->flags |= ROM_FLAG_VERSUS; 
   ppu_setpal(nes_getcontextptr()->ppu, vs_pal);
   log_printf("Game specific palette found -- assuming VS. UniSystem\n");
}

static FILE *rom_findrom(const char *filename, rominfo_t *rominfo)
{
   FILE *fp;

   ASSERT(rominfo);

   if (NULL == filename)
      return NULL;
 
   osd_fullname(rominfo->filename, filename);

   fp = _fopen(rominfo->filename, "rb");
   if (NULL == fp)
   { 
      if (NULL == strrchr(rominfo->filename, '.'))
         strncat(rominfo->filename, ".nes", PATH_MAX - strlen(rominfo->filename)); 
      fp = _fopen(rominfo->filename, "rb");
   }

   return fp;
}
 
static int rom_adddirty(char *filename)
{
#ifdef NOFRENDO_DEBUG
#define  MAX_BUFFER_LENGTH    255
   char buffer[MAX_BUFFER_LENGTH + 1];
   bool found = false;

   FILE *fp = fopen("dirtyrom.txt", "rt");
   if (NULL == fp)
      return -1;

   while (fgets(buffer, MAX_BUFFER_LENGTH, fp))
   {
      if (0 == strncmp(filename, buffer, strlen(filename)))
      {
         found = true;
         break;
      }
   }

   if (false == found)
   { 
      fclose(fp);
      fp = fopen("dirtyrom.txt", "at");
      fprintf(fp, "%s -- dirty header\n", filename);
   }

   fclose(fp);
#endif  

   return 0;
}
 
int rom_checkmagic(const char *filename)
{
   inesheader_t head;
   rominfo_t rominfo;
   FILE *fp;

   fp = rom_findrom(filename, &rominfo);
   if (NULL == fp)
      return -1;

   _fread(&head, 1, sizeof(head), fp);

   _fclose(fp);

   if (0 == memcmp(head.ines_magic, ROM_INES_MAGIC, 4)) 
      return 0;

   return -1;
}

static int rom_getheader(rominfo_t *rominfo)
{
#define  RESERVED_LENGTH   8
   inesheader_t head;
   uint8 reserved[RESERVED_LENGTH];
   bool header_dirty;

   ASSERT(rominfo);
 
   if (rom_read_exact(&head, sizeof(head)))
   {
      gui_sendmsg(GUI_RED, "Failed to read ROM header");
      return -1;
   }
   printf("Head: (%x %x %x %x)\n", head.ines_magic[0], head.ines_magic[1], head.ines_magic[2], head.ines_magic[3]);

   if (memcmp(head.ines_magic, ROM_INES_MAGIC, 4))
   {
      gui_sendmsg(GUI_RED, "%s is not a valid ROM image", rominfo->filename);
      return -1;
   }

   rominfo->rom_banks = head.rom_banks;
   rominfo->vrom_banks = head.vrom_banks; 
   rominfo->sram_banks = 8;  
   rominfo->vram_banks = 1;  
   rominfo->mirror = (head.rom_type & ROM_MIRRORTYPE) ? MIRROR_VERT : MIRROR_HORIZ;
   rominfo->flags = 0;
   if (head.rom_type & ROM_BATTERY)
      rominfo->flags |= ROM_FLAG_BATTERY;
   if (head.rom_type & ROM_TRAINER)
      rominfo->flags |= ROM_FLAG_TRAINER;
   if (head.rom_type & ROM_FOURSCREEN)
      rominfo->flags |= ROM_FLAG_FOURSCREEN; 
   rominfo->mapper_number = head.rom_type >> 4;
 
   memset(reserved, 0, RESERVED_LENGTH);
   if (0 == memcmp(head.reserved, reserved, RESERVED_LENGTH))
   { 
      header_dirty = false;
      rominfo->mapper_number |= (head.mapper_hinybble & 0xF0);
   }
   else
   {
      header_dirty = true;
 
      if (('D' == head.mapper_hinybble) && (0 == memcmp(head.reserved, "iskDude!", 8)))
         log_printf("`DiskDude!' found in ROM header, ignoring high mapper nybble\n");
      else
      {
         log_printf("ROM header dirty, possible problem\n");
         rominfo->mapper_number |= (head.mapper_hinybble & 0xF0);
      }

      rom_adddirty(rominfo->filename);
   }
 
   if (99 == rominfo->mapper_number)
      rominfo->flags |= ROM_FLAG_VERSUS;

   return 0;
}
 
char *rom_getinfo(rominfo_t *rominfo)
{
   static char info[PATH_MAX + 1];
   char romname[PATH_MAX + 1], temp[PATH_MAX + 1];
 
   if (strrchr(rominfo->filename, PATH_SEP))
      strncpy(romname, strrchr(rominfo->filename, PATH_SEP) + 1, PATH_MAX);
   else
      strncpy(romname, rominfo->filename, PATH_MAX);
 
   if (strlen(romname) > ROM_DISP_MAXLEN)
   {
      strncpy(info, romname, ROM_DISP_MAXLEN - 3);
      strcpy(info + (ROM_DISP_MAXLEN - 3), "...");
   }
   else
   {
      strcpy(info, romname);
   }

   sprintf(temp, " [%d] %dk/%dk %c", rominfo->mapper_number,
           rominfo->rom_banks * 16, rominfo->vrom_banks * 8,
           (rominfo->mirror == MIRROR_VERT) ? 'V' : 'H');
    
   strncat(info, temp, PATH_MAX - strlen(info));

   if (rominfo->flags & ROM_FLAG_BATTERY)
      strncat(info, "B", PATH_MAX - strlen(info));
   if (rominfo->flags & ROM_FLAG_TRAINER)
      strncat(info, "T", PATH_MAX - strlen(info));
   if (rominfo->flags & ROM_FLAG_FOURSCREEN)
      strncat(info, "4", PATH_MAX - strlen(info));

   return info;
}
 
rominfo_t *rom_load(const char *filename)
{
   rominfo_t *rominfo;
   if (osd_rom_open(filename))
      return NULL;

   rominfo = malloc(sizeof(rominfo_t));
   if (NULL == rominfo)
   {
      osd_rom_close();
      return NULL;
   }

   memset(rominfo, 0, sizeof(rominfo_t));
   strncpy(rominfo->filename, filename, PATH_MAX);
   rominfo->filename[PATH_MAX - 1] = '\0';
 
	if (rom_getheader(rominfo))
   {
      log_printf("rom_load: header parse failed\n");
      goto _fail;
   }
 
   if (false == mmc_peek(rominfo->mapper_number))
   {
      gui_sendmsg(GUI_RED, "Mapper %d not yet implemented", rominfo->mapper_number);
      log_printf("rom_load: unsupported mapper %d\n", rominfo->mapper_number);
      goto _fail;
   }
 
   if (rom_allocsram(rominfo))
   {
      log_printf("rom_load: sram alloc failed\n");
      goto _fail;
   }

   if (rom_loadtrainer(rominfo))
      goto _fail;

	if (rom_loadrom(rominfo))
   {
      log_printf("rom_load: rom/vrom alloc or copy failed\n");
      goto _fail;
   }

   rom_loadsram(rominfo);
 
//   rom_checkforpal(rominfo);
   gui_sendmsg(GUI_GREEN, "ROM loaded: %s", rom_getinfo(rominfo));
   osd_rom_close();

   return rominfo;

_fail:
   osd_rom_close();
   rom_free(&rominfo);
   return NULL;
}
 
void rom_free(rominfo_t **rominfo)
{
   if (NULL == *rominfo)
   {
      gui_sendmsg(GUI_GREEN, "ROM not loaded");
      return;
   }
 
   if ((*rominfo)->flags & ROM_FLAG_VERSUS)
   { 
      ppu_setdefaultpal(nes_getcontextptr()->ppu);
      log_printf("Default NES palette restored\n");
   }

   rom_savesram(*rominfo);

   if ((*rominfo)->sram)
      free((*rominfo)->sram);
   if ((*rominfo)->rom)
      free((*rominfo)->rom);
   if ((*rominfo)->vrom)
      free((*rominfo)->vrom);
   if ((*rominfo)->vram)
      free((*rominfo)->vram);

   free(*rominfo);

   gui_sendmsg(GUI_GREEN, "ROM freed");
} 