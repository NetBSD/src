/*	$NetBSD: memcpy.S,v 1.6 2013/12/02 21:21:33 joerg Exp $	*/

#include <arm/cdefs.h>

#if !defined(__XSCALE__) || defined(_STANDALONE)
#include "memcpy_arm.S"
#else
#include "memcpy_xscale.S"
#endif

#if defined(__ARM_EABI__)
STRONG_ALIAS(__aeabi_memcpy, memcpy)
#endif
