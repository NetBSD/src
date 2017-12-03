/* $NetBSD: int_const.h,v 1.1.18.2 2017/12/03 11:36:39 jdolecek Exp $ */

#ifndef __INTMAX_C_SUFFIX__

#define __INT8_C_SUFFIX__
#define __INT16_C_SUFFIX__
#define __INT32_C_SUFFIX__
#define __INT64_C_SUFFIX__	LL

#define __UINT8_C_SUFFIX__
#define __UINT16_C_SUFFIX__
#define __UINT32_C_SUFFIX__
#define __UINT64_C_SUFFIX__	ULL

#define __INTMAX_C_SUFFIX__	LL
#define __UINTMAX_C_SUFFIX__	ULL

#endif

#include <sys/common_int_const.h>
