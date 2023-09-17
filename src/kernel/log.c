#include <log.h>

#define COM1_PORT       0x3f8

void log_init() {
    outb(COM1_PORT + 1, 0x00);
    outb(COM1_PORT + 3, 0x80);
    outb(COM1_PORT + 0, 0x3);
    outb(COM1_PORT + 1, 0x00);
    outb(COM1_PORT + 3, 0x03);
    outb(COM1_PORT + 2, 0xc7);
    outb(COM1_PORT + 4, 0x0f);
}


void klog(const char* fmt, ...) {
    char buf[128];
    k_memset(buf, 0, 128);
    va_list args;
    va_start(args, fmt);
    k_vsprint(buf, fmt, args);
    va_end(args);

    irq_state_t state = irq_enter_proection();

    const char *p = buf;
    while(*p != '\0') {
        while( (inb(COM1_PORT + 5) & (1 << 6)) == 0);
        outb(COM1_PORT, *p++);
    }
    outb(COM1_PORT, '\r');  // 回到0列
    outb(COM1_PORT, '\n');  // 向下一行

    irq_leave_proection(state);
}

