#ifndef _DIS6502_H_
#define _DIS6502_H_

#include "noftypes.h" 

#ifdef __cplusplus
extern "C"
{
#endif   

#ifdef NES6502_DEBUG
    extern char *nes6502_disasm(uint32 PC, uint8 P, uint8 A, uint8 X, uint8 Y, uint8 S);
#endif

#ifdef __cplusplus
}
#endif  

#endif