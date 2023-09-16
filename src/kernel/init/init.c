#include <loader/loader.h>
#include <cpu.h>
#include <mtime.h>
#include <log.h>
#include <core/task.h>
#include <comm/cpu_ins.h>

void kernel_init (boot_info_t * boot_info) {
    gdt_init();
    log_init();
    irq_init();
    time_init();
    // irq_enable_global();
}

static task_t task1;
static task_t task2;
static uint32_t task1_stack[1024];
static uint32_t task2_stack[1024];


void task1_func() {
    int count = 0;
    for(;;) {
        klog("task1 say++++: %d", count++);
        far_jump(task2.tss_sel, 0);
    }
}

void task2_func() {
    int count = 0;
    for(;;) {
        klog("task2 say----: %d", count++);
        far_jump(task1.tss_sel, 0);
    }
}

void init_main() {
    klog("Kernal %s is running ... ", "1.0.0");

    tss_init(&task1, 0, 0);
    tss_init(&task2, (uint32_t)task2_func, (uint32_t)&task2_stack[1024]);
    

    write_tr((&task1)->tss_sel);
    task_switch_from_to(&task1, &task2);
}