#ifndef __TASK_H__
#define __TASK_H__

#include <intr.h>
#include <pagemem.h>

struct process {
    // the current limitation is in the range of ~1022 running processes, because we require a full PDT per process, thus an uint16_t is enought to hold any possible id
    uint16_t task_id;
    void* process_memory;
    pde32_t* pdt;
    struct process_allocs *allocs;
    void *kernelland_stack;
    void *userland_stack;
    int_ctx_t *context;
};

#endif // __TASK_H__
