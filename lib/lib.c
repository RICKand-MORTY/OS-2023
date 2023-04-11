#include "lib.h"

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