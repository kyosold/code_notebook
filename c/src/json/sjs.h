#ifndef _S_JSON_H
#define _S_JSON_H

#include "json-c/json.h"

typedef struct json_object sjs;
typedef struct json_object sjs_array;

// 生成 JSON 字符串函数集 -----------
sjs *sjs_new_root();
void sjs_del_root(sjs *js);

void sjs_add_int(sjs *js, char *k, int v);
void sjs_add_int64(sjs *js, char *k, int64_t v);
void sjs_add_double(sjs *js, char *k, double v);
void sjs_add_string(sjs *js, char *k, char *v);
void sjs_add_array(sjs *js, char *k, sjs_array *array);

sjs_array *sjs_new_array();
void sjs_array_add_int(sjs *a, int v);
void sjs_array_add_int64(sjs *a, int64_t v);
void sjs_array_add_double(sjs *a, double v);
void sjs_array_add_string(sjs *a, char *v);
void sjs_array_add_array(sjs *a, sjs_array *array);

const char *sjs_to_string(sjs *a);

// 解析 JSON 字符串 函数集 ----------
sjs *sjs_parse_json_string(char *str);

int sjs_get_int(sjs *js, char *k);
int64_t sjs_get_int64(sjs *js, char *k);
double sjs_get_double(sjs *js, char *k);
const char *sjs_get_string(sjs *js, char *k);

sjs_array *sjs_get_array(sjs *js, char *k);
int sjs_get_array_count(sjs_array *a);
sjs *sjs_get_array_idx(sjs_array *a, int idx);

int sjs_get_array_int(sjs_array *a, int idx);
int sjs_get_array_int64(sjs_array *a, int idx);
int sjs_get_array_double(sjs_array *a, int idx);
const char *sjs_get_array_string(sjs_array *a, int idx);

#endif