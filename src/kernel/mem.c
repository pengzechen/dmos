#include <mem.h>
#include <log.h>
#include <mmu.h>
#include <tty/console.h>

static addr_alloc_t g_paddr_alloc;
static pde_t g_kernel_page_dir[PDE_CNT] __attribute__( (aligned(MEM_PAGE_SIZE)) );


static void 
addr_alloc_init(addr_alloc_t * alloc, uint8_t* bits, uint32_t start, 
uint32_t size, uint32_t page_size) {
    mutex_init(&alloc->mutex);
    alloc->start = start;
    alloc->size = size;
    alloc->page_size = page_size;
    bitmap_init(&alloc->bitmap, bits, alloc->size / page_size, 0);
}


static uint32_t 
addr_alloc_page (addr_alloc_t * alloc, int page_count) {
    uint32_t addr = 0;

    mutex_lock(&alloc->mutex);

    int page_index = bitmap_alloc_nbits(&alloc->bitmap, 0, page_count);
    if (page_index >= 0) {
        addr = alloc->start + page_index * alloc->page_size;
    }

    mutex_unlock(&alloc->mutex);
    return addr;
}


static uint32_t
addr_free_page(addr_alloc_t * alloc, uint32_t addr, int page_count) {
    mutex_lock(&alloc->mutex);
    uint32_t pg_index = (addr - alloc->start) / alloc->page_size;
    bitmap_set_bit(&alloc->bitmap, pg_index, page_count, 0);

    mutex_unlock(&alloc->mutex);
}



// _________________________________________
//       测试
static void 
some_test() {
    addr_alloc_t addr_alloc;
    uint8_t bits[8];
    addr_alloc_init(&addr_alloc, bits, 0x1000, 64*4096, 4096);
    for(int i=0; i<32; i++) {
        uint32_t addr = addr_alloc_page(&addr_alloc, 2);
        klog("alloc addr: 0x%x", addr);
    }
    uint32_t addr = 0x1000;
    for(int i=0; i<32; i++) {
        addr_free_page(&addr_alloc, addr, 2);
        addr += 8192;
        klog("free addr: 0x%x", addr);
    }
}
static void        
show_mem_info(boot_info_t* boot_info) {
    klog("mem region");
    for(int i=0; i<boot_info->ram_region_count; i++) {
        klog("[%d]: 0x%x - 0x%x", i, 
            boot_info->ram_region_cfg[i].start, 
            boot_info->ram_region_cfg[i].size );
    }
    klog("");
}
static uint32_t 
total_mem_size(boot_info_t* boot_info) {
    uint32_t mem_size = 0;
    for(int i=0; i<boot_info->ram_region_count; i++) {
        mem_size += boot_info->ram_region_cfg[i].size;
    }
    return mem_size;
}

//___________________________________________


pte_t * 
find_pte (pde_t * page_dir, uint32_t vaddr, int alloc) {
    pte_t * page_table;

    pde_t *pde = page_dir + pde_index(vaddr);
    if (pde->present) {
        page_table = (pte_t *)pde_paddr(pde);
    } else {
        if (alloc == 0) {
            return (pte_t *)0;
        }
        uint32_t pg_paddr = addr_alloc_page(&g_paddr_alloc, 1);
        if (pg_paddr == 0) {
            return (pte_t *)0;
        }
        pde->v = pg_paddr | PDE_P | PDE_W | PDE_U;

        page_table = (pte_t *)(pg_paddr);
        k_memset(page_table, 0, MEM_PAGE_SIZE);
    }

    return page_table + pte_index(vaddr);
}


int // 创建映射
memory_create_map (pde_t * page_dir, uint32_t vaddr, uint32_t paddr, int count, uint32_t perm) {
    for (int i = 0; i < count; i++) {
        // klog("create map: v-0x%x p-0x%x, perm: 0x%x", vaddr, paddr, perm);

        pte_t * pte = find_pte(page_dir, vaddr, 1);
        if (pte == (pte_t *)0) {
            return -1;
        }
        if (pte->present != 0) klog("error111");

        // klog("pte addr: 0x%x", (uint32_t)pte);

        pte->v = paddr | perm | PTE_P;
        vaddr += MEM_PAGE_SIZE;
        paddr += MEM_PAGE_SIZE;
    }

    return 0;
}


