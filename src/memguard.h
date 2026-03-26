#ifndef _MEMGUARD_H_
#define _MEMGUARD_H_

#include <stdlib.h>
#include <stdbool.h>  
#include <string.h>  
 
#define mem_checkblocks()
#define mem_checkleaks()
#define mem_cleanup()
#define mem_term()
 
void *mem_alloc(int size, bool prefer_fast_memory);
extern void nes_mem_free(void *ptr);
extern void *nes_mem_calloc(size_t nmemb, size_t size);
extern void *nes_mem_malloc(size_t size);
 
#define NOFRENDO_MALLOC(x)  nes_mem_malloc(x)
#define NOFRENDO_CALLOC(x,y) nes_mem_calloc(x,y)
#define NOFRENDO_FREE(x)   nes_mem_free(x)
#define NOFRENDO_STRDUP(s) strdup(s)

#ifdef __cplusplus
extern "C" {
#endif
 
int nofrendo_log_printf(const char *format, ...);
 
#define log_printf nofrendo_log_printf

#ifdef __cplusplus
}
#endif

#endif