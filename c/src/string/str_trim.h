#ifndef _S_TRIM_H
#define _S_TRIM_H

void str_ltrim(char *str, int str_len, char *what, int what_len, char *str_trimmed, int str_trimmed_size);
void str_rtrim(char *str, int str_len, char *what, int what_len, char *str_trimmed, int str_trimmed_size);
void str_trim(char *str, int str_len, char *what, int what_len, char *str_trimmed, int str_trimmed_size);

char *str_ltrim_alloc(char *str, int str_len, char *what, int what_len);
char *str_rtrim_alloc(char *str, int str_len, char *what, int what_len);
char *str_trim_alloc(char *str, int str_len, char *what, int what_len);

#endif