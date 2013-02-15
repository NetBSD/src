/*	$NetBSD: memcpy.S,v 1.1.54.3 2013/02/15 22:38:51 matt Exp $	*/

#include <arm/cdefs.h>

#if !defined(_ARM_ARCH_DWORD_OK) || defined(_STANDALONE)
#include "memcpy_arm.S"
#else
#include "memcpy_xscale.S"
#endif
