#include <comm/cpu_ins.h>
#include <os_cfg.h>
#include <task.h>
#include <sem.h>
#include <irq.h>
#include <cpu.h>
#include <klib.h>
#include <log.h>
#include <mem.h>
#include <mmu.h>
#include <syscall.h>
#include <comm/elf.h>
#include <fs.h>

static task_manager_t   g_task_manager;
static uint32_t         idle_task_stack[2048];
static task_t           g_task_table[128];
static mutex_t          task_table_mutex;

static void idle_task_func() { for(;;) hlt(); }

void simple_switch(uint32_t **from, uint32_t* to);

void 
task_switch_from_to(task_t* from, task_t* to) {
    #ifndef USE_TSS
    simple_switch(&from->stack, to->stack);  // 使用直接跳转机制
    #else
    far_jump(to->tss_sel, 0);             // 使用 tss 机制
    #endif
}


#ifdef USE_TSS
static int 
tss_init(task_t* task, int flag, uint32_t entry, uint32_t esp) {
    int tss_sel = gdt_alloc_desc();

    segment_desc_set(tss_sel, (uint32_t)&task->tss, sizeof(tss_t), 
        SEG_P_PRESENT | SEG_DPL0 | SEG_TYPE_TSS );

    k_memset(&task->tss, 0, sizeof(tss_t));

    uint32_t kernel_stack = memory_alloc_page();
    if (kernel_stack == 0) {
        goto tss_init_failed;
    }

    int code_sel, data_sel;
    if (flag & TASK_FLAGS_SYSTEM) {
        code_sel = KERNEL_SELECTOR_CS;
        data_sel = KERNEL_SELECTOR_DS;
    } else {
        code_sel = g_task_manager.app_code_sel | SEG_RPL3;
        data_sel = g_task_manager.app_data_sel | SEG_RPL3;
    }

    task->tss.eip  = entry;
    task->tss.esp  = esp;
    task->tss.esp0 = kernel_stack + MEM_PAGE_SIZE;
    task->tss.ss  = data_sel;
    task->tss.ss0 = KERNEL_SELECTOR_DS;
    task->tss.es  = data_sel;
    task->tss.ds  = data_sel;
    task->tss.fs  = data_sel;
    task->tss.gs  = data_sel;
    task->tss.cs  = code_sel;
    
    task->tss.eflags = EFLAGES_DEFAULT | EFLAGS_IF;

    uint32_t page_dir = memory_create_uvm();
    if(page_dir == 0) {
        goto tss_init_failed;
    }
    task->tss.cr3 = page_dir;
    task->tss_sel = tss_sel;

    return 0;

tss_init_failed:

    gdt_free_sel(tss_sel);
    if(kernel_stack) {
        memory_free_page(kernel_stack);
    }
    return -1;
}
#endif

// 初始化一个任务
int 
task_init(task_t* task, const char* name, int flag, uint32_t entry, uint32_t esp) {
    
    #ifndef USE_TSS
    uint32_t* pesp = (uint32_t*)esp;
    if(pesp) {
        *(--pesp) = entry;
        *(--pesp) = 0;
        *(--pesp) = 0;
        *(--pesp) = 0;
        *(--pesp) = 0;
        task->stack = pesp;
    }
    #else
    if (tss_init(task, flag, entry, esp) == -1) return -1;  // 使用 tss 机制
    #endif

    k_strncpy(task->name, name, TASK_NAME_SIZE);        // 进程名
    task->state = TASK_CREATED;                         // 状态  created

    task->time_ticks = TASK_TIME_SLICE_DEFAULT;         //  最大时间片
    task->slice_ticks = TASK_TIME_SLICE_DEFAULT;        //  当前时间片
    task->sleep_ticks = 0;
    task->parent = (task_t*)0;
    task->heap_start = 0;
    task->heap_end = 0;
    task->status = 0;
    list_node_init(&task->all_node); 
    list_node_init(&task->run_node);
    list_node_init(&task->wait_node);

    k_memset(&task->file_table, 0, sizeof(file_t*)*TASK_OFILE_NR);
    
    task->pid = (uint32_t)task;

    irq_state_t state = irq_enter_proection();   //--enter protection
        list_insert_last(&g_task_manager.task_list, &task->all_node);  // 加入到所有队列
    irq_leave_proection(state);  //--leave protection

    return 0;
}

