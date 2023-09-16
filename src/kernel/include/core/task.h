#ifndef TASK_H
#define TASK_H

#include <comm/types.h>
#include <cpu.h>

typedef struct _task_s {
    uint32_t* stack;

} task_t;

int task_init(task_t* task, uint32_t entry, uint32_t esp);

void task_switch_from_to(task_t* from, task_t* to);

#endif
