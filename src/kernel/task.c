#include <task.h>

void simple_switch(uint32_t **from, uint32_t* to);


void task_switch_from_to(task_t* from, task_t* to) {

    // 使用 tss 机制
    // far_jump(to->tss_sel, 0);

    // 使用直接跳转机制
    simple_switch(&from->stack, to->stack);
}


int task_init(task_t* task, uint32_t entry, uint32_t esp) {

    // 使用 tss 机制
    // tss_init(task, entry, esp);

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
