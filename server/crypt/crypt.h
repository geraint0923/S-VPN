#ifndef __CRYPT_H__
#define __CRYPT_H__

#ifdef __cplusplus
extern "C" {
#endif

struct code_table
{
	unsigned char encode[256];
	unsigned char decode[256];
};

void build_table(struct code_table* ct, const unsigned char* passmd5, const long long timestamp);

void svpn_encrypt(const struct code_table* ct, const void* input, void* output, unsigned int len);

void svpn_decrypt(const struct code_table* ct, const void* input, void* output, unsigned int len);

#ifdef __cplusplus
}
#endif

#endif
