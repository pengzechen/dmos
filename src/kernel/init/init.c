#include <loader/loader.h>
#include <cpu.h>
#include "mtime.h"

void kernel_init (boot_info_t * boot_info) {
    init_gdt();
    irq_init();
    time_init();
    irq_enable_global();
}

void init_main() {
    // int a = 3 / 0;
    
    for(;;) {}
}