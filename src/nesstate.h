#ifndef _NESSTATE_H_
#define _NESSTATE_H_

#include <nes.h>

extern void state_setslot(int slot);
extern int state_load();
extern int state_save();

#endif 