void 
task_start(task_t* task) {
    irq_state_t state = irq_enter_proection();   //--enter protection
    
        task_set_ready(task);                                        // 加入到就绪队列
    
    irq_leave_proection(state);  //--leave protection
}


// 初始化任务管理
void 
task_manager_init() {
    k_memset(g_task_table, 0, sizeof(g_task_table));
    mutex_init(&task_table_mutex);


    int seld = gdt_alloc_desc();
    segment_desc_set(seld, 0x00000000, 0xffffffff, 
        SEG_P_PRESENT | SEG_DPL3 | SEG_S_NORMAL | SEG_TYPE_DATA
        | SEG_TYPE_RW | SEG_D
    );
    g_task_manager.app_data_sel = seld;

    int selc = gdt_alloc_desc();
    segment_desc_set(selc, 0x00000000, 0xffffffff, 
        SEG_P_PRESENT | SEG_DPL3 | SEG_S_NORMAL | SEG_TYPE_CODE 
        | SEG_TYPE_RW | SEG_D
    );
    g_task_manager.app_code_sel = selc;

    list_init(&g_task_manager.ready_list);
    list_init(&g_task_manager.task_list);
    list_init(&g_task_manager.sleep_list);
    g_task_manager.curr_task = get_first_task();
    
    task_init(&g_task_manager.idle_task,  
            "idle  task", 
            TASK_FLAGS_SYSTEM,
            (uint32_t)idle_task_func, 
            (uint32_t)&idle_task_stack[2048]
    );
    task_start(&g_task_manager.idle_task);
}


void first_task_init() {

    void first_task_entry();
    extern uint8_t s_first_task[], e_first_task[];

    uint32_t copy_size = (uint32_t)(e_first_task - s_first_task);
    uint32_t alloc_size = 10 * MEM_PAGE_SIZE;

    uint32_t first_start = (uint32_t)first_task_entry;

    // first_start + alloc_size 栈顶
    task_init(&g_task_manager.first_task, 
            "first task", 0, 
            first_start, 
            first_start + alloc_size );
    
    g_task_manager.first_task.heap_start = (uint32_t)e_first_task;
    g_task_manager.first_task.heap_end = (uint32_t)e_first_task;
    
    mmu_set_page_dir((&g_task_manager)->first_task.tss.cr3);

    memory_alloc_page_for(first_start, alloc_size, PTE_P | PTE_W | PTE_U);
    k_memcpy( (void*)first_start, (void *)&s_first_task, copy_size );


    write_tr((&g_task_manager)->first_task.tss_sel);

    task_start(&g_task_manager.first_task);
}


task_t* 
get_first_task() {
    return &g_task_manager.first_task;
}


task_t* 
task_current() {
    return g_task_manager.curr_task;
}


// 将任务添加到就绪队列 尾部 设置状态为 ready
void 
task_set_ready(task_t* task) {
    if (task == &g_task_manager.idle_task) {  // 空进程不应该加入就绪队列
        return;
    }

    list_insert_last(&g_task_manager.ready_list, &task->run_node);
    task->state = TASK_READY;
}

// 将 特定 任务从就绪队列删除
void 
task_set_block(task_t* task) {
    if (task == &g_task_manager.idle_task) {  // 空进程不应该删除
        return;
    }
    list_delete(&g_task_manager.ready_list, &task->run_node);
    // list_delete_first(&g_task_manager.ready_list);
}

// 主动放弃cpu执行
int  
sys_yield() {

    irq_state_t state = irq_enter_proection();   //--enter protection
    
    if(list_count(&g_task_manager.ready_list) > 1) {
        task_t* curr = task_current();
        task_set_block(curr);
        task_set_ready(curr);

        task_dispatch();
    }
    
    irq_leave_proection(state);  //--leave protection

    return 0;
}

