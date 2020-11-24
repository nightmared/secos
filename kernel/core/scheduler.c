#include <scheduler.h>
#include <segmem.h>
#include <gdt.h>

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

uint8_t userland_stack[2][0x1000] __attribute__((aligned(16)));
uint8_t kernelland_stack[2][0x1000] __attribute__((aligned(16)));

