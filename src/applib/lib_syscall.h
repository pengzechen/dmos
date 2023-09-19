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


void  msleep(int ms);

int getpid();

void print_msg (const char*fmt, int arg);

int fork();

int execve(const char* name, 
    char* const* argv, char* const * env );

int yield();


#endif  // LIB_SYSCALL_H