/*	$NetBSD: memcpy.S,v 1.6 2003/10/13 19:59:24 scw Exp $	*/

#if !defined(__XSCALE__) || defined(_STANDALONE)
#include "memcpy_arm.S"
#else
#include "memcpy_xscale.S"
#endif
