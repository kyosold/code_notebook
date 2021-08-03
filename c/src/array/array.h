#ifndef _S_ARRAY_H
#define _S_ARRAY_H
#include <stdio.h>

/**
 * @brief 数组对象
 * 
 * 该对象包含一个字符串/字符串关联的列表。每一个
 * 关联由唯一的字符串键标识。查找值
 * 在字典中通过使用一个(希望没有碰撞)来加速哈希函数。
 */
typedef struct array
{
    unsigned int n;    /* Number of entries in dictionary */
    unsigned int size; /* Storage size */
    char **val;        /* List of string values */
    char **key;        /* List of string keys */
    unsigned *hash;    /* List of hash values for keys */
} array;

/* Invalid key token */
#define ARRAY_INVALID_KEY ((char *)-1)

unsigned int array_count(array *a);
unsigned int array_size(array *a);
char *array_key(array *a, unsigned int i);
char *array_value(array *a, unsigned int i);

array *array_new(unsigned int size);
void array_del(array *a);
int array_clean(array *a);

char *array_get(array *a, char *key, char *def);
int array_set(array *a, char *key, char *val);
void array_unset(array *a, char *key);

char *array_implode_alloc(char *sep, array *a);
array *array_explode_alloc(char *sep, char *s);

void array_dump(array *a, FILE *out);

#endif