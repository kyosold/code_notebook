#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int kscal(long long size, char *str, size_t str_size)
{
    long long GB = 1024 * 1024 * 1024;
    long long MB = 1024 * 1024;
    long long KB = 1024;
    int ret = 0;

    if (size > GB)
    {
        ret = snprintf(str, str_size, "%.2f GB", (float)size / GB);
    }
    else if (size > MB)
    {
        ret = snprintf(str, str_size, "%.2f MB", (float)size / MB);
    }
    else if (size > KB)
    {
        ret = snprintf(str, str_size, "%.2f KB", (float)size / KB);
    }
    else
    {
        ret = snprintf(str, str_size, "%.2f B", (float)size);
    }

    return ret;
}

int conv_month_2_string(int month, char *str, size_t str_size)
{
    switch (month)
    {
    case 1:
        snprintf(str, str_size, "Jan");
        break;
    case 2:
        snprintf(str, str_size, "Feb");
        break;
    case 3:
        snprintf(str, str_size, "Mar");
        break;
    case 4:
        snprintf(str, str_size, "Apr");
        break;
    case 5:
        snprintf(str, str_size, "May");
        break;
    case 6:
        snprintf(str, str_size, "Jun");
        break;
    case 7:
        snprintf(str, str_size, "Jul");
        break;
    case 8:
        snprintf(str, str_size, "Aug");
        break;
    case 9:
        snprintf(str, str_size, "Sep");
        break;
    case 10:
        snprintf(str, str_size, "Oct");
        break;
    case 11:
        snprintf(str, str_size, "Nov");
        break;
    case 12:
        snprintf(str, str_size, "Dec");
        break;
    default:
        snprintf(str, str_size, "Unk");
        break;
    }
    return 3;
}

int conv_month_2_int(char *str)
{
    if (strncasecmp(str, "jan", 3) == 0)
    {
        return 1;
    }
    else if (strncasecmp(str, "feb", 3) == 0)
    {
        return 2;
    }
    else if (strncasecmp(str, "mar", 3) == 0)
    {
        return 3;
    }
    else if (strncasecmp(str, "apr", 3) == 0)
    {
        return 4;
    }
    else if (strncasecmp(str, "may", 3) == 0)
    {
        return 5;
    }
    else if (strncasecmp(str, "jun", 3) == 0)
    {
        return 6;
    }
    else if (strncasecmp(str, "jul", 3) == 0)
    {
        return 7;
    }
    else if (strncasecmp(str, "aug", 3) == 0)
    {
        return 8;
    }
    else if (strncasecmp(str, "sep", 3) == 0)
    {
        return 9;
    }
    else if (strncasecmp(str, "oct", 3) == 0)
    {
        return 10;
    }
    else if (strncasecmp(str, "nov", 3) == 0)
    {
        return 11;
    }
    else if (strncasecmp(str, "dec", 3) == 0)
    {
        return 12;
    }
    else
    {
        return -1;
    }
}