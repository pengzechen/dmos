#ifndef MEM_H
#define MEM_H


#include <comm/types.h>
#include <bitmap.h>
#include <mux.h>
#include <loader/loader.h>

#define MEM_EBDA_START              0x00080000
#define MEM_EXT_START               1024*1024
#define MEM_PAGE_SIZE               4096

typedef struct _addr_alloc_t {
    mutex_t mutex;
    bitmap_t bitmap;

    uint32_t start;
    uint32_t size;
    uint32_t page_size;

} addr_alloc_t;

typedef struct _memory_map_t {
    void * vstart;
    void * vend;
    void * pstart;
    uint32_t perm;
} memory_map_t;



void memory_init (boot_info_t* boot_info);

static inline uint32_t down2(uint32_t size, uint32_t bound) {
    return size & ~ (bound - 1);
}
static inline uint32_t up2(uint32_t size, uint32_t bound) {
    return (size + bound - 1) & ~ (bound - 1);
}

#endif

