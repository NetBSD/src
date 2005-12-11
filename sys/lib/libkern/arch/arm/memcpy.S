/*	$NetBSD: memcpy.S,v 1.7 2005/12/11 12:24:42 christos Exp $	*/

#if !defined(__XSCALE__) || defined(_STANDALONE)
#include "memcpy_arm.S"
#else
#include "memcpy_xscale.S"
#endif
