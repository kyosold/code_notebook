#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "array.h"

/* Maximum value size for integers and doubles. */
#define MAXVALSZ 1024

/* Minimal allocated number of entries in a array */
#define ARRAYMINSZ 128

/*--------------------------------------
        Private functions
--------------------------------------*/

/**
 * @brief Doubles the allocated size associated to a pointer
 * 'size' is the current allocated size.
 */
static void *_mem_double(void *ptr, int size)
{
    void *newptr;
    newptr = calloc(2 * size, 1);
    if (newptr == NULL)
        return NULL;

    memcpy(newptr, ptr, size);
    free(ptr);
    return newptr;
}

/**
 * @brief Duplicate a string
 * @param s String to duplicate
 * @return Pointer to a newly allocated string, to be freed with free()
 *
 * This is a replacement for strdup(). This implementation is provided
 * for systems that do not have it.
 */
static char *xstrdup(char *s)
{
    char *t;
    if (!s)
        return NULL;

    t = malloc(strlen(s) + 1);
    if (t)
        strcpy(t, s);

    return t;
}

/**
 * @brief    Create a new array object.
 * @param    size    Optional initial size of the array.
 * @return   1 newly allocated array objet.
 * 
 * 这个函数分配一个新的给定大小的数组对象并返回它。
 * 如果你事先不知道(大概)数组中的条目数，那么将size=0。
 */
array *array_new(unsigned int size)
{
    array *a;

    /* If no size was specified, allocate space for ARRAYMINSZ */
    if (size < ARRAYMINSZ)
        size = ARRAYMINSZ;

    if (!(a = (array *)calloc(1, sizeof(array))))
        return NULL;

    a->size = size;
    a->val = (char **)calloc(size, sizeof(char *));
    a->key = (char **)calloc(size, sizeof(char *));
    a->hash = (unsigned int *)calloc(size, sizeof(unsigned));

    return a;
}

/**
 * @brief    Delete a array object
 * @param    a   array object to deallocate.
 * @return   void
 * 
 * Deallocate a array object and all memory associated to it.
 */
void array_del(array *a)
{
    int i;
    if (a == NULL)
        return;

    for (i = 0; i < a->size; i++)
    {
        if (a->key[i] != NULL)
            free(a->key[i]);
        if (a->val[i] != NULL)
            free(a->val[i]);
    }
    free(a->val);
    free(a->key);
    free(a->hash);
    free(a);
    return;
}

/**
 * @brief    Compute the hash key for a string.
 * @param    key     Character string to use for key.
 * @return   1 unsigned int on at least 32 bits.
 * 
 * 这个哈希函数取自Dobbs博士杂志上的一篇文章。这通常是一个无冲突的函数，它均匀地分配键。
 * 键以任何方式存储在结构中，因此可以通过最后比较键本身来避免冲突。
 */
unsigned array_hash(char *key)
{
    int len;
    unsigned hash;
    int i;

    len = strlen(key);
    for (hash = 0, i = 0; i < len; i++)
    {
        hash += (unsigned)key[i];
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);

    return hash;
}

/**
 * @brief    Get a value from a array.
 * @param    a       array object to search.
 * @param    key     Key to look for in the array.
 * @param    def     Default value to return if key not found.
 * @return   1 pointer to internally allocated character string.
 * 
 * 该函数在字典中定位一个键，并返回指向该键值的指针，如果在字典中找不到该键，
 * 则返回传递的'def'指针。返回的字符指针指向字典对象内部的数据，您不应该试
 * 图释放它或修改它。
 */
char *array_get(array *a, char *key, char *def)
{
    unsigned hash;
    int i;

    hash = array_hash(key);
    for (i = 0; i < a->size; i++)
    {
        if (a->key[i] == NULL)
            continue;
        /* Compare hash */
        if (hash == a->hash[i])
        {
            /* Compare string, to avoid hash collisions */
            if (!strcmp(key, a->key[i]))
                return a->val[i];
        }
    }
    return def;
}

