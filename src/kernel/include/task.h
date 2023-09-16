#ifndef TASK_H
#define TASK_H

#include <comm/types.h>
#include <comm/cpu_ins.h>
#include <cpu.h>
#include <klib.h>
#include <os_cfg.h>
#include <log.h>


typedef struct _task_s {
    uint32_t*        stack;
    tss_t            tss;
    uint32_t   tss_sel;
} task_t;

int tss_init(task_t* task, uint32_t entry, uint32_t esp);

int task_init(task_t* task, uint32_t entry, uint32_t esp);
void task_switch_from_to(task_t* from, task_t* to);



#endif
