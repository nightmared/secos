#include <syscall.h>
#include <debug.h>

void __attribute__((naked)) kernel_syscall() {
    asm volatile(
        "mov %%esp, %%ebp"::
    );

    printf("lol\n");

    asm volatile(
        "mov %%ebp, %%esp \n\t"
        "ret"::
    );
}

