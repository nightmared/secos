#ifndef __SCHEDULER_H__
#define __SCHEDULER_H__

#include <types.h>
#include <task.h>

extern struct elem_entry *process_list_heap;
extern struct elem_entry *process_shared_info_heap;

void prepare_scheduler(void);
bool_t init_process(struct process *out_process, void* fun);

void run_task(struct process *p);

#endif // __SCHEDULER_H__
