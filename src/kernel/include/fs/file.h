#ifndef FILE_H
#define FILE_H

#include <comm/types.h>

#define FILE_NAME_SIZE          32
#define FILE_TABLE_SIZE         1024

typedef enum _file_type_t {
    FILE_UNKNOWN = 0,
    FILE_TTY,
} file_type_t;

typedef struct  _file_t {
    char file_name[FILE_NAME_SIZE];
    file_type_t  type;
    uint32_t     size;
    int ref;
    int dev_id;
    int pos;
    int mod;
}file_t;


file_t* file_alloc();
void file_free(file_t* file);

void file_table_init();

#endif
