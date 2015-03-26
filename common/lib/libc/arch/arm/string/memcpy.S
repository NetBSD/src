/*	$NetBSD: memcpy.S,v 1.7 2015/03/26 13:34:51 justin Exp $	*/

#include <arm/cdefs.h>

#if !defined(__XSCALE__) || defined(_STANDALONE)
#include "memcpy_arm.S"
#else
#include "memcpy_xscale.S"
#endif

#if defined(__ARM_EABI__) && !defined(_RUMPKERNEL)
STRONG_ALIAS(__aeabi_memcpy, memcpy)
#endif
