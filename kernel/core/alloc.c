#include <alloc.h>
#include <scheduler.h>

extern int __userland_start__, __userland_end__;

// create the necessary PDEs/PTEs and return a pointer to the allocated memory region
void* alloc_contiguous_pages(uint32_t nb_pages) {
    for (uint8_t cur_mem_region; cur_mem_region < nb_available_mem_regions; cur_mem_region++) {
        uint32_t base_addr = (uint32_t)available_mem_regions[cur_mem_region].base_ptr;
        uint32_t region_len_pages = available_mem_regions[cur_mem_region].nb_4k_pages;

        // skip the first 4k pages to prevent conflicts with the NULL pointer
        if (base_addr == 0) {
            base_addr += PG_4K_SIZE;
        }

        // it is useless to scan a memory region when it is too small to hold the allocation
        if (nb_pages > region_len_pages) {
            continue;
        }

        pde32_t *pde = NULL;
        pte32_t *pt = NULL;

        uint32_t cur_addr = base_addr;
        uint32_t new_region_start = 0;
        uint32_t found_avail_pages = 0;
        while (cur_addr < base_addr+((region_len_pages+1)<<PG_4K_SHIFT)) {
            pde = pdt+pd32_idx(cur_addr);
            if (pde->p == 1) {
                pt = (pte32_t*)(pde->addr<<PG_4K_SHIFT)+pt32_idx(cur_addr);
            } else {
                if (found_avail_pages == 0) {
                    new_region_start = cur_addr;
                }
                if (pt32_idx(cur_addr) == 0) {
                    found_avail_pages += 1024;
                    cur_addr += PG_4M_SIZE;
                    continue;
                }
            }

            if (pt == NULL || pt->p == 0) {
                if (found_avail_pages == 0) {
                    new_region_start = cur_addr;
                }
                found_avail_pages++;
            } else {
                found_avail_pages = 0;
            }

            if (found_avail_pages >= nb_pages) {
                break;
            }

            cur_addr += PG_4K_SIZE;
        }

        if (found_avail_pages >= nb_pages) {
            for (uint16_t pde_idx = pd32_idx(new_region_start); pde_idx < pd32_idx(new_region_start)+(nb_pages>>10)+1; pde_idx++) {
                pde = pdt+pde_idx;
                if (pde->p == 0) {
                    // claim the necessary pdes
                    pde->p = 1;
                }
            }
            for (uint32_t page_idx = 0; page_idx < nb_pages; page_idx++) {
                get_pte_for_addr(pdt, new_region_start+(page_idx<<PG_4K_SHIFT))->p = 1;
            }

            return (void*)new_region_start;
        }
    }

    return NULL;
}

bool_t map_contiguous_pages_in_process(struct process *p, uint32_t virt_addr, uint32_t nb_pages, uint32_t phys_addr) {
    uint32_t missing_pts = 0;
    sint32_t last_pd32_idx = -1;
    for (uint32_t addr = virt_addr; addr < virt_addr+(nb_pages<<PG_4K_SHIFT); addr += PG_4K_SIZE) {
        if ((p->pdt+pd32_idx(addr))->p == 0 && (sint32_t)pd32_idx(addr) != last_pd32_idx) {
            last_pd32_idx = pd32_idx(addr);
            missing_pts++;
        }
    }

    // allocate space for the PTEs
    if (missing_pts != 0) {
        pte32_t *phys_allocated_pts = (pte32_t*)alloc_contiguous_pages(missing_pts);
        if (phys_allocated_pts == NULL) {
            debug("process_alloc_contiguous_pages: cannot allocate the PTs necessary to hold an allocation\n");
            return false;    
        }
        memset(phys_allocated_pts, 0, nb_pages<<PG_4K_SHIFT);

        uint32_t current_new_pt = 0;
        // map the PTs in the PDT
        for (uint32_t i = 0; i < nb_pages; i++) {
            uint32_t addr = virt_addr+(i<<PG_4K_SHIFT);
            pde32_t *pde = p->pdt+pd32_idx(addr);

            if (pde->p == 0) {
                pde->lvl = 1;
                pde->p = 1;
                pde->rw = 1;
                pde->addr = ((uint32_t)phys_allocated_pts+current_new_pt)>>PG_4K_SHIFT;
                current_new_pt++;
            }
        }
    }

    // set up the PTEs
    for (uint32_t i = 0; i < nb_pages; i++) {
        uint32_t addr = virt_addr+(i<<PG_4K_SHIFT);
        pte32_t *pte = get_pte_for_addr(p->pdt, addr);
        pte->p = 1;
        pte->rw = 1;
        pte->lvl = 1;
        pte->addr = (phys_addr>>PG_4K_SHIFT)+i;
    }

    return true;
}

