#include <lib_syscall.h>
#include <stdio.h>
#include "shell.h"
#include <string.h>

static cli_t cli;
static const char* promot = "sh >>";
static int do_help(int argc, char ** argv) {
    return 0;
}

static cli_cmd_t cmd_list[] = {
    {
        .name = "help",
        .usage = "help -- list support command",
        .do_func = do_help,
    },
};

static void cli_init(const char* promot, cli_cmd_t* cmd_list, int size) {
    cli.promot = promot;
    memset(cli.curr_input, 0, CLI_INPUT_SIZE);
    cli.cmd_start = cmd_list;
    cli.cmd_end = cmd_list + size;
}

void show_promot() {
    printf("%s", cli.promot);
    fflush(stdout);
}

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

    cli_init(promot, cmd_list, sizeof(cmd_list) / sizeof(cmd_list[0]));
    
    for(int i=0; ;i++) {
        show_promot();
        gets(cli.curr_input);
    }
    return 0;
}