/*	$NetBSD: endian.h,v 1.2 2000/03/16 15:09:35 mycroft Exp $	*/

#if 1
#define _BYTE_ORDER _BIG_ENDIAN
#else
#define _BYTE_ORDER _LITTLE_ENDIAN
#endif
#include <sh3/endian.h>