void* process_alloc_contiguous_pages(struct process *p, uint32_t virt_addr, uint32_t nb_pages, uint8_t type) {
    if (p->allocs->size == 255) {
        return NULL;
    }

    uint32_t phy_allocated_addr = (uint32_t)alloc_contiguous_pages(nb_pages);
    if (phy_allocated_addr == 0) {
        debug("process_alloc_contiguous_pages: couldn't allocate %d page(s)\n", nb_pages);
        return NULL;
    }

    struct vmem_contiguous_alloc *info = &p->allocs->allocated_vmem_pages[p->allocs->size];
    p->allocs->size++;

    // TODO: check that the corresponding virtual memory is not already allocated

    info->start = virt_addr;
    info->end = virt_addr+(nb_pages<<PG_4K_SHIFT);
    info->type = type;

    // if the type is shared, allocate the shared_info in the heap
    if (type == MEM_SHARED) {
        struct phys_mem_shared_region *new_shared_region = kmalloc(process_shared_info_heap, sizeof(struct phys_mem_shared_region));
        if (new_shared_region == NULL) {
            debug("process_alloc_contiguous_pages: couldn't create a new metadata entry for a shared region\n");
            free_contiguous_pages(pdt, phy_allocated_addr, phy_allocated_addr+(nb_pages<<PG_4K_SHIFT));
            p->allocs->size--;
            return NULL;
        }
        new_shared_region->nb_owners = 1;
        info->shared_info = new_shared_region;
    }

    if (!map_contiguous_pages_in_process(p, virt_addr, nb_pages, phy_allocated_addr)) {
        debug("map_contiguous_pages_in_process: couldn't map the pages\n");
        free_contiguous_pages(pdt, phy_allocated_addr, phy_allocated_addr+(nb_pages<<PG_4K_SHIFT));

        return NULL;
    }

    return (void*)phy_allocated_addr;
}

bool_t process_add_shared_mem_region(struct process *p, uint32_t virt_addr, uint32_t phys_addr, struct phys_mem_shared_region *metadata, uint32_t nb_pages) {
    if (p->allocs->size == 255) {
        return false;
    }

    struct vmem_contiguous_alloc *info = &p->allocs->allocated_vmem_pages[p->allocs->size];
    p->allocs->size++;
    
    info->start = virt_addr;
    info->end = virt_addr+(nb_pages<<PG_4K_SHIFT);
    info->type = MEM_SHARED;
    info->shared_info = metadata;

    info->shared_info->nb_owners++;

    if (!map_contiguous_pages_in_process(p, virt_addr, nb_pages, phys_addr)) {
        debug("map_contiguous_pages_in_process: couldn't map the pages\n");
        p->allocs->size--;

        return false;
    }

    return true;
}

