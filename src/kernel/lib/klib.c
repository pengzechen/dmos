#include <klib.h>


void k_strcpy(char * dest, const char * src) {
    if (!dest || !src) {
        return;
    }
    while (*dest && *src) {
        *dest++ = *src++;
    }
    *dest = '\0'; 
}
void k_strncpy(char * dest, const char * src, int size) {
    if (!dest || !src || !size) {
        return;
    }
    char* d = dest;
    const char* s = src;
    while(size-- > 0 && (*s)) {
        *d++ = *s++;
    } 
    if (size == 0) {
        *(d - 1) = '\0';
    } else {
        *d = '\0';
    }
}
int k_strncmp(const char * s1, const char * s2, int size) {
    if (!s1 || !s2) return -1;

    while(*s1 && *s2 && (*s1 == *s2) && size) {
        s1 ++;
        s2 ++;
    }

    return !((*s1 == '\0') || (*s2 == '\0') || (*s1 == *s2));
}
int k_strlen(const char *str) {
    if (!str) {
        return 0;
    }

    int len = 0;
    const char * c = str;
    while(*c ++ ) {
        len++;
    }

    return len;
}

void k_memcpy(void* dest, void* src, int size) {
    if (!dest || !src || size) {
        return;
    }
    uint8_t *s = (uint8_t*)src;
    uint8_t *d = (uint8_t*)dest;
    while(size--) {
        *d++ = *s++;
    }
}
void k_memset(void* dest, uint8_t v, int size) {
    if(!dest || !size) return;

    uint8_t* d = (uint8_t*) dest;
    while(size--) {
        *d++ = v;
    }
}

int k_memcmp(void* d1, void* d2, int size) {
    if (!d1 || !d2 || size) {
        return 1;
    }
    uint8_t *p_d1 = (uint8_t*)d1;
    uint8_t *p_d2= (uint8_t*)d2;
    while(size--) {
        if (*p_d1 ++ != *p_d2++)
        return 1;
    }

    return 0;
}

void k_itoa(char * buf, int num, int base) {
    static const char * num2ch = {"FEDCBA9876543210123456789ABCDEF"};
    char *p = buf;
    int old_num = num;

    if ( (base != 2) && (base != 8) && (base != 10) && (base != 16) ){
        *p = '\0';
        return;
    }
    if ( (num < 0) && (base == 10)) {
        *p++ = '-';
    }

    do {
        char ch = num2ch[ num % base +15 ];
        *p++ = ch;
        num /= base;
    } while(num);

    *p-- = '\0';

    char *start = (old_num > 0) ? buf : buf + 1;
    while(start < p) {
        char ch = *start;
        *start = *p;
        *p = ch;

        p--;
        start++;
    }
}

void k_vsprint(char *buf, const char *fmt, va_list args) {
    enum {NORMAL, READ_FMT} state = NORMAL;

    char* curr = buf;
    char ch;
    while((ch = *fmt++)) {
        switch (state) {
        case NORMAL:
            if(ch == '%') {
                state = READ_FMT;
            } else {
                *curr++ = ch;
            }
            break;
        case READ_FMT:
            if (ch == 'd') {
                int num  = va_arg(args, int);
                k_itoa(curr, num, 10);
                curr += k_strlen(curr);
                
            }else if (ch == 'x') {
                int num  = va_arg(args, int);
                k_itoa(curr, num, 16);
                curr += k_strlen(curr);

            }else if (ch == 'c') {
                char c  = va_arg(args, int);
                *curr++ = c;

            }else if (ch == 's') {
                const char* str = va_arg(args, char*);
                int len = k_strlen(str);
                while(len--) {
                    *curr++ = *str++;
                }
            }
            state = NORMAL;
            break;
        
        default:
            break;
        }
    }
}