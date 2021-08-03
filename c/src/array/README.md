# array 数组库

数组是存放在堆上的，每对成员为 key -> value

```
typedef struct array
{
    unsigned int n;    /* 数组内成员的数量 */
    unsigned int size; /* 数组可存放成员的数量 */
    char **val;        /* List of string values */
    char **key;        /* List of string keys */
    unsigned *hash;    /* List of hash values for keys */
} array
```

## 一. 通常用法

a. 引入头文件

```c
#include "array/array.h
```

b. 创建数组对象，参数为设定成员的数量，给 0 则使用默认值 128, 失败返回 NULL

```c
array *list = array_new(0);
if (list == NULL) {
    printf("array_new fail\n");
    exit(1);
}
```

c. 添加成员

```c
char key[128] = {0};
char val[128] = {0};
int i = 0;
for (i=0; i<20; i++) {
    sprintf(key, "%04d_key", i);
    sprintf(val, "%03d_str", i);

    array_set(list, key, val);
}
```

d. 获取一个 key 的值

```c
char *v = NULL;
for (i=0; i<20; i++) {
    sprintf(key, "%04d_key", i);
    v = array_get(list, key, ARRAY_INVALID_KEY);
    if (v == ARRAY_INVALID_KEY) {
        printf("Notfound value for key:%s\n", key);
        continue;
    }
}
```

e. 删除一个 key 成员

```c
char *dkey = "0003_key";
array_unset(list, dkey);
```

f. 把数组成面的 value 组合成字符串

```c
char *str = array_implode_alloc(",", list);
if (str) {
    printf("%s\n", str);
    free(str);
}
```

g. 把字符串按指定的字符串分隔成数组

```c
char *ss = "May 11 14:03:32 filter_true:0";
array *ss_list = array_explode_alloc(" ", ss);
for (i=0; i<array_size(ss_list); i++) {
    if (array_key(ss_list, i)) {
        printf("[%d] %s -> %s\n", i, array_key(ss_list, i), array_value(ss_list, i));
    }
}
array_dump(ss_list, stderr);
```

h. 释放数组

```c
array_del(ss_list);
array_del(list);
```

## 二. 函数:

```
array *array_new(unsigned int size);
```

- size: 分配数组成员数量，设定为 0 则使用默认 128 个
- 返回: 失败为 NULL, 成功为指向 array 的指针

> 在堆上分配的内存

```
int array_set(array *a, char *key, char *val);
```

- a: 保存的数组指针
- key: 保存的 key
- val: 保存的 value
- 返回:

> 如果数组 a 内存满了，此函数会自动扩展。 key 和 val 都是 xstrdup 出来的内存。
> 如果 key 已经存在，则会被覆盖。原来被 free 掉。

```
char *array_get(array *a, char *key, char *def);
```

- a: 数组
- key: 需要获取值的 key
- def: 当没有找到 value 时，返回的值
- return: 找到时，返回指向的 value 指针；如果未找到，则返回 def

> 该函数在字典中定位一个键，并返回指向该键值的指针，如果在数组中找不到该键，则返回传递的'def'指针。
> 返回的字符指针指向字典对象内部的数据，你不应该手动去 free 返回的结果。

```
void array_unset(array *a, char *key);
```

- a: 数组
- key: 需要被删除成员的 key

> 这个函数删除字典中的一个键。如果找不到 key，就什么也做不了。
> 该数组内的 key 和 value 会被 free 掉。

```
char *array_implode_alloc(char *sep, array *a);
```

- sep: 每个数组成员的 value 之间分隔字符串
- a: 数组
- 返回: 组合成的字符串，该字符串为在堆上新分配的内存，使用 free 释放。

> 把数组元素组合为字符串，返回的字符串需要手动释放内存 free()

```
array *array_explode_alloc(char *sep, char *s);
```

- sep: 按此字符串分隔
- s: 需要被分隔的字符串

> 把字符串打散为数组，返回的数组需要手动调用 array_del() 释放内存

```
void array_del(array *a);
```

- a: 需要被释放内存的数组

> 同时会把数组内的所有成员也都一起释放掉。

```
int array_clean(array *a)
```

- a: 需要被释放成员的数组

> 该函数仅会把数组成员给释放掉，并不会释放数组本身，也就意味着下次不用再 array_new，但是 size 还是原来的大小。
