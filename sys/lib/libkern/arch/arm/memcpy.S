/*	$NetBSD: memcpy.S,v 1.4.2.3 2004/09/21 13:36:06 skrll Exp $	*/

#if !defined(__XSCALE__) || defined(_STANDALONE)
#include "memcpy_arm.S"
#else
#include "memcpy_xscale.S"
#endif
