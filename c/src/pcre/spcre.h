#ifndef _S_PCRE_H
#define _S_PCRE_H

#define SPCRE_MATCHED 0
#define SPCRE_NOMATCH 1
#define SPCRE_ERR 2

int s_pcre_match_alloc(const char *str, size_t str_len, const char *pattern, char ***matched_list, size_t *matched_list_num, char *err_str, size_t err_str_size);

void s_pcre_matched_free(char **matched_list, size_t matched_list_num);

int s_pcre_match(const char *str, size_t str_len, const char *pattern, char *err_str, size_t err_str_size);

#endif