/*
 * @Author: songjian <kyosold@qq.com>
 * @Date: 2022-08-03 14:49:04
 * @LastEditors: kyosold kyosold@qq.com
 * @LastEditTime: 2022-08-06 00:13:21
 * @FilePath: /gpkeys/scrypto.c
 * @Description:
 *
 * Copyright (c) 2022 by kyosold kyosold@qq.com, All Rights Reserved.
 */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "scrypto.h"

int verbose = 0;

/**
 * @description: 根据指定的key, 获取RSA*
 * @param {char} *text_key      key的内容(公钥/私钥)
 * @param {int} is_publickey    是否是公钥
 * @return {*}  指向RSA *的指针，需要调用 free_rsakey()释放
 */
RSA *new_rsakey(char *text_key, int is_publickey, char *err, size_t err_size)
{
    RSA *rsa = NULL;

    BIO *in = BIO_new_mem_buf((unsigned char *)text_key, -1);
    if (in == NULL)
    {
        snprintf(err, err_size, "BIO_new_mem_buf fail:%s", OPENSS_ERR);
        return NULL;
    }
    // 如果设置 BIO_FLAGS_BASE64_NO_NL，则传入时要带"\n"（即"Hello\n")，
    // 否则，还原后会丢失两个字符
    BIO_set_flags(in, BIO_FLAGS_BASE64_NO_NL); // 生成的结果是一行，中间不换行

    rsa = RSA_new();
    if (is_publickey)
    {
        if (strstr(text_key, BEGIN_RSA_PUBLIC_KEY))
        {
            rsa = PEM_read_bio_RSAPublicKey(in, &rsa, NULL, NULL);
        }
        else
        {
            rsa = PEM_read_bio_RSA_PUBKEY(in, &rsa, NULL, NULL);
        }
    }
    else
    {
        rsa = PEM_read_bio_RSAPrivateKey(in, &rsa, NULL, NULL);
    }
    if (rsa == NULL)
    {
        snprintf(err, err_size, "%s", OPENSS_ERR);
        BIO_free_all(in);
        return NULL;
    }

    BIO_free(in);

    return rsa;
}
/**
 * @description: free an RSA key from memory
 * @param {RSA} *rsa    The RSA key
 * @return {*}
 */
void free_rsakey(RSA *rsa)
{
    RSA_free(rsa);
}
/**
 * @description:
 * @param {enum SType} type
 * @param {RSA} *rsa
 * @param {char} *data
 * @param {size_t} dlen
 * @param {size_t} *result_len
 * @param {int} padding
 * @param {char} *err           保存错误信息
 * @param {size_t} err_size     错误信息的空间大小
 * @return {*}  返回加密或解密后的数据，以\0结束。
 */
char *process_big_data_by_rsa_alloc(enum SType type, RSA *rsa, char *data, size_t dlen, int padding, size_t *result_len, char *err, size_t err_size)
{
    // 根据data_len的长度，对数据块分块加密，
    // RSA_public/private_encrypt每次最多加密RSA_size(key)的长度
    int rsa_len = RSA_size(rsa);
    int block_len = rsa_len;
    if (type == USE_PUBLIC_ENC || type == USE_PRIVATE_ENC)
    {
        block_len -= 11; // 因为填充方式为RSA_PKCS1_PADDING, 所以要在rsaLen基础上减去11
    }

    int data_len = dlen;
    int slice = data_len / block_len;
    if ((data_len % block_len) != 0)
        slice++;

    // 分配接收数据的总长度
    int n_out_size = slice * rsa_len;
    int p_out_len = 0;
    unsigned char *p_out = (unsigned char *)malloc(n_out_size + 1);
    if (!p_out)
    {
        snprintf(err, err_size, "malloc %d memory fail:%s", n_out_size + 1, strerror(errno));
        return NULL;
    }
    memset(p_out, 0, n_out_size + 1);

    // 分配每次存放buffer的空间
    unsigned char *buf = (unsigned char *)malloc(rsa_len + 1);
    if (!buf)
    {
        free(p_out);
        return NULL;
    }

    char *p_sub = NULL;
    int ret = 0;
    int pos = 0;
    while (pos < data_len)
    {
        memset(buf, 0, rsa_len + 1);
        if ((data_len - pos) < block_len)
            block_len = data_len - pos;

        p_sub = data + pos;

        if (verbose)
            printf("process buffer length: %d\n", block_len);
        if (type == USE_PUBLIC_ENC)
        {
            ret = RSA_public_encrypt(block_len,
                                     (unsigned char *)p_sub,
                                     (unsigned char *)buf,
                                     rsa,
                                     padding);
        }
        else if (type == USE_PUBLIC_DEC)
        {
            ret = RSA_public_decrypt(block_len,
                                     (unsigned char *)p_sub,
                                     (unsigned char *)buf,
                                     rsa,
                                     padding);
        }
        else if (type == USE_PRIVATE_ENC)
        {
            ret = RSA_private_encrypt(block_len,
                                      (unsigned char *)p_sub,
                                      (unsigned char *)buf,
                                      rsa,
                                      padding);
        }
        else if (type == USE_PRIVATE_DEC)
        {
            ret = RSA_private_decrypt(block_len,
                                      (unsigned char *)p_sub,
                                      (unsigned char *)buf,
                                      rsa,
                                      padding);
        }
        if (verbose)
            printf("process buffer result: %d\n", ret);

        if (ret < 0)
        {
            snprintf(err, err_size, "%s", OPENSS_ERR);
            free(buf);
            free(p_out);
            return NULL;
        }
        else
        {
            memcpy(p_out + p_out_len, buf, ret);
            p_out_len += ret;
            pos += block_len;
        }
    }

    free(buf);

    // printf("p_out_len: %d\n", p_out_len);
    *result_len = p_out_len;

    return p_out;
}

