#include <tty/tty.h>
#include <dev.h>
#include <tty/console.h>
#include <tty/kbd.h>
#include <log.h>
#include <irq.h>

static tty_t tty_devs[8];
static int curr_tty = 0;

static tty_t* 
get_tty(device_t* dev) {
    int tty = dev->minor;
    if ((tty < 0) || (tty >= 8) || (!dev->open_count)) {
        klog("tty is not opened");
        return (tty_t*)0;
    }

    return tty_devs + tty;
}

void 
tty_fifo_init(tty_fifo_t* fifo, char* buf, int size) {
    fifo->buf = buf;
    fifo->count = 0;
    fifo->size = size;
    fifo->read = fifo->write = 0;
}

int 
tty_fifo_get (tty_fifo_t * fifo, char * c) {
	if (fifo->count <= 0) {
		return -1;
	}

	irq_state_t state = irq_enter_proection();
	*c = fifo->buf[fifo->read++];
	if (fifo->read >= fifo->size) {
		fifo->read = 0;
	}
	fifo->count--;
	irq_leave_proection(state);
	return 0;
}

int 
tty_fifo_put (tty_fifo_t * fifo, char c) {
	if (fifo->count >= fifo->size) {
		return -1;
	}

	irq_state_t state = irq_enter_proection();
	fifo->buf[fifo->write++] = c;
	if (fifo->write >= fifo->size) {
		fifo->write = 0;
	}
	fifo->count++;
	irq_leave_proection(state);

	return 0;
}

int tty_open (device_t* dev) {
    int index = dev->minor;
    if( ( index < 0) || (index >= 8) ){
        klog("open tty error");
        return -1;
    }

    tty_t* tty = tty_devs + index;
    tty_fifo_init( &tty->ififo, tty->ibuf, TTY_IBUF_SIZE);
    tty_fifo_init( &tty->ofifo, tty->obuf, TTY_OBUF_SIZE);
    sem_init(&tty->osem, TTY_OBUF_SIZE);
    sem_init(&tty->isem, 0);
    tty->oflags = TTY_OCRLF;
    tty->iflags = TTY_IECHO;

    tty->console_idx = index;
    
    kbd_init();
    console_init(index);
    return 0;
}

int 
tty_write (device_t* dev, int addr, char * buf, int size) {
    if (size < 0) {
        return -1;
    }

    tty_t* tty = get_tty(dev);
    if (!tty) {
        return -1;
    }

    int len = 0;
    while (size) {
        char c = *buf++;

        if ((c == '\n') && (tty->oflags & TTY_OCRLF)) {
            sem_wait(&tty->osem);
            int err = tty_fifo_put(&tty->ofifo, '\r');
            if (err < 0) {
                break;
            }
        }

        sem_wait(&tty->osem);
        int err = tty_fifo_put(&tty->ofifo, c);
		if (err < 0) {
			break;
		}

        len++;
		size--;

		console_write(tty);

    }
    return len;
}

int 
tty_read (device_t* dev, int addr, char * buf, int size) {
    if (size < 0) {
        return -1;
    }
    tty_t* tty = get_tty(dev);
    char* pbuf = buf;
    int len = 0;
    while (len < size) {
        sem_wait(&tty->isem);

        char ch;
        tty_fifo_get(&tty->ififo, &ch);
        switch (ch) {
            case '\n':
            if ((tty->iflags & TTY_INCLR) && (len < size -1)) {
                *pbuf++ = '\r';
                len++;
            }
            *pbuf++ = '\n';
            len++;
            break;

            default:
            *pbuf++ = ch;
            len++;
            break;
        }

        if (tty->iflags & TTY_IECHO) {
            tty_write(dev, 0, &ch, 1);
        }

        if ((ch == '\n') || (ch == '\r')) {
            break;
        }
    }

    return len;
}

int 
tty_control(device_t* dev, int cmd, int arg0, int arg1) {
    return 0;
}

int 
tty_close (device_t* dec) {

}

void 
tty_in(char ch) {
    tty_t* tty = tty_devs + curr_tty;

    if (sem_count(&tty->isem) >= TTY_IBUF_SIZE) {
        return;
    }

    tty_fifo_put(&tty->ififo, ch);
    sem_notify(&tty->isem);
}

void 
tty_select(int tty) {
    if (tty != curr_tty) {
        console_select(tty);
        curr_tty = tty;
    }
}


dev_desc_t dev_tty_desc = {
    .name = "tty",
    .major = DEV_TTY,
    .open = tty_open,
    .read = tty_read,
    .write = tty_write,
    .control = tty_control,
    .close = tty_close,
};
