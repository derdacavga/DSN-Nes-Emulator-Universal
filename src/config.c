#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
 
#include "esp_log.h"
 
#include "noftypes.h" 
#include "log.h"
#include "osd.h" 
#include "nofconfig.h"
#include "version.h"

static const char *TAG = "NES_CONFIG";
 
#define stricmp strcasecmp

typedef struct myvar_s
{
   struct myvar_s *less, *greater;
   char *group, *key, *value;
} myvar_t;

static myvar_t *myVars = NULL;
static bool mySaveNeeded = false;
 
static void my_destroy(myvar_t **var)
{
   if (!(*var)) return;

   if ((*var)->group) nes_mem_free((*var)->group);
   if ((*var)->key)   nes_mem_free((*var)->key);
   if ((*var)->value) nes_mem_free((*var)->value);
   
   nes_mem_free(*var);
   *var = NULL;
}

static myvar_t *my_create(const char *group, const char *key, const char *value)
{
   myvar_t *var;
 
   var = (myvar_t *)mem_alloc(sizeof(*var), false);
   if (NULL == var) return NULL;

   var->less = var->greater = NULL;
   var->group = var->key = var->value = NULL;

   var->group = (char *)mem_alloc(strlen(group) + 1, false);
   var->key   = (char *)mem_alloc(strlen(key) + 1, false);
   var->value = (char *)mem_alloc(strlen(value) + 1, false);

   if (var->group && var->key && var->value)
   {
      strcpy(var->group, group);
      strcpy(var->key, key);
      strcpy(var->value, value);
      return var;
   }

   my_destroy(&var);
   return NULL;
}

static myvar_t *my_lookup(const char *group, const char *key)
{
   int cmp;
   myvar_t *current = myVars;

   while (current && ((cmp = stricmp(group, current->group)) || (cmp = stricmp(key, current->key))))
   {
      if (cmp < 0)
         current = current->less;
      else
         current = current->greater;
   }

   return current;
}

static void my_insert(myvar_t *var)
{
   int cmp;
   myvar_t **current = &myVars;

   while (*current && ((cmp = stricmp(var->group, (*current)->group)) || (cmp = stricmp(var->key, (*current)->key))))
   {
      current = (cmp < 0) ? &(*current)->less : &(*current)->greater;
   }

   if (*current)
   {
      var->less = (*current)->less;
      var->greater = (*current)->greater;
      my_destroy(current);
   }
   else
   {
      var->less = var->greater = NULL;
   }

   *current = var;
}

static void my_save(FILE *stream, myvar_t *var, char **group)
{
   if (NULL == var) return;

   my_save(stream, var->less, group);

   if (stricmp(*group, var->group))
   {
      fprintf(stream, "\n[%s]\n", var->group);
      *group = var->group;
   }

   fprintf(stream, "%s=%s\n", var->key, var->value);

   my_save(stream, var->greater, group);
}

static void my_cleanup(myvar_t *var)
{
   if (NULL == var) return;

   my_cleanup(var->less);
   my_cleanup(var->greater);

   if (var->group) nes_mem_free(var->group);
   if (var->key)   nes_mem_free(var->key);
   if (var->value) nes_mem_free(var->value);
   nes_mem_free(var);
}

static char *my_getline(FILE *stream)
{
   char buf[256];
   char *dynamic = NULL;

   do
   {
      if (NULL == (fgets(buf, sizeof(buf), stream)))
      {
         if (dynamic) nes_mem_free(dynamic);
         return NULL;
      }

      size_t buf_len = strlen(buf);
      if (NULL == dynamic)
      {
         dynamic = (char *)mem_alloc(buf_len + 1, false);
         if (NULL == dynamic) return NULL;
         strcpy(dynamic, buf);
      }
      else
      { 
         char *temp;
         size_t old_len = strlen(dynamic);
         temp = (char *)mem_alloc(old_len + buf_len + 1, false);
         if (NULL == temp) {
             nes_mem_free(dynamic);
             return NULL;
         }

         strcpy(temp, dynamic);
         strcat(temp, buf);
         nes_mem_free(dynamic);
         dynamic = temp;
      }

      if (feof(stream)) return dynamic;

   } while (dynamic[strlen(dynamic) - 1] != '\n');

   return dynamic;
}
 
