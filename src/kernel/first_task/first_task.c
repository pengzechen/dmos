#include <applib/lib_syscall.h>



int first_task_main() {
    int xx = 10;
    int pid = getpid();
    for(;;) {

        print_msg("task id=%d", pid);
        msleep(1000);

    }
}