// allocate a kernel PDE per process (4M):
// 0-1: PTTs that describes this very struct ;)
// 2: interrupt description context (save esp, ss and register state for context switching)
// 3: PDTs
// 4: process_allocs (auxiliary allocations done for the process + memory shared with other processes (and thus reference-counted))
// 5: guard page
// 6: 4K kernel stack
// 7-510: unused, reserved for a possible future extension of the kernelland stack of up to 2M (minus 6 4K pages)
// 511: PTT that describe the kernel shared mapping
// 512: guard page
// 513: 4K user stack
// 514-1024: unused, reserved for a possible future extension of the userland stack of up to 2M (minus the 4K guard page)
void* init_process_memory(struct process *p) {
    void *kernel_addr = alloc_contiguous_pages(PG_4M_SIZE>>PG_4K_SHIFT);
    if (kernel_addr == NULL) {
        debug("init_process: couldn't allocate a PDE for the process\n");

        return NULL;
    }

    p->process_memory = kernel_addr;
    p->pdt = (pde32_t*)((uint32_t)kernel_addr+(3<<PG_4K_SHIFT));
    p->allocs = (struct process_allocs*)((uint32_t)kernel_addr+(4<<PG_4K_SHIFT));
    p->allocs->cr3 = p->pdt;
    p->context = (int_ctx_t*)((uint32_t)kernel_addr+(2<<PG_4K_SHIFT));
    p->kernelland_stack = (void*)((uint32_t)kernel_addr+(6<<PG_4K_SHIFT));
    p->userland_stack = (void*)((uint32_t)kernel_addr+(513<<PG_4K_SHIFT));

    memset(kernel_addr, 0, PG_4M_SIZE);

    // <3 identity mapping
    uint32_t cur_addr = (uint32_t)kernel_addr;
    for (int i = 0; i < 1024; i++) {
        if ((p->pdt+pd32_idx(cur_addr))->p == 0) {
            (p->pdt+pd32_idx(cur_addr))->p = 1;
            (p->pdt+pd32_idx(cur_addr))->lvl = 1;
            (p->pdt+pd32_idx(cur_addr))->rw = 1;
            // black magic to select the correct PT address
            (p->pdt+pd32_idx(cur_addr))->addr = ((uint32_t)kernel_addr>>PG_4K_SHIFT)+(pd32_idx(cur_addr)&1);
        }

        pte32_t *pte = get_pte_for_addr(p->pdt, cur_addr);
        pte->addr = ((uint32_t)kernel_addr>>PG_4K_SHIFT)+i;
        pte->rw = 1;

        if (i < 512) {
            pte->lvl = 0;
        } else {
            pte->lvl = 1;
        }

        if (i <= 4 || i == 6 || i == 511 || i == 513) {
            pte->p = 1;
        }

        cur_addr += PG_4K_SIZE;
    }

    // map the kernel shared memory read-only
    pte32_t *shared_pt = (pte32_t*)((uint32_t)kernel_addr+(511<<PG_4K_SHIFT));
    (p->pdt+pd32_idx(0xc0000000))->p = 1;
    (p->pdt+pd32_idx(0xc0000000))->lvl = 1;
    (p->pdt+pd32_idx(0xc0000000))->rw = 0;
    (p->pdt+pd32_idx(0xc0000000))->addr = (uint32_t)shared_pt>>PG_4K_SHIFT;
    setup_shared_pde(shared_pt);

    // map the userland code (with identity mapping of course ;))
    uint32_t size_userland_section = (uint32_t)&__userland_end__-(uint32_t)&__userland_start__;
    uint32_t nb_pages_userland = (size_userland_section>>PG_4K_SHIFT)+((size_userland_section&0x3ff) ? 1 : 0);
    if (!map_contiguous_pages_in_process(p, (uint32_t)&__userland_start__, nb_pages_userland, (uint32_t)&__userland_start__)) {
        debug("init_process_memory: couldn't map the userland pages\n");
        free_contiguous_pages(pdt, (uint32_t)kernel_addr, (uint32_t)kernel_addr+PG_4M_SIZE);

        return NULL;
    }

    return kernel_addr;
}


// free pages that are contiguous in *physical* memory
void free_contiguous_pages(pde32_t *cr3, uint32_t start, uint32_t end) {
    // get physical adresses (the kernel is thankfully identity mapped !) and switch them to "not-present"
    for (uint32_t i = 0; i < end+PG_4K_SIZE; i += PG_4K_SIZE) {
        pte32_t *pte = get_pte_for_addr(cr3, start+i);
        pte32_t *phys_pte = get_pte_for_addr(pdt, ((uint32_t)pte->addr)<<PG_4K_SHIFT);

        if (pte == NULL || phys_pte == NULL) {
            debug("free_contiguous_pages: invalid memory region: got a null PTE\n");
            return;
        }

        pte->p = 0;
        phys_pte->p = 0;
    }

    // cleanup PDEs when necessary
    pte32_t *start_pte = get_pte_for_addr(cr3, start);
    pte32_t *end_pte = get_pte_for_addr(cr3, end);
    uint32_t phys_start_page = start_pte->addr<<PG_4K_SHIFT;
    uint32_t phys_end_page = end_pte->addr<<PG_4K_SHIFT;

    uint32_t nb_pde = (phys_end_page-phys_start_page)>>PG_4M_SHIFT;

    if (nb_pde > 0) {
        // the allocation start at the beginning of a PDE and spans more than a PDE -> we can reclaim this PDE
        if (pt32_idx(start) == 0) {
            (pdt+pd32_idx(start))->p = 0;
        }

        for (uint32_t i = 1; i < nb_pde; i++) {
            (pdt+pd32_idx(start)+i)->p = 0;
        }
    }
}

