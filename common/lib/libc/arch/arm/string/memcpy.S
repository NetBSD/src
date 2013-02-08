/*	$NetBSD: memcpy.S,v 1.3 2013/02/08 01:41:35 matt Exp $	*/

#if !defined(_ARM_ARCH_DWORD_OK) || defined(_STANDALONE)
#include "memcpy_arm.S"
#else
#include "memcpy_xscale.S"
#endif
