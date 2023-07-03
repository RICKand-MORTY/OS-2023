#ifndef __LIB_H__
#define __LIB_H__


typedef unsigned int size_t;

int strlen(const char* s);
void memset(void* addr, int val, unsigned int size);
void* memmove(void *dst, const void *src, unsigned int n);
void *memcpy(void *dest, const void *src, size_t count);
int strcmp(const char *s1, const char *s2);
char *strcpy(char *dest, const char *src);
#endif