/*	$NetBSD: memcpy.S,v 1.4 2013/02/15 22:35:39 matt Exp $	*/

#include <arm/cdefs.h>

#if !defined(_ARM_ARCH_DWORD_OK) || defined(_STANDALONE)
#include "memcpy_arm.S"
#else
#include "memcpy_xscale.S"
#endif
