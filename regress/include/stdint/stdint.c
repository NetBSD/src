/* $NetBSD: stdint.c,v 1.1.2.2 2007/11/06 23:12:14 matt Exp $ */

#include <limits.h>
#include <stdint.h>

#if !(CHAR_MIN < UCHAR_MAX)
#error CHAR
#endif

#if !(SHRT_MIN < USHRT_MAX)
#error SHRT
#endif

#if !(INT8_MIN < UINT8_MAX)
#error INT8
#endif

#if !(INT16_MIN < UINT16_MAX)
#error INT16
#endif
