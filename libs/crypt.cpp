//
//
#include <memory.h>
#include "md5.h"
#include "crypt.h"

// password md5 must have a length of 16
void BuildTable(CodeTable* ct, const unsigned char* passmd5, const long long timestamp)
{
	// calc overall MD5
	unsigned char* buf[16 + sizeof(long long)];
	memcpy(buf, passmd5, 16);
	memcpy(buf + 16, &timestamp, sizeof(long long));
	unsigned char md[17];
	MD5Fast(buf, sizeof(buf), md);

	// build table
	const int PRIME_FOR_GEN = 47;

	for (int i = 0; i < 256; i++)
		ct->encode[i] = (unsigned char)i;
	for (int i = 0; i < 256; i++)
	{
		for (int j = 0; j < 16; j++)
		{
			int k = md[j] * PRIME_FOR_GEN;
			md[j] = (unsigned char)k;
			md[j + 1] += k >> 8;
		}
		unsigned char tmp = ct->encode[md[15]];
		ct->encode[md[15]] = ct->encode[i];
		ct->encode[i] = tmp;
	}
	// gen decode table
	for (int i = 0; i < 256; i++)
		ct->decode[ct->encode[i]] = (unsigned char)i;
}

void Encrypt(const CodeTable* ct, const void* input, void* output, unsigned int len)
{
	unsigned char* uinput = (unsigned char*)input;
	unsigned char* uoutput = (unsigned char*)output;

	for (unsigned int i = 0; i < len; i++)
		uoutput[i] = ct->encode[uinput[i]];
}

void Decrypt(const CodeTable* ct, const void* input, void* output, unsigned int len)
{
	unsigned char* uinput = (unsigned char*)input;
	unsigned char* uoutput = (unsigned char*)output;

	for (unsigned int i = 0; i < len; i++)
		uoutput[i] = ct->decode[uinput[i]];
}
