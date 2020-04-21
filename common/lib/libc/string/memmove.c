/*	$NetBSD: memmove.c,v 1.2.26.2 2020/04/21 19:37:51 martin Exp $	*/

#define MEMMOVE
#include "bcopy.c"

#if defined(__ARM_EABI__)
__strong_alias(__aeabi_memmove, memmove)
__strong_alias(__aeabi_memmove4, memmove)
__strong_alias(__aeabi_memmove8, memmove)
#endif
