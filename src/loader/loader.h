#ifndef LOADER_H
#define LOADER_H

#include <comm/types.h>

#define BOOT_RAM_REGION_MAX			10		// RAM区最大数量


typedef struct _boot_info_t {
    // RAM区信息
    struct {
        uint32_t start;
        uint32_t size;
    }ram_region_cfg[BOOT_RAM_REGION_MAX];
    int ram_region_count;
} boot_info_t;

extern boot_info_t boot_info;			// 启动参数信息

#define SYS_KERNEL_LOAD_ADDR		(1024*1024)		// 内核加载的起始地址

// 保护模式入口函数，在start.asm中定义
void protect_mode_entry (void);

#endif
