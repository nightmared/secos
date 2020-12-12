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
#include <alloc.h>

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

    enable_paging();

    print_pdt();

    // init the kernel TSS
    tss_t *kernel_tss = (tss_t*)&__tss_start__;
    memset(kernel_tss, 0, sizeof(tss_t));
    kernel_tss->s0.ss = gdt_seg_sel(gdt_data_idx, 0);

    prepare_scheduler();

    struct process proc1 = {0}, proc2 = {0};
    if (!init_process(&proc1, userland)) {
        debug("error: couldn't init a process\n");
    }
    if (!init_process(&proc2, userland)) {
        debug("error: couldn't init a process\n");
    }

    (0xc0000000-(uint32_t)&__userland_mapped__+run_task)(&proc1);
    //run_task(&proc1);
}
