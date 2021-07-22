
#include "syslog.h"
#include "slog.h"

static int slog_level = SLOG_INFO;
static char slog_uid[128] = {0};
static char slog_ident[1024] = {0};
static int slog_facility = LOG_USER;
static int slog_opt = LOG_PID | LOG_NDELAY;

static char *slog_priority[] = {
    "EMERG",
    "ALERT",
    "CRIT",
    "ERROR",
    "WARNING",
    "NOTICE",
    "INFO",
    "DEBUG"};

void _slog_write(int level, const char *file, int line,
                 const char *fun, const char *fmt, ...)
{
    // printf("level:%d slog_level:%d\n", level, slog_level);
    if (level > slog_level)
        return;

    int n;
    char new_fmt[4096];
    const char *head_fmt = "[%s] %s:%d %s uid:%s %s";
    n = sprintf(new_fmt, head_fmt, slog_priority[level], file, line, fun, slog_uid, fmt);

    va_list ap;
    va_start(ap, fmt);
    openlog(slog_ident, slog_opt, slog_facility);
    vsyslog(level, new_fmt, ap);
    closelog();
    va_end(ap);
}

/**
 * @brief 日志打开
 * @param ident 程序名称
 * @param facility 日志目标(LOG_MAIL|LOG_USER)
 * @param level 日志等级(SLOG_DEBUG, SLOG_INFO ...)
 * @param uid 日志UID，用于一次会话的标记
 * @return 
 */
void slog_open(const char *ident, int facility, int level, char *uid)
{
    snprintf(slog_ident, sizeof(slog_ident), "%s", ident);
    slog_facility = facility;
    slog_level = level;
    snprintf(slog_uid, sizeof(slog_uid), "%s", uid);
}

/**
 * @brief 设置日志等级
 * @param level 日志等级(SLOG_DEBUG, SLOG_INFO...)
 * @return
 */
void slog_set_level(int level)
{
    slog_level = level;
}

/**
 * @brief 设置日志UID
 * @param uid 日志UID，用于一次会话的标记
 * @return
 */
void slog_set_uid(char *uid)
{
    snprintf(slog_uid, sizeof(slog_uid), "%s", uid);
}

#ifdef _TEST
// gcc -g slog.c -D_TEST
#include <stdio.h>
#include "slog.h"
int main(int argc, char **argv)
{
    slog_open("test", LOG_MAIL, SLOG_DEBUG, "99999");
    slog_debug("Name: %s", "kyosold");

    slog_set_uid("xxxxxxxx");
    slog_set_level(SLOG_INFO);
    slog_debug("Name: %s", "111111111");
    slog_info("Name: %s", "22222222");
    slog_error("NAME: %s", "33333333");

    return 1;
}
#endif
