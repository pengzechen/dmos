#include <log.h>
#include <dev.h>

#define LOG_USE_COM         0


#define COM1_PORT       0x3f8

static mutex_t mutex;
static int log_dev_id;


void log_init() {
    mutex_init(&mutex);
    log_dev_id = dev_open(DEV_TTY, 0, 0);


#if LOG_USE_COM
    outb(COM1_PORT + 1, 0x00);    // Disable all interrupts
    outb(COM1_PORT + 3, 0x80);    // Enable DLAB (set baud rate divisor)
    outb(COM1_PORT + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
    outb(COM1_PORT + 1, 0x00);    //                  (hi byte)
    outb(COM1_PORT + 3, 0x03);    // 8 bits, no parity, one stop bit
    outb(COM1_PORT + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
  
    // If serial is not faulty set it in normal operation mode
    // (not-loopback with IRQs enabled and OUT#1 and OUT#2 bits enabled)
    outb(COM1_PORT + 4, 0x0F);
#endif
}


void klog(const char* fmt, ...) {
    char buf[128];
    k_memset(buf, 0, 128);
    va_list args;
    va_start(args, fmt);
    k_vsprint(buf, fmt, args);
    va_end(args);

    mutex_lock(&mutex);
    // irq_state_t state = irq_enter_proection();

#if LOG_USE_COM
    const char * p = str_buf;    
    while (*p != '\0') {
        while ((inb(COM1_PORT + 5) & (1 << 6)) == 0);
        outb(COM1_PORT, *p++);
    }

    outb(COM1_PORT, '\r');
    outb(COM1_PORT, '\n');
#else
    //console_write(0, str_buf, kernel_strlen(str_buf));
    dev_write(log_dev_id, 0, "log:", 4);
    dev_write(log_dev_id, 0, buf, k_strlen(buf));

    char c = '\n';
    //console_write(0, &c, 1);
    dev_write(log_dev_id, 0, &c, 1);

#endif
    mutex_unlock(&mutex);
    // irq_leave_proection(state);

}

