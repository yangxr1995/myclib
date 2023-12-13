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

#endif
