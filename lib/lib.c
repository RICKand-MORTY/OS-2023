#include "lib.h"

#define NULL ((void*)0)

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

// Copy n bytes from src to dst.
// The memory areas may overlap.
void* memmove(void *dst, const void *src, unsigned int n)
{
  const char *s = src;
  char *d = dst;

  if (s < d && s + n > d) { // 如果源地址小于目标地址，并且有重叠，就从后往前复制
    s += n;
    d += n;
    while (n-- > 0)
      *--d = *--s;
  } else { // 否则就从前往后复制
    while (n-- > 0)
      *d++ = *s++;
  }

  return dst;
}

int strcmp(const char *s1, const char *s2)
{
	while (*s1 && *s2) 
	{
		if (*s1 < *s2) 
			return -1; 
		else if (*s1 > *s2) 
			return 1; 
		else 
		{
			s1++; 
			s2++; 
		}
	}
	if (*s1 == '\0' && *s2 == '\0') 
		return 0; 
	else if (*s1 == '\0') 
		return -1; 
	else 
		return 1; 
}

char *strcpy(char *dest, const char *src)
{
    if (dest == NULL || src == NULL)
    {
        return NULL;
    }
    const char *p = src;
    char *q = dest;
    while (*p != '\0')
    {
        *q = *p;
        p++;
        q++;
    }
    *q = '\0';
    return dest;
}