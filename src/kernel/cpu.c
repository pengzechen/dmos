#include <cpu.h>

static segment_desc_t gdt_table2[GDT_TABLE_SIZE];

void segment_desc_set(int selector, uint32_t base, uint32_t limit, uint16_t attr) {
    segment_desc_t * desc = gdt_table2 + (selector >> 3);

	if (limit > 0xfffff) {
		attr |= 0x8000;
		limit /= 0x1000;
	}
	desc->limit15_0 = limit & 0xffff;
	desc->base15_0 = base & 0xffff;
	desc->base23_16 = (base >> 16) & 0xff;
	desc->attr = attr | (((limit >> 16) & 0xf) << 8);
	desc->base31_24 = (base >> 24) & 0xff;
}

void gdt_init() {
    for (int i = 0; i < GDT_TABLE_SIZE; i++) {
        segment_desc_set(i << 3, 0, 0, 0);
    }

    segment_desc_set(KERNEL_SELECTOR_DS, 0x00000000, 0xFFFFFFFF,
        SEG_P_PRESENT | SEG_DPL0 | SEG_S_NORMAL | SEG_TYPE_DATA
        | SEG_TYPE_RW | SEG_D | SEG_G);

    segment_desc_set(KERNEL_SELECTOR_CS, 0x00000000, 0xFFFFFFFF,
        SEG_P_PRESENT | SEG_DPL0 | SEG_S_NORMAL | SEG_TYPE_CODE
        | SEG_TYPE_RW | SEG_D | SEG_G);

    lgdt((uint32_t)gdt_table2, sizeof(gdt_table2));

}

int  gdt_alloc_desc() {
    int i=1;
    for(; i < GDT_TABLE_SIZE; i++) {
        segment_desc_t* desc = gdt_table2 + i;
        if(desc->attr == 0) {
            return ( i * sizeof(segment_desc_t) );
        }
    }

    return -1;
}


