/* GPLv2 (c) Airbus */
#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <types.h>
#include <print.h>
#include <pit.h>

#define debug(format, ...) printf("[DEBUG, time: %lld ms]\n" format, get_time_ms(), ## __VA_ARGS__)
void stack_trace(offset_t);

#endif
