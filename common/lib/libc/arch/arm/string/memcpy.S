/*	$NetBSD: memcpy.S,v 1.1.50.2 2014/05/22 11:26:28 yamt Exp $	*/

#include <arm/cdefs.h>

#if !defined(__XSCALE__) || defined(_STANDALONE)
#include "memcpy_arm.S"
#else
#include "memcpy_xscale.S"
#endif

#if defined(__ARM_EABI__)
STRONG_ALIAS(__aeabi_memcpy, memcpy)
#endif
