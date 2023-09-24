#include <lib_syscall.h>
#include <stdio.h>

char cmd_buf[5555];

int main(int argc, char **argv) {
#if 0
    // printf("abef\b\b\b\bcd\n");     // 光标左移    cdef
    // printf("abcd\x7f;fg\n");       // 左删一个字符 abc;ef 
    // printf("\0337Hello, world!\0338\n");  // 123lo, world
    // printf("os version: %s\n", "1.0.0");
    // printf("Hello from shell Hello from shell Hello from shell Hello from shell"
    //     "Hello from shell Hello from shell Hello from shell Hello from shell\n");
    // printf("\033[31m");
    // printf("\033[10;10Htest!\n");
    // printf("\033[39;49m\n");
    // printf("\033[31;42mHello, world!\033[39;49m123\n");
    
    for(int i=0; i<argc; i++) {
        print_msg("arg: %s\n", (int)argv[i]);
    }
    
    fork();
    yield();
#endif

    int fd = open(argv[0], 0);    // fd = 0
    dup(fd);
    dup(fd);

    //printf("abcdefg\n");     // 光标左移    cdef
    //fprintf(stderr, "there is error occur\n");

    for(int i=0; ;i++) {
        printf("ok------------");
        gets(cmd_buf);
        puts(cmd_buf);

        // printf("shell pid=%d\n", getpid());
        // msleep(1000);
    }
    return 0;
}