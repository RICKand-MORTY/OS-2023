#ifndef __LIB_H__
#define __LIB_H__

int strlen(const char* s)
{
    int count = 0;
    const char* p = s;
    for (; *p != '\0'; ++count, ++p);
    return count;
}


#endif