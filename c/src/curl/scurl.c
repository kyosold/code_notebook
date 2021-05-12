#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "scurl.h"

size_t _get_http_func(void *buffer, size_t size, size_t nmemb, void *user_p)
{
    struct pstr_s *recv_t = (struct pstr_s *)user_p;
    if (recv_t->str == NULL)
    {
        recv_t->len = 0;
        recv_t->size = size * nmemb * 2 + 1;
        recv_t->str = (char *)malloc(recv_t->size);
        if (recv_t->str == NULL)
        {
            return 0;
        }
    }

    if ((recv_t->size - recv_t->len) < ((size * nmemb) + 1))
    {
        unsigned int new_size = (recv_t->size * 2) + (size * nmemb) + 1;
        char *new_pstr = (char *)realloc(recv_t->str, new_size);
        if (new_pstr == NULL)
        {
            return 0;
        }
        recv_t->str = new_pstr;
        recv_t->size = new_size;
    }

    memcpy((recv_t->str + recv_t->len), buffer, (size * nmemb));
    recv_t->len += (size * nmemb);
    recv_t->str[recv_t->len] = '\0';

    return (size * nmemb);
}

struct curl_slist *scurl_header_add(struct curl_slist *header, char *h)
{
    if (h)
    {
        header = curl_slist_append(header, h);
    }
    return header;
}

/**
 * @brief Execure http post data to the given cURL with param
 * @param url the Given cURL
 * @param headers the custom head, set NULL if not need
 * @param post_data need post data to url, it's http request body
 * @param connect_timeout connect timeout
 * @param timeout the Execution timeout
 * @param save_cookie_fs save cookie information to the specified file, set NULL if not need
 * @param send_cookie_fs send local cookie file content to url, set NULL if not need
 * @param scurl_resp the http response information
 * @return 0:Succ, 1:fail
 */
int scurl_post(char *url, struct curl_slist *headers, char *post_data,
               unsigned int connect_timeout, unsigned int timeout,
               char *save_cookie_fs, char *send_cookie_fs,
               struct scurl_resp_s *scurl_resp)
{
    CURL *curl;
    CURLcode res;
    int ret = 0;

    /* In windows, this will init the winsock stuff */
    res = curl_global_init(CURL_GLOBAL_DEFAULT);

    /* Check for errors */
    if (res != CURLE_OK)
        return 1;

    /* get a curl handle */
    curl = curl_easy_init();
    if (curl == NULL)
        return 1;

    /* First set the URL that is about to receive our POST. */
    curl_easy_setopt(curl, CURLOPT_URL, url);
    /* Now specify we want to POST data */
    curl_easy_setopt(curl, CURLOPT_POST, 1L);

    if (headers != NULL)
    {
        headers = scurl_header_add(headers, "Expect:");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    }

    if (post_data != NULL)
    {
        // 设置POST 数据
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);
    }

    // 设置SSL
    if (strncasecmp(url, "https", 5) == 0)
    {
        curl_easy_setopt(curl, CURLOPT_CAINFO, CA_PEM);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1);
    }

    // 设置函数执行最长时间
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
    // 设置连接服务器最长时间
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, connect_timeout);

    // 设置保存 cookie 到文件
    if (save_cookie_fs != NULL)
    {
        curl_easy_setopt(curl, CURLOPT_COOKIESESSION, 1);
        curl_easy_setopt(curl, CURLOPT_COOKIEJAR, save_cookie_fs);
    }

    // 设置发送的cookie
    if (send_cookie_fs != NULL)
    {
        curl_easy_setopt(curl, CURLOPT_COOKIEFILE, send_cookie_fs);
    }

    // 设置处理接收到头数据的回调函数
    scurl_resp->header.len = 0;
    scurl_resp->header.size = 0;
    scurl_resp->header.str = NULL;
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, _get_http_func);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, (struct pstr_s *)&scurl_resp->header);

    // 设置处理接收到的下载数据的回调函数
    scurl_resp->body.len = 0;
    scurl_resp->body.size = 0;
    scurl_resp->body.str = NULL;
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, _get_http_func);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (struct pstr_s *)&scurl_resp->body);

    res = curl_easy_perform(curl);
    if (res != CURLE_OK)
    {
        ret = 1;
    }
    else
    {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &scurl_resp->response_code);
        ret = 0;
        /*
        printf("Header ------------\n");
        printf("%s\n", scurl_resp->header.str);
        printf("Body ------------\n");
        printf("%s\n", scurl_resp->body.str);
        */
    }

    if (headers != NULL)
        curl_slist_free_all(headers);

    /* always cleanup */
    curl_easy_cleanup(curl);

    curl_global_cleanup();

    return ret;
}

