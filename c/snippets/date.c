#include <time.h>

// 获取当前本地时间
int get_today_date(char *str, size_t str_size)
{
    time_t now;
    time(&now);

    struct tm *tm_now = localtime(&now);
    snprintf(str, str_size, "%04d-%02d-%02d %02d:%02d:%02d",
             tm_now->tm_year + 1900, tm_now->tm_mon + 1, tm_now->tm_mday,
             tm_now->tm_hour, tm_now->tm_min, tm_now->tm_sec);

    return strlen(str);
}

// 生成UTC时间: Thu, 13 May 2021 08:01:34 UTC
void fmt_date_utc(int expire, char *str, size_t str_size)
{
    time_t now = time(0) + expire;
    struct tm *gmtm = gmtime(&now);
    strftime(str, str_size, "%a, %d %b %Y %H:%M:%S UTC", gmtm);
}

// 生成GMT时间: Thu, 13 May 2021 08:01:34 GMT
void fmt_date_gmt(int expire, char *str, size_t str_size)
{
    char szTemp[30] = {0};
    time_t now = time(0) + expire;
    struct tm *gmtm = gmtime(&now);
    strftime(str, str_size, "%a, %d %b %Y %H:%M:%S GMT", gmtm);
}

// 生成0800时间: Tue, 25 May 2021 10:46:06 +0800 (CST)
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