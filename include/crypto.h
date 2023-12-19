#ifndef __RSA_H_
#define __RSA_H_

int generate_rsa_key_to_file(const char *pub_file, 
		const char *pri_file, unsigned int bits);

int rsa_public_en(const unsigned char *data, int len, 
		unsigned char **ciper, unsigned int *ciper_len,
		const char *pub_file);

int rsa_private_de(const unsigned char *ciper, 
		unsigned int ciper_len, unsigned char **plain,
		const char *pri_file);

#define PUB_PEM "pub.pem"
#define PRI_PEM "pri.pem"


unsigned char* base64_decode(const char* input, int length, int* outLength);
char* base64_encode(const unsigned char* input, int length);

int md5_str(const char *data, unsigned int len, char *out, int out_len);

int aes_cbc_pad_pkcs7_de(unsigned char *data, int data_len, 
		unsigned char **out, int *out_len, 
		const unsigned char *key, int keylength, 
		unsigned char *iv);

int aes_cbc_pad_pkcs7_en(const unsigned char *data, int data_len, 
		unsigned char **out, int *out_len, 
		const unsigned char *key, int keylength, 
		unsigned char *iv);

#endif