// 从就绪队列中找到一第一个任务
task_t * 
task_next_run() {
    if (list_count(&g_task_manager.ready_list) == 0) {
        return &g_task_manager.idle_task;
    }

    list_node_t * task_node = list_first(&g_task_manager.ready_list);
    return list_node_parent(task_node, task_t, run_node);
}

// 分配一个任务并从当前任务切换过去
void
task_dispatch() {

    irq_state_t state = irq_enter_proection();   //--enter protection
    task_t * to = task_next_run();
    if (to != g_task_manager.curr_task) {
        task_t * from = task_current();

        g_task_manager.curr_task = to;   // 设置将要切换的任务为“当前任务”
        to->state = TASK_RUNNING;      // 设置 task running

        task_switch_from_to(from, to);
    }
    irq_leave_proection(state);  //--leave protection
   
}

// 检查当前任务的时间片是否用完，若用完强制切换到下一任务
void 
task_time_tick() {
    task_t* curr = task_current();

    int slice = --curr->slice_ticks;
    if( slice == 0 && &g_task_manager.ready_list.count > 0) {
        curr->slice_ticks = curr->time_ticks;

        task_set_block(curr);
        task_set_ready(curr);

        task_dispatch();
    }

    list_node_t* sleep_lists_curr = list_first(&g_task_manager.sleep_list);
    while (sleep_lists_curr) {
        list_node_t * next = list_node_next(sleep_lists_curr);
        task_t* task = list_node_parent(sleep_lists_curr, task_t, run_node);
        
        if(--task->sleep_ticks == 0) {
            task_set_wakeup(task);
            task_set_ready(task);
        }
        sleep_lists_curr = next;
    }

    task_dispatch();
}



void 
sys_sleep(uint32_t ms) {
    irq_state_t state = irq_enter_proection();

    task_t* curr = task_current();
    task_set_block(curr);
    task_set_sleep(curr, ms / OS_TICK_MS);

    task_dispatch();

    irq_leave_proection(state);
}

void 
task_set_sleep(task_t* task, uint32_t ticks) {
    if(ticks <= 0) return;
    task->sleep_ticks = ticks;
    task->state = TASK_SLEEP;
    list_insert_last(&g_task_manager.sleep_list, 
        &task->run_node);
}

void 
task_set_wakeup(task_t* task) {
    list_delete(&g_task_manager.sleep_list, 
        &task->run_node);
}

int 
sys_getpid () {

    task_t* curr = task_current();

    return curr->pid;
}


static task_t* 
alloc_task() {
    task_t* task = (task_t*)0;
    mutex_lock(&task_table_mutex);
    for(int i=0; i<128; i++) {
        task_t* curr = g_task_table + i;
        if(curr->name[0] == '\0') {
            task = curr;
            break;
        }
    }
    mutex_unlock(&task_table_mutex);
    return task;
}

static void 
free_task(task_t* task) {
    mutex_lock(&task_table_mutex);
    task->name[0] = '\0';
    mutex_unlock(&task_table_mutex);
}

static void 
task_uninit(task_t* task) {
    if(task->tss_sel) {
        gdt_free_sel(task->tss_sel);
    }
    if(task->tss.esp0) {
        memory_free_page(task->tss.esp - MEM_PAGE_SIZE);
    }
    if(task->tss.cr3) {
        memory_destory_uvm(task->tss.cr3);
    }

    k_memset(task, 0, sizeof(task_t));
}


int 
sys_fork() {
    task_t* parent_task = task_current();

    task_t* child_task = alloc_task();
    if (child_task == (task_t*)0) goto fork_failed;


    syscall_frame_t* frame = (syscall_frame_t*)( parent_task->tss.esp0 
        - sizeof(syscall_frame_t));


    int err = task_init(child_task, parent_task->name, 0, frame->eip, 
        frame->esp + sizeof(uint32_t) * SYSCALL_PARAM_COUNT );
    if (err < 0) goto fork_failed;

    tss_t* tss = &child_task->tss;
    tss->eax = 0;                       // 子进程返回0
    tss->ebx = frame->ebx;
    tss->ecx = frame->ecx;
    tss->edx = frame->edx;
    tss->esi = frame->esi;
    tss->edi = frame->edi;
    tss->ebp = frame->ebp;

    tss->cs  = frame->cs;
    tss->ds  = frame->ds;
    tss->es  = frame->es;
    tss->fs  = frame->fs;
    tss->gs  = frame->gs;
    tss->eflags = frame->eflags;
    
    child_task->parent = parent_task;

    if( (tss->cr3 = memory_copy_uvm(parent_task->tss.cr3) ) < 0) {
        goto fork_failed;
    }

    // tss->cr3 = parent_task->tss.cr3;
    task_start(child_task);
    return child_task->pid;

fork_failed:
    if(child_task) {
        task_uninit(child_task);
        free_task(child_task);
    }
    return -1;
}



