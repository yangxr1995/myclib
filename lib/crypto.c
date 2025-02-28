#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "crypto.h"

/*
 * 密钥长度和明文长度和生成密文的长度的关系
 *
 * 一次加密的原始数据长度不是由公钥长度决定的，而是由加密算法和实现方式决定的。
 * 
 * 加密算法（如RSA）通常会对较长的原始数据进行分块处理，然后分别对每个块进行加密。
 * 
 * 对于RSA算法，加密的原始数据长度上限取决于密钥的长度。一般情况下，RSA算法中，一次加密的原始数据长度最多可以是密钥长度减去一些填充和其他开销的字节数。对于1024位的RSA公钥，一次加密的原始数据长度通常不会超过117字节。
 * 
 * 生成的密文长度也不是固定的，它与原始数据的长度以及加密算法和实现方式相关。在RSA算法中，一次加密后生成的密文长度等于密钥长度，即1024位的RSA公钥对应的密文长度是128字节。
 *
 */

/*
 * int RSA_public_encrypt(int flen, const unsigned char *from,
 *					   unsigned char *to, RSA *rsa, int padding);
 *  flen	明文数据长度字节数，若padding参数使用RSA_PKCS1_PADDING方式，则该值最大为所使用密钥的位数 / 8 - 11
 *  from	明文数据
 *  to	存放生成的密文数据，该空间大小应该为秘钥位数 / 8，保证可以存放的下
 *  rsa	公钥
 *  padding	填充方式*
 *
 *  注意 :
 *      RSA_public_encrypt一次性只能加密(密钥的位数 / 8 = N)字节的数据，且加密前后数据长度相等。
 *      比如对于1024bit的密钥，可一次性加密128字节，由于采用RSA_PKCS1_PADDING填充，填充需要占用11字节，
 *      故真正的明文数据，最多只占128-11=117字节。当实际明文数据过长时，应采用分段加密，并将加密结果拼到一起即可。
 *      生成的密文长度为密钥的长度，如1024bit的公钥，生成128字节的密文
 *
 * 		
 *
 */

/*
 * int RSA_private_decrypt(int flen, const unsigned char *from,
 *                         unsigned char *to, RSA *rsa, int padding);
 * 
 *    flen	密文数据长度，一般固定为秘钥位数 / 8
 *    from	密文数据
 *    to	存放解密后的明文数据，该空间大小应该为秘钥位数 / 8，保证可以存放的下
 *    rsa	私钥
 *    padding	填充方式
 * 
 * 返回值:
 *      以RSA 1024为例，表示将一段128字节的密文，进行解密，并解除填充，得到的实际明文数据，该数据的长度，作为函数返回值。
 *      该返回值，可以用于从to参数指向的内存中，提取实际长度的明文数据。
 *
 * 注意:
 *      与RSA_public_encrypt类似，RSA_private_decrypt也是一次性只能解密(密钥的位数 / 8 = N)字节的数据，且解密前后数据长度相等。
 *
 */



/*
 * int RSA_generate_key_ex(RSA *rsa, int bits, BIGNUM *e, BN_GENCB *cb);
 * 
 * 参数说明：
 * 
 *     rsa:  指向RSA结构体的指针，用于存储生成的公钥和私钥。
 *     bits: 生成的密钥的位数，通常为2048或4096等常见的RSA密钥长度。
 *     e :   公钥的指数值，通常为固定值RSA_F4（65537）。
 *     cb:   可选参数，用于指定回调函数，
 *           用于跟踪密钥生成的进度或中断生成过程。
 *
 * 函数返回值:
 *    成功时，返回1。
 *    失败时，返回0。
 *
 * 使用RSA_generate_key_ex函数可以生成RSA密钥对，其中包括公钥和私钥。你可以通过提供适当的参数来控制生成的密钥的位数和其他属性。通常情况下，使用默认参数即可满足大多数需求。生成的密钥对将存储在提供的RSA结构体中
 *
 */

/*
 *  功能:
 *      生成指定bits长度的rsa公私钥并保存到文件
 */
