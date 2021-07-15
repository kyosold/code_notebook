#include <stdio.h>
#include <string.h>
#include "str_trim.h"

/**
 * @return 0:succ, 1:fail
 */
static inline int char_mask(unsigned char *input, int len, char *mask)
{
    unsigned char *end;
    unsigned char c;
    int result = 0;

    memset(mask, 0, 256);
    for (end = input + len; input < end; input++)
    {
        c = *input;
        if ((input + 3 < end) && input[1] == '.' && input[2] == '.' && input[3] >= c)
        {
            memset(mask + c, 1, input[3] - c + 1);
            input += 3;
        }
        else if ((input + 1 < end) && input[0] == '.' && input[1] == '.')
        {
            /* Error, try to be as helpful as possible:
                (a range ending/starting with '.' won't be captured here) */
            if (end - len >= input)
            { /* there was no 'left' char */
                // Invalid '..'-range, no character to the left of '..'
                result = 1;
                continue;
            }
            if (input + 2 >= end)
            { /* there is no 'right' char */
                // "Invalid '..'-range, no character to the right of '..'
                result = 1;
                continue;
            }
            if (input[-1] > input[2])
            {
                /* wrong order */
                // Invalid '..'-range, '..'-range needs to be incrementing
                result = 1;
                continue;
            }
            /* FIXME: better error (a..b..c is the only left possibility?) */
            result = 1;
            continue;
        }
        else
        {
            mask[c] = 1;
        }
    }
    return result;
}

/**
 * @param mode 1:trim left, 2:trim right, 3:trim left and right
 * @param what what indicates which chars are to be trimmed. NULL->default (' \t\n\r\v\0')
 */
void _s_trim(char *str, int len, char *what, int what_len, int mode, int *t, int *l)
{
    int i;
    int trimmed = 0;
    char mask[256];

    if (what)
        char_mask((unsigned char *)what, what_len, mask);
    else
        char_mask((unsigned char *)" \n\r\t\v\0", 6, mask);

    if (mode & 1)
    {
        for (i = 0; i < len; i++)
        {
            if (mask[(unsigned char)str[i]])
                trimmed++;
            else
                break;
        }
        len -= trimmed;
        str += trimmed;
    }
    if (mode & 2)
    {
        for (i = len - 1; i >= 0; i--)
        {
            if (mask[(unsigned char)str[i]])
                len--;
            else
                break;
        }
    }

    *t = trimmed;
    *l = len;
}

void str_ltrim(char *str, int str_len, char *what, int what_len, char *str_trimmed, int str_trimmed_size)
{
    int trimmed = 0;
    int len = 0;

    _s_trim(str, str_len, what, what_len, 1, &trimmed, &len);

    snprintf(str_trimmed, str_trimmed_size, "%s", str + trimmed);
    str_trimmed[len] = '\0';
}

void str_rtrim(char *str, int str_len, char *what, int what_len, char *str_trimmed, int str_trimmed_size)
{
    int trimmed = 0;
    int len = 0;

    _s_trim(str, str_len, what, what_len, 2, &trimmed, &len);

    snprintf(str_trimmed, str_trimmed_size, "%s", str + trimmed);
    str_trimmed[len] = '\0';
}

/**
 * @brief Strip whitespace (or other characters) from the beggining and end of a string
 * @param str       The string that will be trimmed
 * @param str_len   Then length of str
 * @param what      The stripped characters, NULL->default (' \t\n\r\v\0')
 * @param what_len  The length of what
 * @param str_trimmed   The trimmed string
 * @param str_trimmed_size  The trimmed string memory size
 */
void str_trim(char *str, int str_len, char *what, int what_len, char *str_trimmed, int str_trimmed_size)
{
    int trimmed = 0;
    int len = 0;

    _s_trim(str, str_len, what, what_len, 3, &trimmed, &len);

    snprintf(str_trimmed, str_trimmed_size, "%s", str + trimmed);
    str_trimmed[len] = '\0';
}

char *str_ltrim_alloc(char *str, int str_len, char *what, int what_len)
{
    int trimmed = 0;
    int len = 0;

    _s_trim(str, str_len, what, what_len, 1, &trimmed, &len);

    char *s = strndup(str + trimmed, len);
    return s;
}

char *str_rtrim_alloc(char *str, int str_len, char *what, int what_len)
{
    int trimmed = 0;
    int len = 0;

    _s_trim(str, str_len, what, what_len, 2, &trimmed, &len);

    char *s = strndup(str + trimmed, len);
    return s;
}

/**
 * @brief Strip whitespace (or other characters) from the beggining and end of a string
 * @param str       The string that will be trimmed
 * @param str_len   Then length of str
 * @param what      The stripped characters, NULL->default (' \t\n\r\v\0')
 * @param what_len  The length of what
 * @return The trimmed string, to be free memory use free()
 */
char *str_trim_alloc(char *str, int str_len, char *what, int what_len)
{
    int trimmed = 0;
    int len = 0;

    _s_trim(str, str_len, what, what_len, 3, &trimmed, &len);

    char *s = strndup(str + trimmed, len);
    return s;
}

#ifdef _TEST
#include <stdio.h>
#include <stdlib.h>
int main(int argc, char **argv)
{
    char *content = argv[1];
    char *sed = argv[2];

    // 第一种使用方法
    char str[1024] = {0};
    str_trim(content, strlen(content), sed, strlen(sed), str, sizeof(str));
    printf("(%s)\n", str);

    // 第二种使用方法
    // what使用默认的NULL，则为 (” \t\n\r\v\0“) 6种
    // 最后记得释放内存
    char *ss = str_trim_alloc(content, strlen(content), NULL, 0);
    printf("(%s)\n", ss);
    free(ss);

    return 0;
}
#endif