static int 
load_phdr(int file, Elf32_Phdr * phdr, uint32_t page_dir) {

    int err = memory_alloc_for_page_dir(page_dir, phdr->p_vaddr, phdr->p_memsz, PTE_P | PTE_U | PTE_W);
    if (err < 0) {
        klog("no memory");
        return -1;
    }

    if (sys_lseek(file, phdr->p_offset, 0) < 0) {
        klog("read file failed");
        return -1;
    }

    uint32_t vaddr = phdr->p_vaddr;
    uint32_t size = phdr->p_filesz;
    while (size > 0) {
        int curr_size = (size > MEM_PAGE_SIZE) ? MEM_PAGE_SIZE : size;

        uint32_t paddr = memory_get_paddr(page_dir, vaddr);

        if (sys_read(file, (char *)paddr, curr_size) <  curr_size) {
            klog("read file failed");
            return -1;
        }

        size -= curr_size;
        vaddr += curr_size;
    }

    return 0;
}

static uint32_t 
load_elf_file(task_t* task, const char* name, uint32_t page_dir) {
    Elf32_Ehdr elf_hdr;

    Elf32_Phdr elf_phdr;

    int file = sys_open(name, 0);
    if ( file < 0 ) {
        klog("open failed: %s", name);
        goto load_failed;
    }

    int cnt = sys_read(file, (char*)&elf_hdr, sizeof(elf_hdr));
    if ( cnt < sizeof(Elf32_Ehdr) ) {
        klog("elf hdr too small. size=%s", cnt);
        goto load_failed;
    }

    if ( (elf_hdr.e_ident[0] != ELF_MAGIC) || (elf_hdr.e_ident[1] != 'E')
        || (elf_hdr.e_ident[2] != 'L') || (elf_hdr.e_ident[3] != 'F')) 
    {
        klog("chekc elf ident failed");
        goto load_failed;
    }

    uint32_t e_phoff = elf_hdr.e_phoff;
    for (int i=0; i < elf_hdr.e_phnum; i++) {
        if( sys_lseek(file, e_phoff, 0) < 0 ) {
            goto load_failed;
        }

        cnt = sys_read(file, (char *)&elf_phdr, sizeof(elf_phdr));
        if (cnt < sizeof(elf_phdr)) {
            goto load_failed;
        }

        if ((elf_phdr.p_type != PT_LOAD) || (elf_phdr.p_vaddr < MEMORY_TASK_BASE)) {
           continue;
        }

        int err = load_phdr(file, &elf_phdr, page_dir);
        if (err < 0) {
            goto load_failed;
        }

        task->heap_start = elf_phdr.p_vaddr + elf_phdr.p_memsz;
        task->heap_end = task->heap_start;

    }

    sys_close(file);
    return elf_hdr.e_entry;

load_failed:
    if (file) {
        sys_close(file);
    }
return 0;
}


