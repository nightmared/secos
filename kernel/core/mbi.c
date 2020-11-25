#include <mbi.h>
#include <info.h>
#include <debug.h>

extern info_t   *info;

void print_mbi_memory_headers() {
    if (info->mbi->flags & MBI_FLAG_MMAP) {
        memory_map_t* mmap_info = (memory_map_t*)info->mbi->mmap_addr;
        while ((uint32_t)mmap_info < (uint32_t)(info->mbi->mmap_addr+info->mbi->mmap_length)) {
            char* type;
            switch (mmap_info->type) {
                case MULTIBOOT_MEMORY_AVAILABLE:
                    type = "available";
                    break;
                case MULTIBOOT_MEMORY_RESERVED:
                    type = "system reserved";
                    break;
                case MULTIBOOT_MEMORY_ACPI_RECLAIMABLE:
                    type = "ACPI";
                    break;
                default:
                    type = "unknown";
            }
            printf("0x%-8x - 0x%-9llx (size: 0x%x bytes, usage: %s)\n", (uint32_t)mmap_info->addr, mmap_info->addr+mmap_info->len, (uint32_t)mmap_info->len, type);
            mmap_info = (memory_map_t*)((uint32_t)mmap_info+mmap_info->size+sizeof(mmap_info->size));
        }
    }
}

bool_t mbi_get_next_available_memory(uint8_t *offset, struct mbi_available_memory *res) {
    if (!(info->mbi->flags & MBI_FLAG_MMAP)) {
        return false;
    }

    memory_map_t* mmap_info = (memory_map_t*)info->mbi->mmap_addr;
    uint32_t max_length = (uint32_t)(info->mbi->mmap_addr+info->mbi->mmap_length);

    uint8_t i = 0;
    while ((uint32_t)mmap_info < max_length) {
        // skip until the offset
        if (i >= *offset && mmap_info->type == MULTIBOOT_MEMORY_AVAILABLE) {
            res->nb_4k_pages = mmap_info->len/(1<<12);
            res->base_ptr = (void*)(uint32_t)mmap_info->addr;
            *offset = i;
            return true;
        }

        i++;
        mmap_info = (memory_map_t*)((uint32_t)mmap_info+mmap_info->size+sizeof(mmap_info->size));
    }

    return false;
}

void mbi_get_available_memory_description(uint8_t *len, struct mbi_available_memory res[*len]) {
    uint8_t new_len = 0;
    uint8_t offset = 0;
    for (int i = 0; i < *len; i++) {
        if (!mbi_get_next_available_memory(&offset, &res[new_len])) {
            break;
        }
        offset++;
        new_len++;
    }
    *len = new_len;
}
