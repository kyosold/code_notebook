
///////////////////////////////////////////////////////////////
/**
 * @brief   用斜杠 \ 转义特殊字符
 *          - single quote (')
 *          - double quote (")
 *          - backslash (\)
 *          - NUL (the NUL byte)
 * @param str   The string to be escaped.
 * @param len   The length of str
 * @param ret_str   Store string to be escaped. (内存应该是2倍的str大小)
 * @return The length of ret_str
 */
int str_addslashes(char *str, int len, char *ret_str)
{
    char *new_str;
    char *source, *target;
    char *end;
    int new_len;
    int local_new_len;

    new_str = ret_str;
    source = str;
    end = source + len;
    target = new_str;

    while (source < end)
    {
        switch (*source)
        {
        case '\0':
            *target++ = '\\';
            *target++ = '0';
            break;
        case '\'':
        case '\"':
        case '\\':
            *target++ = '\\';
            /* break is missing *intentionally* */
        default:
            *target++ = *source;
            break;
        }
        source++;
    }

    *target = 0;
    return (target - new_str);
}
///////////////////////////////////////////////////////////////

/**
 * @brief   字符串转成大写
 * @param s     The string to be uppercase.
 * @param len   The length of str
 * @return Returns string with all alphabetic characters converted to uppercase.
 */
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

/**
 * @brief   字符串转成小写
 * @param s     The string to be lowercase.
 * @param len   The length of str
 * @return Returns string with all alphabetic characters converted to lowercase.
 */
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

#ifdef _TEST
#include <stdio.h>
#include <string.h>
void main(int argc, char **argv)
{
    char str[1024] = {0};
    int r = str_addslashes(argv[1], strlen(argv[1]), str, 1024);
    printf("ret:%d str:(%s)\n", r, str);
}
#endif
