#ifndef MUX_H
#define MUX_H

#include <list.h>
#include <task.h>


typedef struct _mutext {
    task_t * owner;
    int locked_count;
    list_t wait_list;

} mutex_t;

void mutex_init (mutex_t * mutex);
void mutex_lock (mutex_t * mutex);
void mutex_unlock (mutex_t * mutex);

#endif