static int load_config(const char *filename)
{
   FILE *config_file;

   if ((config_file = fopen(filename, "r")))
   {
      char *line;
      char *group = NULL, *key = NULL, *value = NULL;

      mySaveNeeded = true;
      while ((line = my_getline(config_file)))
      {
         char *s;
         size_t len = strlen(line);

         if (len > 0 && '\n' == line[len - 1])
            line[len - 1] = '\0';

         s = line;

         do
         { 
            while (isspace((int)*s)) s++;

            switch (*s)
            {
            case ';':
            case '#':
            case '\0':
               *s = '\0';
               break;

            case '[':
               if (group) nes_mem_free(group);

               group = ++s;
               char *end_bracket = strchr(s, ']');
               
               if (NULL == end_bracket)
               {
                  ESP_LOGW(TAG, "load_config: missing ']' after group");
                  s = group + strlen(group);
               }
               else
               {
                  *end_bracket = '\0';
                  s = end_bracket + 1;
               }

               if ((value = (char *)mem_alloc(strlen(group) + 1, false)))
               {
                  strcpy(value, group);
               }
               group = value;
               break;

            default:
               key = s;
               s = strchr(s, '=');
               if (NULL == s)
               {
                  ESP_LOGW(TAG, "load_config: missing '=' after key");
                  s = key + strlen(key);
               }
               else
               {
                  *s++ = '\0';
               }

               char *end = key + strlen(key) - 1;
               while (end > key && isspace((int)*end)) *end-- = '\0';

               while (isspace((int)*s)) s++;

               end = s + strlen(s) - 1;
               while (end > s && isspace((int)*end)) *end-- = '\0';

               {
                  myvar_t *var = my_create(group ? group : "", key, s);
                  if (NULL == var)
                  {
                     ESP_LOGE(TAG, "load_config: my_create failed");
                     nes_mem_free(line);
                     if (group) nes_mem_free(group);
                     fclose(config_file);
                     return -1;
                  }
                  my_insert(var);
               }
               s += strlen(s);
            }
         } while (*s);

         nes_mem_free(line);
      }

      if (group) nes_mem_free(group);
      fclose(config_file);
      ESP_LOGI(TAG, "Config loaded: %s", filename);
   }
   else 
   {
       ESP_LOGW(TAG, "Config file not found: %s", filename);
       return -1;
   }

   return 0;
}
 
static int save_config(const char *filename)
{
   FILE *config_file;
   char *group = "";

   config_file = fopen(filename, "w");
   if (NULL == config_file)
   {
      ESP_LOGE(TAG, "save_config failed: %s", filename);
      return -1;
   }

   fprintf(config_file, ";; ESP32 NES EMU Config\n");
   my_save(config_file, myVars, &group);
   fclose(config_file);
   ESP_LOGI(TAG, "Config saved: %s", filename);

   return 0;
}

static bool open_config(void)
{
   return (load_config(config.filename) == 0);
}

static void close_config(void)
{
   if (true == mySaveNeeded)
   {
      save_config(config.filename);
   }

   my_cleanup(myVars);
   myVars = NULL;
}

static void write_int(const char *group, const char *key, int value)
{
   char buf[32];
   myvar_t *var;

   snprintf(buf, sizeof(buf), "%d", value);

   var = my_create(group, key, buf);
   if (NULL == var) return;

   my_insert(var);
   mySaveNeeded = true;
}
 
static int read_int(const char *group, const char *key, int def)
{
   myvar_t *var;

   var = my_lookup(group, key);
   if (NULL == var)
   {
      write_int(group, key, def);
      return def;
   }

   return (int)strtoul(var->value, NULL, 0);
}

static void write_string(const char *group, const char *key, const char *value)
{
   myvar_t *var;

   var = my_create(group, key, value);
   if (NULL == var) return;

   my_insert(var);
   mySaveNeeded = true;
}
 
static const char *read_string(const char *group, const char *key, const char *def)
{
   myvar_t *var;

   var = my_lookup(group, key);
   if (NULL == var)
   {
      if (def != NULL)
         write_string(group, key, def);
      return def;
   }

   return var->value;
}
  
config_t config =
    {
        open_config,
        close_config,
        read_int,
        read_string,
        write_int,
        write_string,
        "/sd/nes.cfg"
    };