/**
 * @brief    Set a value in a array.
 * @param    a       array object to modify.
 * @param    key     Key to modify or add.
 * @param    val     Value to add.
 * @return   int     0 if Ok, anything else otherwise
 * 
 * 如果给定的key已经存在，则val将被替换。如果不存在，则添加。
 * 
 * 可以设置val是NULL，但是设置array或key为NULL是错误的，这种情况下，
 * 函数将立即返回。
 * 
 * 注意: 如果array_set变量为NULL，调用array_get将返回NULL值:找到
 * 变量并返回它的值(NULL)，换句话说，将变量设置为NULL相当于将变量从
 * 数组中删除。在数组中没有val的key是不可能的。
 */
int array_set(array *a, char *key, char *val)
{
    int i;
    unsigned hash;

    if (a == NULL || key == NULL)
        return -1;

    /* Compute hash for this key */
    hash = array_hash(key);

    /* Find if value is already in array */
    if (a->n > 0)
    {
        for (i = 0; i < a->size; i++)
        {
            if (a->key[i] == NULL)
                continue;
            if (hash == a->hash[i])
            {
                /* Same hash value */
                if (!strcmp(key, a->key[i]))
                {
                    /* Found a value: modify and return */
                    if (a->val[i] != NULL)
                        free(a->val[i]);
                    a->val[i] = val ? xstrdup(val) : NULL;
                    /* Value has been modified: return */
                    return 0;
                }
            }
        }
    }

    /* Add a new value */
    /* See if array needs to grow */
    if (a->n == a->size)
    {
        /* Reached maximum size: reallocate array */
        a->val = (char **)_mem_double(a->val, a->size * sizeof(char *));
        a->key = (char **)_mem_double(a->key, a->size * sizeof(char *));
        a->hash = (unsigned int *)_mem_double(a->hash, a->size * sizeof(unsigned));
        if ((a->val == NULL) || (a->key == NULL) || (a->hash == NULL))
            /* Cannot grow array */
            return -1;
        /* Double size */
        a->size *= 2;
    }

    /* Insert key in the first empty slot */
    for (i = 0; i < a->size; i++)
    {
        if (a->key[i] == NULL)
            /* Add key here */
            break;
    }

    /* Copy key */
    a->key[i] = xstrdup(key);
    a->val[i] = val ? xstrdup(val) : NULL;
    a->hash[i] = hash;
    a->n++;

    return 0;
}

/**
 * @brief    Delete a key in a dictionary
 * @param    a       array object to modify.
 * @param    key     Key to remove.
 * @return   void
 * 
 * 这个函数删除字典中的一个键。如果找不到 key，就什么也做不了。
 */
void array_unset(array *a, char *key)
{
    unsigned hash;
    int i;

    if (key == NULL)
        return;

    hash = array_hash(key);
    for (i = 0; i < a->size; i++)
    {
        if (a->key[i] == NULL)
            continue;
        /* Compare hash */
        if (hash == a->hash[i])
        {
            /* Compare string, to avoid hash collisions */
            if (!strcmp(key, a->key[i]))
                /* Found key */
                break;
        }
    }
    if (i >= a->size)
        /* Key not found */
        return;

    free(a->key[i]);
    a->key[i] = NULL;
    if (a->val[i] != NULL)
    {
        free(a->val[i]);
        a->val[i] = NULL;
    }
    a->hash[i] = 0;
    a->n--;

    return;
}

/**
 * @brief Count all elements in an array
 * @param An array
 * @return Returns the number of elements in array.
 */
unsigned int array_count(array *a)
{
    return a->n;
}

unsigned int array_size(array *a)
{
    return a->size;
}

char *array_key(array *a, unsigned int i)
{
    return a->key[i];
}

char *array_value(array *a, unsigned int i)
{
    return a->val[i];
}

/**
 * @brief   Join array elements with a string
 * @param   sep     Specifies what to place between the elements of an array
 * @param   a       array of strings to implode
 * @return  Pointer to a newly allocated string, to be freed with free()
 * 
 * 把数组元素组合为字符串，返回的字符串需要手动释放内存 free()
 */
