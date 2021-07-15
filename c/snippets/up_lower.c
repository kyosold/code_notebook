#include <ctype.h>

char *str_toupper(char *s, unsigned int len)
{
    unsigned char *c, *e;

    c = (unsigned char *)s;
    e = (unsigned char *)c + len;

    while (c < e)
    {
        *c = toupper(*c);
        c++;
    }
    return s;
}

char *str_tolower(char *s, unsigned int len)
{
    unsigned char *c, *e;

    c = (unsigned char *)s;
    e = (unsigned char *)c + len;

    while (c < e)
    {
        *c = tolower(*c);
        c++;
    }
    return s;
}