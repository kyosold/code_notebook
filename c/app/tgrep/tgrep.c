#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "utils.h"
#include "version.h"

unsigned char eolbyte = '\n';

char *buffer;                 // Base of buffer
size_t bufalloc;              // Allocated buffer size, counting slop.
#define INITIAL_BUFSIZE 32768 // Initial buffer size, not counting slop.
size_t pagesize;              // alignment of memory pages
/*  Return VAL aligned to the next multiple of ALIGNMENT.  VAL can be
    an integer or a pointer.  Both args must be free of side effects.  */
#define ALIGN_TO(val, alignment)      \
    ((size_t)(val) % (alignment) == 0 \
         ? (val)                      \
         : (val) + ((alignment) - (size_t)(val) % (alignment)))
// int reset(int fd, char const *file)
int init_memory()
{
    if (!pagesize)
    {
        pagesize = getpagesize();
        if (pagesize == 0 || 2 * pagesize + 1 <= pagesize)
        {
            return 0;
        }
        bufalloc = ALIGN_TO(INITIAL_BUFSIZE, pagesize) + pagesize + 1;
        buffer = (char *)malloc(bufalloc);
        // printf("buffer alloc: %d\n", bufalloc);
    }

    return bufalloc;
}

typedef struct datetime_line_t
{
    int month;
    int day;
    int hour;
    int minute;
    int second;
} datetime_line_st;

typedef struct ret_pos_t
{
    long pos;
    struct datetime_line_t line_dt;
} ret_post_st;

typedef struct matcher_t
{
    int start;
    int end;
    int ignore_case;
    int verbose;
    char pattern[2048];
    char file[1024];
    int show_filename;
} matcher_st;

int do_search(struct matcher_t *matcher);
int get_pos(char *file, int match_hour, int v, int flag, struct ret_pos_t *ret_pos);
int get_datatime_with_line(char *line, struct datetime_line_t *datetime_line);
void cp_datetime_line(struct datetime_line_t *dst, struct datetime_line_t *src);
void grep_str(struct matcher_t *matcher, char *buf, char *substr);

void usage(char *prog)
{
    printf("----------------------------------------------\n");
    printf("Version: %s\n\n", VERSION);
    printf("Usage:\n");
    printf("  %s -v -s15 -e17 [pattern] [file]\n", prog);
    printf("\n");
    printf("Options:\n");
    printf("  -s: 从指定的小时开始查找\n");
    printf("  -e: 到指定的小时结束查找, 不写默认到文件结尾\n");
    printf("  -i: 不区分大小写\n");
    printf("  -v: 显示详细信息\n");
    printf("  -S: 显示文件名\n");
    printf("\n");
    printf("Others:\n");
    printf("  1. 如果文件是gzip，先解压再查询，如:\n");
    printf("    a. gzip -dvc abc.0.gz > abc.0\n");
    printf("    b. tgrep -st=8 -et=10 'pattern' abc.0\n");
    printf("----------------------------------------------\n");
    printf("\n");
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        usage(argv[0]);
        return 1;
    }

    // Matcher init
    struct matcher_t matcher;
    matcher.start = -1;
    matcher.end = -1;
    matcher.ignore_case = 0;
    matcher.verbose = 0;
    matcher.show_filename = 0;

    // command
    int i, ch;
    const char *args = "s:e:SivVh";
    while ((ch = getopt(argc, argv, args)) != -1)
    {
        switch (ch)
        {
        case 's':
            matcher.start = atoi(optarg);
            break;
        case 'e':
            matcher.end = atoi(optarg);
            break;
        case 'i':
            matcher.ignore_case = 1;
            break;
        case 'v':
            matcher.verbose = 1;
            break;
        case 'S':
            matcher.show_filename = 1;
            break;
        case 'V':
            printf("Version: %s\n", VERSION);
            exit(1);
        case 'h':
        default:
            usage(argv[0]);
            exit(1);
        }
    }
    if ((argc - optind) != 2)
    {
        usage(argv[0]);
        exit(1);
    }

    snprintf(matcher.pattern, sizeof(matcher.pattern), "%s", argv[optind++]);
    snprintf(matcher.file, sizeof(matcher.file), "%s", argv[optind++]);

    printf("**************************\n");
    printf("Start Hour: %d\n", matcher.start);
    printf("End Hour: %d\n", matcher.end);
    printf("Ignore Case: %d\n", matcher.ignore_case);
    printf("Verbose: %d\n", matcher.verbose);
    printf("Pattern: %s\n", matcher.pattern);
    printf("File: %s\n", matcher.file);
    printf("**************************\n");

    if (matcher.start > 0 && matcher.end > 0 && matcher.start > matcher.end)
    {
        printf("Error: -s can't more than -e value\n");
        exit(1);
    }

    char *ext = strrchr(matcher.file, '.');
    if (ext && (strcasecmp(ext, ".gz") == 0 ||
                strcasecmp(ext, ".gzip") == 0))
    {
        printf("Error: file(%s) is gzip file\n", matcher.file);
        printf("  Run: 'gzip -dvc %s > tmp.log'\n", matcher.file);
        exit(1);
    }

    do_search(&matcher);

    return 0;
}

