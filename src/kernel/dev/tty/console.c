#include <tty/console.h>
#include <irq.h>
#include <klib.h>
#include <comm/cpu_ins.h>
#include <tty/tty.h>

#define CONSOLE_NR          8

static console_t console_buf[CONSOLE_NR];

static int   // 硬件相关
read_cursor_pos (void) {
    int pos;

    irq_state_t state = irq_enter_proection();
 	outb(0x3D4, 0x0F);		// 写低地址
	pos = inb(0x3D5);
	outb(0x3D4, 0x0E);		// 写高地址
	pos |= inb(0x3D5) << 8;   
    irq_leave_proection(state);
    return pos;
}

static void  // 硬件相关
update_cursor_pos (console_t * console) {
	uint16_t pos =  (console - console_buf) * 
                    (console->display_cols * console->display_rows);
    pos += console->cursor_row *  console->display_cols + console->cursor_col;

    irq_state_t state = irq_enter_proection();
	outb(0x3D4, 0x0F);		// 写低地址
	outb(0x3D5, (uint8_t) (pos & 0xFF));
	outb(0x3D4, 0x0E);		// 写高地址
	outb(0x3D5, (uint8_t) ((pos >> 8) & 0xFF));
    irq_leave_proection(state);
}



static void 
erease_rows(console_t* console, int from, int to) {
    disp_char_t * dis_from = console->disp_base + console->display_cols * from;
    disp_char_t * dis_to = console->disp_base + console->display_cols * (to + 1);
    while(dis_from < dis_to) {
        dis_from->c = ' ';
        dis_from->foreground = console->foreground;
        dis_from->background = console->background;

        dis_from++;
    }
}

static void 
scroll_up(console_t* console, int lines) {
    disp_char_t* dest = console->disp_base;
    disp_char_t* src = console->disp_base + console->display_cols * lines;
    uint32_t size = (console->display_rows - lines) * console->display_cols * sizeof(disp_char_t);

    k_memcpy(dest, src, size);
    erease_rows(console, console->display_rows - lines, console->display_rows - 1);
    
    console->cursor_row --;
}

static void 
move_forward(console_t* console, int n) {
    for ( int i=0; i < n; i++) {
        if (++ console->cursor_col >= console->display_cols) {
            console->cursor_row++;
            console->cursor_col = 0;

            if(console->cursor_row >= console->display_rows) {
                scroll_up(console, 1);
            }
        }
    }
}

static int 
move_backward(console_t* console, int n) {
    int status = -1;

    for(int i=0; i < n; i++) {
        if (console->cursor_col > 0) {
            console->cursor_col--;
            status = 0;
        } else if (console->cursor_row > 0) {
            console->cursor_row--;
            console->cursor_col = console->display_cols - 1;
            status = 0;
        }
    }

    return status;
}

static void 
show_char(console_t* console, char c) {

    int offset = console->cursor_col + console->cursor_row * console->display_cols;

    disp_char_t* p = console->disp_base + offset;

    p->c = c;
    p->foreground = console->foreground;
    p->background = console->background;

    move_forward(console, 1);
}

static void 
erease_back(console_t* console) {
    if (move_backward(console, 1) == 0) {
        show_char(console, ' ');
        move_backward(console, 1);
    }
}



void 
clear_display(console_t* console) {
    int size = console->display_cols * console->display_rows;

    disp_char_t * start = console->disp_base;
    for(int i=0; i <size; i++, start++ ) {
        start->c = ' ';
        start->foreground = console->foreground;
        start->background = console->background;
    }
}

int  
console_init(int idx) {
    console_t* console = console_buf + idx;
    console->cursor_row = console->cursor_col = 0;
    console->display_cols = CONSOLE_COL_MAX;
    console->display_rows = CONSOLE_ROW_MAX;

    console->disp_base = (disp_char_t*)CONSOLE_DISP_ADDR + idx * (CONSOLE_COL_MAX*CONSOLE_ROW_MAX);

    console->foreground = COLOR_White;
    console->background = COLOR_Black;

    if (idx == 0) {
        int cursor_pos = read_cursor_pos();
        console->cursor_row = cursor_pos / console->display_cols;
        console->cursor_col = cursor_pos % console->display_cols;
    }else {
        console->cursor_row = 0;
        console->cursor_col = 0;
        clear_display(console);
        update_cursor_pos(console);
    }  
    console ->old_cursor_col = console->cursor_col;
    console->old_cursor_row = console->cursor_row;

    console->write_state = CONSOLE_WRITE_NOMAL;
    mutex_init(&console->mutex);
    return 0;
}

void
mov_to_col0(console_t* console) {
    console->cursor_col = 0;
}

void 
mov_next_row(console_t* console) {
    console->cursor_row++;
    if ( console->cursor_row >= console->display_rows) {
        scroll_up(console, 1);
    }
}


static void 
move_left (console_t * console, int n) {
    if (n == 0) {
        n = 1;
    }

    int col = console->cursor_col - n;
    console->cursor_col = (col >= 0) ? col : 0;
}

static void 
move_right (console_t * console, int n) {
    if (n == 0) {
        n = 1;
    }

    int col = console->cursor_col + n;
    if (col >= console->display_cols) {
        console->cursor_col = console->display_cols - 1;
    } else {
        console->cursor_col = col;
    }
}