void process_list_allocations(struct process *p) {
    printf("Process allocations list for task #%d:\n", p->task_id);
    for (uint32_t i = 0; i < p->allocs->size; i++) {
        struct vmem_contiguous_alloc *alloc = p->allocs->allocated_vmem_pages + i;
        switch (alloc->type) {
        case MEM_PRIVATE:
            printf("%8p - %8p (private)\n", alloc->start, alloc->end);
            break;
        case MEM_SHARED:
            printf("%8p - %8p (shared over %d tasks)\n", alloc->start, alloc->end, alloc->shared_info->nb_owners);
            break;
        default:
            debug("process_list_allocations: invalid allocation type %p\n", alloc->type);
        }
    }
}

// we only support contiguous pages for simplicity
void free_process_allocs(struct process *p) {
    for (uint32_t i = 0; i < p->allocs->size; i++) {
        struct vmem_contiguous_alloc *alloc = p->allocs->allocated_vmem_pages + i;
        switch (alloc->type) {
        case MEM_PRIVATE:
            free_contiguous_pages(p->allocs->cr3, alloc->start, alloc->end);
            break;
        case MEM_SHARED:
            // Reference-counting in action ;)
            alloc->shared_info->nb_owners--;
            if (alloc->shared_info->nb_owners == 0) {
                kfree(process_shared_info_heap, alloc->shared_info);
                free_contiguous_pages(p->allocs->cr3, alloc->start, alloc->end);
            }
            break;
        default:
            debug("free_process_alloc: invalid allocation type %p\n", alloc->type);
        }
    }
}

void free_process_memory(struct process *p) {
    free_process_allocs(p);
    free_contiguous_pages(pdt, (uint32_t)p->process_memory, (uint32_t)p->process_memory+PG_4M_SIZE);
}

// create the following memory "heap":
// -----------------------------------------------------------------------------------------------
// | PAGE 1                   | PAGE 2                         | PAGE 3                          |
// | [first_entry] ................................................................ [last_entry] |
// -----------------------------------------------------------------------------------------------
// Warning: first_entry and last_entry must not be freed ! (but we protect against this anyway)
struct elem_entry* create_heap(uint32_t nb_pages, struct page p[nb_pages]) {
    struct elem_entry* first_entry = (struct elem_entry*)p;
    struct elem_entry* last_entry = (struct elem_entry*)((uint32_t)(p+nb_pages-1)+sizeof(struct page)-sizeof(struct elem_entry));
    first_entry->previous_entry = NULL;
    first_entry->next_entry = last_entry;
    first_entry->size = 0;

    last_entry->previous_entry = first_entry;
    last_entry->next_entry = NULL;
    last_entry->size = 0;

    return first_entry;
}

struct elem_entry* find_elem_before_free(struct elem_entry *heap, uint32_t needed_size) {
    struct elem_entry *cur = heap;

    while (cur != NULL && cur->next_entry != NULL) {
        uint32_t distance_data_size = (uint32_t)cur->next_entry - ((uint32_t)cur + cur->size + sizeof(struct elem_entry));

        if (distance_data_size >= sizeof(struct elem_entry) + needed_size) {
            return cur;
        }

        cur = cur->next_entry;
    }

    return NULL;
}

// not great for fragmentation, but who gives a shit in a tiny kernel like this ?
void* kmalloc(struct elem_entry* heap, uint32_t size) {
    struct elem_entry* entry = find_elem_before_free(heap, size);
    if (entry == NULL) {
        return NULL;
    }

    struct elem_entry* new_alloc = (struct elem_entry*)((uint32_t)entry + entry->size + sizeof(struct elem_entry));
    new_alloc->previous_entry = entry;
    new_alloc->next_entry = entry->next_entry;
    new_alloc->previous_entry->next_entry = new_alloc;
    new_alloc->next_entry->previous_entry = new_alloc;
    new_alloc->size = size;

    return (void*)((uint32_t)new_alloc + sizeof(struct elem_entry));
}

void kfree(struct elem_entry* heap, void *ptr) {
    struct elem_entry* source_heap = heap;

    if ((uint32_t)ptr <= (uint32_t)heap) {
        goto out_of;
    }

    // skip the first entry
    heap = heap->next_entry;

    while (heap->next_entry != NULL) {
        if ((uint32_t)ptr == (uint32_t)heap + sizeof(struct elem_entry)) {
            heap->previous_entry->next_entry = heap->next_entry;
            heap->next_entry->previous_entry= heap->previous_entry;
            return;
        }

        heap = heap->next_entry;
    }

out_of:
    debug("free: invalid address outside of target heap: PTR@%p out of HEAP@%p\n,", ptr, source_heap);
}