int do_search(struct matcher_t *matcher)
{
    int ret = 0;
    int flag = 0; // 0:start 1:end

    struct ret_pos_t ret_pos_start;
    struct ret_pos_t ret_pos_end;

    // get start pos
    if (matcher->verbose > 0)
        printf("START >>>>>>>\n");
    ret = get_pos(matcher->file,
                  matcher->start,
                  matcher->verbose,
                  flag,
                  &ret_pos_start);

    // get end pos
    if (matcher->verbose > 0)
        printf("\nEND >>>>>>>\n");
    flag = 1;
    ret = get_pos(matcher->file,
                  matcher->end,
                  matcher->verbose,
                  flag,
                  &ret_pos_end);

    if (matcher->verbose > 0)
    {
        char scal_str[128] = {0};
        kscal((ret_pos_end.pos - ret_pos_start.pos), scal_str, sizeof(scal_str));

        printf("\n--------------------------\n");
        printf("Final: Start:[%ld][%d:%d] --- End:[%ld][%d:%d] Need Scan[%s]\n",
               ret_pos_start.pos, ret_pos_start.line_dt.hour, ret_pos_start.line_dt.minute,
               ret_pos_end.pos, ret_pos_end.line_dt.hour, ret_pos_end.line_dt.minute, scal_str);
        printf("--------------------------\n\n");
    }

    int fd = open(matcher->file, O_RDONLY);
    if (fd == -1)
    {
        printf("Error: open file(%s) fail: %s\n", matcher->file, strerror(errno));
        return 1;
    }

    if (!init_memory())
        goto ERR_EXIT;
    lseek(fd, ret_pos_start.pos, SEEK_SET);

    char *line_buf = NULL;
    char *line_end = NULL;
    int len = 0;
    long total_bytes = ret_pos_end.pos - ret_pos_start.pos;

    while (total_bytes > 0)
    {
        if (total_bytes < bufalloc)
            bufalloc = total_bytes;

        ssize_t n = read(fd, buffer + len, bufalloc);
        if (n < 0)
        {
            printf("Error: read file(%s) fail: %s\n", matcher->file, strerror(errno));
            close(fd);
            return 1;
        }
        else if (n == 0)
        {
            printf("Error: read eof\n");
            close(fd);
            return 0;
        }

        total_bytes -= n;

        if (len == 0)
        {
            // bufbeg 是第一行开始的指针
            line_buf = memchr(buffer, eolbyte, n);
            if (line_buf)
            {
                line_buf++;
                int m = line_buf - buffer;
                n -= m;
            }
            else
            {
                printf("Error: buffer alloca is small, is not include \\n at buffer, (you should inc buffer alloca size)\n");
                goto ERR_EXIT;
            }
        }
        else
        {
            line_buf = buffer;
            n += len;
        }

        if (line_buf)
        {
            for (;;)
            {
                line_end = memchr(line_buf, eolbyte, n);
                if (line_end)
                    *line_end = 0;
                else
                {
                    // 保留 line_buf，不是完整的一行
                    memmove(buffer, line_buf, n);
                    len = n;
                    break;
                }

                grep_str(matcher, line_buf, matcher->pattern);
                n -= (line_end - line_buf + 1);
                line_buf = line_end + 1;
            }
        }
    }

ERR_EXIT:
    close(fd);
    return 1;

OK_EXIT:
    close(fd);
    return 0;
}

/**
 * @brief Get the pos object
 * 
 * @param file 
 * @param match_hour 
 * @param v 
 * @param flag          0:查询start, 1:查询end
 * @param ret_pos 
 * @return int 
 */
