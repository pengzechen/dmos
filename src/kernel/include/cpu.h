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
#define SEG_DPL3            (3 << 5)

#define SEG_RPL0                (0 << 0)
#define SEG_RPL3                (3 << 0)

#define SEG_S_SYSTEM        (0 << 4)
#define SEG_S_NORMAL        (1 << 4)

#define SEG_TYPE_CODE       (1 << 3)
#define SEG_TYPE_DATA       (0 << 3)
#define SEG_TYPE_TSS        (9 << 0)              // TSS
#define SEG_TYPE_RW         (1 << 1)


void segment_desc_set(int selector, uint32_t base, uint32_t limit, uint16_t attr);
void gdt_init();
int gdt_alloc_desc();


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

int  irq_install(int irq_num, uint32_t handler);
void irq_enable (int irq_num );
void irq_disable(int irq_num );
void irq_enable_global();
void irq_disable_global();

#define IRQ0_DE             0
#define IRQ1_DB             1
#define IRQ2_NMI            2
#define IRQ3_BP             3
#define IRQ4_OF             4
#define IRQ5_BR             5
#define IRQ6_UD             6
#define IRQ7_NM             7
#define IRQ8_DF             8
#define IRQ10_TS            10
#define IRQ11_NP            11
#define IRQ12_SS            12
#define IRQ13_GP            13
#define IRQ14_PF            14
#define IRQ16_MF            16
#define IRQ17_AC            17
#define IRQ18_MC            18
#define IRQ19_XM            19
#define IRQ20_VE            20
#define IRQ_TIMER           0x20
void exception_handler_unknown (void);
void exception_handler_divider (void);
void exception_handler_Debug (void);
void exception_handler_NMI (void);
void exception_handler_breakpoint (void);
void exception_handler_overflow (void);
void exception_handler_bound_range (void);
void exception_handler_invalid_opcode (void);
void exception_handler_device_unavailable (void);
void exception_handler_double_fault (void);
void exception_handler_invalid_tss (void);
void exception_handler_segment_not_present (void);
void exception_handler_stack_segment_fault (void);
void exception_handler_general_protection (void);
void exception_handler_page_fault (void);
void exception_handler_fpu_error (void);
void exception_handler_alignment_check (void);
void exception_handler_machine_check (void);
void exception_handler_smd_exception (void);
void exception_handler_virtual_exception (void);

// 8259 芯片相关
// PIC控制器相关的寄存器及位配置
#define PIC0_ICW1			0x20
#define PIC0_ICW2			0x21
#define PIC0_ICW3			0x21
#define PIC0_ICW4			0x21
#define PIC0_IMR			0x21
#define PIC0_OCW2           0x20

#define PIC1_ICW1			0xa0
#define PIC1_ICW2			0xa1
#define PIC1_ICW3			0xa1
#define PIC1_ICW4			0xa1
#define PIC1_IMR			0xa1
#define PIC1_OCW2           0xa0

#define PIC_ICW1_ICW4		(1 << 0)		// 1 - 需要初始化ICW4
#define PIC_ICW1_ALWAYS_1	(1 << 4)		// 总为1的位
#define PIC_ICW4_8086	    (1 << 0)        // 8086工作模式

#define PIC_OCW2_EOI        0x20

#define IRQ_PIC_START		0x20			// PIC中断起始号
#define IRQ0_TIMER          0x20


void pic_send_eoi(int irq_num);


// tss ----------------------------------------------
#pragma pack(1)
typedef struct _tss_t {
    uint32_t pre_link;
    uint32_t esp0, ss0, esp1, ss1, esp2, ss2;
    uint32_t cr3;
    uint32_t eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi;
    uint32_t es, cs, ss, ds, fs, gs;
    uint32_t ldt;
    uint32_t iomap;
}tss_t;
#pragma pack()

#define EFLAGES_DEFAULT   (1 << 1)
#define EFLAGS_IF         (1 << 9)
#endif