/**
 * @brief Execure http get from the given cURL with param
 * @param url the Given cURL, include(?A=a&B=b)
 * @param headers the custom head, set NULL if not need
 * @param connect_timeout connect timeout
 * @param timeout the Execution timeout
 * @param save_cookie_fs save cookie information to the specified file, set NULL if not need
 * @param send_cookie_fs send local cookie file content to url, set NULL if not need
 * @param scurl_resp the http response information
 * @return 0:Succ, 1:fail
 */
int scurl_get(char *url, struct curl_slist *headers,
              unsigned int connect_timeout, unsigned int timeout,
              char *save_cookie_fs, char *send_cookie_fs,
              struct scurl_resp_s *scurl_resp)
{
    CURL *curl;
    CURLcode res;
    int ret = 0;

    /* In windows, this will init the winsock stuff */
    res = curl_global_init(CURL_GLOBAL_DEFAULT);

    /* Check for errors */
    if (res != CURLE_OK)
        return 1;

    /* get a curl handle */
    curl = curl_easy_init();
    if (curl == NULL)
        return 1;

    /* First set the URL that is about to receive our POST. */
    curl_easy_setopt(curl, CURLOPT_URL, url);

    if (headers != NULL)
    {
        headers = scurl_header_add(headers, "Expect:");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    }

    // 设置SSL
    if (strncasecmp(url, "https", 5) == 0)
    {
        curl_easy_setopt(curl, CURLOPT_CAINFO, CA_PEM);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1);
    }

    // 设置函数执行最长时间
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
    // 设置连接服务器最长时间
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, connect_timeout);

    // 设置保存 cookie 到文件
    if (save_cookie_fs != NULL)
    {
        curl_easy_setopt(curl, CURLOPT_COOKIESESSION, 1);
        curl_easy_setopt(curl, CURLOPT_COOKIEJAR, save_cookie_fs);
    }

    // 设置发送的cookie
    if (send_cookie_fs != NULL)
    {
        curl_easy_setopt(curl, CURLOPT_COOKIEFILE, send_cookie_fs);
    }

    // 设置处理接收到头数据的回调函数
    scurl_resp->header.len = 0;
    scurl_resp->header.size = 0;
    scurl_resp->header.str = NULL;
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, _get_http_func);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, (struct pstr_s *)&scurl_resp->header);

    // 设置处理接收到的下载数据的回调函数
    scurl_resp->body.len = 0;
    scurl_resp->body.size = 0;
    scurl_resp->body.str = NULL;
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, _get_http_func);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (struct pstr_s *)&scurl_resp->body);

    res = curl_easy_perform(curl);
    if (res != CURLE_OK)
    {
        ret = 1;
    }
    else
    {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &scurl_resp->response_code);
        ret = 0;
        /*
        printf("Header ------------\n");
        printf("%s\n", scurl_resp->header.str);
        printf("Body ------------\n");
        printf("%s\n", scurl_resp->body.str);
        */
    }

    if (headers != NULL)
        curl_slist_free_all(headers);

    /* always cleanup */
    curl_easy_cleanup(curl);

    curl_global_cleanup();

    return ret;
}

typedef struct
{
    char *data;
    char *pos;
    char *last;
} drp_upload_ctx;

size_t _put_data_func(void *buffer, size_t size, size_t nmemb, void *user_p)
{
    drp_upload_ctx *ctx = (drp_upload_ctx *)user_p;
    size_t len = 0;

    if (ctx->pos >= ctx->last)
        return 0;

    if ((size == 0) || (nmemb == 0) || ((size * nmemb) < 1))
        return 0;

    len = ctx->last - ctx->pos;
    if (len > size * nmemb)
        len = size * nmemb;

    memcpy(buffer, ctx->pos, len);
    ctx->pos += len;

    return len;
}

int scurl_put_file(char *url, struct curl_slist *headers, char *upload_file,
                   unsigned int connect_timeout, unsigned int timeout,
                   char *save_cookie_fs, char *send_cookie_fs,
                   struct scurl_resp_s *scurl_resp)
{
    /* read file body */
    char *file_content = NULL;
    drp_upload_ctx *upload_ctx = NULL;
    struct stat st;
    if (stat(upload_file, &st) == -1)
    {
        return 1;
    }
    if ((st.st_mode & S_IFMT) != S_IFREG)
    {
        /* file is not normal file */
        return 1;
    }
    file_content = (char *)malloc(st.st_size + 1);
    if (file_content == NULL)
    {
        return 1;
    }
    FILE *fp = fopen(upload_file, "r");
    fread(file_content, 1, st.st_size, fp);
    fclose(fp);

