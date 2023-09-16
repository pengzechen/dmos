#include <core/task.h>
#include <klib.h>
#include <os_cfg.h>
#include <cpu.h>
#include <log.h>
#include <comm/cpu_ins.h>

int tss_init(task_t* task, uint32_t entry, uint32_t esp) {

    int tss_sel = gdt_alloc_desc();
    if (tss_sel < 0) {
        klog("alloc tss failed.\n");
        return -1;
    }

    segment_desc_set(tss_sel, (uint32_t)&task->tss, sizeof(tss_t), SEG_P_PRESENT | SEG_DPL0 | SEG_TYPE_TSS);
    k_memset(&task->tss, 0, sizeof(tss_t));


    task->tss.eip = entry;

    task->tss.esp = esp;         // 栈空间
    task->tss.esp0= esp;         // privilege 0

    task->tss.ss  = KERNEL_SELECTOR_DS;
    task->tss.ss0 = KERNEL_SELECTOR_DS;     // privilege 0

    task->tss.es  = KERNEL_SELECTOR_DS;
    task->tss.ds  = KERNEL_SELECTOR_DS;
    task->tss.fs  = KERNEL_SELECTOR_DS;
    task->tss.gs  = KERNEL_SELECTOR_DS;

    task->tss.cs  = KERNEL_SELECTOR_CS;
    task->tss.eflags =  EFLAGES_DEFAULT | EFLAGS_IF ;

    task->tss_sel = tss_sel;    // save tss seg

    return 0;
}

void task_switch_from_to(task_t* from, task_t* to) {
    far_jump(to->tss_sel, 0);
}

int task_init(task_t* task, uint32_t entry, uint32_t esp) {

    tss_init(task, entry, esp);

    return 0;
}