void 
create_kernel_table (void) {
    extern uint8_t s_text[], e_text[], s_data[], e_data[];
    extern uint8_t kernel_base[];

    static memory_map_t kernel_map[] = {
        {kernel_base,              s_text,                         0,                           PTE_W},      // 内核栈区
        {s_text,                   e_text,                         s_text,                      0    },      // 内核代码区
        {s_data,                   (void *)(MEM_EBDA_START),       s_data,                      PTE_W},      // 内核数据区
        {(void*)CONSOLE_DISP_ADDR, (void*)CONSOLE_DISP_END,        (void*)CONSOLE_DISP_ADDR,    PTE_W},      // 显存区域
        {(void*)MEM_EXT_START,     (void*)MEM_EXT_END,             (void*)MEM_EXT_START,        PTE_W},      // 剩余所有区域
    };

    for (int i = 0; i < sizeof(kernel_map) / sizeof(memory_map_t); i++) {
        memory_map_t * map = kernel_map + i;

        int vstart = down2((uint32_t)map->vstart, MEM_PAGE_SIZE);
        int vend   = up2  ((uint32_t)map->vend, MEM_PAGE_SIZE);
        int page_count = (vend - vstart) / MEM_PAGE_SIZE;

        memory_create_map(g_kernel_page_dir, vstart, (uint32_t)map->pstart, page_count, map->perm);
    }
}


void 
memory_init (boot_info_t* boot_info) {
    extern uint8_t * mem_free_start;
    uint8_t * mem_free = (uint8_t*)&mem_free_start;
    klog("memory init");
    show_mem_info(boot_info);

    
    uint32_t mem_up1MB_free = total_mem_size(boot_info) - MEM_EXT_START;
    mem_up1MB_free = down2(mem_up1MB_free, MEM_PAGE_SIZE);

    klog("free memory: 0x%x, size: 0x%x", MEM_EXT_START, mem_up1MB_free);
    addr_alloc_init(&g_paddr_alloc, mem_free, MEM_EXT_START, mem_up1MB_free, MEM_PAGE_SIZE);

    mem_free += bitmap_byte_count(g_paddr_alloc.size / MEM_PAGE_SIZE);
    if (mem_free < (uint8_t *)0x800000) {
        klog("simple test ok!");
    }

    k_memset(g_kernel_page_dir, 0, sizeof(g_kernel_page_dir));  // 清空内核页表目录

    create_kernel_table();


    // 在 cr3 寄存器中写入页表目录  // 重新设置页表目录
    mmu_set_page_dir((uint32_t)g_kernel_page_dir);              
    
}


uint32_t 
memory_create_uvm() {
    pde_t* page_dir = (pde_t*)addr_alloc_page(&g_paddr_alloc, 1);
    if(page_dir == (pde_t*)0) {
        return 0;  // error
    }

    k_memset((void*)page_dir, 0, MEM_PAGE_SIZE);
    uint32_t user_pde_start = pde_index( MEMORY_TASK_BASE );
    
    for(int i=0; i<user_pde_start; i++) {
        page_dir[i].v = g_kernel_page_dir[i].v;
    }

    return (uint32_t)page_dir;
}


uint32_t 
memory_alloc_for_page_dir (uint32_t page_dir, uint32_t vaddr, uint32_t size, int perm) {
    uint32_t curr_vaddr = vaddr;
    int page_count = up2(size, MEM_PAGE_SIZE) / MEM_PAGE_SIZE;
    vaddr = down2(vaddr, MEM_PAGE_SIZE);

    for (int i = 0; i < page_count; i++) {
        uint32_t paddr = addr_alloc_page(&g_paddr_alloc, 1);
        if (paddr == 0) {
            klog("mem alloc failed. no memory");
            return 0;
        }

        int err = memory_create_map((pde_t *)page_dir, curr_vaddr, paddr, 1, perm);
        if (err < 0) {
            klog("create memory map failed. err = %d", err);
            addr_free_page(&g_paddr_alloc, vaddr, i + 1);
            return -1;
        }

        curr_vaddr += MEM_PAGE_SIZE;
    }

    return 0;
}


int 
memory_alloc_page_for(uint32_t addr, uint32_t size, int perm) {
    uint32_t cr3 = task_current()->tss.cr3;
    return memory_alloc_for_page_dir(cr3, addr, size, perm);
}


// 0x80000000 以下
uint32_t 
memory_alloc_page() {
    uint32_t addr = addr_alloc_page(&g_paddr_alloc, 1);
    return addr;
}


static pde_t* 
curr_page_dir() {
    return (pde_t*)( task_current()->tss.cr3 );
}


void 
memory_free_page(uint32_t addr) {
    if( addr < MEMORY_TASK_BASE ) {
        addr_free_page(&g_paddr_alloc, addr, 1);
    } else {
        pte_t* pte = find_pte(curr_page_dir(), addr, 0);
        if(pte == 0 && !pte->present) klog("free err");

        addr_free_page(&g_paddr_alloc, pte_paddr(pte), 1);
        pte->v = 0;
    }
}


