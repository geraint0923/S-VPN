#ifndef __CRYPT_H__
#define __CRYPT_H__

#ifdef __cplusplus
extern "C" {
#endif

#define IP_MAGIC_HEADER 0x45

struct CodeTable
{
	unsigned char encode[256];
	unsigned char decode[256];
};

void BuildTable(struct CodeTable* ct, const unsigned char* passmd5, unsigned char identity);

void Encrypt(const struct CodeTable* ct, const void* input, void* output, unsigned int len);

void Decrypt(const struct CodeTable* ct, const void* input, void* output, unsigned int len);

#ifdef __cplusplus
}
#endif

#endif
