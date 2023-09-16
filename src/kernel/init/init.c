#include <loader/loader.h>
#include <cpu.h>
#include <mtime.h>
#include <log.h>
#include <core/task.h>
#include <comm/cpu_ins.h>

void kernel_init (boot_info_t * boot_info) {
    log_init();

    gdt_init();
    irq_init();
    time_init();
    irq_enable_global();
}

static task_t task1;
static task_t task2;
static uint32_t task1_stack[1024];
static uint32_t task2_stack[1024];


void task1_func() {
    int count = 0;
    for(;;) {
        klog("task1 say++++: %d", count++);
        task_switch_from_to(&task1, &task2);
    }
}

void task2_func() {
    int count = 0;
    for(;;) {
        klog("task2 say----: %d", count++);
        task_switch_from_to(&task2, &task1);
    }
}


void init_main() {
    klog("Kernal %s is running ... ", "1.0.0");

    task_init(&task1, (uint32_t)task1_func, (uint32_t)&task1_stack[1024]);
    task_init(&task2, (uint32_t)task2_func, (uint32_t)&task2_stack[1024]);
    task_switch_from_to(0, &task1);

}