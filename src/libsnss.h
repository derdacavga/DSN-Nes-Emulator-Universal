#ifndef _LIBSNSS_H_
#define _LIBSNSS_H_

#include <stdio.h>
#include "memguard.h"
 
#define USE_LITTLE_ENDIAN

typedef enum _SNSS_OPEN_MODES
{
   SNSS_OPEN_READ,
   SNSS_OPEN_WRITE
} SNSS_OPEN_MODE;
 
typedef enum _SNSS_BLOCK_TYPES
{
   SNSS_BASR,
   SNSS_VRAM,
   SNSS_SRAM,
   SNSS_MPRD,
   SNSS_CNTR,
   SNSS_SOUN,
   SNSS_UNKNOWN_BLOCK
} SNSS_BLOCK_TYPE;
 
typedef enum _SNSS_RETURN_CODES
{
   SNSS_OK,
   SNSS_BAD_FILE_TAG,
   SNSS_OPEN_FAILED,
   SNSS_CLOSE_FAILED,
   SNSS_READ_FAILED,
   SNSS_WRITE_FAILED,
   SNSS_OUT_OF_MEMORY,
   SNSS_UNSUPPORTED_BLOCK
} SNSS_RETURN_CODE;

#define TAG_LENGTH 4
#define SNSS_BLOCK_VERSION 1
 
struct mapper1Data
{
   unsigned char registers[4];
   unsigned char latch;
   unsigned char numberOfBits;
};

struct mapper4Data
{
   unsigned char irqCounter;
   unsigned char irqLatchCounter;
   unsigned char irqCounterEnabled;
   unsigned char last8000Write;
};

struct mapper5Data
{
   unsigned char dummy;  
};

struct mapper6Data
{
   unsigned char irqCounter;
   unsigned char irqLatchCounter;
   unsigned char irqCounterEnabled;
   unsigned char last43FEWrite;
   unsigned char last4500Write;
};

struct mapper9Data
{
   unsigned char latch[2];
   unsigned char lastB000Write;
   unsigned char lastC000Write;
   unsigned char lastD000Write;
   unsigned char lastE000Write;
};

struct mapper10Data
{
   unsigned char latch[2];
   unsigned char lastB000Write;
   unsigned char lastC000Write;
   unsigned char lastD000Write;
   unsigned char lastE000Write;
};

struct mapper16Data
{
   unsigned char irqCounterLowByte;
   unsigned char irqCounterHighByte;
   unsigned char irqCounterEnabled;
};

struct mapper17Data
{
   unsigned char irqCounterLowByte;
   unsigned char irqCounterHighByte;
   unsigned char irqCounterEnabled;
};

struct mapper18Data
{
   unsigned char irqCounterLowByte;
   unsigned char irqCounterHighByte;
   unsigned char irqCounterEnabled;
};

struct mapper19Data
{
   unsigned char irqCounterLowByte;
   unsigned char irqCounterHighByte;
   unsigned char irqCounterEnabled;
};

struct mapper21Data
{
   unsigned char irqCounter;
   unsigned char irqCounterEnabled;
};

struct mapper24Data
{
   unsigned char irqCounter;
   unsigned char irqCounterEnabled;
};

struct mapper40Data
{
   unsigned char irqCounter;
   unsigned char irqCounterEnabled;
};

struct mapper69Data
{
   unsigned char irqCounterLowByte;
   unsigned char irqCounterHighByte;
   unsigned char irqCounterEnabled;
};

struct mapper90Data
{
   unsigned char irqCounter;
   unsigned char irqLatchCounter;
   unsigned char irqCounterEnabled;
};

struct mapper224Data
{
   unsigned char chrRamWriteable;
};

struct mapper225Data
{
   unsigned char prgSize;
   unsigned char registers[4];
};

struct mapper226Data
{
   unsigned char chrRamWriteable;
};

struct mapper228Data
{
   unsigned char prgChipSelected;
};

struct mapper230Data
{
   unsigned char numberOfResets;
};

typedef struct _SnssFileHeader
{
   char tag[TAG_LENGTH + 1];
   unsigned int numberOfBlocks;
} SnssFileHeader;
 
typedef struct _SnssBlockHeader
{
   char tag[TAG_LENGTH + 1];
   unsigned int blockVersion;
   unsigned int blockLength;
} SnssBlockHeader;

