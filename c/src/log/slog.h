#ifndef _SLOG_H
#define _SLOG_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <libgen.h>
#include <syslog.h>

#define SLOG_DEBUG 7
#define SLOG_INFO 6
#define SLOG_NOTICE 5
#define SLOG_WARNING 4
#define SLOG_ERROR 3
#define SLOG_CRIT 2
#define SLOG_ALERT 1
#define SLOG_EMERG 0

void slog_open(const char *ident, int facility, int level, char *uid);
void slog_set_level(int level);
void slog_set_uid(char *uid);

void _slog_write(int level, const char *file, int line, const char *fun, const char *fmt, ...);

#define slog_debug(fmt, args...) \
    _slog_write(SLOG_DEBUG, basename(__FILE__), __LINE__, __FUNCTION__, fmt, ##args)
#define slog_info(fmt, args...) \
    _slog_write(SLOG_INFO, basename(__FILE__), __LINE__, __FUNCTION__, fmt, ##args)
#define slog_notice(fmt, args...) \
    _slog_write(SLOG_NOTICE, basename(__FILE__), __LINE__, __FUNCTION__, fmt, ##args)
#define slog_warning(fmt, args...) \
    _slog_write(SLOG_WARNING, basename(__FILE__), __LINE__, __FUNCTION__, fmt, ##args)
#define slog_error(fmt, args...) \
    _slog_write(SLOG_ERROR, basename(__FILE__), __LINE__, __FUNCTION__, fmt, ##args)
#define slog_crit(fmt, args...) \
    _slog_write(SLOG_CRIT, basename(__FILE__), __LINE__, __FUNCTION__, fmt, ##args)
#define slog_alert(fmt, args...) \
    _slog_write(SLOG_ALERT, basename(__FILE__), __LINE__, __FUNCTION__, fmt, ##args)
#define slog_emerg(fmt, args...) \
    _slog_write(SLOG_EMERG, basename(__FILE__), __LINE__, __FUNCTION__, fmt, ##args)

#endif
