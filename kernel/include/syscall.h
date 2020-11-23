#ifndef __SYSCALL_H__
#define __SYSCALL_H__

#include <types.h>
#include <intr.h>

extern uint8_t userland_stack[0x4000] __attribute__((aligned(16)));
extern uint8_t kernelland_stack[0x4000] __attribute__((aligned(16)));

void __regparm__(1) kernel_syscall(int_ctx_t *ctx);

#endif // __SYSCALL_H__
