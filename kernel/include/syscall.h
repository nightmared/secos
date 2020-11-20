#ifndef __SYSCALL_H__
#define __SYSCALL_H__

// kernel calling convention:
// argv1->4: eax, ebx, ecx, edx
void kernel_syscall();

#endif // __SYSCALL_H__
