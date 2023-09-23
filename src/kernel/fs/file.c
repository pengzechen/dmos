#include <fs/file.h>
#include <mux.h>
#include <klib.h>

static file_t file_tb[FILE_TABLE_SIZE];
static mutex_t file_alloc_mutex;

file_t* file_alloc() {
    file_t* file = (file_t*)0;
    mutex_lock(&file_alloc_mutex);
    for (int i=0; i<FILE_TABLE_SIZE; i++) {
        file_t* pfile = file_tb + i;
        if (pfile->ref == 0) {
            k_memset(pfile, 0, sizeof(file_t));
            pfile->ref = 1;
            file = pfile;
            break;
        }
    }
    mutex_unlock(&file_alloc_mutex);
    return file;
}

void file_free(file_t* file) {
    mutex_lock(&file_alloc_mutex);
    if (file->ref) {
        file->ref--;
    }
    mutex_unlock(&file_alloc_mutex);
}

void file_table_init() {
    mutex_init(&file_alloc_mutex);
    k_memset(file_tb, 0, sizeof(file_tb));
}