/* GPLv2 (c) Airbus */
#include <debug.h>
#include <info.h>
#include <mbi.h>
#include <gdt.h>
#include <segmem.h>
#include <stack.h>
#include <syscall.h>
#include <scheduler.h>
#include <paging.h>

void userland() {
    printf("test\n");
    while (1) {
        execute_syscall(4, 0, 1, 2, 3, 4);
        sleep(250);
    }
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

    spawn_task(userland);
}
