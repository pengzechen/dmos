#ifndef LIB_SYSCALL_H
#define LIB_SYSCALL_H
#include <os_cfg.h>
#include <syscall.h>
#include <sys/stat.h>

typedef struct _syscall_arg_t {
    int id;
    int arg0;
    int arg1;
    int arg2;
    int arg3;
} syscall_arg_t ;


void msleep(int ms);
int getpid();
void print_msg (const char*fmt, int arg);
int fork();
int execve(const char* name, 
    char* const* argv, char* const * env );
int yield();
int open(const char* name, int flags, ...);
int read(int file, char* ptr, int len);
int write(int file, char* ptr, int len);
int close(int file);
int lseek(int file, int ptr, int dir);



int isatty(int file);
int fstat(int file, struct stat* st);
void * sbrk(ptrdiff_t incr);

int dup(int fd);
void _exit(int status);

#endif  // LIB_SYSCALL_H