#include "os_cfg.h"

.text
 	.global _start
	.extern kernel_init
	.extern init_main
_start:
    # 第一种方法
    # push %ebp
    # mov %esp, %ebp
    # mov 0x8(%ebp), %eax
    # push %eax

    # 第二种方法
    # mov 4(%esp), %eax
    # push %eax

    # 第三种方法
    push 4(%esp)

    # kernel_init(boot_info)
    call kernel_init

	// 重新加载GDT
	jmp $KERNEL_SELECTOR_CS, $gdt_reload

gdt_reload:
	mov $KERNEL_SELECTOR_DS, %ax		// 16为数据段选择子
	mov %ax, %ds
    mov %ax, %ss
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs

	// 栈设置
	mov $(stack + KERNEL_STACK_SIZE), %esp

	// 栈和段等沿用之前的设置
	jmp init_main

	.bss
.comm stack, KERNEL_STACK_SIZE    // comm 声明未初始化的通用内存区域，以字节计


.macro exception_handler name num with_error_code
    .text
    .extern handle_\name
    .global exception_handler_\name
    
exception_handler_\name:
    .if \with_error_code == 0
        push $0
    .endif

    push $\num
    pusha
    push %ds
    push %es
    push %fs
    push %gs
    push %esp           # 指向最后一个指令
    call handle_\name   # 栈中内容
    add $(1*4), %esp    # 跳过esp
    pop %gs
    pop %fs
    pop %es
    pop %ds
    popa

    add $(2*4), %esp    # 跳过 with_error_code num
    iret                # 中断返回

.endm

exception_handler unknown, -1, 0
exception_handler divider, 0, 0
exception_handler Debug, 1, 0
exception_handler NMI, 2, 0
exception_handler breakpoint, 3, 0
exception_handler overflow, 4, 0
exception_handler bound_range, 5, 0
exception_handler invalid_opcode, 6, 0
exception_handler device_unavailable, 7, 0
exception_handler double_fault, 8, 1
exception_handler invalid_tss, 10, 1
exception_handler segment_not_present, 11, 1
exception_handler stack_segment_fault, 12, 1
exception_handler general_protection, 13, 1
exception_handler page_fault, 14, 1
exception_handler fpu_error, 16, 0
exception_handler alignment_check, 17, 1
exception_handler machine_check, 18, 0
exception_handler smd_exception, 19, 0
exception_handler virtual_exception, 20, 0

exception_handler time, 0x20, 0
exception_handler kbd, 0x21, 0


    // simple_switch(&from, &to)
.text
.global simple_switch

simple_switch:
    mov 4(%esp), %eax
    mov 8(%esp), %edx

    push %ebp
    push %ebx
    push %esi
    push %edi
    mov %esp, (%eax)

    mov %edx, %esp
    pop %edi
    pop %esi
    pop %ebx
    pop %ebp
    
    sti
    ret


    .global exception_handle_syscall
    .extern do_handler_syscall
exception_handle_syscall:

    pusha
    push %ds
    push %es
    push %fs
    push %gs
    pushf

    mov %esp, %eax
    push %eax
    call do_handler_syscall
    add $4, %esp

    popf
    pop %gs
    pop %fs
    pop %es
    pop %ds
    popa


    retf $( 5 * 4)         // SYSCALL_PARAM_COUNT