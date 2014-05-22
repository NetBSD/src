/*	$NetBSD: memcpy.c,v 1.1.50.1 2014/05/22 11:26:30 yamt Exp $	*/

#define MEMCOPY
#include "bcopy.c"

#if defined(__ARM_EABI__)
__strong_alias(__aeabi_memcpy, memcpy)
#endif
