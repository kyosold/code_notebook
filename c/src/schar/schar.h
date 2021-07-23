#ifndef _S_CHAR_H
#define _S_CHAR_H

#define SCHAR_SUCC 0
#define SCHAR_FAIL 1
#define SCHAR_NOTFOUND -1

typedef struct schar
{
    unsigned int len;
    unsigned int size;
    char *s;
} schar;

// ------------------------------------
/**
 * @brief 创建生成一个schar结构指针对象
 * @return schar *, 失败返回 NULL.
 * @note to be free use schar_delete()
 * @see schar_delete()
 */
schar *schar_new();

/**
 * @brief 释放销毁schar *, schar内s指向的内存也一并释放掉
 * @param x 需要被释放的 schar
 * @see schar_new()
 */
void schar_delete(schar *x);
// ------------------------------------

/**
 * @brief 检查x的剩余空间是否足够放下n个字节，不够则自动扩展，
 *          如果x是空指针，则malloc给它
 * @param x 被检查的结构
 * @param n 需要存放的大小
 * @return 0:succ, 1:fail
 * @note 使用 schar_clean 释放
 * @see schar_clean()
 */
int schar_ready(schar *x, unsigned int n);

/**
 * @brief 释放x中s指向的内存
 * @param x 需要被释放的schar结构指针
 * @see schar_ready()
 */
void schar_clean(schar *x);
// ------------------------------------

/**
 * @brief 复制src前n个字节到dest里，并以\0结束
 * @param dest  指向复制的目标对象
 * @param src   指向被自制的对象
 * @param n     要复制的字节数
 * @return 0:succ, 1:fail
 * @see schar_copys(), schar_copy()
 */
int schar_copyb(schar *dest, char *src, unsigned int n);
/**
 * @brief 复制str到dest中，注意:str必须以'\0'结尾
 * @param dest  指向复制的目标对象
 * @param str   以'\0'结尾的字符串
 * @return 0:succ, 1:fail
 * @see schar_copyb(), schar_copy()
 */
int schar_copys(schar *dest, char *str);
/**
 * @brief 复制source到dest中
 * @param dest      指向复制的目标对象
 * @param source    指向被复制的源对象
 * @return 0:succ, 1:fail
 * @see schar_copys(), schar_copys()
 */
int schar_copy(schar *dest, schar *source);
// ------------------------------------

/**
 * @brief 追加src前n个字节到dest里，并以\0结束
 * @param dest  指向追加的目标对象
 * @param src   指向被复制的对象
 * @param n     要追加的字节数
 * @return 0:succ, 1:fail
 * @see schar_cats(), schar_cat()
 */
int schar_catb(schar *dest, char *src, unsigned int n);
/**
 * @brief 追加str到dest中，注意:str必须以'\0'结尾
 * @param dest  指向被追加的目标对象
 * @param str   以'\0'结尾的字符串
 * @return 0:succ, 1:fail
 * @see schar_catb(), schar_cat()
 */
int schar_cats(schar *dest, char *str);
/**
 * @brief 追加source到dest中
 * @param dest      指向被追加的目标对象
 * @param source    指向复制的源对象
 * @return 0:succ, 1:fail
 * @see schar_catb(), schar_cats()
 */
int schar_cat(schar *dest, schar *source);
// ------------------------------------

/**
 * @brief 转换字符串为小写
 * @param x     需要被转换的 schar
 * @note x 会被直接替换成小写，所以 x 会被修改
 * @see schar_strtoupper()
 */
void schar_strtolower(schar *x);
/**
 * @brief 转换字符串为大写
 * 与上面相同
 * @see schar_strtolower()
 */
void schar_strtoupper(schar *x);
// ------------------------------------

/**
 * @brief 查找子字符串substr在dest中第一次出现的位置
 * @param dest          被查找的目标
 * @param substr        查找的子字符串
 * @param substr_len    查找的子字符串的长度
 * @param offset        查找会从offset的位置开始.
 * @return 返回str第一次出现在dest中的位置,如果找不到，返回-1
 * @see schar_stripos()
 */
int schar_strpos(schar *dest, char *substr, int substr_len, unsigned int offset);
/**
 * @brief 查找子字符串substr在dest中第一次出现的位置, 不区分大小写
 * @param dest          被查找的目标
 * @param substr        查找的子字符串
 * @param substr_len    查找的子字符串的长度
 * @param offset        查找会从offset的位置开始.
 * @return 返回str第一次出现在dest中的位置,如果找不到，返回-1
 * @see schar_strpos()
 */
int schar_stripos(schar *dest, char *substr, int substr_len, unsigned int offset);
/**
 * @brief 查找子字符串substr在dest中第一次出现
 * @param dest          查找的目标
 * @param substr        查找的子字符串
 * @param substr_len    子字符串的长度
 * @param offset        查找会从offset的位置开始
 * @param before_substr 为1时，返回的是substr第一次出现之前的字符串，默认应该是0
 * @return 返回查找到的字符串，失败返回 NULL
 * @note 该字符串必须手动free()
 * @see schar_stristr_alloc
 */
char *schar_strstr_alloc(schar *dest, char *substr, int substr_len, unsigned int offset, unsigned int before_substr);
/**
 * @brief 查找子字符串在dest中第一次出现, 不区分大小写
 * @param dest          查找的目标
 * @param substr        查找的子字符串
 * @param substr_len    子字符串的长度
 * @param offset        查找会从offset的位置开始
 * @param before_substr 为1时，返回的是substr第一次出现之前的字符串，默认应该是0
 * @return 返回查找到的字符串，失败返回 NULL
 * @note 该字符串必须手动free()
 * @see schar_strstr_alloc
 */
char *schar_stristr_alloc(schar *dest, char *substr, int substr_len, unsigned int offset, unsigned int before_substr);
// ------------------------------------

#endif