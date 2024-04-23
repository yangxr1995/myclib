#include <signal.h>
#include <sys/time.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "str.h"
#include "assert.h"
#include "mm_pool.h"
#include "fmt.h"
#include "debug.h"
#include "logger.h"
#include "timer_list.h"
#include "thread_pool.h"
#include "task.h"
#include "crypto.h"


#define PUB_PEM "./pub.pem"
#define PRI_PEM "./pri.pem"

static int test_sign() 
{
    // 加载私钥
    FILE *privateKeyFile = fopen("./pri.pem", "r");
    RSA *rsaPrivateKey = PEM_read_RSAPrivateKey(privateKeyFile, NULL, NULL, NULL);
	if (rsaPrivateKey == NULL) {
        printf("read private key failed. : %s\n", ERR_error_string(ERR_get_error(), NULL));
		return -1;
	}
    fclose(privateKeyFile);

    // 加载公钥
    FILE *publicKeyFile = fopen("./pub.pem", "r");
    RSA *rsaPublicKey = PEM_read_RSAPublicKey(publicKeyFile, NULL, NULL, NULL);
	if (rsaPublicKey == NULL) {
        printf("read public key failed. : %s\n", ERR_error_string(ERR_get_error(), NULL));
		return -1;
	}
    fclose(publicKeyFile);

    // 待签名的消息
    unsigned char message[] = "Hello, RSA!";
    int messageLength = strlen((char *)message);

    // 创建签名缓冲区
    unsigned char signature[RSA_size(rsaPrivateKey)];
    unsigned int signatureLength = RSA_size(rsaPrivateKey);

    // 使用私钥进行签名
	/*
     *   int RSA_sign(int type, const unsigned char *m, unsigned int m_len,
     *                unsigned char *sigret, unsigned int *siglen, RSA *rsa);
	 *   使用type类型的哈希算法对文本 m 长度 m_len 得到摘要，
	 *   并使用私钥rsa，对摘要进行非对称加密，得到数字签名 sigret
	 *   返回的数字签名为二进制，若要传输，需要 base64_encode
	 *   
	 */
    int signResult = RSA_sign(NID_sha1, message, messageLength, signature, &signatureLength, rsaPrivateKey);
    if (signResult != 1) {
        printf("RSA_sign failed. : %s\n", ERR_error_string(ERR_get_error(), NULL));
        return 1;
    }

    printf("Signature created successfully.\n");

	const char *signature_base64_encode;

	signature_base64_encode = base64_encode(signature, signatureLength);
	printf("signature base64 : \n%s\n", signature_base64_encode);

	const unsigned char *signature_base64_decode;
	int signature_decode_len;

	signature_base64_decode = base64_decode(signature_base64_encode, strlen(signature_base64_encode), (int *)&signature_decode_len);

	if (signatureLength != signature_decode_len) {
		printf("base64 error signatureLength(%d) != signature_decode_len(%d)\n", signatureLength, signature_decode_len);
		return -1;
	}

    // 使用公钥进行验证
	// 使用NID_sha1哈希算法对message计算摘要得到hash1
	// 使用公钥对数字签名解密得到hash2
	// 如果 hash1 == hash2 则通过验证
    int verifyResult = RSA_verify(NID_sha1, message, messageLength, signature_base64_decode, signature_decode_len, rsaPublicKey);
    if (verifyResult != 1) {
        printf("RSA_verify failed.\n");
        return 1;
    }

    printf("Signature verified successfully.\n");

	free((void *)signature_base64_decode);
	free((void *)signature_base64_encode);

    // 释放RSA结构体
    RSA_free(rsaPrivateKey);
    RSA_free(rsaPublicKey);

    return 0;
}

static int test_rsa()
{
	unsigned char *ciper;
	unsigned int ciper_len;
	unsigned char *plain;
	int i;
	char s[117 + 1000 + 1] = {0};

	if (generate_rsa_key_to_file(PUB_PEM, PRI_PEM, 1024) < 0) {
		printf("Failed to generate_rsa_key_to_file : %s\n",
				strerror(errno));
		ERR_print_errors_fp(stderr);
		return -1;
	}
	

	for (i = 0; i < sizeof(s); i++)
		s[i] = '1';
	s[sizeof(s) - 1] = 0;

	if (rsa_public_en((const unsigned char *)s, strlen(s), 
				&ciper, &ciper_len, PUB_PEM) < 0) {
		printf("Failed to rsa_public_en : %s\n",
				strerror(errno));
		ERR_print_errors_fp(stderr);
		return -1;
	}

	printf("ciper : %p\n", ciper);

	if (rsa_private_de(ciper, ciper_len, 
				&plain, PRI_PEM) < 0) {
		printf("Failed to rsa_private_de : %s\n",
				strerror(errno));
		ERR_print_errors_fp(stderr);
		return -1;
	}

	printf("orgin : %s\n", s);
	printf("orgin len : %ld\n", strlen(s));

	printf("plain : %s\n", plain);
	printf("plain len : %ld\n", strlen((const char *)plain));

	free(plain);
	free(ciper);

	return 0;
}

int main(int argc, char **argv)
{
	test_rsa();
	test_sign();

	return 0;
}