    CURL *curl;
    CURLcode res;

    /* In windows, this will init the winsock stuff */
    res = curl_global_init(CURL_GLOBAL_DEFAULT);

    /* Check for errors */
    if (res != CURLE_OK)
    {
        goto CFAIL;
    }

    /* get a curl handle */
    curl = curl_easy_init();
    if (curl == NULL)
    {
        goto CFAIL;
    }

    /* post data to urlencode */
    char *post_data = curl_easy_escape(curl, file_content, st.st_size);
    if (!post_data)
    {
        post_data = file_content;
    }

    if (headers != NULL)
    {
        headers = scurl_header_add(headers, "Expect:");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    }

    upload_ctx = (drp_upload_ctx *)malloc(sizeof(drp_upload_ctx));
    if (upload_ctx == NULL)
    {
        goto CFAIL;
    }
    upload_ctx->data = file_content;
    upload_ctx->pos = file_content;
    upload_ctx->last = upload_ctx->pos + st.st_size;

    /* First set the URL that is about to receive our POST. */
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, connect_timeout);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);

    curl_easy_setopt(curl, CURLOPT_PUT, 1L);
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

    /* libcurl需要读取数据传递给远程主机时调用 */
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, _put_data_func);
    curl_easy_setopt(curl, CURLOPT_READDATA, upload_ctx);
    curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)(upload_ctx->last - upload_ctx->pos));

    /* set headers */
    if (headers != NULL)
    {
        headers = scurl_header_add(headers, "Expect:");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    }

    // 设置处理接收到头数据的回调函数
    scurl_resp->header.len = 0;
    scurl_resp->header.size = 0;
    scurl_resp->header.str = NULL;
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, _get_http_func);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, (struct pstr_s *)&scurl_resp->header);

    // 设置处理接收到的下载数据的回调函数
    scurl_resp->body.len = 0;
    scurl_resp->body.size = 0;
    scurl_resp->body.str = NULL;
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, _get_http_func);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (struct pstr_s *)&scurl_resp->body);

    res = curl_easy_perform(curl);
    if (res != CURLE_OK)
    {
        goto CFAIL;
    }
    else
    {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &scurl_resp->response_code);
        /*
        printf("Header ------------\n");
        printf("%s\n", scurl_resp->header.str);
        printf("Body ------------\n");
        printf("%s\n", scurl_resp->body.str);
        */

        if (file_content != NULL)
        {
            free(file_content);
            file_content = NULL;
        }

        if (upload_ctx != NULL)
        {
            free(upload_ctx);
            upload_ctx = NULL;
        }

        if (headers != NULL)
            curl_slist_free_all(headers);

        curl_easy_cleanup(curl);
        curl_global_cleanup();

        return 0;
    }

CFAIL:
    if (file_content != NULL)
    {
        free(file_content);
        file_content = NULL;
    }

    if (upload_ctx != NULL)
    {
        free(upload_ctx);
        upload_ctx = NULL;
    }

    if (headers != NULL)
        curl_slist_free_all(headers);

    curl_easy_cleanup(curl);
    curl_global_cleanup();
    return 1;
}

#ifdef _TEST
// gcc -g scurl -D_TEST -lcurl
void main(int argc, char **argv)
{
    struct curl_slist *headers = NULL;
    headers = scurl_header_add(headers, "X-SJ-User: kyosold");
    headers = scurl_header_add(headers, "X-SJ-UID: 111222333");

    struct scurl_resp_s scurl_resp;

    /*int ret = scurl_post("http://www.baidu.com",
                         headers,
                         "account=kyosold&password=123qwe&remember=1",
                         3,
                         3,
                         NULL,
                         NULL,
                         &scurl_resp);
                         */
    int ret = scurl_get("http://www.baidu.com/index.php?id=1000&name=sj",
                        headers,
                        3,
                        3,
                        NULL,
                        NULL,
                        &scurl_resp);
    printf("ret:%d code:%d\n", ret, scurl_resp.response_code);
    printf("----------------\n%s\n%s\n-------------\n",
           scurl_resp.header.str, scurl_resp.body.str);
}
#endif