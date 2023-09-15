#include <core/task.h>
#include <klib.h>
#include <os_cfg.h>
#include <cpu.h>
#include <log.h>
#include <comm/cpu_ins.h>

static int tss_init(task_t* task, uint32_t entry, uint32_t esp) {
    k_memset(&task->tss, 0, sizeof(tss_t));

    int tss_sel = gdt_alloc_desc();
    if (tss_sel < 0) {
        klog("alloc tss seg failed");
        return -1;
    }
    segment_desc_set(tss_sel, (uint32_t)&task->tss, sizeof(tss_t), 
        SEG_P_PRESENT | SEG_DPL0 | SEG_TYPE_TSS );

    task->tss_sel = tss_sel;    // save tss seg

    task->tss.eip = entry;
    task->tss.esp = task->tss.esp0 = esp;  // 栈空间       // privilege 0
    task->tss.ss = task->tss.ss0 = KERNEL_SELECTOR_DS;     // privilege 0

    task->tss.es = task->tss.ds = task->tss.fs = task->tss.gs = KERNEL_SELECTOR_DS;
    task->tss.cs = KERNEL_SELECTOR_CS;
    task->tss.eflags = EFLAGS_IF | EFLAGES_DEFAULT ;

    return 0;
}

void task_switch_from_to(task_t* from, task_t* to) {
    far_jump(to->tss_sel, 0);
}

int task_init(task_t* task, uint32_t entry, uint32_t esp) {

    tss_init(task, entry, esp);

    return 0;
}