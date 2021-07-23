#include <stdio.h>
#include <string.h>
#include "log/slog.h"
#include "array/array.h"
#include "code/base64.h"
#include "code/md5.h"
#include "code/sha1.h"
#include "code/crc32.h"
#include "schar/schar.h"

int main(int argc, char **argv)
{
    slog_open("libsc_example", LOG_MAIL, SLOG_DEBUG, "99999999");
    slog_debug("Use libsc library start...");

    slog_set_uid("0x110011");
    slog_set_level(SLOG_INFO);

    array *list = NULL;
    char key[128] = {0};
    char val[128] = {0};
    int count = 20;
    int i = 0;

    slog_info("------ Test array ----------");
    slog_info("allocating array");
    list = array_new(0);

    slog_info("set array: %d", count);
    for (i = 0; i < count; i++)
    {
        sprintf(key, "%04d", i);
        sprintf(val, "%04d_val", i);
        array_set(list, key, val);
        slog_info("add item %s -> %s", key, val);
    }
    slog_info("get array: %d", count);
    for (i = 0; i < count; i++)
    {
        sprintf(key, "%04d", i);
        char *v = array_get(list, key, NULL);
        if (v == NULL)
            slog_info("cann't get value for key %s", key);
        else
            slog_info("get item %s -> %s", key, v);
    }

    slog_info("Use 'implode'...");
    char *str = array_implode_alloc(",", list);
    if (str)
    {
        slog_info("implode string: %s", str);
        free(str);
    }
    slog_info("deallocate list");
    array_del(list);

    char *str2 = "May 11 14:03:32 10.75.30.234 spam_filter[39641]: log_headers";
    array *s_list = array_explode_alloc(" ", str2);
    for (i = 0; i < s_list->size; i++)
    {
        if (s_list->key[i])
        {
            slog_info("[%d]: %s", i, s_list->val[i]);
        }
    }
    slog_info("deallocate s_list");
    array_del(s_list);

    slog_info("------ Test code ----------");
    slog_info("Base64 ----");
    int outlen = 0;
    char *src_str = "123qwe";
    char *out = s_base64_encode_alloc(src_str, strlen(src_str), &outlen);
    slog_info("src:%s base64 encode str:%s", src_str, out);

    char *out2 = s_base64_decode_alloc(out, outlen, &outlen);
    slog_info("src:%s base64 decode str:%s", out, out2);

    free(out);
    free(out2);

    slog_info("MD5 ----");
    char md5_res[1024] = {0};
    s_md5(src_str, strlen(src_str), 0, md5_res, sizeof(md5_res));
    slog_info("src:%s md5 str:%s", src_str, md5_res);

    slog_info("Sha1 ----");
    char sha1_res[1024] = {0};
    s_sha1(src_str, strlen(src_str), 0, sha1_res, sizeof(sha1_res));
    slog_info("src:%s sha1 str:%s", src_str, sha1_res);

    slog_info("Crc32 ----");
    char crc32_res[1024] = {0};
    s_crc32(src_str, strlen(src_str), crc32_res, sizeof(crc32_res));
    slog_info("src:%s crc32 str:%s", src_str, crc32_res);

    slog_info("------ Test schar ----------");
    schar *x = schar_new();
    if (!x)
    {
        slog_error("allocate fail");
        return 1;
    }
    schar_copyb(x, "123456789", 9);
    schar_catb(x, " , ", 3);
    schar_cats(x, "hello world");
    slog_info("len:%d size:%d s:%s", x->len, x->size, x->s);

    int pos = schar_stripos(x, "el", 2, 0);
    slog_info("[schar_stripos('el')] pos:%d str:[%s]\n", pos, x->s + pos);

    char *s = schar_strstr_alloc(x, "or", 2, 0, 0);
    slog_info("[schar_strstr('or')] str:[%s]", s);
    if (s)
        free(s);

    schar_delete(x);

    return 0;
}