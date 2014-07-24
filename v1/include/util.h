//
#ifndef __UTIL_H__
#define __UTIL_H__

#define LINFO 0
#define LWARN 1
#define LERROR 2
#define LFATAL 3

void mprintf(int level, const char* fmt, ...);

#endif
