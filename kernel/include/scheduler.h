#ifndef __SCHEDULER_H__
#define __SCHEDULER_H__

#include <types.h>
#include <task.h>

extern struct elem_entry *process_list_heap;
extern struct elem_entry *process_shared_info_heap;

extern struct process *current_process;

void prepare_scheduler(void);
void start_scheduler();
struct process *init_process(void* fun);

void run_task(struct process *p);
void switch_to_next_task(int_ctx_t *ctx);

#endif // __SCHEDULER_H__
