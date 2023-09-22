#ifndef IRQ_H
#define IRQ_H

#include <comm/cpu_ins.h>
#include <comm/types.h>
#include <os_cfg.h>
#include <cpu.h>


#define GATE_P_PRESENT              (1 << 15)
#define GATE_DPL0                   (0 << 13)
#define GATE_DPL3                   (3 << 13)

#define GATE_TYPE_IDT		        (0xE << 8)		// 中断32位门描述符
#define GATE_TYPE_SYSCALL           (0xC << 8)

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
#define IRQ_KEYBOARD        0X21

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


// error
#define ERR_PAGE_P          (1 << 0)
#define ERR_PAGE_WR          (1 << 1)
#define ERR_PAGE_US          (1 << 1)

#define ERR_EXT             (1 << 0)
#define ERR_IDT             (1 << 1)


typedef struct _exception_frame_t {
    uint32_t gs, fs, es, ds;
    uint32_t edi, esi, edp, esp, ebx, edx, ecx, eax;
    uint32_t num, err_code;
    uint32_t eip, cs, eflags;
    uint32_t esp3, ss3;
} exception_frame_t;

typedef void (*irq_handler_t) (exception_frame_t * frame);


void exception_handler_unknown ();
void exception_handler_divider ();
void exception_handler_Debug ();
void exception_handler_NMI ();
void exception_handler_breakpoint ();
void exception_handler_overflow ();
void exception_handler_bound_range ();
void exception_handler_invalid_opcode ();
void exception_handler_device_unavailable ();
void exception_handler_double_fault ();
void exception_handler_invalid_tss ();
void exception_handler_segment_not_present ();
void exception_handler_stack_segment_fault ();
void exception_handler_general_protection ();
void exception_handler_page_fault ();
void exception_handler_fpu_error ();
void exception_handler_alignment_check ();
void exception_handler_machine_check ();
void exception_handler_smd_exception ();
void exception_handler_virtual_exception ();


int  irq_install(int irq_num, uint32_t handler);
void irq_enable (int irq_num );
void irq_disable(int irq_num );
void irq_enable_global();
void irq_disable_global();


void gate_desc_set(gate_desc_t * desc, uint16_t selector, uint32_t offset, uint16_t attr);

void irq_init ();

void pic_send_eoi(int irq_num);    // 中断处理完通知 8259

typedef uint32_t irq_state_t;

irq_state_t irq_enter_proection();

void irq_leave_proection(irq_state_t state);

#endif