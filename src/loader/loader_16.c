__asm__(".code16gcc");

#include <comm/types.h>
#include <comm/cpu_ins.h>
#include <loader/loader.h>

typedef struct SMAP_entry {
    uint32_t BaseL; 	// base address uint64_t
    uint32_t BaseH;
    uint32_t LengthL; 	// length uint64_t
    uint32_t LengthH;
    uint32_t Type; 		// entry Type
    uint32_t ACPI; 		// extended
} __attribute__((packed)) SMAP_entry_t;

#define BOOT_RAM_REGION_MAX			10		// RAM区最大数量


boot_info_t boot_info;			// 启动参数信息

uint16_t gdt_table[][4] = {
    {0, 0, 0, 0},
    {0xFFFF, 0x0000, 0x9A00, 0x00CF},
    {0xFFFF, 0x0000, 0x9200, 0x00CF},
};


static void show_msg (const char * msg) {
    char c;
	while ((c = *msg++) != '\0') {
		__asm__ __volatile__( 
			"mov $0xe, %%ah\n\t"
			"mov %[ch], %%al\n\t"
			"int $0x10"::[ch]"r"(c) );
	}
}


static void  detect_memory(void) {
	uint32_t contID = 0;
	SMAP_entry_t smap_entry;
	int signature, bytes;

    show_msg("try to detect memory:");

	boot_info.ram_region_count = 0;
	for (int i = 0; i < BOOT_RAM_REGION_MAX; i++) {
		SMAP_entry_t * entry = &smap_entry;

		__asm__ __volatile__("int  $0x15"
			: "=a"(signature), "=c"(bytes), "=b"(contID)
			: "a"(0xE820), "b"(contID), "c"(24), "d"(0x534D4150), "D"(entry));
		if (signature != 0x534D4150) {
            show_msg("failed.\r\n");
			return;
		}

		if (bytes > 20 && (entry->ACPI & 0x0001) == 0){
			continue;
		}

        if (entry->Type == 1) {
            boot_info.ram_region_cfg[boot_info.ram_region_count].start = entry->BaseL;
            boot_info.ram_region_cfg[boot_info.ram_region_count].size = entry->LengthL;
            boot_info.ram_region_count++;
        }

		if (contID == 0) {
			break;
		}
	}
    show_msg("ok.\r\n");
}


void loader_entry(void) {
    show_msg("System Loading ...\r\n");

	detect_memory();

    cli();

    uint8_t v = inb(0x92);
    outb(0x92, v | 0x2);

	lgdt((uint32_t)gdt_table, sizeof(gdt_table));

    uint32_t cr0 = read_cr0();
    write_cr0(cr0 | (1 << 0));

	
    far_jump(8, (uint32_t)protect_mode_entry);

	for(;;){}
}
