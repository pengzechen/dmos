#include <comm/cpu_ins.h>
#include <cpu.h>
#include <mux.h>


static segment_desc_t g_gdt_table[GDT_TABLE_SIZE];
static mutex_t g_mutex;

void segment_desc_set(int selector, uint32_t base, uint32_t limit, uint16_t attr) {
    segment_desc_t * desc = g_gdt_table + (selector >> 3);

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
    mutex_init(&g_mutex);

    for (int i = 0; i < GDT_TABLE_SIZE; i++) {
        segment_desc_set(i * sizeof(segment_desc_t), 0, 0, 0);
    }

    segment_desc_set(KERNEL_SELECTOR_DS, 0x00000000, 0xFFFFFFFF,
        SEG_P_PRESENT | SEG_DPL0 | SEG_S_NORMAL | SEG_TYPE_DATA
        | SEG_TYPE_RW | SEG_D | SEG_G);

    segment_desc_set(KERNEL_SELECTOR_CS, 0x00000000, 0xFFFFFFFF,
        SEG_P_PRESENT | SEG_DPL0 | SEG_S_NORMAL | SEG_TYPE_CODE
        | SEG_TYPE_RW | SEG_D | SEG_G);

    lgdt((uint32_t)g_gdt_table, sizeof(g_gdt_table));

}

int  gdt_alloc_desc() {
    mutex_lock(&g_mutex);
    int i = 1;
    for(; i < GDT_TABLE_SIZE; i++) {
        segment_desc_t* desc = g_gdt_table + i;
        if(desc->attr == 0) {
            mutex_unlock(&g_mutex);
            return ( i * sizeof(segment_desc_t) );
        }
    }
    mutex_unlock(&g_mutex);
    return -1;
}

void gdt_free_sel(int sel) {
    mutex_lock(&g_mutex);
    g_gdt_table[sel/sizeof(segment_desc_t)].attr = 0;
    mutex_unlock(&g_mutex);
}


