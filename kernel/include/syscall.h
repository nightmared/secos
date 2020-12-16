#ifndef __SYSCALL_H__
#define __SYSCALL_H__

#include <types.h>
#include <intr.h>

#define SYSCALL_EXIT 1
#define SYSCALL_PRINTF 2
#define SYSCALL_DEBUG 3

extern int __userland_start__;

void __regparm__(1) kernel_syscall(int_ctx_t *ctx);

void userland_execute_syscall(uint8_t nb_args, uint32_t syscall_number, ...);
void __attribute__((section(".userland_code"))) __attribute__((naked)) userland_return_from_syscall();

#endif // __SYSCALL_H__
