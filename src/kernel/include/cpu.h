#ifndef CPU_H
#define CPU_H

#include <comm/types.h>

typedef struct _segment_desc_t segment_desc_t;
#pragma pack(1)
struct _segment_desc_t {
    uint16_t limit15_0;
    uint16_t base15_0;
    uint8_t base23_16;
    uint16_t attr;
    uint8_t base31_24;
};
#pragma pack()

#define SEG_G               (1 << 15)
#define SEG_D               (1 << 14)
#define SEG_P_PRESENT       (1 << 7)

#define SEG_DPL0            (0 << 5)
#define SEF_DPL3            (3 << 5)

#define SEG_S_SYSTEM        (0 << 4)
#define SEG_S_NOMAL         (1 << 4)

#define SEG_TYPE_CODE       (1 << 3)
#define SEG_TYPE_DATA       (0 << 3)
#define SEG_TYPE_RW         (1 << 1)


void segment_desc_set(int selector, uint32_t base, uint32_t limit, uint16_t attr);
void init_gdt();


// idt --------------------------------------------------------------------------------------

#pragma pack(1)
typedef struct _gate_desc_t {
	uint16_t offset15_0;
	uint16_t selector;
	uint16_t attr;
	uint16_t offset31_16;
}gate_desc_t;
#pragma pack()

#define GATE_P_PRESENT              (1 << 15)
#define GATE_DPL0                   (0 << 13)
#define GATE_DPL3                   (3 << 13)
#define GATE_TYPE_IDT		        (0xE << 8)		// 中断32位门描述符


void gate_desc_set(gate_desc_t * desc, uint16_t selector, uint32_t offset, uint16_t attr);
void irq_init ();


typedef struct _exception_frame_t {
    uint32_t gs, fs, es, ds;
    uint32_t edi, esi, edp, esp, ebx, edx, ecx, eax;
    uint32_t num, err_code;
    uint32_t eip, cs, eflags;
} exception_frame_t;


typedef void (*irq_handler_t) (exception_frame_t * frame);

int  irq_install(int irq_num, irq_handler_t handler);
void irq_enable (int irq_num );
void irq_disable(int irq_num );
void irq_enable_global();
void irq_disable_global();

#define IRQ_TIMER 0x20



// 8259 芯片相关
#define PIC0_ICW1			0x20
#define PIC0_ICW2			0x21
#define PIC0_ICW3			0x21
#define PIC0_ICW4			0x21
#define PIC0_IMR			0x21

#define PIC1_ICW1			0xa0
#define PIC1_ICW2			0xa1
#define PIC1_ICW3			0xa1
#define PIC1_ICW4			0xa1
#define PIC1_IMR			0xa1

#define PIC_ICW1_ICW4		(1 << 0)		// 1 - 需要初始化ICW4
#define PIC_ICW1_ALWAYS_1	(1 << 4)		// 总为1的位
#define PIC_ICW4_8086	    (1 << 0)        // 8086工作模式
#define IRQ_PIC_START		0x20			// PIC中断起始号





#endif