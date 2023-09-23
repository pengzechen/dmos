#ifndef TTY_H
#define TTY_H
#include <sem.h>
#define TTY_OBUF_SIZE   512
#define TTY_IBUF_SIZE   512

#define TTY_OCRLF      (1 << 0)
#define TTY_INCLR      (1 << 0)
#define TTY_IECHO      (1 << 1)

typedef struct _tty_fifo_t {
    char* buf;
    int size;
    int read;
    int write;
    int count;
} tty_fifo_t ;


typedef struct _tty_t {
    char obuf[TTY_OBUF_SIZE];
    char ibuf[TTY_IBUF_SIZE];

    tty_fifo_t  ififo;
    tty_fifo_t  ofifo;
    sem_t       osem;
    sem_t       isem;
    int         iflags;
    int         oflags;

    int console_idx;

} tty_t;



int tty_fifo_get (tty_fifo_t * fifo, char * c) ;

int tty_fifo_put (tty_fifo_t * fifo, char c);

void tty_in(char ch);

void tty_select(int tty);

#endif