int get_pos(char *file, int match_hour, int v, int flag, struct ret_pos_t *ret_pos)
{
    char buf[4096] = {0};
    int matched_previous = 0;
    int matched_last = 0;
    int change_times = 0;

    struct stat st;
    if (stat(file, &st) == -1)
    {
        printf("Error: %s\n", strerror(errno));
        return 1;
    }

    if (match_hour == -1)
    {
        if (flag == 0)
            ret_pos->pos = 0;
        else
            ret_pos->pos = st.st_size;
        ret_pos->line_dt.hour = -1;
        ret_pos->line_dt.minute = -1;
        return 1;
    }

    struct datetime_line_t datetime_line;

    unsigned long s_pos = 0;          // start pos
    unsigned long e_pos = st.st_size; // end pos
    unsigned long c_pos = e_pos / 2;  // current pos

    FILE *fp = fopen(file, "r");
    if (fp == NULL)
    {
        printf("Error: %s\n", strerror(errno));
        return 1;
    }

    fseek(fp, c_pos, SEEK_SET);
    fgets(buf, sizeof(buf), fp);

    while (fgets(buf, sizeof(buf), fp) != NULL)
    {
        // printf("%s\n", buf);
        get_datatime_with_line(buf, &datetime_line);
        ret_pos->pos = c_pos;

        if (flag == 0)
        {
            // 查 start

            if (match_hour <= datetime_line.hour)
            {
                // 向 前 查

                if (v > 0)
                    printf("  Pre: Match[%d] Line[%d:%d] pos[s:%ld c:%ld e:%ld]\n", match_hour, datetime_line.hour, datetime_line.minute, s_pos, c_pos, e_pos);

                matched_previous = 1;
                if (matched_last)
                {
                    change_times++;
                    matched_last = 0;
                }

                if (change_times > 1)
                {
                    ret_pos->pos = s_pos;
                    fseek(fp, ret_pos->pos, SEEK_SET);
                    fgets(buf, sizeof(buf), fp);
                    fgets(buf, sizeof(buf), fp);
                    get_datatime_with_line(buf, &datetime_line);
                    cp_datetime_line(&ret_pos->line_dt, &datetime_line);

                    printf(" FinalA: Match[%d] Line[%d:%d] pos:%ld\n", match_hour, ret_pos->line_dt.hour, ret_pos->line_dt.minute, ret_pos->pos);
                    break;
                }

                e_pos = c_pos;
                c_pos = e_pos / 2;
            }
            else
            {
                // 向 后 查

                if (v > 0)
                    printf("  Las: Match[%d] Line[%d:%d] pos[s:%ld c:%ld e:%ld]\n", match_hour, datetime_line.hour, datetime_line.minute, s_pos, c_pos, e_pos);

                matched_last = 1;
                if (matched_previous)
                {
                    change_times++;
                    matched_previous = 0;
                }

                if (change_times > 1)
                {
                    ret_pos->pos = s_pos;
                    fseek(fp, ret_pos->pos, SEEK_SET);
                    fgets(buf, sizeof(buf), fp);
                    fgets(buf, sizeof(buf), fp);
                    get_datatime_with_line(buf, &datetime_line);
                    cp_datetime_line(&ret_pos->line_dt, &datetime_line);

                    printf(" FinalB: Match[%d] Line[%d:%d] pos:%ld\n", match_hour, ret_pos->line_dt.hour, ret_pos->line_dt.minute, ret_pos->pos);
                    break;
                }

                s_pos = c_pos;
                c_pos = (e_pos - c_pos) / 2 + c_pos;
            }
        }
        else
        {
            // 查 end

            if (match_hour <= datetime_line.hour)
            {
                // 向 前 查
                if (v > 0)
                    printf("  Pre: Match[%d] Line[%d:%d] pos[s:%ld c:%ld e:%ld]\n", match_hour, datetime_line.hour, datetime_line.minute, s_pos, c_pos, e_pos);

                matched_previous = 1;
                if (matched_last)
                {
                    ret_pos->pos = c_pos;
                    fseek(fp, ret_pos->pos, SEEK_SET);
                    fgets(buf, sizeof(buf), fp);
                    fgets(buf, sizeof(buf), fp);
                    get_datatime_with_line(buf, &datetime_line);
                    cp_datetime_line(&ret_pos->line_dt, &datetime_line);

                    printf("  Final A: Match[%d] Line[%d:%d] pos:%ld\n", match_hour, ret_pos->line_dt.hour, ret_pos->line_dt.minute, ret_pos->pos);
                    break;
                }

                e_pos = c_pos;
                c_pos = e_pos / 2;
            }
            else
            {
                // 向 后 查
                if (v > 0)
                    printf("  Las: Match[%d] Line[%d:%d] pos[s:%ld c:%ld e:%ld]\n", match_hour, datetime_line.hour, datetime_line.minute, s_pos, c_pos, e_pos);

                matched_last = 1;
                if (matched_previous)
                {
                    ret_pos->pos = e_pos;
                    fseek(fp, ret_pos->pos, SEEK_SET);
                    fgets(buf, sizeof(buf), fp);
                    fgets(buf, sizeof(buf), fp);
                    get_datatime_with_line(buf, &datetime_line);
                    cp_datetime_line(&ret_pos->line_dt, &datetime_line);

                    printf("  Final B: Match[%d] Line[%d:%d] pos:%ld\n", match_hour, ret_pos->line_dt.hour, ret_pos->line_dt.minute, ret_pos->pos);
                    break;
                }

                s_pos = c_pos;
                c_pos = (e_pos - c_pos) / 2 + c_pos;
            }
        }

        fseek(fp, c_pos, SEEK_SET);
        fgets(buf, sizeof(buf), fp);
    }

    if (feof(fp))
    {
        ret_pos->pos = e_pos;
        cp_datetime_line(&ret_pos->line_dt, &datetime_line);
        printf("------ EOF -----------\n");
        goto SUCC_EXIT;
    }

FAIL_EXIT:
    fclose(fp);
    return 1;

SUCC_EXIT:
    fclose(fp);
    return 0;
}

