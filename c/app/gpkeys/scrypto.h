#ifndef _SCRYPTO_H_
#define _SCRYPTO_H_

/*************************************************
 * 证书生成（命令行）:
 *  1. 生成私钥:
 *    openssl genrsa -out private.pem 1024
 *  2. 根据私钥生成公钥:
 *    openssl rsa -in private.pem -out public.pem -pubout -outform PEM
 **************************************************/

#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/conf.h>
#include <openssl/aes.h>

#define PADDING RSA_PKCS1_PADDING
#define KEYSIZE 32
#define IVSIZE 32
#define BLOCKSIZE 256
#define SALTSIZE 8

#define BEGIN_RSA_PUBLIC_KEY "BEGIN RSA PUBLIC KEY"
#define BEGIN_PUBLIC_KEY "BEGIN PUBLIC KEY"

#define OPENSS_ERR ERR_error_string(ERR_get_error(), NULL)

extern int verbose;

enum SType
{
    USE_PRIVATE_ENC = 101,
    USE_PRIVATE_DEC,
    USE_PUBLIC_ENC,
    USE_PUBLIC_DEC
};
/**
 * @description: 从文件中读取文件的文本内容
 * @param {char} *file          文件名
 * @param {char} *err           保存错误信息
 * @param {size_t} err_size     错误信息的空间大小
 * @return {*}  文件内容，以\0结束，如果出错返回NULL
 */
char *read_text_from_file_alloc(char *file, char *err, size_t err_size);
/**
 * @description: 从文件中读取文件的文本内容
 * @param {char} *file          文件名
 * @param {char} *text          保存文本内容的buffer
 * @param {size_t} text_size    buffer的空间大小，要大于文件size+1
 * @param {char} *err           保存错误信息
 * @param {size_t} err_size     错误信息的空间大小
 * @return {*}  文件内容，以\0结束，如果出错返回NULL
 */
int read_text_from_file(char *file, char *text, size_t text_size, char *err, size_t err_size);

/**
 * @description: base64 encode
 * @param {unsigned char} *data 需要编码的数据
 * @param {size_t} dlen         数据data的长度
 * @param {int} multi_line      结果是否中间换行，以64个字符换一行的形式 (1:多行，0:1行)
 * @param {size_t} encode_len   编码后数据的长度
 * @param {char} *err           保存错误信息
 * @param {size_t} err_size     错误信息的空间大小
 * @return {*}  base64 encode 后的数据，以\0结束，需要手动free()释放内存，如果出错则返回NULL
 */
char *openssl_base64_encode_alloc(unsigned char *data, size_t dlen, int multi_line, size_t *encode_len, char *err, size_t err_size);
/**
 * @description: base64 decode
 * @param {unsigned char} *data 需要解码的数据
 * @param {size_t} dlen         数据data的长度
 * @param {int} multi_line      结果是否中间换行，以64个字符换一行的形式 (1:多行，0:1行)
 * @param {size_t} decode_len   解码后数据的长度 （因为解码后的数据有可能是二进制数据）
 * @param {char} *err           保存错误信息
 * @param {size_t} err_size     错误信息的空间大小
 * @return {*}  base64 decode 后的数据，需要手动free()释放内存，如果出错则返回NULL
 */
char *openssl_base64_decode_alloc(unsigned char *data, size_t dlen, int multi_line, size_t *decode_len, char *err, size_t err_size);

////////////////////////////////////////////////////
/// 生成公钥与私钥
////////////////////////////////////////////////////
/**
 * @description: 生成新的公钥和私钥
 * @param {int} bits                位数，推荐使用2048
 * @param {char} **pub_key_alloc    保存生成的公钥，需要手动free()
 * @param {size_t} *pub_key_len     保存生成的公钥长度
 * @param {char} **pri_key_alloc    保存生成的私钥，需要手动free()
 * @param {size_t} *pri_key_len     保存生成的私钥长度
 * @param {char} *err               保存出错信息
 * @param {size_t} err_size         保存出错信息的空间大小
 * @return {*}  成功返回0，失败返回1。
 *
 * 使用方法:
 *  char err[1024] = {0};
 *  char *pub_key = NULL, *pri_key = NULL;
 *  size_t pub_len = 0, pri_len = 0;
 *  int ret = openssl_pkey_new(2048,
 *                  &pub_key, &pub_len,
 *                  &pri_key, &pri_len,
 *                  err, sizeof(err));
 */
int openssl_pkey_new(int bits, char **pub_key_alloc, size_t *pub_key_len, char **pri_key_alloc, size_t *pri_key_len, char *err, size_t err_size);

////////////////////////////////////////////////////
/// 私钥加密 -> 公钥解密
////////////////////////////////////////////////////
/**
 * @description: 私钥加密
 * @param {char} *private_key       私钥文件
 * @param {char} *data              明文数据，需要被加密
 * @param {size_t} dlen             明文数据的长度
 * @param {size_t} encrypted_len    保存加密后的数据的长度
 * @param {char} *err               保存出错信息
 * @param {size_t} err_size         保存出错信息的空间大小
 * @return {*}  返回加密后的数据，需要手动 free() 回收内存
 */
char *openssl_private_encrypt_alloc(char *private_key, char *data, size_t dlen, size_t *encrypted_len, char *err, size_t err_size);

/**
 * @description: 公钥解密
 * @param {char} *public_key        公钥文件
 * @param {char} *data              加密的数据
 * @param {size_t} dlen             加密的数据的长度
 * @param {size_t} decrypted_len    保存解密后数据的长度
 * @param {char} *err               保存出错信息
 * @param {size_t} err_size         保存出错信息的空间大小
 * @return {*}  返回解密后的数据，需要手动 free() 回收内存
 */
char *openssl_public_decrypt_alloc(char *public_key, char *data, size_t dlen, size_t *decrypted_len, char *err, size_t err_size);

////////////////////////////////////////////////////
/// 公钥加密 -> 私钥解密
////////////////////////////////////////////////////
/**
 * @description: 公钥加密
 * @param {char} *public_key        公钥文件
 * @param {char} *data              明文数据，需要被加密的内容
 * @param {size_t} dlen             明文数据的长度
 * @param {size_t} *encrypted_len   加密后的数据的长度 (因为是二进制，有可能中间有\0)
 * @param {char} *err               保存出错信息
 * @param {size_t} err_size         记录err的空间大小
 * @return {*}  返回加密后的数据，需要手动 free() 释放内存
 */
char *openssl_public_encrypt_alloc(char *public_key, char *data, size_t dlen, size_t *encrypted_len, char *err, size_t err_size);
/**
 * @description: 私钥解密
 * @param {char} *private_key       私钥文件
 * @param {char} *data              加密的数据
 * @param {size_t} dlen             加密的数据的长度
 * @param {size_t} *decrypted_len   保存解密后的数据的长度
 * @param {char} *err               保存出错信息
 * @param {size_t} err_size         保存出错信息的大小
 * @return {*}  返回加密后的数据，需要手动 free() 释放内存
 */
char *openssl_private_decrypt_alloc(char *private_key, char *data, size_t dlen, size_t *decrypted_len, char *err, size_t err_size);

#endif
