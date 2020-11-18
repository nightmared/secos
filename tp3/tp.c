/* GPLv2 (c) Airbus */
#include <debug.h>
#include <info.h>
#include <mbi.h>
#include <gdt.h>
#include <segmem.h>
#include <stack.h>

void userland() {
    printf("test\n");
    asm volatile("int3");
    uint32_t cr0 = 0;
    asm volatile("mov %0, %%cr0" : "=a"(cr0));
    debug("test %d\n", cr0);
}

tss_t *tss = (tss_t*)&__tss_start__;

static uint8_t userland_stack[0x8000];

inline void __attribute__((always_inline)) update_tss() {
    uint32_t esp0 = 0;
    asm volatile("mov %0, %%esp" : "=r"(esp0));
    tss->s0.esp = esp0;
    asm volatile("ltr %%ax" :: "a"(gdt_seg_sel(gdt_tss_idx, 3)));
}

void spawn_task(void* fun) {
    update_tss();
    uint16_t data_sel = gdt_seg_sel(gdt_data_idx+GDT_RING3_OFFSET, 3);
    uint16_t code_sel = gdt_seg_sel(gdt_code_idx+GDT_RING3_OFFSET, 3);
    asm volatile(
        "mov %%ds, %2 \n\t"
        "mov %%es, %2 \n\t"
        "mov %%fs, %2 \n\t"
        "mov %%gs, %2 \n\t"
        "push %2 \n\t"
        "push %3 \n\t"
        "pushf \n\t"
        "push %0 \n\t"
        "push %1 \n\t"
        "iret"
        :: "r"(code_sel), "r"(fun), "r"(data_sel), "r"(userland_stack+sizeof(userland_stack)): "eax"
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

    // init the TSS
    memset(tss, 0, sizeof(tss_t));
    tss->s0.ss = gdt_seg_sel(gdt_data_idx, 0);
    tss->maps.addr = (uint32_t)&__tss_start__+sizeof(tss_t);
    memset(tss->maps.io, 0xff, sizeof(tss->maps.io));

    spawn_task(userland);

    debug("hola !\n");
    char  src[64];
    char  real_dst[64];
    memcpy(real_dst, src, 64);
}
