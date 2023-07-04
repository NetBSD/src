/* $NetBSD: int_const.h,v 1.2 2023/07/04 01:02:50 riastradh Exp $ */

#ifndef __INTMAX_C_SUFFIX__

#define __INT8_C_SUFFIX__
#define __INT16_C_SUFFIX__
#define __INT32_C_SUFFIX__
#ifdef _LP64
#define __INT64_C_SUFFIX__	L
#else
#define __INT64_C_SUFFIX__	LL
#endif

#define __UINT8_C_SUFFIX__
#define __UINT16_C_SUFFIX__
#define __UINT32_C_SUFFIX__
#ifdef _LP64
#define __UINT64_C_SUFFIX__	UL
#else
#define __UINT64_C_SUFFIX__	ULL
#endif

#ifdef _LP64
#define __INTMAX_C_SUFFIX__	L
#define __UINTMAX_C_SUFFIX__	UL
#else
#define __INTMAX_C_SUFFIX__	LL
#define __UINTMAX_C_SUFFIX__	ULL
#endif

#endif

#include <sys/common_int_const.h>
