#ifndef SHELL_H
#define SHELL_H


#define CLI_INPUT_SIZE    1024

typedef struct _cli_cmd_t {
    const char * name;
    const char * usage;
    int (*do_func)(int argc, char** argv);
} cli_cmd_t;


typedef struct _cli_t {
    char curr_input[CLI_INPUT_SIZE];
    const cli_cmd_t * cmd_start;
    const cli_cmd_t * cmd_end;
    const char      * promot;
} cli_t;

#endif
