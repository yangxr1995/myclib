#include <openssl/bn.h>
#include <openssl/pem.h>
#include <openssl/err.h> /* errors */
#include <openssl/rsa.h>
#include <openssl/ssl.h> /* core library */
#include <openssl/bio.h> /* BasicInput/Output streams */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

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


#if 0
int test()
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
#endif