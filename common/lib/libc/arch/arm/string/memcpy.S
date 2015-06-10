/*	$NetBSD: memcpy.S,v 1.6.4.1 2015/06/10 20:57:22 snj Exp $	*/

#include <arm/cdefs.h>

#if !defined(__XSCALE__) || defined(_STANDALONE)
#include "memcpy_arm.S"
#else
#include "memcpy_xscale.S"
#endif

#if defined(__ARM_EABI__)
STRONG_ALIAS(__aeabi_memcpy, memcpy)
STRONG_ALIAS(__aeabi_memcpy4, memcpy)
STRONG_ALIAS(__aeabi_memcpy8, memcpy)
#endif
