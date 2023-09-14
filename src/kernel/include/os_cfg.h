#ifndef CPU_CFG
#define CPU_CFG

#define GDT_TABLE_SIZE      256
#define IDT_TABLE_NR        128


#define KERNEL_SELECTOR_CS  (1 * 8)
#define KERNEL_SELECTOR_DS  (2 * 8)

#define OS_TICK_MS   10

#endif