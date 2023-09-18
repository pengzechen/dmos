#include <task.h>
#include <log.h>

int first_task_main() {
    for(;;) {
        klog("first task");
        sys_sleep(1000);
    }
}