#ifndef _S_PRIOQ_H
#define _S_PRIOQ_H

typedef struct prioq_elt
{
    unsigned long id; // 此值必须唯一
    unsigned long dt; // 此值的大小决定优先级顺序，越小越优先
} prioq_elt;

typedef struct prioq
{
    prioq_elt *p;
    unsigned int len;
    unsigned int size;
} prioq;

struct prioq *spq_new();
int spq_add(struct prioq *pq, struct prioq_elt *pe);
int spq_get(struct prioq *pq, struct prioq_elt *pe);
void spq_clean(struct prioq *pq);

#endif