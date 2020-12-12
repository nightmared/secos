#include <scheduler.h>
#include <segmem.h>
#include <alloc.h>
#include <gdt.h>

struct page process_list_reserved_memory[4];
struct elem_entry *process_list_heap;
struct page process_shared_info[2];
struct elem_entry *process_shared_info_heap;

void prepare_scheduler(void) {
    process_list_heap = create_heap(4, process_list_reserved_memory);
    if (process_list_heap == NULL) {
        debug("prepare_scheduler: couldn't populate a heap for storing the userland tasks\n");
        panic("halted !");
    }

    process_shared_info_heap = create_heap(2, process_shared_info);
    if (process_shared_info_heap == NULL) {
        debug("prepare_scheduler: couldn't populate a heap for storing the metadata regarding shared mappings\n");
        panic("halted !");
    }
}

bool_t init_process(struct process *out_process, void* fun) {
    // get an unused task id
    uint16_t min = 1;
    struct elem_entry *list = process_list_heap->next_entry;
    while (list->next_entry != NULL) {
        struct process *p = (struct process*)((uint32_t)list+sizeof(struct elem_entry));
        if (p->task_id >= min) {
            min = p->task_id+1;
        }

        list = list->next_entry;
    }
    out_process->task_id = min;

    if (init_process_memory(out_process) == NULL) {
        debug("init_process: couldn't allocate memory for the process\n");
        return false;
    }

    out_process->context->eip.raw = (uint32_t)fun;
    out_process->context->esp.raw = (uint32_t)out_process->userland_stack+PG_4K_SIZE;
    out_process->context->cs.raw = gdt_seg_sel(gdt_code_idx+GDT_RING3_OFFSET, 3);
    out_process->context->ss.raw = gdt_seg_sel(gdt_data_idx+GDT_RING3_OFFSET, 3);

    return true;
}

inline void __attribute__((always_inline)) update_tss(struct process *p) {
    tss_t *kernel_tss = (tss_t*)&__tss_start__;
    //uint32_t esp0 = 0;
    //asm volatile("mov %%esp, %0" : "=m"(esp0));
    //kernel_tss->s0.esp = esp0;
    //TODO: fix this ?
    kernel_tss->s0.esp = (uint32_t)p->kernelland_stack+PG_4K_SIZE;
    asm volatile("ltr %%ax" :: "a"(gdt_seg_sel(gdt_tss_idx, 0)));
}

void __attribute__((section(".userland_shared_code"))) run_task(struct process *p) {
    update_tss(p);
    asm volatile(
        "mov %4, %%cr3 \n\t"
        "mov %3, %%ds \n\t"
        "mov %3, %%es \n\t"
        "mov %3, %%fs \n\t"
        "mov %3, %%gs \n\t"
        "push %3 \n\t"
        "push %2 \n\t"
        "pushf \n\t"
        "push %1 \n\t"
        "push %0 \n\t"
        "iret"
        ::
        "r"(p->context->eip.raw),
        "r"(p->context->cs.raw),
        "r"(p->context->esp.raw),
        "r"(p->context->ss.raw),
        "r"(p->pdt)
    );
}
