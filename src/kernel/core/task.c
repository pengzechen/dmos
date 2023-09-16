#include <core/task.h>
#include <klib.h>
#include <os_cfg.h>
#include <cpu.h>
#include <log.h>
#include <comm/cpu_ins.h>

void simple_switch(uint32_t **from, uint32_t* to);

void task_switch_from_to(task_t* from, task_t* to) {

    simple_switch(&from->stack, to->stack);
}

int task_init(task_t* task, uint32_t entry, uint32_t esp) {

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