# prioq

优先级队列

## 一. 使用方法

a. 引入头文件

```c
#include "prioq/prioq.h"
```

b. 创建 prioq 对象

```c
struct prioq *pqchan = NULL;
pqchan = spq_new();
if (pqchan == NULL) {
    printf("pq_new fail\n");
    return 1;
}
```

c. 添加 item 到 prioq 队列中

```c
int i = 0;
for (i=0; i<5; i++) {
    struct prioq_elt pe;
    pe.id = i;
    pe.dt = rand();
    if (spq_add(pqchan, &pe)) {
        printf("add item:(%d, %d) fail\n", pe.id, ped.dt);
        continue;
    }
    printf("add item:(%ld, %d) succ\n", pe.id, pe.dt);
}
```

d. 获取一个最优先的成员项

```c
struct prioq_elt pe;
int ret = spq_get(pqchan, &pe);
if (ret == 1)
    printf("get item fail\n");
else
    printf("get item:(%ld, %lu)\n", pe.id, pe.dt);
```

e. 清理队列

```c
spq_clean(pqchan);
```

## 二. 函数说明

```
struct prioq *spq_new()
```

- 返回: 分配的 struct prioq 对象, 失败为 NULL

> 分配一个指向 struct prioq 的对象

```
int spq_add(struct prioq *pq, struct prioq_elt *pe)
```

- pq: 保存优先级的队列 pq
- pe: 需要被添加的成员项
- 返回: 0 成功, 其它为错误

> 添加一个成员到优先级队列中

```
int spq_get(struct prioq *pq, struct prioq_elt *pe)
```

- pq: 保存优先级的队列 pq
- pe: 保存优先级最高的成员
- 返回: 0 成功, 其它为失败

> 从优先级队列 pqchan 中取出一个优先级最高的项并保存在 pe 中

```
void spq_clean(struct prioq *pq)
```

- pq: 需要被清理的队列

> 释放并清理优先级队列
