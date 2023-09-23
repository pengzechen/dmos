#ifndef DEV_H
#define DEV_H

#define DEV_NAME_SIZE       64

enum {
    DEV_UNKNOWN = 0,
    DEV_TTY,

};

typedef  struct  _device_t device_t;

// 手机
typedef struct _dev_desc_t {
    char name[DEV_NAME_SIZE];
    int major;

    int (*open)   (device_t* dev);
    int (*read)   (device_t* dev, int addr, char * buf, int size);
    int (*write)  (device_t* dev, int addr, char * buf, int size);
    int (*control)(device_t* dev, int cmd, int arg0, int arg1);
    int (*close)  (device_t* dec);
} dev_desc_t;


// vivo
struct  _device_t {
    dev_desc_t* desc;

    int     mode;
    int     minor;
    void *  data;
    int     open_count;
};

int dev_open(int major, int minor, void* data);
int dev_read(int dev_id, int addr, char* buf, int size);
int dev_write(int dev_id, int addr, char* buf, int size);
int dev_control(int dev_id, int cmd, char* buf, int arg0, int arg1);
void dev_close(int dev_id);

#endif

