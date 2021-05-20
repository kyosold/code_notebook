#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <pcre.h>
#include "spcre.h"

#define OVECCOUNT 30 // should be a multiple of 3

/**
 * @brief 匹配正则，并获取匹配到的字符串
 * @param str       被匹配的字符串
 * @param str_len   被匹配的字符串的长度
 * @param pattern   正则表达式
 * @param matched_list  命中后的字符串，有命中才有分配内存，需要 s_pcre_matched_free() 释放
 * @param matched_list_num  命中的字符串数量
 * @param err_str   错误描述
 * @param err_str_size 错误描述的空间
 * @return 0:命中，1:未命中, 2:错误
 * 
 * 注意: 
 *  如果命中，则 matched_list 将分配堆上内存，用完后需要手动 s_pcre_matched_free 它；
 *  如果未命中，此值为 NULL, 不需要 s_pcre_matched_free
 * 
 *  如果你仅需要知道是否命中，而不关心命中的哪个字符串，则仅需要调用 s_pcre_match() 函数
 */
int s_pcre_match_alloc(const char *str, size_t str_len, const char *pattern, char ***matched_list, size_t *matched_list_num, char *err_str, size_t err_str_size)
{
    const char *error;
    int err_offset;
    int ovector[OVECCOUNT];
    int rc;
    int ret = SPCRE_ERR;
    pcre *re = NULL;
    pcre_extra *re_extra = NULL;

    // 将一个正则表达式编译成一个内部的pcre结构
    re = pcre_compile(pattern,     // 正则表达式
                      0,           // 为0， 或者其他参数选项
                      &error,      // 返回出错信息
                      &err_offset, // 返回出错位置
                      NULL         // 用来指定字符表，一般情况用NULL
    );
    if (re == NULL)
    {
        snprintf(err_str, err_str_size, "pcre_compile pattern(%s) failed at offset %d: %s", pattern, err_offset, error);
        return SPCRE_ERR;
    }

    // 对编译的模式进行学习，提取可以加速匹配过程的信息
    re_extra = pcre_study(re, 0, &error);

    rc = pcre_exec(re,       // 编译好的模式
                   re_extra, // 指向一个pcre_extra结构体，可以为NULL
                   str,      // 需要匹配的字符串
                   str_len,  // 匹配的字符串长度
                   0,        // 匹配的开始位置
                   0,        // 选项位
                   ovector,  // 返回匹配位置偏移量的数组
                   OVECCOUNT // 数组大小, 一般应为3的整数倍
    );
    if (rc < 0)
    {
        // 没有匹配，返回错误信息
        if (rc == PCRE_ERROR_NOMATCH)
            ret = SPCRE_NOMATCH;
        else
        {
            snprintf(err_str, err_str_size, "pcre pattern:%s error %d: %s", pattern, rc, error);
            ret = SPCRE_ERR;
        }
    }
    else
    {
        // 命中匹配
        ret = SPCRE_MATCHED;

        if (1)
        {
            *matched_list_num = rc;
            char **ml = (char **)calloc(rc + 1, sizeof(char *));
            if (ml != NULL)
            {
                int i = 0;
                const char **m_list = NULL;
                pcre_get_substring_list(str, ovector, rc, &m_list);
                const char **p = m_list;
                while (*p != NULL)
                {
                    if (i >= rc)
                        break;

                    ml[i] = strdup(*p);

                    // 目前不知道命中多少
                    printf("[MATCHED]: %s\n", *p);
                    p++;
                    i++;
                }
                pcre_free_substring_list(m_list);
            }
            *matched_list = ml;
        }
    }

    pcre_free(re_extra);
    pcre_free(re);

    return ret;
}

/**
 * @brief 释放命中的数组列表内存
 * @param matched_list 命中的数组
 * @param matched_list_num 数组的成员个数
 * @return none
 */
void s_pcre_matched_free(char **matched_list, size_t matched_list_num)
{
    if (matched_list == NULL || matched_list_num <= 0)
        return;

    int i = 0;
    for (i = 0; i < matched_list_num; i++)
    {
        if (matched_list[i] != NULL)
        {
            free(matched_list[i]);
            matched_list[i] = NULL;
        }
    }

    free(matched_list);
    matched_list = NULL;

    return;
}

/**
 * @brief 匹配正则
 * @param str       被匹配的字符串
 * @param str_len   被匹配的字符串的长度
 * @param pattern   正则表达式
 * @param err_str   错误描述
 * @param err_str_size 错误描述的空间
 * @return 0:命中，1:未命中, 2:错误
 */
int s_pcre_match(const char *str, size_t str_len, const char *pattern, char *err_str, size_t err_str_size)
{
    const char *error;
    int err_offset;
    int ovector[OVECCOUNT];
    int rc;
    int ret = SPCRE_ERR;
    pcre *re = NULL;
    pcre_extra *re_extra = NULL;

    // 将一个正则表达式编译成一个内部的pcre结构
    re = pcre_compile(pattern,     // 正则表达式
                      0,           // 为0， 或者其他参数选项
                      &error,      // 返回出错信息
                      &err_offset, // 返回出错位置
                      NULL         // 用来指定字符表，一般情况用NULL
    );
    if (re == NULL)
    {
        snprintf(err_str, err_str_size, "pcre_compile pattern(%s) failed at offset %d: %s", pattern, err_offset, error);
        return SPCRE_ERR;
    }

    // 对编译的模式进行学习，提取可以加速匹配过程的信息
    re_extra = pcre_study(re, 0, &error);

    rc = pcre_exec(re,       // 编译好的模式
                   re_extra, // 指向一个pcre_extra结构体，可以为NULL
                   str,      // 需要匹配的字符串
                   str_len,  // 匹配的字符串长度
                   0,        // 匹配的开始位置
                   0,        // 选项位
                   ovector,  // 返回匹配位置偏移量的数组
                   OVECCOUNT // 数组大小, 一般应为3的整数倍
    );
    if (rc < 0)
    {
        // 没有匹配，返回错误信息
        if (rc == PCRE_ERROR_NOMATCH)
            ret = SPCRE_NOMATCH;
        else
        {
            snprintf(err_str, err_str_size, "pcre pattern:%s error %d: %s", pattern, rc, error);
            ret = SPCRE_ERR;
        }
    }
    else
    {
        // 命中匹配
        ret = SPCRE_MATCHED;
    }

    pcre_free(re_extra);
    pcre_free(re);

    return ret;
}

#ifdef _TEST
// gcc -g spcre.c -lpcre
// ./a.out 'abc((.*)cf(exec))test' '11111abcword1cfexectest11111'
void main(int argc, char **argv)
{
    if (argc != 3)
    {
        printf("Usage: %s [pattern] [str]\n", argv[0]);
        return;
    }

    char *pattern = argv[1];
    char *str = argv[2];

    int is_matched = SPCRE_NOMATCH;
    char **ml = NULL;
    size_t ml_num;
    char err[1024] = {0};

    is_matched = s_pcre_match_alloc(str, strlen(str),
                                    pattern,
                                    &ml, &ml_num,
                                    err, sizeof(err));

    printf("ret: %d\n", is_matched);

    if (is_matched == SPCRE_MATCHED)
    {
        int i = 0;
        for (i = 0; i < ml_num; i++)
        {
            printf("  [%d]: %s\n", i, ml[i]);
            // free(ml[i]);
        }

        // free(ml);
        s_pcre_matched_free(ml, ml_num);
    }
}
#endif