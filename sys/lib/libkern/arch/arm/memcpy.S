/*	$NetBSD: memcpy.S,v 1.4.2.1 2004/08/03 10:53:42 skrll Exp $	*/

#if !defined(__XSCALE__) || defined(_STANDALONE)
#include "memcpy_arm.S"
#else
#include "memcpy_xscale.S"
#endif