char *array_implode_alloc(char *sep, array *a)
{
    int i;
    unsigned int len = 0;
    unsigned int size = 1024 + (strlen(sep) * a->size);
    char *res = NULL;

    if (a == NULL)
        return NULL;
    if (a->n < 1)
        return NULL;

    res = (char *)malloc(size);
    if (res == NULL)
        return NULL;

    for (i = 0; i < a->size; i++)
    {
        if (a->key[i] && (a->val[i] != NULL))
        {
            if (i == 0)
            {
                snprintf(res, size, "%s", a->val[i]);
            }
            else
            {
                if ((size - len) <= (strlen(sep) + strlen(a->val[i])))
                {
                    res = _mem_double(res, size);
                    if (res == NULL)
                        return NULL;
                    size *= 2;
                }
                snprintf(res + len, size - len, "%s%s", sep, a->val[i]);
            }

            len = strlen(res);
        }
    }

    return res;
}

/**
 * @brief   Split a string by a string
 * @param   sep     The boundary string.
 * @param   s       The input string.
 * @return  Pointer to a newly allocated array, to be freed with array_del()
 * 
 * 把字符串打散为数组，返回的数组需要手动释放内存 array_del()
 */
array *array_explode_alloc(char *sep, char *s)
{
    if (sep == NULL || s == NULL)
        return NULL;

    char *s_dup = xstrdup(s);
    if (s_dup == NULL)
        return NULL;

    unsigned int i = 0;
    char key[1024] = {0};

    array *a;
    a = array_new(0);

    char *tok = strtok(s_dup, sep);
    while (tok)
    {
        sprintf(key, "%d", i++);
        array_set(a, key, tok);

        tok = strtok(NULL, sep);
    }

    free(s_dup);

    return a;
}

/**
 * @brief    Dump a array to an opened file pointer.
 * @param    a   Array to dump
 * @param    f   Opened file pointer.
 * @return   void
 * 
 * 将数组输出到打开的文件指针上，键值对打印为["key"]=>"value"，每
 * 行一个，可以使用stdout或stderr作为输出文件的指针
 */
void array_dump(array *a, FILE *out)
{
    int i;
    if (a == NULL)
        return;

    fprintf(out, "array(%d,<%d>) {\n", a->n, a->size);
    for (i = 0; i < a->size; i++)
    {
        if (a->key[i])
        {
            fprintf(out, "  [\"%s\"] => \"%s\"\n",
                    a->key[i],
                    a->val[i] ? a->val[i] : "UNDEF");
        }
    }
    fprintf(out, "}\n");

    return;
}

#ifdef _TEST
#define NVALS 20
void main(int argc, char **argv)
{
    array *a;
    char *val;
    int i;
    char ckey[90];
    char cval[90];

    /* Allocate array */
    printf("allocating...\n");
    a = array_new(0);

    /* Set values in array */
    printf("setting %d values...\n", NVALS);
    for (i = 0; i < NVALS; i++)
    {
        sprintf(ckey, "%04d", i);
        sprintf(cval, "%04d_val", i);
        array_set(a, ckey, cval);
    }
    printf("getting %d values...\n", NVALS);
    for (i = 0; i < NVALS; i++)
    {
        sprintf(ckey, "%04d", i);
        val = array_get(a, ckey, ARRAY_INVALID_KEY);
        if (val == ARRAY_INVALID_KEY)
            printf("cannot get value for key [%s]\n", cval);
        printf("%s => %s\n", ckey, val);
    }
    printf("implode...\n");
    char *str = array_implode_alloc(",", a);
    if (str)
    {
        printf("%s\n", str);
        free(str);
    }

    char *ss = "May 11 14:03:32 10.75.30.234 spam_filter[39641]: log_headers 13831F99630C4DC2A47EDB590F43191A LOGPrase log_headers[Subject: Njg4Mzg4ODAxMCAtIENPU0NPIFNISVBQSU5HIExp] filter_true:0 cip:10.75.30.48 envfrom:postmaster@sinanet.com envto:mailback@sunway-logistics.sinanet.com";
    array *ss_array = array_explode_alloc(" ", ss);
    array_dump(ss_array, stderr);
    array_del(ss_array);

    printf("unsetting %d values...\n", NVALS);
    for (i = 0; i < NVALS; i++)
    {
        sprintf(cval, "%04d", i);
        array_unset(a, cval);
    }

    if (a->n != 0)
        printf("error deleting values\n");

    printf("deallocating...\n");
    array_del(a);

    return;
}
#endif