uint32_t 
memory_get_paddr (uint32_t page_dir, uint32_t vaddr) {
    pte_t * pte = find_pte((pde_t *)page_dir, vaddr, 0);
    if (pte == (pte_t *)0) {
        return 0;
    }

    return pte_paddr(pte) + (vaddr & (MEM_PAGE_SIZE - 1));
}


void 
memory_destory_uvm (uint32_t page_dir) {
    uint32_t user_pde_start = pde_index(MEMORY_TASK_BASE);
    pde_t * pde = (pde_t *)page_dir + user_pde_start;

    for (int i = user_pde_start; i < PDE_CNT; i++, pde++) {
        if (!pde->present) {
            continue;
        }
        pte_t * pte = (pte_t *)pde_paddr(pde);
        for (int j = 0; j < PTE_CNT; j++, pte++) {
            if (!pte->present) {
                continue;
            }
            addr_free_page(&g_paddr_alloc, pte_paddr(pte), 1);
        }
        addr_free_page(&g_paddr_alloc, (uint32_t)pde_paddr(pde), 1);
    }
    addr_free_page(&g_paddr_alloc, page_dir, 1);
}


uint32_t 
memory_copy_uvm (uint32_t page_dir) {
    uint32_t to_page_dir = memory_create_uvm();
    if (to_page_dir == 0) {
        goto copy_uvm_failed;
    }

    uint32_t user_pde_start = pde_index(MEMORY_TASK_BASE);
    pde_t * pde = (pde_t *)page_dir + user_pde_start;

    for (int i = user_pde_start; i < PDE_CNT; i++, pde++) {
        if (!pde->present) {
            continue;
        }
        pte_t * pte = (pte_t *)pde_paddr(pde);
        for (int j = 0; j < PTE_CNT; j++, pte++) {
            if (!pte->present) {
                continue;
            }
            uint32_t page = addr_alloc_page(&g_paddr_alloc, 1);
            if (page == 0) {
                goto copy_uvm_failed;
            }
            uint32_t vaddr = (i << 22) | (j << 12);
            int err = memory_create_map((pde_t *)to_page_dir, vaddr, page, 1, get_pte_perm(pte));
            if (err < 0) {
                goto copy_uvm_failed;
            }
            k_memcpy((void *)page, (void *)vaddr, MEM_PAGE_SIZE);
        }
    }
    return to_page_dir;


copy_uvm_failed:
    if (to_page_dir) {
        memory_destory_uvm(to_page_dir);
    }
    return -1;
}


int 
memory_copy_uvm_data(uint32_t to, uint32_t page_dir, uint32_t from, uint32_t size) {
    char *buf, *pa0;

    while(size > 0){
        uint32_t to_paddr = memory_get_paddr(page_dir, to);
        if (to_paddr == 0) {
            return -1;
        }
        uint32_t offset_in_page = to_paddr & (MEM_PAGE_SIZE - 1);
        uint32_t curr_size = MEM_PAGE_SIZE - offset_in_page;
        if (curr_size > size) {
            curr_size = size;       // 如果比较大，超过页边界，则只拷贝此页内的
        }
        k_memcpy((void *)to_paddr, (void *)from, curr_size);

        size -= curr_size;
        to += curr_size;
        from += curr_size;
  }
  return 0;
}


char * sys_sbrk(int incr) {
    task_t * task = task_current();
    char * pre_heap_end = (char * )task->heap_end;
    int pre_incr = incr;

    // ASSERT(incr >= 0);

    // 如果地址为0，则返回有效的heap区域的顶端
    if (incr == 0) {
        klog("sbrk(0): end = 0x%x", pre_heap_end);
        return pre_heap_end;
    } 
    
    uint32_t start = task->heap_end;
    uint32_t end = start + incr;

    int start_offset = start % MEM_PAGE_SIZE;
    if (start_offset) {
        if (start_offset + incr <= MEM_PAGE_SIZE) {
            task->heap_end = end;
            return pre_heap_end;
        } else {
            uint32_t curr_size = MEM_PAGE_SIZE - start_offset;
            start += curr_size;
            incr -= curr_size;
        }
    }

    if (incr) {
        uint32_t curr_size = end - start;
        int err = memory_alloc_page_for(start, curr_size, PTE_P | PTE_U | PTE_W);
        if (err < 0) {
            klog("sbrk: alloc mem failed.");
            return (char *)-1;
        }
    }

    klog("sbrk(%d): end = 0x%x", pre_incr, end);
    task->heap_end = end;
    return (char * )pre_heap_end;        
}