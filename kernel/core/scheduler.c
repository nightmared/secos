#include <scheduler.h>
#include <syscall.h>
#include <segmem.h>
#include <alloc.h>
#include <gdt.h>

struct page process_list_reserved_memory[4];
struct elem_entry *process_list_heap;
struct page process_shared_info[2];
struct elem_entry *process_shared_info_heap;

struct process *current_process;
bool_t scheduler_started = false;

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

void start_scheduler() {
    // take the first task if any
    struct elem_entry *list = process_list_heap->next_entry;
    if (list->next_entry == NULL) {
        debug("start_scheduler: no task was registered\n");
        panic("halted");
    }

    current_process = (struct process*)((uint32_t)list+sizeof(struct elem_entry));
    scheduler_started = true;

    run_task(current_process);
}

struct process *init_process(void* fun) {
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

    struct process *out_process = kmalloc(process_list_heap, sizeof(struct process));
    if (out_process == NULL) {
        debug("init_process: allocating an entry in the process_list heap failed\n");
        return NULL;
    }
    out_process->task_id = min;


    if (init_process_memory(out_process) == NULL) {
        debug("init_process: couldn't allocate memory for the process\n");
        return NULL;
    }

    out_process->context->eip.raw = (uint32_t)fun;
    out_process->context->esp.raw = (uint32_t)out_process->userland_stack+PG_4K_SIZE;
    out_process->context->gpr.esp.raw = (uint32_t)out_process->userland_stack+PG_4K_SIZE;
    out_process->context->gpr.ebp.raw = (uint32_t)out_process->userland_stack+PG_4K_SIZE;

    return out_process;
}

inline void __attribute__((always_inline)) __attribute__((section(".userland_shared_code"))) update_tss(struct process *p) {
    tss_t *kernel_tss = (tss_t*)&__tss_start__;
    //uint32_t esp0 = 0;
    //asm volatile("mov %%esp, %0" : "=m"(esp0));
    //kernel_tss->s0.esp = esp0;
    //TODO: fix this ?

    // size_t (*print)(const char *format, ...) = printf;
    kernel_tss->s0.esp = (uint32_t)p->kernelland_stack+PG_4K_SIZE;
    // say that the tss is available and not busy (useful for switching from a task to another)
    mygdt[gdt_tss_idx].type = SEG_DESC_SYS_TSS_AVL_32;
    asm volatile("ltr %%ax" :: "a"(gdt_seg_sel(gdt_tss_idx, 0)));
}

void __attribute__((section(".userland_shared_code"))) __run_task(struct process *p) {
    current_process = p;
    update_tss(p);

    // recopy the context of the process to prepare the return
    p->context->esp.raw -= 4;
    uint32_t esp = p->context->esp.raw - sizeof(gpr_ctx_t);
    void* (*mcpy)(void*, void*, size_t) = memcpy;
    mcpy((void*)esp, &p->context->gpr, sizeof(gpr_ctx_t));
    mcpy((void*)p->context->esp.raw, &p->context->eip.raw, sizeof(uint32_t));

    //size_t (*print)(const char *format, ...) = printf;
    //(print)("%p\n", p->context->eip.raw);
    //(print)("%p\n", p->context->gpr.ebp.raw);
    //(print)("%p\n", p->context->gpr.esp.raw);
    //(print)("%p\n", p->context->esp.raw);
    asm volatile(
        "mov %3, %%ds \n\t"
        "mov %3, %%es \n\t"
        "mov %3, %%fs \n\t"
        "mov %3, %%gs \n\t"
        "mov %2, %%esp \n\t"
        "push %3 \n\t"
        "push %2 \n\t"
        // enables interrupts
        "push $0x202 \n\t"
        "push %1 \n\t"
        "push %0 \n\t"
        "mov %4, %%cr3 \n\t"
        "iret"
        ::
        "r"(userland_return_from_syscall),
        "r"(gdt_seg_sel(gdt_code_idx+GDT_RING3_OFFSET, 3)),
        "r"(esp),
        "r"(gdt_seg_sel(gdt_data_idx+GDT_RING3_OFFSET, 3)),
        "r"(p->pdt)
    );
}

void run_task(struct process *p) {
    // who doesn't like doing manual relocations ?
    (0xc0000000-(uint32_t)&__userland_mapped__+__run_task)(p);
}

void switch_to_next_task(int_ctx_t *ctx) {
    if (!scheduler_started) {
        return;
    }

    printf("Switching from task %d\n", current_process->task_id);

    // save the context of the old running process
    memcpy(current_process->context, ctx, sizeof(int_ctx_t));

    struct elem_entry *p_holder = (struct elem_entry*)((uint32_t)current_process-sizeof(struct elem_entry));
    struct elem_entry *list = p_holder->next_entry;
    if (list->next_entry != NULL) {
        struct process *np = (struct process*)((uint32_t)list+sizeof(struct elem_entry));
        run_task(np);
    } else {
        // restart at the beginning of the list
        while (list->previous_entry->previous_entry != NULL) {
            list = list->previous_entry;
        }
        struct process *np = (struct process*)((uint32_t)list+sizeof(struct elem_entry));
        run_task(np);
    }
    // TODO: handle the case where there is no longer any process in the list
    printf("fuuuuuck\n");
}
