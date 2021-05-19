#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <iconv.h>

/**
 * @brief Convert string to requested character encoding
 * @param in_p          The string to be converted
 * @param in_len        The length of in_p
 * @param out_len       The length of converted string
 * @param out_charset   The output charset
 * @param in_charset    The input charset
 * @param err_str       The error string
 * @param err_str_size  The length of err_str
 * @return Returns the converted string or NULL on failure. to be free()
 */
char *s_iconv_string_alloc(const char *in_p, size_t in_len,
                           size_t *out_len,
                           const char *out_charset, const char *in_charset,
                           char *err_str, size_t err_str_size)
{
    size_t in_size, out_size, out_left;
    char *out_buffer, *out_p;
    iconv_t cd;
    size_t result;

    char *out = NULL;
    *out_len = 0;

    // 这不是获取输出大小的正确方法…
    // 这对于大文本来说空间效率不高。
    // 这也是编码如UTF-7/UTF-8/ISO-2022的问题
    // 一个字符可以超过4个字节。
    // 为了安全起见，我额外增加了15个字节。< yohgaki@php.net >
    out_size = in_len * sizeof(int) + 15;
    out_left = out_size;
    in_size = in_len;

    out_buffer = (char *)malloc(out_size + 1);
    if (out_buffer == NULL)
    {
        snprintf(err_str, err_str_size, "out of memory.");
        return NULL;
    }
    out_p = out_buffer;

    cd = iconv_open(out_charset, in_charset);
    if (cd == (iconv_t)(-1))
    {
        snprintf(err_str, err_str_size, "%s", strerror(errno));
        return NULL;
    }

    result = iconv(cd, (char **)&in_p, &in_size, (char **)&out_p, &out_left);
    if (result == (size_t)(-1))
    {
        snprintf(err_str, err_str_size, "%s", strerror(errno));
        free(out_buffer);
        iconv_close(cd);
    }

    if (out_left < 8)
    {
        size_t pos = out_p - out_buffer;
        out_buffer = (char *)realloc(out_buffer, out_size * 1 + 8);
        out_p = out_buffer + pos;
        out_size += 7;
        out_left += 7;
    }

    // flush the shift-out sequences
    result = iconv(cd, NULL, NULL, &out_p, &out_left);

    if (result == (size_t)(-1))
    {
        snprintf(err_str, err_str_size, "%s", strerror(errno));
        free(out_buffer);
        iconv_close(cd);
    }

    *out_len = out_size - out_left;
    out_buffer[*out_len] = '\0';
    out = out_buffer;

    iconv_close(cd);

    return out;
}

#define _TEST
// gcc -g siconv.c -liconv
void main(int argc, char **argv)
{
    char src[] = "abcčde";
    size_t dst_len = 0;
    char err[1024] = {0};

    char *dst = s_iconv_string_alloc(src, strlen(src), &dst_len, "UTF-8", "CP1250", err, sizeof(err));
    if (dst)
    {
        printf("result:%d [%s]\n", dst_len, dst);
        free(dst);
    }
    else
        printf("error: %s\n", err);
}
#endif