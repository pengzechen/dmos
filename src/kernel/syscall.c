#include <syscall.h>
#include <task.h>
#include <log.h>
#include <console.h>
#include <fs.h>
#include <mem.h>

typedef int (*sys_handle_t)(uint32_t arg0, uint32_t arg1, uint32_t arg2, uint32_t arg3);

// 临时使用
void sys_print_msg(const char* fmt, int arg) {
    klog(fmt, arg);
    // console_write(fmt);
}


static const sys_handle_t sys_table[] = {
    [SYS_sleep] = (sys_handle_t)sys_sleep,
    [SYS_getpid] = (sys_handle_t)sys_getpid,
    [SYS_print_msg] = (sys_handle_t)sys_print_msg,
    [SYS_fork] = (sys_handle_t)sys_fork,
    [SYS_execve] = (sys_handle_t)sys_execve,
    [SYS_yield] = (sys_handle_t)sys_yield,

    [SYS_open] = (sys_handle_t)sys_open,
    [SYS_read] = (sys_handle_t)sys_read,
    [SYS_write] = (sys_handle_t)sys_write,
    [SYS_close] = (sys_handle_t)sys_close,
    [SYS_lseek] = (sys_handle_t)sys_lseek,

    [SYS_isatty] = (sys_handle_t)sys_isatty,
    [SYS_sbrk] = (sys_handle_t)sys_sbrk,
    [SYS_fstat] = (sys_handle_t)sys_fstat,

};



void do_handler_syscall(syscall_frame_t * frame) {
    if(frame->func_id < sizeof(sys_table) / sizeof(sys_table[0])) {
        sys_handle_t handler = sys_table[frame->func_id];
        if (handler) {
            int ret = handler(frame->arg0, frame->arg1, frame->arg2, frame->arg3);
            frame->eax = ret;   // eax 防止返回值
            return;
        }
    }

    task_t * task = task_current();
    klog("task: %s, Unkown syscall: %d", task->name, frame->func_id);

    frame->eax = -1;
}