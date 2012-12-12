/*	$NetBSD: memcpy.S,v 1.2 2012/12/12 15:51:09 matt Exp $	*/

#if !defined(_ARM_ARCH_DWORK_OK) || defined(_STANDALONE)
#include "memcpy_arm.S"
#else
#include "memcpy_xscale.S"
#endif
