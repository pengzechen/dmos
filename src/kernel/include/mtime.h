#ifndef TIME_H
#define TIME_H

#include <comm/cpu_ins.h>
#include <comm/types.h>
#include <os_cfg.h>
#include <irq.h>
#include <log.h>

#define PIT_OSC_FREQ            1193182
#define PIT_COMMAND_MODE_PORT   0x43
#define PIT_CHANNEL0_DATA_PORT  0x40

#define PIT_CHANNLE0    (0 << 6)
#define PIT_LOAD_LOHI   (3 << 4)
#define PIT_MODE3       (3 << 1)

void time_init();
void exception_handler_time();

#endif
