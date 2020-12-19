#include <syscall.h>
#include <scheduler.h>
#include <debug.h>

extern int __x86_kernel_structs_end__, __x86_kernel_structs_start__, __userland_mapped__, __kernel_end__;

#define IN_KERNEL(x) ((x >= (uint32_t)&__x86_kernel_structs_start__ && x < (uint32_t)&__x86_kernel_structs_end__) || (x >= 0x100000 && x < (uint32_t)&__kernel_end__) || (x >= 0xc0000000 && x < (uint32_t)&__x86_kernel_structs_end__+0xc0000000-(uint32_t)&__userland_mapped__))

void __attribute__((section(".userland_code"))) __attribute__((naked)) userland_return_from_syscall() {
    asm volatile(
            "popa \n\t"
            "ret"
            ::);
}

void __attribute__((section(".userland_code"))) userland_execute_syscall(uint8_t nb_args, uint32_t syscall_number, ...) {
    uint32_t eax, ebx, ecx, edx, esi;
    va_list params;

    eax = syscall_number;

    va_start(params, syscall_number);
    for (int i = 0; i < nb_args && i <= 3; i++) {
        uint32_t val = va_arg(params, uint32_t);
        switch (i) {
            case 0:
                ebx = val;
                break;
            case 1:
                ecx = val;
                break;
            case 2:
                edx = val;
                break;
            case 3:
                esi = val;
                break;
        }
    }
    va_end(params);
    asm volatile (
        "int $0x80 \n\t"
        :: "a"(eax), "b"(ebx), "c"(ecx), "d"(edx), "S"(esi)
    );
}

void __regparm__(1) kernel_syscall(int_ctx_t *ctx) {
    uint32_t syscall_nb = ctx->gpr.eax.raw;
    uint32_t __attribute__((unused)) arg2 = ctx->gpr.ebx.raw;
    uint32_t __attribute__((unused)) arg3 = ctx->gpr.ecx.raw;
    uint32_t __attribute__((unused)) arg4 = ctx->gpr.edx.raw;
    uint32_t __attribute__((unused)) arg5 = ctx->gpr.esi.raw;

    switch (syscall_nb) {
        case SYSCALL_EXIT:
            //TODO
            break;

        case SYSCALL_PRINTF:
            // Beware, not enought security checks here
            if (IN_KERNEL(arg2)) {
                debug("Trying to access kernel memory in a syscall\n");
                return;
            }
            printf((char*)arg2, arg3, arg4, arg5);

            break;
        default:
            debug("Unsupported syscall %d from task %d\n", syscall_nb, current_process->task_id);
    }

    switch_to_next_task(ctx);
}

