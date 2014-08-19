/*	$NetBSD: memcpy.c,v 1.1.56.1 2014/08/19 23:45:14 tls Exp $	*/

#define MEMCOPY
#include "bcopy.c"

#if defined(__ARM_EABI__)
__strong_alias(__aeabi_memcpy, memcpy)
#endif
