//
#ifndef __COMPRESS_H__
#define __COMPRESS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>

#include "minilzo.h"

// allocate memory
lzo_align_t* __LZO_MMODEL working_memory;

int compress_init()
{
	working_memory = (lzo_align_t*)malloc(LZO1X_1_MEM_COMPRESS + (sizeof(lzo_align_t) - 1));
	//
	if (working_memory == NULL)
		return -1;
	if (lzo_init() != LZO_E_OK)
		return -1;
	return 0;
}

int compress(unsigned char* output, size_t* outputlen, const unsigned char* input, size_t inputlen)
{
	lzo_uint outlen;
	int ret = lzo1x_1_compress(input, inputlen,
		output, &outlen, &working_memory);
	*outputlen = outlen;
	return ret;
}

int decompress(unsigned char* output, unsigned int* outputlen, const unsigned char* input, unsigned int inputlen)
{
	lzo_uint outlen;
	int ret = lzo1x_decompress(input, inputlen,
		output, &outlen, &working_memory);
	*outputlen = outlen;
	return ret;
}

#ifdef __cplusplus
}
#endif

#endif

///*
// * Step 4: decompress again, now going from 'out' to 'in'
// */
//    new_len = in_len;
//    r = lzo1x_decompress(out,out_len,in,&new_len,NULL);
//    if (r == LZO_E_OK && new_len == in_len)
//        printf("decompressed %lu bytes back into %lu bytes\n",
//            (unsigned long) out_len, (unsigned long) in_len);
//    else
//    {
//        /* this should NEVER happen */
//        printf("internal error - decompression failed: %d\n", r);
//        return 1;
//    }
//
//    printf("\nminiLZO simple compression test passed.\n");
//    return 0;
//}
//
///*
//vi:ts=4:et
//*/
//
