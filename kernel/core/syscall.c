#include <syscall.h>
#include <debug.h>

uint8_t userland_stack[0x4000] __attribute__((aligned(16)));
uint8_t kernelland_stack[0x4000] __attribute__((aligned(16)));

void __attribute__((naked)) __regparm__(1) kernel_syscall(int_ctx_t *ctx) {
    asm volatile(
        "mov %%esp, %%ebp"::
    );

    debug("lol: %x\n", ctx->gpr.eax.raw);

    asm volatile(
        "mov %%ebp, %%esp \n\t"
        "ret"::
    );
}

