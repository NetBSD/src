/*	$NetBSD: memcpy.S,v 1.1.56.1 2013/02/25 00:23:55 tls Exp $	*/

#include <arm/cdefs.h>

#if !defined(_ARM_ARCH_DWORD_OK) || defined(_STANDALONE)
#include "memcpy_arm.S"
#else
#include "memcpy_xscale.S"
#endif
