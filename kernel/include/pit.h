#ifndef __PIT_H__
#define __PIT_H__

#include <types.h>

void pit_init();
void time_incr();
uint64_t get_time_ms();
double get_precision_ms();

bool_t add_waiter(uint64_t wait_ms, void (fun)(), void* arg);
void del_waiter(uint8_t idx);
// WARNING: blocking !
void sleep(uint64_t nb_ms);

#endif // __PIT_H__
