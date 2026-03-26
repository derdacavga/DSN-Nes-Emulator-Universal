#ifndef _NES6502_H_
#define _NES6502_H_

#include "noftypes.h"
 
/*#define  NES6502_DECIMAL*/

#define NES6502_NUMBANKS 16
#define NES6502_BANKSHIFT 12
#define NES6502_BANKSIZE (0x10000 / NES6502_NUMBANKS)
#define NES6502_BANKMASK (NES6502_BANKSIZE - 1)
 
#define N_FLAG 0x80
#define V_FLAG 0x40
#define R_FLAG 0x20  
#define B_FLAG 0x10
#define D_FLAG 0x08
#define I_FLAG 0x04
#define Z_FLAG 0x02
#define C_FLAG 0x01
 
#define NMI_VECTOR 0xFFFA
#define RESET_VECTOR 0xFFFC
#define IRQ_VECTOR 0xFFFE
 
#define INT_CYCLES 7
#define RESET_CYCLES 6

#define NMI_MASK 0x01
#define IRQ_MASK 0x02
 
#define STACK_OFFSET 0x0100

typedef struct
{
   uint32 min_range, max_range;
   uint8 (*read_func)(uint32 address);
} nes6502_memread;

typedef struct
{
   uint32 min_range, max_range;
   void (*write_func)(uint32 address, uint8 value);
} nes6502_memwrite;

typedef struct
{
   uint8 *mem_page[NES6502_NUMBANKS];  

   nes6502_memread *read_handler;
   nes6502_memwrite *write_handler;

   uint32 pc_reg;
   uint8 a_reg, p_reg;
   uint8 x_reg, y_reg;
   uint8 s_reg;

   uint8 jammed; 

   uint8 int_pending, int_latency;

   int32 total_cycles, burn_cycles;
} nes6502_context;

#ifdef __cplusplus
extern "C"
{
#endif 
   extern void nes6502_reset(void);
   extern int nes6502_execute(int total_cycles);
   extern void nes6502_nmi(void);
   extern void nes6502_irq(void);
   extern uint8 nes6502_getbyte(uint32 address);
   extern uint32 nes6502_getcycles(bool reset_flag);
   extern void nes6502_burn(int cycles);
   extern void nes6502_release(void);
 
   extern void nes6502_setcontext(nes6502_context *cpu);
   extern void nes6502_getcontext(nes6502_context *cpu);

#ifdef __cplusplus
}
#endif 

#endif 