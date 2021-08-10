# sjs

封装了 libjson-c 库的操作

## 使用方法

### 一. 生成 json 字符串

```c
#include "json/sjs.h"

// 创建 root 节点
sjs *js_root = sjs_new_root();

// 添加元素
sjs_add_int(js_root, "abc", 231);
sjs_add_string(js_root, "name", "kyosold@qq.com");
sjs_add_double(js_root, "sore", 83.2);

// 创建 json 数组
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

// 添加数组到节点中
sjs_add_array(js_root, "list", list);

// 生成 json 字符串
printf("%s\n", sjs_to_string(js_root));

// 释放内存
sjs_del_root(js_root);

```

### 二. 解析 json 字符串

```c
#include "json/sjs.h"

char *str = "{ \"abc\": 231, \"name\": \"kyosold@qq.com\", \"sore\": 83.200000000000003, \"list\": [ [ 0, 1, 2, 3 ], [ \"val_0_end\", \"val_1_end\", \"val_2_end\", \"val_3_end\" ] ] }\"";

// json 字符串解析为对像
sjs *js_root = sjs_parse_json_string(str);

// 从对象上解析各元素
printf("abc: %d\n", sjs_get_int(js_root, "abc"));
printf("name: %s\n", sjs_get_string(js_root, "name"));
printf("sore: %lf\n", sjs_get_double(js_root, "sore"));
printf("list:\n");

// 解析为数组对象
sjs_array *list = sjs_get_array(js_root, "list");

printf("  int list:\n");
// 数组只能按索引解析各元素
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

// 释放内存
sjs_del_root(js_root);
```

### 编译

使用 -ljson-c

```bash
-I/usr/include/ -L/usr/lib -ljson-c
```
