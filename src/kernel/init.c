#include <loader/loader.h>
#include <cpu.h>
#include <irq.h>
#include <mtime.h>
#include <log.h>
#include <task.h>
#include <comm/cpu_ins.h>
#include <list.h>

void kernel_init (boot_info_t * boot_info) {
    log_init();
    // gdt_init();
    irq_init();
    time_init();

}


void show_list(list_t list) {
    klog("list: first=0x%x, last=0x%x, count=%d",
         list_first(&list), list_last(&list), list_count(&list) );
}

void list_test() {
    list_t list;
    list_node_t nodes[5];

    list_init(&list);

    show_list(list);
    for(int i=0; i<5; i++) {
        list_node_t * node = nodes + i;
        klog("insert first to list: %d, 0x%x", i, (uint32_t)node);
        list_insert_first(&list, node);
    }
    show_list(list);

    list_init(&list);
    show_list(list);
    for(int i=0; i<5; i++) {
        list_node_t * node = nodes + i;

        klog("insert first to list: %d, 0x%x", i, (uint32_t)node);
        list_insert_last(&list, node);
    }
    show_list(list);
// ------------------
    
    for(int i=0; i<5; i++) {
        list_node_t* node = list_delete_first(&list);
        klog("delete first from list: %d, 0x%x", i, (uint32_t)node);
    }

    show_list(list);

}

void offset_test() {
    struct type_t {
        int i;
        list_node_t node;
    }v = {0x123456};

    list_node_t * v_node = &v.node;
    struct type_t* p = list_node_parent(v_node, struct type_t, node);
    
    if(p->i != 0x123456) {
        klog("error");
    }
}


void init_main() {
    klog("Kernal %s is running ... ", "1.0.0");

    task_manager_init();
    task1_func_init();
    irq_enable_global();
    
    task_switch_from_to(0, get_first_task());
}