#define BASE_BLOCK_LENGTH 0x1931
typedef struct _SnssBaseBlock
{
   unsigned char regA;
   unsigned char regX;
   unsigned char regY;
   unsigned char regFlags;
   unsigned char regStack;
   unsigned short regPc;
   unsigned char reg2000;
   unsigned char reg2001;
   unsigned char cpuRam[0x800];
   unsigned char spriteRam[0x100];
   unsigned char ppuRam[0x1000];
   unsigned char palette[0x20];
   unsigned char mirrorState[4];
   unsigned short vramAddress;
   unsigned char spriteRamAddress;
   unsigned char tileXOffset;
} SnssBaseBlock;

#define VRAM_8K 0x2000
#define VRAM_16K 0x4000
typedef struct _SnssVramBlock
{
   unsigned short vramSize;
   unsigned char vram[VRAM_16K];
} SnssVramBlock;

#define SRAM_1K 0x0400
#define SRAM_2K 0x0800
#define SRAM_3K 0x0C00
#define SRAM_4K 0x1000
#define SRAM_5K 0x1400
#define SRAM_6K 0x1800
#define SRAM_7K 0x1C00
#define SRAM_8K 0x2000
typedef struct _SnssSramBlock
{
   unsigned short sramSize;
   unsigned char sramEnabled;
   unsigned char sram[SRAM_8K];
} SnssSramBlock;

#define MAPPER_BLOCK_LENGTH 0x98
typedef struct _SnssMapperBlock
{
   unsigned short prgPages[4];
   unsigned short chrPages[8];

   union _extraData
   {
      unsigned char mapperData[128];
      struct mapper1Data mapper1;
      struct mapper4Data mapper4;
      struct mapper5Data mapper5;
      struct mapper6Data mapper6;
      struct mapper9Data mapper9;
      struct mapper10Data mapper10;
      struct mapper16Data mapper16;
      struct mapper17Data mapper17;
      struct mapper18Data mapper18;
      struct mapper19Data mapper19;
      struct mapper21Data mapper21;
      struct mapper24Data mapper24;
      struct mapper40Data mapper40;
      struct mapper69Data mapper69;
      struct mapper90Data mapper90;
      struct mapper224Data mapper224;
      struct mapper225Data mapper225;
      struct mapper226Data mapper226;
      struct mapper228Data mapper228;
      struct mapper230Data mapper230;
   } extraData;
} SnssMapperBlock;

typedef struct _SnssControllersBlock
{
   unsigned char dummy;  
} SnssControllersBlock;

#define SOUND_BLOCK_LENGTH 0x16
typedef struct _SnssSoundBlock
{
   unsigned char soundRegisters[0x16];
} SnssSoundBlock;
 
typedef struct _SNSS_FILE
{
   FILE *fp;
   SNSS_OPEN_MODE mode;
   SnssFileHeader headerBlock;
   SnssBaseBlock baseBlock;
   SnssVramBlock vramBlock;
   SnssSramBlock sramBlock;
   SnssMapperBlock mapperBlock;
   SnssControllersBlock contBlock;
   SnssSoundBlock soundBlock;
} SNSS_FILE;

#ifdef __cplusplus
extern "C"
{
#endif 
 
   SNSS_RETURN_CODE SNSS_OpenFile(SNSS_FILE **snssFile, const char *filename,
                                  SNSS_OPEN_MODE mode);
   SNSS_RETURN_CODE SNSS_CloseFile(SNSS_FILE **snssFile);
 
   SNSS_RETURN_CODE SNSS_GetNextBlockType(SNSS_BLOCK_TYPE *blockType,
                                          SNSS_FILE *snssFile);
   SNSS_RETURN_CODE SNSS_SkipNextBlock(SNSS_FILE *snssFile);
 
   SNSS_RETURN_CODE SNSS_ReadBlock(SNSS_FILE *snssFile, SNSS_BLOCK_TYPE blockType);
   SNSS_RETURN_CODE SNSS_WriteBlock(SNSS_FILE *snssFile, SNSS_BLOCK_TYPE blockType);
 
   const char *SNSS_GetErrorString(SNSS_RETURN_CODE code);

   static SNSS_FILE *snssFileBlock;
   static char *blockBytes;

#ifdef __cplusplus
}
#endif  

#endif