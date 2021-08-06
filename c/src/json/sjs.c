#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "json-c/json.h"
#include "sjs.h"

// Generate JSON String -----------------------------
sjs *sjs_new_root()
{
    return json_object_new_object();
}
void sjs_del_root(sjs *js)
{
    json_object_put(js);
}

void sjs_add_int(sjs *js, char *k, int v)
{
    json_object_object_add(js, k, json_object_new_int(v));
}
void sjs_add_int64(sjs *js, char *k, int64_t v)
{
    json_object_object_add(js, k, json_object_new_int64(v));
}
void sjs_add_double(sjs *js, char *k, double v)
{
    json_object_object_add(js, k, json_object_new_double(v));
}
void sjs_add_string(sjs *js, char *k, char *v)
{
    json_object_object_add(js, k, json_object_new_string(v));
}
void sjs_add_array(sjs *js, char *k, sjs_array *array)
{
    json_object_object_add(js, k, array);
}

sjs_array *sjs_new_array()
{
    return json_object_new_array();
}

void sjs_array_add_int(sjs *a, int v)
{
    json_object_array_add(a, json_object_new_int(v));
}
void sjs_array_add_int64(sjs *a, int64_t v)
{
    json_object_array_add(a, json_object_new_int64(v));
}
void sjs_array_add_double(sjs *a, double v)
{
    json_object_array_add(a, json_object_new_double(v));
}
void sjs_array_add_string(sjs *a, char *v)
{
    json_object_array_add(a, json_object_new_string(v));
}
void sjs_array_add_array(sjs *a, sjs_array *array)
{
    json_object_array_add(a, array);
}

const char *sjs_to_string(sjs *a)
{
    return json_object_to_json_string(a);
}

// Parse JSON String -----------------------------
sjs *sjs_parse_json_string(char *str)
{
    return json_tokener_parse(str);
}

int sjs_get_int(sjs *js, char *k)
{
    sjs *js_tmp = json_object_object_get(js, k);
    return json_object_get_int(js_tmp);
}
int64_t sjs_get_int64(sjs *js, char *k)
{
    sjs *js_tmp = json_object_object_get(js, k);
    return json_object_get_int64(js_tmp);
}
double sjs_get_double(sjs *js, char *k)
{
    sjs *js_tmp = json_object_object_get(js, k);
    return json_object_get_double(js_tmp);
}
const char *sjs_get_string(sjs *js, char *k)
{
    sjs *js_tmp = json_object_object_get(js, k);
    return json_object_get_string(js_tmp);
}

sjs_array *sjs_get_array(sjs *js, char *k)
{
    return json_object_object_get(js, k);
}

int sjs_get_array_count(sjs_array *a)
{
    return json_object_array_length(a);
}
sjs *sjs_get_array_idx(sjs_array *a, int idx)
{
    return json_object_array_get_idx(a, idx);
}
int sjs_get_array_int(sjs_array *a, int idx)
{
    sjs *js_tmp = sjs_get_array_idx(a, idx);
    return json_object_get_int(js_tmp);
}
int sjs_get_array_int64(sjs_array *a, int idx)
{
    sjs *js_tmp = sjs_get_array_idx(a, idx);
    return json_object_get_int64(js_tmp);
}
int sjs_get_array_double(sjs_array *a, int idx)
{
    sjs *js_tmp = sjs_get_array_idx(a, idx);
    return json_object_get_double(js_tmp);
}
const char *sjs_get_array_string(sjs_array *a, int idx)
{
    sjs *js_tmp = sjs_get_array_idx(a, idx);
    return json_object_get_string(js_tmp);
}

#ifdef _TEST
void parse_json_string()
{
    char *str = "{ \"abc\": 231, \"name\": \"kyosold@qq.com\", \"sore\": 83.200000000000003, \"list\": [ [ 0, 1, 2, 3 ], [ \"val_0_end\", \"val_1_end\", \"val_2_end\", \"val_3_end\" ] ] }\"";

    sjs *js_root = sjs_parse_json_string(str);
    printf("abc: %d\n", sjs_get_int(js_root, "abc"));
    printf("name: %s\n", sjs_get_string(js_root, "name"));
    printf("sore: %lf\n", sjs_get_double(js_root, "sore"));
    printf("list:\n");

    sjs_array *list = sjs_get_array(js_root, "list");

    printf("  int list:\n");
    sjs_array *int_list = sjs_get_array_idx(list, 0);
    int i = 0;
    for (i = 0; i < sjs_get_array_count(int_list); i++)
    {
        printf("    [%d]: %d\n", i, sjs_get_array_int(int_list, i));
    }

    printf("  str list:\n");
    sjs_array *str_list = sjs_get_array_idx(list, 1);
    for (i = 0; i < sjs_get_array_count(str_list); i++)
    {
        printf("    [%d]:%s\n", i, sjs_get_array_string(str_list, i));
    }

    sjs_del_root(js_root);
}

void generate_json_string()
{
    sjs *js_root = sjs_new_root();
    sjs_add_int(js_root, "abc", 231);
    sjs_add_string(js_root, "name", "kyosold@qq.com");
    sjs_add_double(js_root, "sore", 83.2);

    sjs_array *list = sjs_new_array();
    sjs_array *int_list = sjs_new_array();
    sjs_array *str_list = sjs_new_array();
    int i = 0;
    for (i = 0; i < 4; i++)
    {
        sjs_array_add_int(int_list, i);

        char item[128] = {0};
        snprintf(item, 128, "val_%d_end", i);
        sjs_array_add_string(str_list, item);
    }
    sjs_array_add_array(list, int_list);
    sjs_array_add_array(list, str_list);
    sjs_add_array(js_root, "list", list);

    printf("%s\n", sjs_to_string(js_root));

    sjs_del_root(js_root);
}

void main(void)
{
    generate_json_string();
    parse_json_string();
}
#endif