char *read_text_from_file_alloc(char *file, char *err, size_t err_size)
{
    char *p_out = NULL;

    struct stat sb;
    if (stat(file, &sb) == -1)
    {
        snprintf(err, err_size, "stat file(%s) fail:%s", file, strerror(errno));
        return NULL;
    }
    p_out = (char *)malloc(sb.st_size + 1);
    if (!p_out)
    {
        snprintf(err, err_size, "malloc %d memory fail:%s", sb.st_size + 1, strerror(errno));
        return NULL;
    }
    memset(p_out, 0, sb.st_size + 1);

    FILE *fp = fopen(file, "r");
    if (fp == NULL)
    {
        snprintf(err, err_size, "fopen file(%s) fail:%s", file, strerror(errno));
        free(p_out);
        return NULL;
    }
    size_t ret = fread(p_out, sb.st_size, 1, fp);
    if (ret != 1)
    {
        snprintf(err, err_size, "fread file(%s) fail:%s", file, strerror(errno));
        free(p_out);
        fclose(fp);
        return NULL;
    }
    fclose(fp);

    return p_out;
}

int read_text_from_file(char *file, char *text, size_t text_size, char *err, size_t err_size)
{
    struct stat sb;
    if (stat(file, &sb) == -1)
    {
        snprintf(err, err_size, "stat file(%s) fail:%s", file, strerror(errno));
        return 1;
    }
    if (text_size < (sb.st_size + 1))
    {
        snprintf(err, err_size, "buffer size not enought for store text, buffer size(%d) file size(%ld)", text_size, sb.st_size);
        return 1;
    }

    memset(text, 0, text_size);

    FILE *fp = fopen(file, "r");
    if (fp == NULL)
    {
        snprintf(err, err_size, "fopen file(%s) fail:%s", file, strerror(errno));
        return 1;
    }
    size_t ret = fread(text, sb.st_size, 1, fp);
    if (ret != 1)
    {
        snprintf(err, err_size, "fread file(%s) fail:%s", file, strerror(errno));
        fclose(fp);
        return 1;
    }
    fclose(fp);

    return 0;
}

////////////////////////////////////////////////////
/// Base64 encode/decode
////////////////////////////////////////////////////
char *openssl_base64_encode_alloc(unsigned char *data, size_t dlen, int multi_line, size_t *encode_len, char *err, size_t err_size)
{
    BIO *bio, *b64;
    BUF_MEM *bptr;

    b64 = BIO_new(BIO_f_base64());
    if (multi_line != 1)
        BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL); // 结果中间不换行
    bio = BIO_new(BIO_s_mem());

    b64 = BIO_push(b64, bio);
    BIO_write(b64, data, dlen); // encode
    BIO_flush(b64);

    BIO_get_mem_ptr(b64, &bptr);

    int elen = bptr->length;
    char *encode = (char *)malloc(elen + 1);
    if (encode == NULL)
    {
        snprintf(err, err_size, "malloc %d memory fail:%s", elen + 1, strerror(errno));
        BIO_free_all(b64);
        return NULL;
    }
    memcpy(encode, bptr->data, bptr->length);
    encode[bptr->length] = '\0';
    *encode_len = bptr->length;

    BIO_free_all(b64);

    return encode;
}

