#ifndef CONSOLE_H
#define CONSOLE_H
#include <comm/types.h>
#include <tty/tty.h>
#include <mux.h>

#define CONSOLE_DISP_ADDR   0Xb8000
#define CONSOLE_DISP_END    (0XB8000 + 32*1024)
#define CONSOLE_ROW_MAX     25
#define CONSOLE_COL_MAX     80


#define ASCII_ESC       0X1B // \033
#define ESC_PARAM_MAX   10


typedef enum _color_t {
    COLOR_Black			= 0,
    COLOR_Blue			= 1,
    COLOR_Green			= 2,
    COLOR_Cyan			= 3,
    COLOR_Red			= 4,
    COLOR_Magenta		= 5,
    COLOR_Brown			= 6,
    COLOR_Gray			= 7,
    COLOR_Dark_Gray 	= 8,
    COLOR_Light_Blue	= 9,
    COLOR_Light_Green	= 10,
    COLOR_Light_Cyan	= 11,
    COLOR_Light_Red		= 12,
    COLOR_Light_Magenta	= 13,
    COLOR_Yellow		= 14,
    COLOR_White			= 15
}color_t;


typedef union _disp_char_t {
    struct {
        char c;
        char foreground : 4;
        char background : 3;
    };
    uint16_t v;

}disp_char_t;

// ESC 7,8
// ESC [xx,xxm
typedef struct _console_t {
    enum {
        CONSOLE_WRITE_NOMAL,
        CONSOLE_WRITE_ESC,
        CONSOLE_WRITE_SQUARE,
    }write_state;
    
    disp_char_t * disp_base;
    int display_rows, display_cols;
    int cursor_row, cursor_col;
    color_t foreground, background;

    int old_cursor_col, old_cursor_row;
    
    int esc_param[ESC_PARAM_MAX];
    int curr_param_index;

    mutex_t mutex;                  // 写互斥锁
}console_t;

int  console_init(int id);
int  console_write(tty_t *tty) ;
void console_close(int console);
void console_select(int idx);

#endif  // CONSOLE_H