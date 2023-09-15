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
	jmp $KERNEL_SELECTOR_CS, $gdt_reload

gdt_reload:
	mov $KERNEL_SELECTOR_DS, %ax		// 16为数据段选择子
	mov %ax, %ds
    mov %ax, %ss
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    jmp init_main

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
exception_handler divider,  0, 0

exception_handler time, 0x20, 0