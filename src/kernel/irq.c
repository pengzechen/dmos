#include <irq.h>


//  -----------------------------------------   中断相关    -------------------------------------------
#define IDT_TABLE_NR 128
static gate_desc_t idt_table[IDT_TABLE_NR];

static void default_handler (exception_frame_t * frame, const char * message) {
    klog("--------------------------------");
    klog("IRQ/Exception happend: %s.", message);
    for (;;) {
        hlt();
    }
}
void handle_unknown (exception_frame_t * frame) {
	default_handler(frame, "Unknown exception.");
}
void handle_divider(exception_frame_t * frame) {
	default_handler(frame, "Divider Error.");
}
void handle_Debug(exception_frame_t * frame) {
	default_handler(frame, "Debug Exception");
}
void handle_NMI(exception_frame_t * frame) {
	default_handler(frame, "NMI Interrupt.");
}
void handle_breakpoint(exception_frame_t * frame) {
	default_handler(frame, "Breakpoint.");
}
void handle_overflow(exception_frame_t * frame) {
	default_handler(frame, "Overflow.");
}
void handle_bound_range(exception_frame_t * frame) {
	default_handler(frame, "BOUND Range Exceeded.");
}
void handle_invalid_opcode(exception_frame_t * frame) {
	default_handler(frame, "Invalid Opcode.");
}
void handle_device_unavailable(exception_frame_t * frame) {
	default_handler(frame, "Device Not Available.");
}
void handle_double_fault(exception_frame_t * frame) {
	default_handler(frame, "Double Fault.");
}
void handle_invalid_tss(exception_frame_t * frame) {
	default_handler(frame, "Invalid TSS");
}
void handle_segment_not_present(exception_frame_t * frame) {
	default_handler(frame, "Segment Not Present.");
}
void handle_stack_segment_fault(exception_frame_t * frame) {
	default_handler(frame, "Stack-Segment Fault.");
}
void handle_general_protection(exception_frame_t * frame) {
	default_handler(frame, "IRQ/Exception happend: General Protection.");
}
void handle_page_fault(exception_frame_t * frame) {
	default_handler(frame, "IRQ/Exception happend: Page fault.");
}
void handle_fpu_error(exception_frame_t * frame) {
	default_handler(frame, "X87 FPU Floating Point Error.");
}
void handle_alignment_check(exception_frame_t * frame) {
	default_handler(frame, "Alignment Check.");
}
void handle_machine_check(exception_frame_t * frame) {
	default_handler(frame, "Machine Check.");
}
void handle_smd_exception(exception_frame_t * frame) {
	default_handler(frame, "SIMD Floating Point Exception.");
}
void handle_virtual_exception(exception_frame_t * frame) {
	default_handler(frame, "Virtualization Exception.");
}

void gate_desc_set(gate_desc_t * desc, uint16_t selector, uint32_t offset, uint16_t attr) {
	desc->offset15_0 = offset & 0xffff;
	desc->selector = selector;
	desc->attr = attr;
	desc->offset31_16 = (offset >> 16) & 0xffff;
}

int irq_install(int irq_num, uint32_t handler) {
    if(irq_num >= IDT_TABLE_NR) {
        return -1;
    }

    gate_desc_set(idt_table + irq_num, KERNEL_SELECTOR_CS, (uint32_t)handler, 
        GATE_P_PRESENT | GATE_DPL0 | GATE_TYPE_IDT);
}

static void init_pic() {
    outb(PIC0_ICW1, PIC_ICW1_ALWAYS_1 | PIC_ICW1_ICW4);
    outb(PIC0_ICW2, IRQ_PIC_START);
    outb(PIC0_ICW3, 1 << 2);
    outb(PIC0_ICW4, PIC_ICW4_8086);
    outb(PIC1_ICW1, PIC_ICW1_ICW4 | PIC_ICW1_ALWAYS_1);
    outb(PIC1_ICW2, IRQ_PIC_START + 8);
    outb(PIC1_ICW3, 2);
    outb(PIC1_ICW4, PIC_ICW4_8086);
    outb(PIC0_IMR, 0xFF & ~(1 << 2));
    outb(PIC1_IMR, 0xFF);
}

void irq_init () {
    for (uint32_t i = 0; i < IDT_TABLE_NR; i++) 
    	gate_desc_set(idt_table + i, KERNEL_SELECTOR_CS, (uint32_t) exception_handler_unknown,
            GATE_P_PRESENT | GATE_DPL0 | GATE_TYPE_IDT);

    lidt((uint32_t)idt_table, sizeof(idt_table));

    // 设置异常处理接口
    irq_install(IRQ0_DE, (uint32_t)exception_handler_divider);
	irq_install(IRQ1_DB, (uint32_t)exception_handler_Debug);
	irq_install(IRQ2_NMI, (uint32_t)exception_handler_NMI);
	irq_install(IRQ3_BP, (uint32_t)exception_handler_breakpoint);
	irq_install(IRQ4_OF, (uint32_t)exception_handler_overflow);
	irq_install(IRQ5_BR, (uint32_t)exception_handler_bound_range);
	irq_install(IRQ6_UD, (uint32_t)exception_handler_invalid_opcode);
	irq_install(IRQ7_NM, (uint32_t)exception_handler_device_unavailable);
	irq_install(IRQ8_DF, (uint32_t)exception_handler_double_fault);
	irq_install(IRQ10_TS, (uint32_t)exception_handler_invalid_tss);
	irq_install(IRQ11_NP, (uint32_t)exception_handler_segment_not_present);
	irq_install(IRQ12_SS, (uint32_t)exception_handler_stack_segment_fault);
	irq_install(IRQ13_GP, (uint32_t)exception_handler_general_protection);
	irq_install(IRQ14_PF, (uint32_t)exception_handler_page_fault);
	irq_install(IRQ16_MF, (uint32_t)exception_handler_fpu_error);
	irq_install(IRQ17_AC, (uint32_t)exception_handler_alignment_check);
	irq_install(IRQ18_MC, (uint32_t)exception_handler_machine_check);
	irq_install(IRQ19_XM, (uint32_t)exception_handler_smd_exception);
	irq_install(IRQ20_VE, (uint32_t)exception_handler_virtual_exception);

    init_pic();
}


// 中断的打开与关闭
void irq_enable(int irq_num) {
    if (irq_num < IRQ_PIC_START) {
        return;
    }

    irq_num -= IRQ_PIC_START;
    if (irq_num < 8) {
        uint8_t mask = inb(PIC0_IMR) & ~(1 << irq_num);
        outb(PIC0_IMR, mask);
    } else {
        irq_num -= 8;
        uint8_t mask = inb(PIC1_IMR) & ~(1 << irq_num);
        outb(PIC1_IMR, mask);
    }
}

void irq_disable(int irq_num) {
    if (irq_num < IRQ_PIC_START) {
        return;
    }

    irq_num -= IRQ_PIC_START;
    if (irq_num < 8) {
        uint8_t mask = inb(PIC0_IMR) | (1 << irq_num);
        outb(PIC0_IMR, mask);
    } else {
        irq_num -= 8;
        uint8_t mask = inb(PIC1_IMR) | (1 << irq_num);
        outb(PIC1_IMR, mask);
    }
}

void irq_disable_global(void) {
    cli();
}

void irq_enable_global(void) {
    sti();
}

void pic_send_eoi(int irq_num) {
    irq_num -= IRQ_PIC_START;
    if (irq_num >= 8) 
        outb(PIC1_OCW2, PIC_OCW2_EOI);
    
    outb(PIC0_OCW2, PIC_OCW2_EOI);
}
