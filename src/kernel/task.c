#include <task.h>

void simple_switch(uint32_t **from, uint32_t* to);


void task_switch_from_to(task_t* from, task_t* to) {
    // far_jump(to->tss_sel, 0);  // 使用 tss 机制

    simple_switch(&from->stack, to->stack);  // 使用直接跳转机制
}


static task_manager_t task_manager;


void task_manager_init() {
    list_init(&task_manager.ready_list);
    list_init(&task_manager.task_list);
    task_manager.curr_task = get_first_task();
}


int task_init(task_t* task, const char* name, uint32_t entry, uint32_t esp) {
    // tss_init(task, entry, esp);  // 使用 tss 机制

    // 使用直接跳转机制
    uint32_t* pesp = (uint32_t*)esp;
    if(pesp) {
        *(--pesp) = entry;
        *(--pesp) = 0;
        *(--pesp) = 0;
        *(--pesp) = 0;
        *(--pesp) = 0;
        task->stack = pesp;
    }

    k_strncpy(task->name, name, TASK_NAME_SIZE);        // 进程名
    task->state = TASK_CREATED;                         // 状态  created

    task->time_ticks = TASK_TIME_SLICE_DEFAULT;         //  最大时间片
    task->slice_ticks = TASK_TIME_SLICE_DEFAULT;        //  当前时间片

    list_node_init(&task->all_node); 
    list_node_init(&task->run_node);
    task_set_ready(task);                                        // 加入到就绪队列
    list_insert_last(&task_manager.task_list, &task->all_node);  // 加入到所有队列

    return 0;
}


int tss_init(task_t* task, uint32_t entry, uint32_t esp) {
    int tss_sel = gdt_alloc_desc();


    segment_desc_set(tss_sel, (uint32_t)&task->tss, sizeof(tss_t), 
        SEG_P_PRESENT | SEG_DPL0 | SEG_TYPE_TSS
    );

    k_memset(&task->tss, 0, sizeof(tss_t));

    task->tss.eip  = entry;
    task->tss.esp  = esp;
    task->tss.esp0 = esp;

    task->tss.ss  = KERNEL_SELECTOR_DS;
    task->tss.ss0 = KERNEL_SELECTOR_DS;
    task->tss.es  = KERNEL_SELECTOR_DS;
    task->tss.ds  = KERNEL_SELECTOR_DS;
    task->tss.fs  = KERNEL_SELECTOR_DS;
    task->tss.gs  = KERNEL_SELECTOR_DS;

    task->tss.cs  = KERNEL_SELECTOR_CS;

    task->tss.eflags = EFLAGES_DEFAULT | EFLAGS_IF;

    task->tss_sel = tss_sel;
}


static uint32_t task1_stack[2048];
static uint32_t task2_stack[2048];
static uint32_t task3_stack[2048];

static task_t task2;
static task_t task3;

void task1_func() {
    int count = 0;
    for(;;) {
        klog("-------------------------11111---------------------------: %d", count++);
        
        // sys_sched_yield();
    }
}
void task2_func() {
    // irq_enable_global();
    int count = 0;
    for(;;) {
        klog("------------------------222222----------------------------: %d", count++);

        // sys_sched_yield();
    }
}
void task3_func() {
    // irq_enable_global();
    int count = 0;
    for(;;) {
        klog("--------------------------3333--------------------------: %d", count++);

        // sys_sched_yield();
    }
}

void task1_func_init() {
    task_init(&task_manager.first_task, "task11", (uint32_t)task1_func, (uint32_t)&task1_stack[1024]);
    task_init(&task2,                   "task22", (uint32_t)task2_func, (uint32_t)&task2_stack[1024]);
    task_init(&task3,                   "task33", (uint32_t)task3_func, (uint32_t)&task3_stack[1024]);

}


task_t* get_first_task() {
    return &task_manager.first_task;
}


task_t* task_current() {
    return task_manager.curr_task;
}

// 将任务添加到就绪队列 尾部 设置状态为 ready
void task_set_ready(task_t* task) {
    list_insert_last(&task_manager.ready_list, &task->run_node);
    task->state = TASK_READY;
}

// 将 特定 任务从就绪队列删除
void task_set_block(task_t* task) {
    list_delete(&task_manager.ready_list, &task->run_node);
    // list_delete_first(&task_manager.ready_list);
}

// 主动放弃cpu执行
int  sys_sched_yield() {
    if(list_count(&task_manager.ready_list) > 1) {
        task_t* curr = task_current();
        task_set_block(curr);
        task_set_ready(curr);

        task_dispatch();
    }
    return 0;
}

// 从就绪队列中找到一第一个任务
task_t * task_next_run() {
    list_node_t * task_node = list_first(&task_manager.ready_list);
    return list_node_parent(task_node, task_t, run_node);
}

// 分配一个任务并从当前任务切换过去
void task_dispatch() {

    task_t * to = task_next_run();
    if (to != task_manager.curr_task) {
        task_t * from = task_current();

        task_manager.curr_task = to;   // 设置将要切换的任务为“当前任务”
        to->state = TASK_RUNNING;      // 设置 task running

        task_switch_from_to(from, to);
    }
}

// 检查当前任务的时间片是否用完，若用完强制切换到下一任务
void task_time_tick() {
    task_t* curr = task_current();

    int slice = --curr->slice_ticks;
    if( slice == 0 && &task_manager.ready_list.count > 0) {
        curr->slice_ticks = curr->time_ticks;

        task_set_block(curr);
        task_set_ready(curr);

        task_dispatch();
    }
}