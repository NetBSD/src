/*	$NetBSD: memcpy.c,v 1.2.26.2 2020/04/21 19:37:51 martin Exp $	*/

#define MEMCOPY
#include "bcopy.c"

#if defined(__ARM_EABI__)
__strong_alias(__aeabi_memcpy, memcpy)
#endif