char *openssl_base64_decode_alloc(unsigned char *data, size_t dlen, int multi_line, size_t *decode_len, char *err, size_t err_size)
{
    char *out = (char *)malloc(dlen + 1); // 因为解码后的数据的长度一定比编码的要少
    if (out == NULL)
    {
        snprintf(err, err_size, "malloc %d memory fail:%s", dlen + 1, strerror(errno));
        return NULL;
    }

    BIO *bmem, *b64;

    b64 = BIO_new(BIO_f_base64());
    if (multi_line != 1)
        BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    bmem = BIO_new_mem_buf(data, dlen);
    if (bmem == NULL || b64 == NULL)
    {
        snprintf(err, err_size, "BIO_new fail:%s", OPENSS_ERR);
        free(out);
        return NULL;
    }

    bmem = BIO_push(b64, bmem);

    int len = BIO_read(bmem, out, dlen);
    *decode_len = len;

    BIO_free_all(bmem);

    return out;
}

////////////////////////////////////////////////////
/// 生成公钥与私钥
////////////////////////////////////////////////////
int openssl_pkey_new(int bits, char **pub_key_alloc, size_t *pub_key_len, char **pri_key_alloc, size_t *pri_key_len, char *err, size_t err_size)
{
    int ret = 0;
    RSA *rsa = NULL;
    BIGNUM *bne = NULL;
    BIO *bio_public = NULL, *bio_private = NULL;
    unsigned long e = RSA_F4;
    size_t pub_len = 0, pri_len = 0;
    char *pub_key = NULL, *pri_key = NULL;

    // 1. generate rsa key
    bne = BN_new();
    ret = BN_set_word(bne, e);
    if (ret != 1)
    {
        snprintf(err, err_size, "BN_new() fail:%s", OPENSS_ERR);
        goto free_all;
    }

    rsa = RSA_new();
    ret = RSA_generate_key_ex(rsa, bits, bne, NULL);
    if (ret != 1)
    {
        snprintf(err, err_size, "RSA_generate_key_ex() fail:%s", OPENSS_ERR);
        goto free_all;
    }

    // 2. save public key
    bio_public = BIO_new(BIO_s_mem());
    // 生成第一种格式公钥使用: PEM_write_bio_RSAPublicKey(bioPub, rsa);
    ret = PEM_write_bio_RSA_PUBKEY(bio_public, rsa);
    if (ret != 1)
    {
        snprintf(err, err_size, "PEM_write_bio_RSA_PUBKEY() fail:%s", OPENSS_ERR);
        goto free_all;
    }
    pub_len = BIO_pending(bio_public);
    pub_key = (char *)malloc(pub_len + 1);
    if (!pub_key)
    {
        snprintf(err, err_size, "malloc %d memory fail:%s", pub_len + 1, strerror(errno));
        ret = 0;
        goto free_all;
    }
    BIO_read(bio_public, pub_key, pub_len);
    pub_key[pub_len] = '\0';

    // 3. save private key
    bio_private = BIO_new(BIO_s_mem());
    ret = PEM_write_bio_RSAPrivateKey(bio_private, rsa, NULL, NULL, 0, NULL, NULL);
    if (ret != 1)
    {
        snprintf(err, err_size, "PEM_write_bio_RSAPrivateKey() fail:%s", OPENSS_ERR);
        free(pub_key);
        goto free_all;
    }
    pri_len = BIO_pending(bio_private);
    pri_key = (char *)malloc(pri_len + 1);
    if (!pri_key)
    {
        snprintf(err, err_size, "malloc %d memory fail:%s", pub_len + 1, strerror(errno));
        ret = 0;
        free(pub_key);
        goto free_all;
    }
    BIO_read(bio_private, pri_key, pri_len);
    pri_key[pri_len] = '\0';

    *pub_key_alloc = pub_key;
    *pub_key_len = pub_len;
    *pri_key_alloc = pri_key;
    *pri_key_len = pri_len;

    ret = 1;

free_all:
    BIO_free_all(bio_public);
    BIO_free_all(bio_private);
    RSA_free(rsa);
    BN_free(bne);

    return !(ret == 1); // 0:succ, 1:fail
}

