#include <sem.h>
#include <irq.h>

void sem_init(sem_t* sem, int init_count) {
    sem->count = init_count;
    list_init(&sem->wait_list);
}

void sem_wait(sem_t* sem) {
    irq_state_t state = irq_enter_proection();
    if (sem->count > 0) {
        sem->count--;
    } else {
        task_t * curr = task_current();
        task_set_block(curr);
        list_insert_last(&sem->wait_list, &curr->wait_node);
        task_dispatch();
    }
    irq_leave_proection(state);
}

void sem_notify(sem_t* sem) {

    irq_state_t state = irq_enter_proection();
    if (list_count(&sem->wait_list)) {
        list_node_t* node = list_delete_first(&sem->wait_list);

        task_t* task = list_node_parent(node, task_t, wait_node);
        task_set_ready(task);
        task_dispatch();
    } else {
        sem->count++;
    }
    irq_leave_proection(state);

}

int  sem_count(sem_t* sem) {
    irq_state_t state = irq_enter_proection();
    int count = sem->count;
    irq_leave_proection(state);
    return count;
}