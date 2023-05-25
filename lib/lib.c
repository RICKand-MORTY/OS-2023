#include "lib.h"

typedef unsigned int size_t;

int strlen(const char* s)
{
    int count = 0;
    const char* p = s;
    for (; *p != '\0'; ++count, ++p);
    return count;
}

void memset(void* addr, int val, unsigned int size)
{
    char *a = addr;
    int i=0;
    for(i = 0;i < size;i++)
    {
        *a = val;
        a++;
    }
}

void *memcpy(void *dest, const void *src, size_t count)
{
	char *tmp = dest;
	const char *s = src;
	while (count--)
	{
        *tmp = *s;
        tmp++;
        s++;
    }
	return dest;
}
