  	.code16
 	.text
	.global _start
	# .extern boot_entry
_start:

	mov $0, %ax
	mov %ax, %ds
	mov %ax, %ss
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs

	mov $_start, %esp

	# mov $0xe, %ah
	# mov $'D', %al
	# int $0x10
	# mov $0xe, %ah
	# mov $'M', %al
	# int $0x10
	# mov $0xe, %ah
	# mov $'O', %al
	# int $0x10
	# mov $0xe, %ah
	# mov $'S', %al
	# int $0x10
	# mov $0xe, %ah
	# mov $'\r', %al
	# int $0x10
	# mov $0xe, %ah
	# mov $'\n', %al
	# int $0x10

read_loader:
	mov $0x8000, %bx	# 读取到的内存地址
	
    mov $0x2, %cx		# ch:磁道号，cl起始扇区号
	mov $0x2, %ah		# ah: 0x2读磁盘命令
	mov $64, %al		# al: 读取的扇区数量, 必须小于128，暂设置成32KB
	mov $0x0080, %dx	# dh: 磁头号，dl驱动器号0x80(磁盘1)
	int $0x13
	jc read_loader

	jmp 0x8000

	.section boot_end, "ax"
boot_sig: .byte 0x55, 0xaa

