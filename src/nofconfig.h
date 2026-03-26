#ifndef _NOFCONFIG_H_
#define _NOFCONFIG_H_

#include <stdbool.h>

#ifndef CONFIG_FILE
#define CONFIG_FILE "nofrendo.cfg"
#endif

typedef struct config_s
{
 
   bool (*open)(void);
 
   void (*close)(void);
 
   int (*read_int)(const char *group, const char *key, int def);
 
   const char *(*read_string)(const char *group, const char *key, const char *def);

   void (*write_int)(const char *group, const char *key, int value);
   void (*write_string)(const char *group, const char *key, const char *value);
   char *filename;
} config_t;

extern config_t config;

#endif 