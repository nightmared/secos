#ifndef __SYSCALL_H__
#define __SYSCALL_H__

#include <types.h>
#include <intr.h>

extern int __userland_start__;

void __regparm__(1) kernel_syscall(int_ctx_t *ctx);

void userland_execute_syscall(uint8_t nb_args, uint32_t syscall_number, ...);

#endif // __SYSCALL_H__
