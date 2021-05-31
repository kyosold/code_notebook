/*
    把一个文件内容做成邮件体，并生成 eml 文件

    Build:
        gcc -g s_generate_eml.c base64.c

    Usage:
        ./a.out -f'songjian@luobo.sina.net' -t'kyosold@qq.com' -s'上周账单提醒' -i./test.log -o test.eml
*/

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <getopt.h>
#include <time.h>
#include "base64.h"

#define HOUR_MIN 60
#define MIN_SEC 60
#define DAY_MIN (24 * HOUR_MIN)

/**
 * @brief 产生 min ~ max 之间的随机数，包括 min 和 max
 */
#define randnum(min, max) ((rand() % (int)(((max) + 1))) + (min))

void usage(char *prog)
{
    printf("Usage:\n");
    printf("  %s -f[from] -t[to] -s[subject] -i[in file] -o[out file]\n\n", prog);
    printf("参数说明:\n");
    printf("  -f  发件人邮件地址\n");
    printf("  -t  收件人邮件地址,多个收件地址中间用,(逗号)分割\n");
    printf("  -s  邮件主题内容\n");
    printf("  -i  需要转换的文件，此文件内容会转成邮件内容\n");
    printf("  -o  最终生成的eml文件\n");
}

void generate_random_boundary_string(char *res, size_t res_size)
{
    // 64 characters that can be _safely_ used in a boundary string
    static const char bchars[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ-+";
    unsigned int bchars_len = 64;

    srand(time(NULL));
    int count = randnum(0, 11) + 30;
    printf("count: %d\n", count);

    res[0] = '=';
    res[1] = '_';
    int i = 0;
    for (i = 2; i < count; i++)
    {
        res[i] = bchars[randnum(0, bchars_len)];
    }
}

void fmt_date_0800(char *res, size_t res_size)
{
    time_t now;
    struct tm *lt;
    struct tm gmt;
    int gmtoff;

    char vp[100] = {0};
    char *p = vp;

    time(&now);
    gmt = *gmtime(&now);
    lt = localtime(&now);

    gmtoff = (lt->tm_hour - gmt.tm_hour) * HOUR_MIN + lt->tm_min - gmt.tm_min;
    if (lt->tm_year < gmt.tm_year)
        gmtoff -= DAY_MIN;
    else if (lt->tm_year > gmt.tm_year)
        gmtoff += DAY_MIN;
    else if (lt->tm_yday < gmt.tm_yday)
        gmtoff -= DAY_MIN;
    else if (lt->tm_yday > gmt.tm_yday)
        gmtoff += DAY_MIN;

    if (lt->tm_sec <= gmt.tm_sec - MIN_SEC)
        gmtoff -= 1;
    else if (lt->tm_sec >= gmt.tm_sec + MIN_SEC)
        gmtoff += 1;

#ifdef MISSING_STRFTIME_E
#define STRFTIME_FMT "%a, %d %b %Y %H:%M:%S "
#else
#define STRFTIME_FMT "%a, %e %b %Y %H:%M:%S "
#endif

    int len = strftime(p, 100, STRFTIME_FMT, lt);
    p += len;

    len = sprintf(p, "%+03d%02d", (int)(gmtoff / HOUR_MIN), (int)(abs(gmtoff) % HOUR_MIN));
    p += strlen(p);

    len = strftime(p, 100, " (%Z)", lt);

    snprintf(res, res_size, "%s", vp);
}

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        usage(argv[0]);
        return 1;
    }

    char *ibuf = NULL;
    char *obuf_b64 = NULL;
    char *from = NULL;
    char *to = NULL;
    char *subject = NULL;
    char *subject_b64 = NULL;
    FILE *ifp = NULL;
    FILE *ofp = NULL;

    char ifile[1024] = {0};
    char ofile[1024] = {0};

    char obuf[4096] = {0};
    size_t obuf_size = 4096;

    int i, ch;
    const char *args = "f:t:s:i:o:h";
    while ((ch = getopt(argc, argv, args)) != -1)
    {
        switch (ch)
        {
        case 'f':
            from = strdup(optarg);
            break;
        case 't':
            to = strdup(optarg);
            break;
        case 's':
            subject = strdup(optarg);
            break;
        case 'i':
            snprintf(ifile, sizeof(ifile), "%s", optarg);
            break;
        case 'o':
            snprintf(ofile, sizeof(ofile), "%s", optarg);
            break;

        case 'h':
        default:
            usage(argv[0]);
            exit(1);
        }
    }

    if (access(ifile, F_OK) != 0)
    {
        printf("Fail: %s\n", strerror(errno));
        goto SFAIL;
    }

    struct stat fst;
    if (stat(ifile, &fst) == -1)
    {
        printf("Fail: %s\n", strerror(errno));
        goto SFAIL;
    }

    ibuf = (char *)calloc(1, fst.st_size + 1);
    if (ibuf == NULL)
    {
        printf("Fail: out of memory\n");
        goto SFAIL;
    }

    ifp = fopen(ifile, "r");
    if (ifp == NULL)
    {
        printf("Fail: %s\n", strerror(errno));
        goto SFAIL;
    }

    fread(ibuf, 1, fst.st_size, ifp);

    fclose(ifp);
    ifp = NULL;

    int obuf_b64_len = 0;
    obuf_b64 = sbase64_encode_alloc(ibuf, strlen(ibuf), &obuf_b64_len);
    if (obuf_b64 == NULL)
    {
        printf("sbase64_encode_alloc fail\n");
        goto SFAIL;
    }

    free(ibuf);
    ibuf = NULL;

    int subject_b64_len = 0;
    subject_b64 = sbase64_encode_alloc(subject, strlen(subject), &subject_b64_len);

    if (access(ofile, F_OK) == 0)
    {
        unlink(ofile);
    }

    ofp = fopen(ofile, "a+");
    if (ofp == NULL)
    {
        printf("Fail: %s\n", strerror(errno));
        goto SFAIL;
    }

    char date_s[100] = {0};
    fmt_date_0800(date_s, sizeof(date_s));
    char h_d[] = "Date: ";
    fwrite(h_d, 1, strlen(h_d), ofp);
    fwrite(date_s, 1, strlen(date_s), ofp);
    fwrite("\r\n", 1, 2, ofp);

    if (subject_b64)
    {
        char h_s[] = "Subject: =?UTF-8?B?";
        fwrite(h_s, 1, strlen(h_s), ofp);
        fwrite(subject_b64, 1, subject_b64_len, ofp);
        fwrite("?=\r\n", 1, 4, ofp);
    }
    if (from)
    {
        char h_f[] = "From: <";
        fwrite(h_f, 1, strlen(h_f), ofp);
        fwrite(from, 1, strlen(from), ofp);
        fwrite(">\r\n", 1, 3, ofp);
    }
    if (to)
    {
        char *pto = to;
        char *tok = NULL;
        while (pto != NULL)
        {
            tok = (char *)memchr(pto, ',', strlen(pto));
            if (tok != NULL)
            {
                *tok = 0;

                char h_t[] = "To: <";
                fwrite(h_t, 1, strlen(h_t), ofp);
                fwrite(pto, 1, strlen(pto), ofp);
                fwrite(">\r\n", 1, 3, ofp);

                *tok = ',';
                pto = tok + 1;
            }
            else
            {
                char h_t[] = "To: <";
                fwrite(h_t, 1, strlen(h_t), ofp);
                fwrite(pto, 1, strlen(pto), ofp);
                fwrite(">\r\n", 1, 3, ofp);

                break;
            }
        }

        // char h_t[] = "To: <";
        // fwrite(h_t, 1, strlen(h_t), ofp);
        // fwrite(to, 1, strlen(to), ofp);
        // fwrite(">\r\n", 1, 3, ofp);
    }
    char boundary[2 + 48 + 1] = {0};
    generate_random_boundary_string(boundary, sizeof(boundary));
    // printf("boundary: %s\n", boundary);

    char content_type[1024] = {0};
    snprintf(content_type, sizeof(content_type), "Content-Type: multipart/alternative;\r\n\tboundary=\"%s\"", boundary);

    fwrite(content_type, 1, strlen(content_type), ofp);
    fwrite("\r\n\r\n--", 1, 6, ofp);
    fwrite(boundary, 1, strlen(boundary), ofp);
    fwrite("\r\n", 1, 2, ofp);

    char sub_ct[] = "Content-Type: text/plain; charset=\"utf-8\"\r\nContent-Transfer-Encoding: base64\r\n\r\n";
    fwrite(sub_ct, 1, strlen(sub_ct), ofp);

    i = 0;
    char *b64_buf = obuf_b64;

    while (*b64_buf != '\0')
    {
        if (i == 76)
        {
            strncat(obuf, "\r\n", 2);
            fwrite(obuf, 1, i + 2, ofp);
            i = 0;
            memset(obuf, 0, obuf_size);
        }

        *(obuf + i) = *b64_buf;
        b64_buf++;
        i++;
    }

    if (i > 0)
    {
        fwrite(obuf, 1, i, ofp);
    }

    fwrite("\r\n\r\n--", 1, 6, ofp);
    fwrite(boundary, 1, strlen(boundary), ofp);
    fwrite("--", 1, 2, ofp);

    fclose(ofp);
    ofp = NULL;

    free(obuf_b64);
    obuf_b64 = NULL;

    free(from);
    from = NULL;

    free(to);
    to = NULL;

    free(subject);
    subject = NULL;

    if (subject_b64)
        free(subject_b64);

    return 0;

SFAIL:
    if (from)
        free(from);
    if (to)
        free(to);
    if (subject)
        free(subject);
    if (subject_b64)
        free(subject_b64);

    if (ibuf)
        free(ibuf);
    if (obuf_b64)
        free(obuf_b64);

    if (ifp)
        fclose(ifp);
    if (ofp)
        fclose(ofp);

    return 1;
}