int 
generate_rsa_key_to_file(const char *pub_file, 
		const char *pri_file, unsigned int bits)
{
	RSA* rsa = NULL;
	FILE *fp = NULL;
	BIGNUM *e = NULL;

	rsa = RSA_new();
	e = BN_new();
	BN_set_word(e, 12345);

	RSA_generate_key_ex(rsa, bits, e, NULL);

	if ((fp = fopen(pub_file, "w")) == NULL)
		goto _err_;

	if (PEM_write_RSAPublicKey(fp, rsa) == 0)
		goto _err_;

	fclose(fp);

	if ((fp = fopen(pri_file, "w")) == NULL)
		goto _err_;

	if (PEM_write_RSAPrivateKey(fp, rsa, NULL, NULL, 
				0, NULL, NULL) == 0)
		goto _err_;

	fclose(fp);
	RSA_free(rsa);
	BN_free(e);

	return 0;

_err_:
	if (fp)
		fclose(fp);
	if (rsa)
		RSA_free(rsa);
	if (e)
		BN_free(e);

	return -1;
}

#define __round_mask(x, y) ((__typeof__(x))((y)-1))
#define round_up(x, y) ((((x)-1) | __round_mask(x, y))+1)

inline static RSA *
rsa_private_pem_read(const char *file)
{
	RSA *key;
	FILE *fp;

	if ((fp = fopen(file, "r")) == NULL)
		return NULL;

	if ((key = PEM_read_RSAPrivateKey(fp, NULL, NULL, NULL)) == NULL) {
		fclose(fp);
		return NULL;
	}

	fclose(fp);

	return key;
}

inline static RSA *
rsa_public_pem_read(const char *file)
{
	RSA *key;
	FILE *fp;

	if ((fp = fopen(file, "r")) == NULL)
		return NULL;

	// 读取RSA格式的公钥，注意公钥文件可以有多种格式
	// 必须保证读取方法和目标格式匹配
	if ((key = PEM_read_RSAPublicKey(fp, NULL, NULL, NULL)) == NULL) {
		fclose(fp);
		return NULL;
	}

	fclose(fp);

	return key;
}

/*
 * 功能
 *     使用pub_file做公钥文件，读取公钥，对数据data进行
 *     加密，malloc合适的空间存放密文，并通过ciper返回 
 */
int
rsa_public_en(const unsigned char *data, int len, 
		unsigned char **ciper, unsigned int *ciper_len,
		const char *pub_file)
{
	RSA *key = NULL;
	unsigned int key_len, size, round_max_len, plain_len;
	unsigned char *out;
	const unsigned char *p1, *end, *p2;
	char err_buf[256];

	if ((key = rsa_public_pem_read(pub_file)) == NULL)
		goto _err_;

	// 获得密钥的长度，密钥的长度决定了一次加密的明文最大长度
	key_len = RSA_size(key);

	// 根据不同的补全方式，明文最大长度不同
	// RSA_PKCS1_PADDING 需要11字节用于存放补齐数据
	// 补齐数据内容是随机生成的，这样每次加密得到的密文都不同
	round_max_len = key_len - 11;

	// 计算密文需要的空间
	// 每轮加密1-117字节数据，都需要128字节的空间存放密文
	// 如 明文长度 118 ，则会生成 256 字节的密文
	size = ((len / round_max_len) + (len % round_max_len)) * key_len;
	size = round_up(size, sizeof(int));
	
	out = malloc(size);
	*ciper = out;

	*ciper_len = 0;

	// 进行加密，每次只能加密有限长度的明文，
	// 具体长度由密钥决定，如 1024 bit的密钥，
	// 最多加密 1024/8字节的明文，若使用随机数补齐则为117
	for (p1 = data, end = data + len,
			p2 = p1 + round_max_len > end ? 
			end : p1 + round_max_len;
			p1 < end;
			p1 = p2,
			p2 = p1 + round_max_len > end ? 
			end : p1 + round_max_len
		) {

		// 此轮输入明文的长度
		plain_len = end - p1;
		if (plain_len > round_max_len)
			plain_len = round_max_len;

		// 进行加密
		if (RSA_public_encrypt(plain_len, p1,
				out, key, RSA_PKCS1_PADDING) < 0) {
			ERR_error_string(ERR_get_error(), err_buf);
			fprintf(stderr, "RSA_public_encrypt error : %s\n", err_buf);
			goto _err_;
		}

		*ciper_len += key_len;
		out += key_len;
	}

	RSA_free(key);
	return 0;

_err_:
	if (key)
		RSA_free(key);

	return -1;
}

