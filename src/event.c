#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "event.h"
#include "log.h"

static event_t event_handlers[event_last];

void event_init(void)
{
   int i;
   for (i = 0; i < event_last; i++)
   {
      event_handlers[i] = 0;
   }
}

void event_set(int index, event_t handler)
{
   if (index >= 0 && index < event_last)
   {
      event_handlers[index] = handler;
   }
}

event_t event_get(int index)
{
   if (index >= 0 && index < event_last)
   {
      return event_handlers[index];
   }
   return 0;
}

void event_set_system(system_t type)
{
   (void)type;
}

void event_trigger(int index, int para)
{
   if (index >= 0 && index < event_last)
   {
      if (event_handlers[index])
      {
         event_handlers[index](para);
      }
   }
}