/**
 * @brief 获取每行的时间
 * 
 * @param line 
 * @param datetime_line 
 * @return int 0:成功, 1:失败
 */
int get_datatime_with_line(char *line, struct datetime_line_t *datetime_line)
{
    char month[128] = {0};
    char day[128] = {0};
    char time[128] = {0};
    int i = 0, j = 0, x = 0;
    for (i = 0; i < strlen(line); i++)
    {
        if (line[i] == ' ')
        {
            if (line[i + 1] == ' ')
                continue;
            else
            {
                x++;
                j = 0;
            }
            continue;
        }

        if (x == 0)
        {
            month[j++] = line[i];
            month[j] = 0;
        }
        else if (x == 1)
        {
            day[j++] = line[i];
            day[j] = 0;
        }
        else if (x == 2)
        {
            time[j++] = line[i];
            time[j] = 0;
        }
        else
            break;
    }
    // printf("month:%s day:%s time:%s\n", month, day, time);

    int mm = conv_month_2_int(month);
    int dd = atoi(day);
    int hou = 0;
    int min = 0;
    int sec = 0;
    char tmp[128] = {0};
    i = 0;
    j = 0;
    x = 0;
    while (time[i] != 0)
    {
        if (time[i] == ':')
        {
            if (x == 0)
                hou = atoi(tmp);
            else if (x == 1)
                min = atoi(tmp);
            else
                sec = atoi(tmp);

            x++;
            i++;
            j = 0;
            continue;
        }
        tmp[j++] = time[i++];
        tmp[j] = 0;
    }
    sec = atoi(tmp);

    // printf("hour:%d", hou);

    datetime_line->month = mm;
    datetime_line->day = dd;
    datetime_line->hour = hou;
    datetime_line->minute = min;
    datetime_line->second = sec;

    return 0;
}

void cp_datetime_line(struct datetime_line_t *dst, struct datetime_line_t *src)
{
    dst->month = src->month;
    dst->day = src->day;
    dst->hour = src->hour;
    dst->minute = src->minute;
    dst->second = src->second;
}

void grep_str(struct matcher_t *matcher, char *buf, char *substr)
{
    char *pos = NULL;
    if (matcher->ignore_case)
    {
        pos = strcasestr(buf, substr);
    }
    else
    {
        pos = strstr(buf, substr);
    }

    if (pos != NULL)
    {
        // printf("[MATCH] pattern: %s\n", substr);
        // found
        if (matcher->show_filename)
        {
            printf("%s: %s%c", matcher->file, buf, eolbyte);
        }
        else
        {
            printf("%s%c", buf, eolbyte);
        }
    }
}
