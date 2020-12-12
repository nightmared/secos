#ifndef __ALLOC_H__
#define __ALLOC_H__

#include <types.h>
#include <paging.h>
#include <mbi.h>
#include <debug.h>
#include <task.h>

#define MEM_PRIVATE 1
#define MEM_SHARED 2

struct phys_mem_shared_region {
    // reference count the number of owner to deallocate when necessary
    uint32_t nb_owners;
};

struct vmem_contiguous_alloc {
    uint32_t start;
    uint32_t end;
    uint8_t type;
    // information when the page is shared
    struct phys_mem_shared_region *shared_info;
};


// List of all the allocations within the process. This structure Fits within a single page.
struct process_allocs {
    struct vmem_contiguous_alloc allocated_vmem_pages[255];
    uint32_t size;
    // process cr3
    pde32_t *cr3;
};

void* alloc_contiguous_pages(uint32_t nb_pages);
void free_contiguous_pages(pde32_t *cr3, uint32_t start, uint32_t end);
void* init_process_memory(struct process *p);
void* process_alloc_contiguous_pages(struct process *p, uint32_t virt_addr, uint32_t nb_pages, uint8_t type);
bool_t process_add_shared_mem_region(struct process *p, uint32_t virt_addr, uint32_t phys_addr, struct phys_mem_shared_region *metadata, uint32_t nb_pages);
void free_process_allocs(struct process *p);
void free_process_memory(struct process *p);

struct elem_entry {
    struct elem_entry* previous_entry;
    struct elem_entry* next_entry;
    uint32_t size;
};

// use 4k memory pages, keep allocations simple !
struct page {
    char elems[1<<12];
};

struct elem_entry* create_heap(uint32_t nb_pages, struct page p[nb_pages]);
void* kmalloc(struct elem_entry* heap, uint32_t size);
void kfree(struct elem_entry* heap, void *ptr);

#endif // __ALLOC_H__
