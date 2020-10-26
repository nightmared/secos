#ifndef __PIT_H__
#define __PIT_H__

#include <types.h>

void pit_init();
void time_incr();
uint64_t get_time_ms();

#endif // __PIT_H__
