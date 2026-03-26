#ifndef _LOG_H_
#define _LOG_H_

#include <stdio.h>

extern int nofrendo_log_init(void);
extern void nofrendo_log_shutdown(void);
extern int nofrendo_log_print(const char *string);
extern int nofrendo_log_printf(const char *format, ...);
extern void nofrendo_log_chain_logfunc(int (*logfunc)(const char *string));
extern void nofrendo_log_assert(int expr, int line, const char *file, char *msg);

#endif