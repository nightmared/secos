/* GPLv2 (c) Airbus */
#include <debug.h>
#include <info.h>
#include <mbi.h>
#include <gdt.h>
#include <segmem.h>
#include <stack.h>
#include <syscall.h>

void __attribute__((naked)) back_to_kernel() {
    asm volatile (
        "mov $17, %eax \n\t"
        "int $0x80 \n\t"
        "ret"
    );
}

void userland() {
    printf("test\n");
    //asm volatile("int3");
    //asm volatile("mov %eax, %cr0");
    printf("fdljdknghflgv,\n");
    while (1) {
        back_to_kernel();
        sleep(250);
    }
}

inline void __attribute__((always_inline)) update_tss() {
    tss_t *kernel_tss = (tss_t*)&__tss_start__;
    //uint32_t esp0 = 0;
    //asm volatile("mov %%esp, %0" : "=m"(esp0));
    //kernel_tss->s0.esp = esp0;
    kernel_tss->s0.esp = (uint32_t)&kernelland_stack+sizeof(kernelland_stack);
    asm volatile("ltr %%ax" :: "a"(gdt_seg_sel(gdt_tss_idx, 0)));
}

void spawn_task(void* fun) {
    update_tss();
    uint32_t code_sel = gdt_seg_sel(gdt_code_idx+GDT_RING3_OFFSET, 3);
    uint32_t data_sel = gdt_seg_sel(gdt_data_idx+GDT_RING3_OFFSET, 3);
    asm volatile(
        "mov %2, %%ds \n\t"
        "mov %2, %%es \n\t"
        "mov %2, %%fs \n\t"
        "mov %2, %%gs \n\t"
        "push %2 \n\t"
        "push %3 \n\t"
        "pushf \n\t"
        "push %0 \n\t"
        "push %1 \n\t"
        "iret"
        :: "r"(code_sel), "r"(fun), "r"(data_sel), "r"((uint32_t)&userland_stack+sizeof(userland_stack))
    );
}

void tp() {
    printf("\nMultiboot memory information:\n");
    print_mbi_memory_headers();

    init_gdt_flat();
    printf("\nGDT description:\n");
    print_gdt();

    // enable interrupts
    asm volatile("sti");

    // init the kernel TSS
    tss_t *kernel_tss = (tss_t*)&__tss_start__;
    memset(kernel_tss, 0, sizeof(tss_t));
    kernel_tss->s0.ss = gdt_seg_sel(gdt_data_idx, 0);
    //kernel_tss->maps.addr = (uint32_t)&__tss_start__+sizeof(tss_t);
    //memset(kernel_tss->maps.io, 0xff, sizeof(kernel_tss->maps.io));

    // init the userlansd TSS
    //tss_t *userland_tss = (tss_t*)&__tss_start__+0x2500;
    //userland_tss->s0.ss = gdt_seg_sel(gdt_data_idx+GDT_RING3_OFFSET, 3);
    //memset(userland_tss->maps.io, 0xff, sizeof(userland_tss->maps.io));


    spawn_task(userland);

    debug("hola !\n");
    char  src[64];
    char  real_dst[64];
    memcpy(real_dst, src, 64);
}
