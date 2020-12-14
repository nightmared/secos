#include <syscall.h>
#include <debug.h>

void __attribute__((section(".userland_code"))) __userland_execute_syscall(uint8_t nb_args, uint32_t syscall_number, ...) {
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
        "leave \n\t"
        "ret"
        :: "a"(eax), "b"(ebx), "c"(ecx), "d"(edx), "S"(esi)
    );
}

void __regparm__(1) kernel_syscall(int_ctx_t *ctx) {
    debug("eax: %p\n", ctx->gpr.eax.raw);
    printf("ebx: %p\n", ctx->gpr.ebx.raw);
    printf("ecx: %p\n", ctx->gpr.ecx.raw);
    printf("edx: %p\n", ctx->gpr.edx.raw);
    printf("esi: %p\n", ctx->gpr.esi.raw);
}

