#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "prioq.h"

/**************************************************/
#define ALIGNMENT 16 /* assuming that this alignment is enough */

struct prioq_elt *_alloc(unsigned int n)
{
    struct prioq_elt *x;
    n = ALIGNMENT + n - (n & (ALIGNMENT - 1));

    x = (struct prioq_elt *)malloc(n);
    return x;
}

void _free(struct prioq_elt *x)
{
    if (x)
    {
        free(x);
        x = NULL;
    }
}

int _alloc_re(struct prioq_elt **x, unsigned int old_size, unsigned int new_size)
{
    struct prioq_elt *y = _alloc(new_size);
    if (!y)
        return 1;

    memcpy(y, *x, old_size);
    _free(*x);
    *x = y;

    return 0;
}
/**************************************************/

/**
 * @return 0:succ, other fail
 */
int prioq_readplus(register prioq *x, register unsigned int n)
{
    register unsigned int i;
    if (x->p)
    {
        i = x->size;
        n += x->len;
        if (n >= i)
        {
            x->size = 100 + n + (n >> 3);
            if (_alloc_re(&x->p, i * sizeof(struct prioq_elt), x->size * sizeof(struct prioq_elt)) == 0)
                return 0;

            x->size = i;
            return 1;
        }
        return 0;
    }
    x->len = 0;

    return !(x->p = (struct prioq_elt *)_alloc((x->size = n) * sizeof(struct prioq_elt)));
}

/**
 * @brief 将 pe 插入到 pq 优先级队列中
 * @return 0:succ, other fail
 * 
 * 实际是将 pe 的内存区域复制到 pq 中 p 的某一段内存区域
 */
int prioq_insert(prioq *pq, struct prioq_elt *pe)
{
    int i, j;
    if (prioq_readplus(pq, 1))
        return 1;

    j = pq->len++;
    while (j)
    {
        i = (j - 1) / 2;
        if (pq->p[i].dt <= pe->dt)
            break;

        pq->p[j] = pq->p[i];
        j = i;
    }
    pq->p[j] = *pe;

    return 0;
}

/**
 * @brief 从 pq 队列中获取优先级最高的 pe
 * @return 0:succ, other is error
 */
int prioq_min(prioq *pq, struct prioq_elt *pe)
{
    if (!pq->p)
        return 1;
    if (!pq->len)
        return 1;

    *pe = pq->p[0];

    return 0;
}

/**
 * @brief 从 pq 队列中删除优先级最高的 pe
 * 
 * 实际上是把最高优先级指向到次高优先级内存
 */
void prioq_del_min(prioq *pq)
{
    int i, j, n;
    if (!pq->p)
        return;

    n = pq->len;
    if (!n)
        return;

    i = 0;
    --n;
    for (;;)
    {
        j = i + i + 2;
        if (j > n)
            break;

        if (pq->p[j - 1].dt <= pq->p[j].dt)
            --j;

        if (pq->p[n].dt <= pq->p[j].dt)
            break;

        pq->p[i] = pq->p[j];
        i = j;
    }
    pq->p[i] = pq->p[n];
    pq->len = n;
}

/**
 * @brief 释放掉 prioq 内部链表上的的 prioq_elt 对象
 * 
 * 但是 prioq 本身并不释放，只是释放 prioq_elt 成员
 */
void prioq_free(prioq *pq)
{
    if (pq && pq->p)
    {
        free(pq->p);
        pq->p = NULL;
    }
    pq->len = 0;
    pq->size = 0;
}

/**
 * @brief 分配一个指向 struct prioq 的对象
 * @return 指向 struct prioq 的对象，失败返回 NULL
 * 
 * 需要调用 pq_clean 去释放内存
 */
struct prioq *pq_new()
{
    struct prioq *pq = NULL;
    pq = (struct prioq *)malloc(sizeof(struct prioq));
    if (pq)
    {
        pq->len = 0;
        pq->size = 0;
        pq->p = NULL;
    }
    return pq;
}

/**
 * @brief 添加一个 struct prioq_elt 成员到 pq 中
 * @param pq 被添加的pqchan
 * @param pe 需要添加进去的成员
 * @return 0:succ, other is error
 */
int pq_add(struct prioq *pq, struct prioq_elt *pe)
{
    if (pq == NULL || pe == NULL)
        return 1;

    if (prioq_insert(pq, pe))
        return 1;

    return 0;
}

/**
 * @brief 从 pqchan 中获取先优先的 pe
 * @param pq 从该 pqchan 中获取
 * @param pe 取出来的 pe 填充内存
 * @return 0:succ, other is error
 * 
 * pe 取出来后，就会从 pqchan 中删除。
 */
int pq_get(struct prioq *pq, struct prioq_elt *pe)
{
    if (pq == NULL)
        return 1;

    if (prioq_min(pq, pe))
        return 1;

    prioq_del_min(pq);

    return 0;
}

/**
 * @brief 释放掉从 pq_new 中创建的 pqchan
 * 
 * 会把 pqchan 内部所有的 pe 也都释放掉
 */
void pq_clean(struct prioq *pq)
{
    if (pq == NULL)
        return;

    prioq_free(pq);

    free(pq);
    pq = NULL;
}

#ifdef _TEST
// gcc -g prioq.c
#include <time.h>
void main(int argc, char **argv)
{
    struct prioq *pqchan = NULL;
    pqchan = pq_new();
    if (pqchan == NULL)
    {
        printf("pq_new fail\n");
        return;
    }

    int i = 0;
    for (i = 1; i < 6; i++)
    {
        struct prioq_elt pe;
        pe.id = i;
        // pe.dt = time(NULL);
        pe.dt = rand();
        if (pq_add(pqchan, &pe))
        {
            printf("add fail\n");
            return;
        }
        printf("add pe:%ld -> %d succ\n", pe.id, pe.dt);
        sleep(1);
    }

    sleep(2);
    printf("start get --------\n");

    for (i = 1; i < 6; i++)
    {
        struct prioq_elt pe;
        int ret = pq_get(pqchan, &pe);
        if (ret == 1)
            printf("get pe:%ld fail\n", i);
        else
            printf("get pe:%ld [%ld]=>[%lu]\n", i, pe.id, pe.dt);
    }

    pq_clean(pqchan);
}
#endif
