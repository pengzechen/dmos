#ifndef CPU_H
#define CPU_H

#include <os_cfg.h>
#include <comm/types.h>

#define SEG_G               (1 << 15)
#define SEG_D               (1 << 14)
#define SEG_P_PRESENT       (1 << 7)

#define SEG_DPL0            (0 << 5)
#define SEG_DPL3            (3 << 5)

#define SEG_RPL0            (0 << 0)
#define SEG_RPL3            (3 << 0)

#define SEG_S_SYSTEM        (0 << 4)
#define SEG_S_NORMAL        (1 << 4)

#define SEG_TYPE_CODE       (1 << 3)
#define SEG_TYPE_DATA       (0 << 3)              
#define SEG_TYPE_TSS        (9 << 0)              // TSS
#define SEG_TYPE_RW         (1 << 1)              // 读写


#define EFLAGES_DEFAULT     (1 << 1)
#define EFLAGS_IF           (1 << 9)



typedef struct _segment_desc_t segment_desc_t;

#pragma pack(1)
    struct _segment_desc_t {
        uint16_t limit15_0;
        uint16_t base15_0;
        uint8_t base23_16;
        uint16_t attr;
        uint8_t base31_24;
    };

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

#pragma pack(1)
typedef struct _gate_desc_t {
	uint16_t offset15_0;
	uint16_t selector;
	uint16_t attr;
	uint16_t offset31_16;
}gate_desc_t;
#pragma pack()



void segment_desc_set(int selector, uint32_t base, uint32_t limit, uint16_t attr);
void gdt_init();
int  gdt_alloc_desc();
void gdt_free_sel(int sel);
#endif