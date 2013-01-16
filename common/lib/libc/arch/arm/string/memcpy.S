/*	$NetBSD: memcpy.S,v 1.1.50.1 2013/01/16 05:25:53 yamt Exp $	*/

#if !defined(_ARM_ARCH_DWORK_OK) || defined(_STANDALONE)
#include "memcpy_arm.S"
#else
#include "memcpy_xscale.S"
#endif
