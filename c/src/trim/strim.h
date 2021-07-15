#ifndef _S_TRIM_H
#define _S_TRIM_H

void ltrim(char *str, int str_len, char *what, int what_len, char *str_trimmed, int str_trimmed_size);
void rtrim(char *str, int str_len, char *what, int what_len, char *str_trimmed, int str_trimmed_size);
void trim(char *str, int str_len, char *what, int what_len, char *str_trimmed, int str_trimmed_size);

char *ltrim_alloc(char *str, int str_len, char *what, int what_len);
char *rtrim_alloc(char *str, int str_len, char *what, int what_len);
char *trim_alloc(char *str, int str_len, char *what, int what_len);

#endif