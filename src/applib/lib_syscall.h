#ifndef LIB_SYSCALL_H
#define LIB_SYSCALL_H
#include <os_cfg.h>
#include <syscall.h>

typedef struct _syscall_arg_t {
    int id;
    int arg0;
    int arg1;
    int arg2;
    int arg3;
} syscall_arg_t ;



static inline int sys_call (syscall_arg_t * args) {
    int ret;
	volatile uint32_t addr[] = {0,  SELECTOR_SYSCALL | 0};
	__asm__ __volatile__(
        "push %[arg3]\n\t"
        "push %[arg2]\n\t"
        "push %[arg1]\n\t"
        "push %[arg0]\n\t"
        "push %[id]\n\t"
        "lcalll *(%[a])"       
        :"=a"(ret)              //  output
        :[arg3]"r"(args->arg3), //  input
        [arg2]"r"(args->arg2), 
        [arg1]"r"(args->arg1), 
        [arg0]"r"(args->arg0), 
        [id]"r"(args->id), 
        [a]"r"(addr)
    );

    return ret;
}


static inline void msleep(int ms) {
    if(ms <= 0) { return; }
    syscall_arg_t args;
    args.id = SYS_sleep;
    args.arg0 = ms;
    sys_call(&args);
}

static inline int getpid() {
    syscall_arg_t args;
    args.id = SYS_getpid;
    return sys_call(&args);
}

static inline void print_msg (const char*fmt, int arg) {
    syscall_arg_t args;
    args.id = SYS_print_msg;
    args.arg0 = (int)fmt;
    args.arg1 = arg;
    sys_call(&args);
}



#endif  // LIB_SYSCALL_H