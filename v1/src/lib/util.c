#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

inline void mprintf(int level, const char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	switch (level)
	{
		case LINFO:
			printf("[Info] ");
			break;
		case LWARN:
			printf("[Warn] ");
			break;
		case LERROR:
			printf("[Error] ");
			break;
		case LFATAL:
			printf("[Fatal] ");
			break;
		default:
			break;
	}
	vprintf(fmt, ap);
	va_end(ap);
	printf("\n");
}

