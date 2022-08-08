/*
 * @Author: songjian <kyosold@qq.com>
 * @Date: 2022-08-03 17:18:52
 * @LastEditors: kyosold kyosold@qq.com
 * @LastEditTime: 2022-08-06 00:12:29
 * @FilePath: /gpkeys/gpkeys.c
 * @Description:
 *
 * Copyright (c) 2022 by kyosold kyosold@qq.com, All Rights Reserved.
 */
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "scrypto.h"

void usage(char *prog)
{
    printf("%s -v [options] [key file] [data]\n", prog);
    printf("options:\n");
    printf(" 1: public encrypto\n");
    printf(" 2: private decrypto\n");
    printf(" 3: private encrypto\n");
    printf(" 4: public decrypto\n");
    printf(" be: base64 encode\n");
    printf(" bd: base64 decode\n");
    printf(" pk: generate new key pairs\n");
    printf("\nexample:\n");
    printf(" %s 0 /data1/rsakeys/public.pem 'data of test.'\n", prog);
}

int main(int argc, char **argv)
{
    if (argc != 4 && argc != 5)
    {
        if (argc == 2 && strcasecmp(argv[1], "pk") == 0)
        {
        }
        else
        {
            usage(argv[0]);
            exit(1);
        }
    }

    char *type = NULL;
    char *key_file = NULL;
    char *data_src = NULL;

    if (argc == 5)
    {
        verbose = 1;
        type = argv[2];
        key_file = argv[3];
        data_src = argv[4];
    }
    else if (argc == 4)
    {
        type = argv[1];
        key_file = argv[2];
        data_src = argv[3];
    }
    else if (argc == 2)
    {
        type = argv[1];
    }

    char err[1024];
    size_t err_size = sizeof(err);

    if (strcasecmp(type, "1") == 0) // public encrypto
    {
        // base64 encode
        size_t data_src_b64_len = 0;
        char *data_src_b64 = openssl_base64_encode_alloc(data_src, strlen(data_src), 0, &data_src_b64_len, err, err_size);
        if (data_src_b64 == NULL)
        {
            printf("openssl_base64_encode_alloc fail: %s\n", err);
            return 1;
        }
        if (verbose)
            printf("base64 encode:\n%s\n", data_src_b64);

        // 加密
        size_t data_encrypted_len = 0;
        char *data_encrypted = openssl_public_encrypt_alloc(key_file,
                                                            data_src_b64,
                                                            data_src_b64_len,
                                                            &data_encrypted_len,
                                                            err,
                                                            err_size);
        if (data_encrypted == NULL)
        {
            printf("openssl_public_encrypt_alloc fail: %s\n", err);
            free(data_src_b64);
            return 1;
        }
        free(data_src_b64);
        if (verbose)
            printf("public encrypt length: %d\n", strlen(data_encrypted));

        // base64 encode
        size_t res_len = 0;
        char *res = openssl_base64_encode_alloc(data_encrypted, data_encrypted_len, 0, &res_len, err, err_size);
        if (res == NULL)
        {
            free(data_encrypted);
            printf("openssl_base64_encode_alloc fail: %s\n", err);
            return 1;
        }
        free(data_encrypted);
        printf("encrypted: %d\n%s\n", res_len, res);

        free(res);
        return 0;
    }
    else if (strcasecmp(type, "2") == 0) // private decrypto
    {
        // base64 decode
        size_t data_len = 0;
        char *data = openssl_base64_decode_alloc(data_src, strlen(data_src), 0, &data_len, err, err_size);
        if (data == NULL)
        {
            printf("openssl_base64_decode_alloc fail: %s\n", err);
            return 1;
        }
        if (verbose)
            printf("base decode: %d\n", data_len);

        // 解密
        size_t decrypted_len = 0;
        char *decrypted = openssl_private_decrypt_alloc(key_file, data, data_len, &decrypted_len, err, err_size);
        if (decrypted == NULL)
        {
            printf("openssl_private_decrypt_alloc fail: %s\n", err);
            free(data);
            return 1;
        }
        free(data);
        // printf("decrypted: %s\n", decrypted);

        // base64 decode
        size_t res_len = 0;
        char *res = openssl_base64_decode_alloc(decrypted, decrypted_len, 0, &res_len, err, err_size);
        if (res == NULL)
        {
            free(decrypted);
            printf("openssl_base64_decode_alloc fail:%s\n", err);
            return 1;
        }
        printf("decrypted: %d\n%s\n", res_len, res);
        free(res);

        return 0;
    }
    else if (strcasecmp(type, "3") == 0) // private encrypto
    {
        // base64 encode
        size_t data_b64_len = 0;
        char *data_b64 = openssl_base64_encode_alloc(data_src, strlen(data_src), 0, &data_b64_len, err, err_size);
        if (data_b64 == NULL)
        {
            printf("base64 encode fail:%s\n", err);
            return 1;
        }

        // private encrypto
        size_t encrypted_len = 0;
        char *encrypted = openssl_private_encrypt_alloc(key_file, data_b64, data_b64_len, &encrypted_len, err, err_size);
        if (encrypted == NULL)
        {
            printf("openssl_private_encrypt_alloc fail: %s\n", err);
            free(data_b64);
            return 1;
        }
        free(data_b64);

        // base64 encode
        size_t res_len = 0;
        char *res = openssl_base64_encode_alloc(encrypted, encrypted_len, 0, &res_len, err, err_size);
        if (res == NULL)
        {
            printf("openssl_base64_encode_alloc fail: %s\n", err);
            free(encrypted);
            return 1;
        }
        free(encrypted);
        printf("encrypted: %d\n%s\n", res_len, res);
        free(res);

        return 0;
    }
    else if (strcasecmp(type, "4") == 0) // public decrypto
    {
        // base64 decode
        size_t data_len = 0;
        char *data = openssl_base64_decode_alloc(data_src, strlen(data_src), 0, &data_len, err, err_size);
        if (data == NULL)
        {
            printf("openssl_base64_decode_alloc fail: %s\n", err);
            return 1;
        }

        // 解密
        size_t decrypted_len = 0;
        char *decrypted = openssl_public_decrypt_alloc(key_file, data, data_len, &decrypted_len, err, err_size);
        if (decrypted == NULL)
        {
            printf("openssl_public_decrypt_alloc fail: %s\n", err);
            free(data);
            return 1;
        }
        free(data);
        if (verbose)
            printf("decrypted: \n%s\n", decrypted);

        // base64 decode
        size_t res_len = 0;
        char *res = openssl_base64_decode_alloc(decrypted, decrypted_len, 0, &res_len, err, err_size);
        if (res == NULL)
        {
            printf("openssl_base64_decode_alloc fail:%s\n", err);
            free(decrypted);
            return 1;
        }
        free(decrypted);

        printf("decrypted: %d\n%s\n", res_len, res);

        free(res);
        return 0;
    }
    else if (strcasecmp(type, "be") == 0)
    {
        // base64 encode
        size_t data_b64_len = 0;
        char *data_b64 = openssl_base64_encode_alloc(data_src, strlen(data_src), 0, &data_b64_len, err, err_size);
        printf("base64 encode: %d\n%s\n", data_b64_len, data_b64);
        if (data_b64)
            free(data_b64);
    }
    else if (strcasecmp(type, "bd") == 0)
    {
        // base64 decode
        size_t data_len = 0;
        char *data = openssl_base64_decode_alloc(data_src, strlen(data_src), 0, &data_len, err, err_size);
        printf("base64 decode: \n%s\n", data);
        if (data)
            free(data);
    }
    else if (strcasecmp(type, "pk") == 0)
    {
        // generate key
        char *pub_key = NULL, *pri_key = NULL;
        size_t pub_key_len = 0, pri_key_len = 0;
        int ret = openssl_pkey_new(2048,
                                   &pub_key, &pub_key_len,
                                   &pri_key, &pri_key_len,
                                   err, err_size);
        if (ret != 0)
        {
            printf("openssl_pkey_new: %s\n", err);
            return 1;
        }

        printf("New Public Key: %d\n%s\n", pub_key_len, pub_key);
        printf("New Private Key: %d\n%s\n", pri_key_len, pri_key);

        free(pub_key);
        free(pri_key);
    }

    return 0;
}
