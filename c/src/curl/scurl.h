#ifndef _S_CURL_H
#define _S_CURL_H

/**
 * Build:
 *  gcc -g -o test scurl.c -lcurl
 */

#include <stdio.h>
#include <curl/curl.h>

typedef struct pstr_s
{
    char *str;
    size_t len;
    size_t size;
} pstr_t;

typedef struct scurl_resp_s
{
    unsigned long response_code;
    struct pstr_s header;
    struct pstr_s body;
} scurl_resp_t;

#define CA_PEM "cacert.pem"

int scurl_post(char *url, struct curl_slist *headers, char *post_data,
               unsigned int connect_timeout, unsigned int timeout,
               char *save_cookie_fs, char *send_cookie_fs,
               struct scurl_resp_s *scurl_resp);

#endif
