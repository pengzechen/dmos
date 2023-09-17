#include <task.h>

void simple_switch(uint32_t **from, uint32_t* to);

static void idle_task_func() { for(;;) hlt(); }

void task_switch_from_to(task_t* from, task_t* to) {
    // far_jump(to->tss_sel, 0);  // 使用 tss 机制

    simple_switch(&from->stack, to->stack);  // 使用直接跳转机制
}

static task_manager_t task_manager;
static uint32_t task1_stack[2048];
static uint32_t idle_task_stack[2048];
static uint32_t task3_stack[2048];
static task_t task3;
sem_t  sem_test;


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

// 初始化一个任务
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
    task->sleep_ticks = 0;
    list_node_init(&task->all_node); 
    list_node_init(&task->run_node);
    list_node_init(&task->wait_node);

    irq_state_t state = irq_enter_proection();   //--enter protection
        task_set_ready(task);                                        // 加入到就绪队列
        list_insert_last(&task_manager.task_list, &task->all_node);  // 加入到所有队列
    irq_leave_proection(state);  //--leave protection

    return 0;
}

// 初始化任务管理
void task_manager_init() {
    sem_init(&sem_test, 0);
    list_init(&task_manager.ready_list);
    list_init(&task_manager.task_list);
    list_init(&task_manager.sleep_list);
    task_manager.curr_task = get_first_task();
}


void task1_func() {
    int count = 0;
    for(;;) {
        klog("-------------------------11111-----------------: %d", count--);
        sem_notify(&sem_test);
        sys_sleep(1000);
    }
}


void task3_func() {
    int count = 0;
    for(;;) {
        sem_wait(&sem_test);
        klog("-------------------------33333-----------------: %d", count++);
    }
}


void task1_func_init() {
    task_init(&task_manager.first_task, "first task", 
        (uint32_t)task1_func, (uint32_t)&task1_stack[2048]);
    task_init(&task_manager.idle_task,  "idle  task", 
        (uint32_t)idle_task_func, (uint32_t)&idle_task_stack[2048]);
    task_init(&task3,                   "test  task", 
        (uint32_t)task3_func, (uint32_t)&task3_stack[2048]);

}


task_t* get_first_task() {
    return &task_manager.first_task;
}

task_t* task_current() {
    return task_manager.curr_task;
}


// 将任务添加到就绪队列 尾部 设置状态为 ready
void task_set_ready(task_t* task) {
    if (task == &task_manager.idle_task) {  // 空进程不应该加入就绪队列
        return;
    }

    list_insert_last(&task_manager.ready_list, &task->run_node);
    task->state = TASK_READY;
}

// 将 特定 任务从就绪队列删除
void task_set_block(task_t* task) {
    if (task == &task_manager.idle_task) {  // 空进程不应该删除
        return;
    }
    list_delete(&task_manager.ready_list, &task->run_node);
    // list_delete_first(&task_manager.ready_list);
}

// 主动放弃cpu执行
int  sys_sched_yield() {

irq_state_t state = irq_enter_proection();   //--enter protection
    if(list_count(&task_manager.ready_list) > 1) {
        task_t* curr = task_current();
        task_set_block(curr);
        task_set_ready(curr);

        task_dispatch();
    }
irq_leave_proection(state);  //--leave protection

    return 0;
}

// 从就绪队列中找到一第一个任务
task_t * task_next_run() {
    if (list_count(&task_manager.ready_list) == 0) {
        return &task_manager.idle_task;
    }

    list_node_t * task_node = list_first(&task_manager.ready_list);
    return list_node_parent(task_node, task_t, run_node);
}

// 分配一个任务并从当前任务切换过去
void task_dispatch() {

    irq_state_t state = irq_enter_proection();   //--enter protection
    task_t * to = task_next_run();
    if (to != task_manager.curr_task) {
        task_t * from = task_current();

        task_manager.curr_task = to;   // 设置将要切换的任务为“当前任务”
        to->state = TASK_RUNNING;      // 设置 task running

        task_switch_from_to(from, to);
    }
    irq_leave_proection(state);  //--leave protection
   
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

    list_node_t* sleep_lists_curr = list_first(&task_manager.sleep_list);
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



void sys_sleep(uint32_t ms) {
    irq_state_t state = irq_enter_proection();

    task_t* curr = task_current();
    task_set_block(curr);
    task_set_sleep(curr, ms / OS_TICK_MS);

    task_dispatch();

    irq_leave_proection(state);
}

void task_set_sleep(task_t* task, uint32_t ticks) {
    if(ticks <= 0) return;
    task->sleep_ticks = ticks;
    task->state = TASK_SLEEP;
    list_insert_last(&task_manager.sleep_list, 
        &task->run_node);
}

void task_set_wakeup(task_t* task) {
    list_delete(&task_manager.sleep_list, 
        &task->run_node);
}