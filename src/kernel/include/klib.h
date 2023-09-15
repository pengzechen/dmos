#ifndef KLIB_H
#define KLIB_H

#include <comm/types.h>
#include <stdarg.h>

void k_strcpy(char * dest, const char * src);
void k_strncpy(char * dest, const char * src, int size);
int k_strncmp(const char * s1, const char * s2, int size);
int k_strlen(const char *str);

void k_memcpy(void* dest, void* src, int size);
void k_memset(void* dest, uint8_t v, int size);

int k_memcmp(void* d1, void* d2, int size);
void k_vsprint(char *buf, const char *fmt, va_list args) ;
#endif