static int 
copy_args (char * to, uint32_t page_dir, int argc, char **argv) {
    task_args_t task_args;
    task_args.argc = argc;
    task_args.argv = (char **)(to + sizeof(task_args_t));

    char * dest_arg = to + sizeof(task_args_t) + sizeof(char *) * (argc + 1);   // 留出结束符
    
    char ** dest_argv_tb = (char **)memory_get_paddr(page_dir, (uint32_t)(to + sizeof(task_args_t)));

    for (int i = 0; i < argc; i++) {
        char * from = argv[i];

        int len = k_strlen(from) + 1;   // 包含结束符
        int err = memory_copy_uvm_data((uint32_t)dest_arg, page_dir, (uint32_t)from, len);

        dest_argv_tb[i] = dest_arg;
        dest_arg += len;
    }
    if (argc) {
        dest_argv_tb[argc] = '\0';
    }
     // 写入task_args
    return memory_copy_uvm_data((uint32_t)to, page_dir, (uint32_t)&task_args, sizeof(task_args_t));
}


int         // 用当前进程运行新的代码
sys_execve(char* name, char** argv, char** env) {
    task_t* curr_task = task_current();

    k_strncpy(curr_task->name, get_file_name(name), TASK_NAME_SIZE);

    uint32_t old_page_dir = curr_task->tss.cr3;

    // 删除原来进程的页表
    uint32_t new_page_dir = memory_create_uvm();
    if(!new_page_dir) goto exec_failed;

    // 根据elf文件找到运行地址
    uint32_t entry = load_elf_file(curr_task, name, new_page_dir);
    if(entry == 0) goto exec_failed;

    uint32_t stack_top = MEM_TASK_STACK_TOP - MEM_TASK_ARG_SIZE;
    int err = memory_alloc_for_page_dir(
        new_page_dir, MEM_TASK_STACK_TOP - MEM_TASK_STACK_SIZE,
        MEM_TASK_STACK_SIZE, PTE_P | PTE_U | PTE_W
    );
    if (err < 0) {
        goto exec_failed;
    }

    int argc = strings_count(argv);
    int err2 = copy_args((char* )stack_top, new_page_dir, argc, argv);
    if (err2 < 0) {
        goto exec_failed;
    }

    syscall_frame_t * frame = (syscall_frame_t *)(curr_task->tss.esp0 
        - sizeof(syscall_frame_t));
    frame->eip = entry;
    frame->eax = frame->ebx = frame->ecx = frame->edx = 0;
    frame->esi = frame->edi = frame->ebp = 0;
    frame->eflags = EFLAGES_DEFAULT| EFLAGS_IF;  // 段寄存器无需修改

    frame->esp = stack_top - sizeof(uint32_t)*SYSCALL_PARAM_COUNT;

    curr_task->tss.cr3 = new_page_dir;          // 设置页表
    mmu_set_page_dir(new_page_dir);             // 跟新页表

    memory_destory_uvm(old_page_dir);           // 删除之前的页表

    return 0;
exec_failed:
    if(new_page_dir) {
        curr_task->tss.cr3 = old_page_dir;
        mmu_set_page_dir(old_page_dir);
        memory_destory_uvm(new_page_dir);
    }
return -1;

}



// 进程文件相关

int         // 为当前任务分配一个空闲的 fd
task_alloc_fd (file_t* file) {
    task_t* task = task_current();
    for (int i=0; i < TASK_OFILE_NR; i++) {
        file_t* p = task->file_table[i];
        if (p == (file_t*)0) {
            task->file_table[i] = file;
            return i;
        }
    }

    return -1;
}

void       // 删除当前任务的指定文件
task_remove_fd (int fd) {
    if ((fd >= 0) && (fd < TASK_OFILE_NR)) {
        task_current()->file_table[fd] = (file_t*)0;
    }
}

file_t*    // 通过fd找到当前进程的打开的文件
task_file (int fd) {
    if ((fd >= 0) && (fd < TASK_OFILE_NR)) {
        file_t* file = task_current()->file_table[fd];
        return file;
    }
    return (file_t*)0;
}


void sys_exit(int status) {
    task_t* curr_task = task_current();

    for (int fd = 0; fd < TASK_OFILE_NR; fd++) {
        file_t* file = curr_task->file_table[fd];
        if (file) {
            sys_close(fd);
            curr_task->file_table[fd] = (file_t*)0;
        }
    }

    irq_state_t state = irq_enter_proection();
    curr_task->status = status;
    curr_task->state = TASK_ZOMBIE;
    task_set_block(curr_task);
    task_dispatch();
    irq_leave_proection(state);
}