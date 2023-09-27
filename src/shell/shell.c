#include <lib_syscall.h>
#include <stdio.h>
#include "shell.h"
#include <string.h>
#include <getopt.h>
#include <stdlib.h>

static cli_t cli;
static const char* promot = "sh >>";

static int do_help(int argc, char ** argv) {
    const cli_cmd_t * start = cli.cmd_start;
    while (start < cli.cmd_end) {
        printf("%s %s\n", start->name, start->usage);
        start++;
    }
    return 0;
}

static int do_clear  (int argc, char** argv) {
    printf("%s", ESC_CLEAR_SCREEN);
    printf("%s", ESC_MOVE_CURSOR(0,0));
    return 0;
}

static int do_echo (int argc, char** argv) {
    if (argc == 1) {
        char msg_buf[128];
        fgets(msg_buf, sizeof(msg_buf), stdin);
        msg_buf[sizeof(msg_buf) - 1] = '\0';
        puts(msg_buf);
        return 0;
    }

    int count = 1;
    int ch;
    optind = 1;
    while ((ch = getopt(argc, argv, "n:h")) != -1) {
        switch (ch)
        {
        case 'h':
            puts("echo any message");
            puts("Usage: echo [-n count] message");
            return 0;
        case 'n':
            count = atoi( optarg );
            break;
        case '?':
            if (optarg) {
                fprintf(stderr, "Unkown option -%s\n", optarg);
            }
            return -1;
        default:
            break;
        }
    }

    if (optind > argc -1) {
        fprintf(stderr, "Message is empty\n");
        return -1;
    } 

    char* msg = argv[optind];
    for (int i=0; i < count; i++) {
        puts(msg);
    }
    
    return 0;
}

static int do_exit (int argc, char** argv) {
    exit(0);
    return 0;
}


static cli_cmd_t cmd_list[] = {
    {
        .name = "help",
        .usage = "help -- list support command",
        .do_func = do_help,
    },{
        .name = "clear",
        .usage = "clear -- clear screen",
        .do_func = do_clear,
    },{
        .name = "echo", 
        .usage = "echo [-n count] msg -- echo something",
        .do_func = do_echo,
    },{
        .name = "quit",
        .usage = "quit test", 
        .do_func = do_exit,
    }
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

static const cli_cmd_t* find_built_in( const char *name) {
    for (const cli_cmd_t* cmd = cli.cmd_start; cmd < cli.cmd_end; cmd++ ) {
        if (strcmp(cmd->name, name) != 0) {
            continue;
        }
        return cmd;
    }

    return (const cli_cmd_t*)0;

}

static void run_built_in( const cli_cmd_t* cmd, int argc, char ** argv) {
    int ret = cmd->do_func(argc, argv);
    if (ret < 0) {
        fprintf(stderr, "error: %d\n", ret);
    }
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
        char* str = fgets(cli.curr_input, CLI_INPUT_SIZE, stdin);
        if (!str) {
            continue;
        }
        char* cr = strchr(cli.curr_input, '\n');
        if (cr) {
            *cr = '\0';
        }
        cr = strchr(cli.curr_input, '\r');
        if (cr) {
            *cr = '\0';
        }

        int argc = 0;
        char * argv[10];
        memset(argv, 0, sizeof(argv));

        const char * space = " ";
        char* token = strtok(cli.curr_input, space);
        while (token) {
            argv[argc++] =  token;
            token = strtok(NULL, space);
        }

        if (argc == 0) {
            continue;
        }

        const cli_cmd_t* cmd = find_built_in(argv[0]);
        if (cmd) {
            run_built_in(cmd, argc, argv);
            continue;
        }

        // exec
        fprintf(stderr, ESC_COLOR_ERROR"Unkown command: %s\n"ESC_COLOR_DEFAULT, cli.curr_input);
    }
    return 0;
}