/*
 * 功能
 *     从pri_file加载私钥，解密ciper，并分配合适空间，
 *     写入明文，通过plain返回
 */
int 
rsa_private_de(const unsigned char *ciper, 
		unsigned int ciper_len, unsigned char **plain,
		const char *pri_file)
{
	RSA *key = NULL;
	unsigned int key_len, size, round_len;
	unsigned char *out;
	const unsigned char *p1, *end, *p2;
	char err_buf[256];

	if ((key = rsa_private_pem_read(pri_file)) == NULL)
		goto _err_;

	key_len = RSA_size(key);

	// 明文长度一定小于密文长度
	out = malloc(ciper_len);
	memset(out, 0x0, ciper_len);
	*plain = out;

	for (p1 = ciper, end = ciper + ciper_len;
			p1 < end;
			p1 += key_len) {

		if (RSA_private_decrypt(key_len, p1, 
				out, key, RSA_PKCS1_PADDING) < 0) {
			ERR_error_string(ERR_get_error(), err_buf);
			fprintf(stderr, "RSA_private_decrypt error : %s\n", err_buf);
			goto _err_;
		}
		
		out += strlen((const char *)out);
	}

	RSA_free(key);
	return 0;

_err_:
	if (key)
		RSA_free(key);

	return -1;
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

int md5_str(const char *data, unsigned int len, char *out, int out_len)
{
	int i;
	char *p;
    unsigned char digest[MD5_DIGEST_LENGTH];

    MD5((unsigned char*)data, len, digest);

	if (out_len <= MD5_DIGEST_LENGTH * 2)
		return -1;

    for(p = out, i = 0; i < MD5_DIGEST_LENGTH; i++, p += 2)
		sprintf(p, "%02x", digest[i]);
	
	return 0;
}

static int test_md5()
{
	const char *str = "hello\n";
	char buf[MD5_DIGEST_LENGTH * 2 + 1];

	md5_str(str, strlen(str), buf, sizeof(buf));
	printf("%s\n", buf);

	return 0;
}

// Base64编码
char* base64_encode(const unsigned char* input, int length) 
{
    BIO *bio, *b64;
    BUF_MEM *bufferPtr;
    char *encoded;

    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new(BIO_s_mem());

    bio = BIO_push(b64, bio);

    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(bio, input, length);
    BIO_flush(bio);

    BIO_get_mem_ptr(bio, &bufferPtr);

    encoded = (char *)malloc(bufferPtr->length + 1);
    memcpy(encoded, bufferPtr->data, bufferPtr->length);
	encoded[bufferPtr->length] = '\0';

	BIO_free_all(b64);

    return encoded;
}

// 计算Base64解码后的长度
static int calcDecodeLength(const char* input, int length) 
{
    int padding = 0;

    if (input[length - 1] == '=' && input[length - 2] == '=')
        padding = 2;
    else if (input[length - 1] == '=')
        padding = 1;

    return (length * 3) / 4 - padding;
}

// Base64解码
unsigned char* base64_decode(const char* input, int length, int* outLength) 
{
    BIO *bio, *b64;
    int decodedLength = calcDecodeLength(input, length);
    unsigned char *decoded = (unsigned char *)calloc(1, decodedLength + 1);

    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new_mem_buf(input, length);
    bio = BIO_push(b64, bio);

    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
    *outLength = BIO_read(bio, decoded, length);
    BIO_free_all(bio);

    return decoded;
}

static int test_base64() 
{
    const unsigned char plaintext[] = "Hello, Base64!";
    int plaintextLength = strlen((char *)plaintext);

    // Base64编码
    char* encodedText = base64_encode(plaintext, plaintextLength);
    printf("Encoded text: %s\n", encodedText);

    // Base64解码
    int decodedLength;
    unsigned char* decodedText = NULL;

	decodedText = base64_decode(encodedText, strlen(encodedText), &decodedLength);
    printf("Decoded text: %s\n", decodedText);

    free(encodedText);
    free(decodedText);

    return 0;
}

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

/*
 * AES 的基本操作顺序为
 * 		encode : 补齐 -> 加密
 * 		decode : 解密 -> 删除多余
 */

// a simple hex-print routine. could be modified to print 16 bytes-per-line
static void hex_print(const void* pv, size_t len)
{
    const unsigned char * p = (const unsigned char*)pv;
    if (NULL == pv) {
        printf("NULL");
    }
    else {
        size_t i = 0;
        for (; i<len;++i) {
            printf("%02X ", *p++);
        }
    }
    printf("\n");
}

/*
 * PKCS#7补齐就是缺几补几
 * 如果数据刚好对齐，则补一个完整的块
 */
inline static void pkcs7_padding(unsigned char *data, int length, int block_size) 
{
    int padding_length = block_size - (length % block_size);
    for (int i = 0; i < padding_length; i++) {
        data[length + i] = padding_length;
    }
}

inline static void pkcs7_unpadding(unsigned char *data, unsigned int *length) 
{
    int padding_length = data[*length - 1];
    *length -= padding_length;
}

int aes_cbc_pad_pkcs7_de(unsigned char *data, int data_len, 
		unsigned char **out, int *out_len, 
		const unsigned char *key, int keylength, 
		unsigned char *iv)
{
	unsigned int len;
	unsigned char *buf;
	AES_KEY aes_key;

	if (keylength != 128 &&
			keylength != 192 &&
			keylength != 256) {
		fprintf(stderr, "keylength must be 128/192/256\n");
		return -1;
	}

	// 解密数据长度一定小于等于密文长度
	len = data_len;
	buf = (unsigned char *)malloc(len);

	AES_set_decrypt_key(key, keylength, &aes_key);
	AES_cbc_encrypt(data, buf, data_len, &aes_key, iv, AES_DECRYPT);

	// 去掉补齐的数据
	pkcs7_unpadding(buf, (unsigned int *)&len);

	*out = buf;
	*out_len = len;

	return 0;
}

int aes_cbc_pad_pkcs7_en(const unsigned char *data, int data_len, 
		unsigned char **out, int *out_len, 
		const unsigned char *key, int keylength, 
		unsigned char *iv)
{
	unsigned int len;
	unsigned char *plain, *buf;
	AES_KEY aes_key;

	if (keylength != 128 &&
			keylength != 192 &&
			keylength != 256) {
		fprintf(stderr, "keylength must be 128/192/256\n");
		return -1;
	}

	// 获得补齐后的数据，做原始数据
	// 如果数据大小刚好对齐，则多加一个 AES_BLOCK_SIZE 块
	// 总之一定有补齐操作
	len = (data_len / AES_BLOCK_SIZE + 1) * AES_BLOCK_SIZE;
	plain = (unsigned char *)malloc(len);
	memcpy(plain, data, data_len);
	pkcs7_padding(plain, data_len, AES_BLOCK_SIZE);

	buf = (unsigned char *)malloc(len);

	// 将补齐后的数据做原始数据 进行加密
	AES_set_encrypt_key(key, keylength, &aes_key);
	AES_cbc_encrypt(plain, buf, len, &aes_key, iv, AES_ENCRYPT);

	*out = buf;
	*out_len = len;

	return 0;
}

/*
 * aes-cbc-128 aes-cbc-192 aes-cbc-256
 * padding zero
 * iv 必须为 AES_BLOCK_SIZE 大小
 * 补零的缺点是，如原始数据以0结尾则解密时无法区分
 */
int aes_cbc_pad_zero_en(const unsigned char *data, int data_len, 
		unsigned char **out, int *out_len, 
		const unsigned char *key, int keylength, 
		unsigned char *iv)
{
	unsigned int len;
	unsigned char *buf;
	AES_KEY aes_key;

	if (keylength != 128 &&
			keylength != 192 &&
			keylength != 256) {
		fprintf(stderr, "keylength must be 128/192/256\n");
		return -1;
	}

	// 对输入数据以 AES_BLOCK_SIZE 向上对齐
	// 初始化数据为0，即padding zero
	len = ((data_len + AES_BLOCK_SIZE) / AES_BLOCK_SIZE) * AES_BLOCK_SIZE;
	buf = (unsigned char *)malloc(len);
	memset(buf, 0x0, len);
		
	AES_set_encrypt_key(key, keylength, &aes_key);
	AES_cbc_encrypt(data, buf, data_len, &aes_key, iv, AES_ENCRYPT);

	*out = buf;
	*out_len = len;

	return 0;
}

int aes_cbc_pad_zero_de(const unsigned char *data, int data_len, 
		unsigned char **out, int *out_len, 
		const unsigned char *key, int keylength, 
		unsigned char *iv)
{
	unsigned int len;
	unsigned char *buf;
	AES_KEY aes_key;

	if (keylength != 128 &&
			keylength != 192 &&
			keylength != 256) {
		fprintf(stderr, "keylength must be 128/192/256\n");
		return -1;
	}

	// 解密数据长度一定小于等于密文长度
	len = data_len;
	buf = (unsigned char *)malloc(len);
	memset(buf, 0x0, len);
		
	AES_set_decrypt_key(key, keylength, &aes_key);
	AES_cbc_encrypt(data, buf, data_len, &aes_key, iv, AES_DECRYPT);

	*out = buf;

	int i;
	for (i = len ; i > 0; i--) {
		if (buf[i - 1] != 0)
			break;	
	}

	*out_len = i;

	return 0;
}


// main entrypoint
int test_aes(int argc, char **argv)
{
    int keylength;
    printf("Give a key length [only 128 or 192 or 256!]:\n");
    scanf("%d", &keylength);
 
    /* generate a key with a given length */
    unsigned char aes_key[keylength/8];
    memset(aes_key, 0, keylength/8);
    if (!RAND_bytes(aes_key, keylength/8)) {
        exit(-1);
    }
 
    size_t inputslength = 0;
    printf("Give an input's length:\n");
    scanf("%lu", &inputslength);
 
    /* generate input with a given length */
    unsigned char aes_input[inputslength];
    memset(aes_input, 'X', inputslength);
 
    /* init vector */
	// 注意 iv 每次调用 AES_cbc_encrypt 时 iv都会被改变，
	// 所以要备份初始iv
    unsigned char iv_enc[AES_BLOCK_SIZE], iv_dec[AES_BLOCK_SIZE];
    RAND_bytes(iv_enc, AES_BLOCK_SIZE);
    memcpy(iv_dec, iv_enc, AES_BLOCK_SIZE);
 
	unsigned char *en;
	int en_len;

	aes_cbc_pad_pkcs7_en(aes_input, inputslength, 
		&en, &en_len, 
		aes_key, keylength, 
		iv_enc);

	printf("en_len : %d\n", en_len);

	unsigned char *de;
	int de_len;

	// iv_dec 一定要保证没有被修改过
	// 即没有参与 AES_cbc_encrypt 
	aes_cbc_pad_pkcs7_de(en, en_len, 
		&de, &de_len, 
		aes_key, keylength, 
		iv_dec);

    printf("original:\t");
    hex_print(aes_input, sizeof(aes_input));
 
    printf("encrypt:\t");
    hex_print(en, en_len);
 
    printf("decrypt:\t");
    hex_print(de, de_len);

    return 0;
}

