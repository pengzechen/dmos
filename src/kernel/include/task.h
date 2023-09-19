#ifndef TASK_H
#define TASK_H

#include <comm/types.h>
#include <list.h>
#include <cpu.h>

#define USE_TSS

#define TASK_NAME_SIZE           32
#define TASK_TIME_SLICE_DEFAULT  10

#define TASK_FLAGS_SYSTEM   (1 << 0)

typedef struct _task_s task_t;

struct _task_s {
    uint32_t    *       stack;

    enum {
        TASK_CREATED,
        TASK_RUNNING,
        TASK_SLEEP,
        TASK_READY,
        TASK_WAITING,
    }state;
    char name[TASK_NAME_SIZE];

    int pid;
    task_t * parent;

    int slice_ticks;
    int time_ticks;
    int sleep_ticks;

    list_node_t         run_node;
    list_node_t         all_node;
    list_node_t         wait_node;

#ifdef USE_TSS
    tss_t               tss;
    uint32_t            tss_sel;
#endif

} ;


typedef struct _task_manager_t {

    task_t * curr_task;
    list_t   ready_list;
    list_t   sleep_list;
    list_t   task_list;
    
    task_t   first_task;
    task_t   idle_task;         // 空闲进程

    int app_code_sel;
    int app_data_sel;

} task_manager_t;


typedef struct _task_args_t {
	uint32_t ret_addr;		// 返回地址，无用
	uint32_t argc;
	char **argv;
}task_args_t;


int  task_init(task_t* task, const char* name, int flag, uint32_t entry, uint32_t esp);
void task_switch_from_to(task_t* from, task_t* to);



void        task_manager_init();
void        first_task_init();

task_t   *  get_first_task();
task_t   *  task_current();

void        task_set_ready(task_t* task);
void        task_set_block(task_t* task);

int         sys_sched_yield();


task_t   *  task_next_run();

void        task_dispatch();

void        task_time_tick();

void        sys_sleep(uint32_t ms);
void        task_set_sleep(task_t* task, uint32_t ticks);
void        task_set_wakeup(task_t* task);

int         sys_getpid ();


int         sys_fork();

int         sys_execve(char* name, char** argv, char** env);

#endif
