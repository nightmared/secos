#ifndef __SCHEDULER_H__
#define __SCHEDULER_H__

#include <types.h>

extern uint8_t userland_stack[2][0x1000] __attribute__((aligned(16)));
extern uint8_t kernelland_stack[2][0x1000] __attribute__((aligned(16)));

void spawn_task(void* fun);

#endif // __SCHEDULER_H__
