/*	$NetBSD: memcpy.S,v 1.8.16.2 2020/04/21 19:37:42 martin Exp $	*/

#include <arm/cdefs.h>

#if !defined(__XSCALE__) || defined(_STANDALONE)
#include "memcpy_arm.S"
#else
#include "memcpy_xscale.S"
#endif

#if defined(__ARM_EABI__) && !defined(_RUMPKERNEL)
STRONG_ALIAS(__aeabi_memcpy, memcpy)
STRONG_ALIAS(__aeabi_memcpy4, memcpy)
STRONG_ALIAS(__aeabi_memcpy8, memcpy)
#endif
