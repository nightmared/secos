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

static __attribute__((section(".userland_data"))) char show_int[] = "%d\n";

void __attribute__((section(".userland_code"))) task1() {
    uint32_t v = 0;
    while (1) {
        userland_execute_syscall(2, SYSCALL_PRINTF, (uint32_t)show_int, v);
        v++;
        //sleep(250);
    }
}

void __attribute__((section(".userland_code"))) task2() {
    while (1) {
        //userland_execute_syscall(4, 0, 1, 2, 3, 4);
        //sleep(250);
    }
}

void tp() {
    printf("\nMultiboot memory information:\n");
    print_mbi_memory_headers();

    init_gdt_flat();
    printf("\nGDT description:\n");
    print_gdt();

    // we need to configure paging prior to enabling interrupts because we rellocated the interrupt handlers to the shared memory mapping located at 0xc0000000
    enable_paging();

    // enable interrupts
    intr_init();
    pit_init();
    asm volatile("sti");

   // now that the mapping at 0xc0000000 is on, update the gdtr to point to this region so that the gdt and the tss stay valid when running in user tasks
    mygdt = (seg_desc_t*)(0xc0000000+(uint32_t)&__gdt_start__-(uint32_t)&__userland_mapped__);
    update_gdtr();
    uint32_t new_tss_addr = 0xc0000000+(uint32_t)&__tss_start__-(uint32_t)&__userland_mapped__;
    init_segment(&mygdt[gdt_tss_idx], new_tss_addr, new_tss_addr+sizeof(tss_t));
    mygdt[gdt_tss_idx].d = 0;
    mygdt[gdt_tss_idx].g = 0;
    mygdt[gdt_tss_idx].s = 0; // system kind
    mygdt[gdt_tss_idx].type = SEG_DESC_SYS_TSS_AVL_32;
    mygdt[gdt_tss_idx].dpl = 3;

    // init the kernel TSS
    tss_t *kernel_tss = (tss_t*)&__tss_start__;
    memset(kernel_tss, 0, sizeof(tss_t));
    kernel_tss->s0.ss = gdt_seg_sel(gdt_data_idx, 0);

    prepare_scheduler();

    struct process *proc1, *proc2;
    if ((proc1 = init_process(task1)) == NULL) {
        debug("error: couldn't init a process\n");
        return;
    }
    if ((proc2 = init_process(task2)) == NULL) {
        debug("error: couldn't init a process\n");
        return;
    }

    // alloc a shared memory region
    void* shared_region = process_alloc_contiguous_pages(proc1, 0x200000, 1, MEM_SHARED);
    if (shared_region == NULL) {
        debug("error: cannot init a shared region\n");
        return;
    }

    struct phys_mem_shared_region *shared_info = (proc1->allocs->allocated_vmem_pages+proc1->allocs->size-1)->shared_info;
    if (!process_add_shared_mem_region(proc2, 0x300000, (uint32_t)shared_region, shared_info, 1)) {
        debug("error: couldn't map the shared region in the second process\n");
        return;
    }

    print_pdt(pdt);
    print_pdt(proc1->pdt);
    print_pdt(proc2->pdt);
    printf("\n");
    process_list_allocations(proc1);
    process_list_allocations(proc2);

    run_task(proc1);
}