////////////////////////////////////////////////////
/// 私钥加密 -> 公钥解密
////////////////////////////////////////////////////
char *openssl_private_encrypt_alloc(char *private_key, char *data, size_t dlen, size_t *encrypted_len, char *err, size_t err_size)
{
    // 1. 读取私钥内容
    char *p_text_private_key = NULL;
    // malloc it for key
    p_text_private_key = read_text_from_file_alloc(private_key, err, err_size);
    if (p_text_private_key == NULL)
        return NULL;

    // 2. 获取 RSA*
    RSA *rsa = NULL;
    rsa = new_rsakey(p_text_private_key, 0, err, err_size);
    if (rsa == NULL)
    {
        free(p_text_private_key);
        return NULL;
    }

    // 3. 加密
    char *encrypted = process_big_data_by_rsa_alloc(USE_PRIVATE_ENC,
                                                    rsa,
                                                    data,
                                                    dlen,
                                                    PADDING,
                                                    encrypted_len,
                                                    err,
                                                    err_size);
    if (verbose)
        printf("encrypted_len: %d data_len: %d\n", *encrypted_len, (encrypted == NULL) ? 0 : strlen(encrypted));

    free_rsakey(rsa);
    free(p_text_private_key);

    return encrypted;
}

char *openssl_public_decrypt_alloc(char *public_key, char *data, size_t dlen, size_t *decrypted_len, char *err, size_t err_size)
{
    // 1. 读取公钥内容
    char *p_text_public_key = NULL;
    // malloc it for key
    p_text_public_key = read_text_from_file_alloc(public_key, err, err_size);
    if (p_text_public_key == NULL)
        return NULL;

    // 2. 获取 RSA*
    RSA *rsa = NULL;
    rsa = new_rsakey(p_text_public_key, 1, err, err_size);
    if (rsa == NULL)
    {
        free(p_text_public_key);
        return NULL;
    }

    // 3. 解密

    char *decrypted = process_big_data_by_rsa_alloc(USE_PUBLIC_DEC,
                                                    rsa,
                                                    data,
                                                    dlen,
                                                    PADDING,
                                                    decrypted_len,
                                                    err,
                                                    err_size);
    if (verbose)
        printf("decrypted_len: %d data_len: %d\n", *decrypted_len, (decrypted == NULL) ? 0 : strlen(decrypted));

    free_rsakey(rsa);
    free(p_text_public_key);

    return decrypted;
}

////////////////////////////////////////////////////
/// 公钥加密 -> 私钥解密
////////////////////////////////////////////////////
char *openssl_public_encrypt_alloc(char *public_key, char *data, size_t dlen, size_t *encrypted_len, char *err, size_t err_size)
{
    // 1. 读取公钥内容
    char *p_text_public_key = NULL;
    p_text_public_key = read_text_from_file_alloc(public_key, err, err_size);
    if (p_text_public_key == NULL)
        return NULL;

    // 2. 获取 RSA*
    RSA *rsa = NULL;
    rsa = new_rsakey(p_text_public_key, 1, err, err_size);
    if (rsa == NULL)
    {
        free(p_text_public_key);
        return NULL;
    }

    // 3. 加密
    char *encrypted = process_big_data_by_rsa_alloc(USE_PUBLIC_ENC,
                                                    rsa,
                                                    data,
                                                    dlen,
                                                    PADDING,
                                                    encrypted_len,
                                                    err,
                                                    err_size);
    if (verbose)
        printf("encrypted_len: %d data_len: %d\n", *encrypted_len, (encrypted == NULL) ? 0 : strlen(encrypted));

    free_rsakey(rsa);
    free(p_text_public_key);

    return encrypted;
}

char *openssl_private_decrypt_alloc(char *private_key, char *data, size_t dlen, size_t *decrypted_len, char *err, size_t err_size)
{
    // 1. 读取私钥内容
    char *p_text_private_key = NULL;
    p_text_private_key = read_text_from_file_alloc(private_key, err, err_size);
    if (p_text_private_key == NULL)
        return NULL;

    // 2. 获取 RSA*
    RSA *rsa = NULL;
    rsa = new_rsakey(p_text_private_key, 0, err, err_size);
    if (rsa == NULL)
    {
        free(p_text_private_key);
        return NULL;
    }

    // 3. 加密
    char *decrypted = process_big_data_by_rsa_alloc(USE_PRIVATE_DEC,
                                                    rsa,
                                                    data,
                                                    dlen,
                                                    PADDING,
                                                    decrypted_len,
                                                    err,
                                                    err_size);
    if (verbose)
        printf("decrypted_len: %d data_len: %d\n", *decrypted_len, (decrypted == NULL) ? 0 : strlen(decrypted));

    free_rsakey(rsa);
    free(p_text_private_key);

    return decrypted;
}