static void 
clear_esc_param (console_t * console) {
	k_memset(console->esc_param, 0, sizeof(console->esc_param));
	console->curr_param_index = 0;
}

static void 
set_font_style (console_t * console) {
	static const color_t color_table[] = {
			COLOR_Black, COLOR_Red, COLOR_Green, COLOR_Yellow, // 0-3
			COLOR_Blue, COLOR_Magenta, COLOR_Cyan, COLOR_White, // 4-7
	};

	for (int i = 0; i < console->curr_param_index; i++) {
		int param = console->esc_param[i];
		if ((param >= 30) && (param <= 37)) {  // 前景色：30-37
			console->foreground = color_table[param - 30];
		} else if ((param >= 40) && (param <= 47)) {
			console->background = color_table[param - 40];
		} else if (param == 39) { // 39=默认前景色
			console->foreground = COLOR_White;
		} else if (param == 49) { // 49=默认背景色
			console->background = COLOR_Black;
		}
	}
}



void
save_cursor(console_t *console) {
    console->old_cursor_col = console->cursor_col;
    console->old_cursor_row = console->cursor_row;
}

void
restore_cursor(console_t* console) {
    console->cursor_col = console->old_cursor_col;
    console->cursor_row = console->old_cursor_row;
}

static void 
move_cursor(console_t * console) {
	if (console->curr_param_index >= 1) {
		console->cursor_row = console->esc_param[0];
	}

	if (console->curr_param_index >= 2) {
		console->cursor_col = console->esc_param[1];
	}
}

static void 
erase_in_display(console_t * console) {
	if (console->curr_param_index <= 0) {
		return;
	}

	int param = console->esc_param[0];
	if (param == 2) {
		erease_rows(console, 0, console->display_rows - 1);
        console->cursor_col = console->cursor_row = 0;
	}
}




static void    // CONSOLE_WRITE_NOMAL
write_nomal(console_t* console, char ch) {
    switch(ch) {
        case ASCII_ESC:
            console->write_state = CONSOLE_WRITE_ESC;
            break;  
        case 0x7f:
            erease_back(console);
            break;
        case '\b':
            move_backward(console, 1);
            break;
        case '\r':
            mov_to_col0(console);
            break;
        case '\n':
            mov_to_col0(console);
            mov_next_row(console);
            break;
        default:
            if ((ch >= ' ') && (ch <= '~'))
            show_char(console, ch);
            break;
    }
}

static void    // CONSOLE_WRITE_ESC
write_esc(console_t* console, char ch) {
    switch (ch)
    {
    case '7':
        save_cursor(console);
        console->write_state = CONSOLE_WRITE_NOMAL;
        break;
    case '8':
        restore_cursor(console);
        console->write_state = CONSOLE_WRITE_NOMAL;
        break;
    case '[':
        clear_esc_param(console);
        console->write_state = CONSOLE_WRITE_SQUARE;
        break;
    default:
        console->write_state = CONSOLE_WRITE_NOMAL;
        break;
    }
}

static void  // CONSOLE_WRITE_SQUARE
write_esc_square (console_t * console, char c) {
    if ((c >= '0') && (c <= '9')) {
        int * param = &console->esc_param[console->curr_param_index];
        *param = *param * 10 + c - '0';
    } else if ((c == ';') && console->curr_param_index < ESC_PARAM_MAX) {
        console->curr_param_index++;
    } else {
        console->curr_param_index++;
        switch (c) {
        case 'm': // 设置字符属性
            set_font_style(console);
            break;
        case 'D':	// 光标左移n个位置 ESC [Pn D
            move_left(console, console->esc_param[0]);
            break;
        case 'C':
            move_right(console, console->esc_param[0]);
            break;
        case 'H':
        case 'f':
            move_cursor(console);
            break;
        case 'J':
            erase_in_display(console);
            break;
        }
        console->write_state = CONSOLE_WRITE_NOMAL;
    }
}

int  
console_write(tty_t *tty) {

    console_t* curr_c = console_buf + tty->console_idx;

    mutex_lock(&curr_c->mutex);
    int len = 0;

    do {
        char ch;
        int err = tty_fifo_get(&tty->ofifo, &ch);
        if (err < 0) {
            break;
        }
        sem_notify(&tty->osem);
    
        switch (curr_c->write_state)
        {
        case CONSOLE_WRITE_NOMAL:
            write_nomal(curr_c, ch);
            break;
        
        case CONSOLE_WRITE_ESC:
            write_esc(curr_c, ch);
            break;
        case CONSOLE_WRITE_SQUARE:
            write_esc_square(curr_c, ch);
            break;
        default:
            break;
        }

        len++;
    } while(1);

    mutex_unlock(&curr_c->mutex);


    update_cursor_pos(curr_c);
    return len;
}

void 
console_close(int console) {}


void 
console_select(int idx) {
    console_t * console = console_buf + idx;
    if (console->disp_base == 0) {
        console_init(idx);
    }

	uint16_t pos = idx * console->display_cols * console->display_rows;

	outb(0x3D4, 0xC);		// 写高地址
	outb(0x3D5, (uint8_t) ((pos >> 8) & 0xFF));
	outb(0x3D4, 0xD);		// 写低地址
	outb(0x3D5, (uint8_t) (pos & 0xFF));

    // 更新光标到当前屏幕
    update_cursor_pos(console);
    // char num = idx + '0';
    // show_char(console, num);
}