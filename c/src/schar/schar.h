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

schar *schar_new();
void schar_delete(schar *x);

int schar_ready(schar *x, unsigned int n);
void schar_clean(schar *x);

int schar_copyb(schar *dest, char *src, unsigned int n);
int schar_copys(schar *dest, char *str);
int schar_copy(schar *dest, schar *source);

int schar_catb(schar *dest, char *src, unsigned int n);
int schar_cats(schar *dest, char *str);
int schar_cat(schar *dest, schar *source);

char *schar_strtolower(char *s, size_t len);
char *schar_strtoupper(char *s, size_t len);

int schar_strpos(schar *dest, char *substr, int substr_len, unsigned int offset);
int schar_stripos(schar *dest, char *substr, int substr_len, unsigned int offset);
char *schar_strstr_alloc(schar *dest, char *substr, int substr_len, unsigned int offset, unsigned int before_substr);
char *schar_stristr_alloc(schar *dest, char *substr, int substr_len, unsigned int offset, unsigned int before_substr);

#endif