#ifndef __SYSCALL_H__
#define __SYSCALL_H__

#include <types.h>
#include <intr.h>

void __regparm__(1) kernel_syscall(int_ctx_t *ctx);

#define userland_execute_syscall(nb_args, syscall_number, args...) (0xc0000000-(uint32_t)&__userland_mapped__+__userland_execute_syscall)(nb_args, syscall_number, args)

void __userland_execute_syscall(uint8_t nb_args, uint32_t syscall_number, ...);

#endif // __SYSCALL_H__
