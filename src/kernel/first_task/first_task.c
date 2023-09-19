#include <applib/lib_syscall.h>


int first_task_main() {
    int count = 1;
    int pare_pid = getpid();

    int pid = fork();
    if(pid < 0) {
        print_msg("fork failed %d", 0);

    } else if (pid == 0) {
        count += 20;
        print_msg("create child:  id=%d", 0);

        char* argv[] = {"arg0", "arg1", "arg2", "arg3"};
        execve("/shell.elf", argv, (char**)0);

    } else {
        count += 1000;
        print_msg("child task  :  id=%d", pid);
        print_msg("parent      :  id=%d", pare_pid);
    }

    for(;;) {

        //print_msg("current id=%d", pid);
        msleep(1000);
    }
}