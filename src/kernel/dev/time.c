#include "mtime.h"
#include "comm/types.h"
#include "os_cfg.h"
#include "cpu.h"
#include "comm/cpu_ins.h"

static uint32_t sys_tick;

void exception_handler_divider();
void handle_divider(exception_frame_t * frame) {}


void exception_handler_time();
void handle_time(exception_frame_t * frame) {
    sys_tick++;
    pic_send_eoi(IRQ0_TIMER);
}

static void pit_init (void) {
    uint32_t reload_count = PIT_OSC_FREQ / (1000.0 / OS_TICK_MS);
    outb(PIT_COMMAND_MODE_PORT, PIT_CHANNLE0 | PIT_LOAD_LOHI | PIT_MODE3);
    outb(PIT_CHANNEL0_DATA_PORT, reload_count & 0xFF);   // 加载低8位
    outb(PIT_CHANNEL0_DATA_PORT, (reload_count >> 8) & 0xFF); // 再加载高8位
    irq_install(IRQ0_TIMER, exception_handler_time);
    irq_install(0, exception_handler_divider);
    
    irq_enable(IRQ0_TIMER);
}

void time_init() {
    sys_tick = 0;

    pit_init();
}