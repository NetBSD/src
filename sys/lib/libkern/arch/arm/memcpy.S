/*	$NetBSD: memcpy.S,v 1.4.2.2 2004/09/18 14:53:53 skrll Exp $	*/

#if !defined(__XSCALE__) || defined(_STANDALONE)
#include "memcpy_arm.S"
#else
#include "memcpy_xscale.S"
#endif
