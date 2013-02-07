/*	$NetBSD: memcpy.S,v 1.1.54.1 2013/02/07 07:05:59 matt Exp $	*/

#if !defined(_ARM_ARCH_DWORK_OK) || defined(_STANDALONE)
#include "memcpy_arm.S"
#else
#include "memcpy_xscale.S"
#endif
