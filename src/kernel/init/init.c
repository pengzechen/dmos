#include <loader/loader.h>
#include <cpu.h>
#include <mtime.h>
#include <log.h>

void kernel_init (boot_info_t * boot_info) {
    gdt_init();

    log_init();
    irq_init();
    time_init();
    
    irq_enable_global();
}

void init_main() {
    // int a = 3 / 0;
    klog("Kernal %s is running ... %d %d %x %c ... ", "1.0.1", 1255, -8985, 0x7f53, 'L');
    
    for(;;) {}
}