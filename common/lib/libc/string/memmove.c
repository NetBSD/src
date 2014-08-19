/*	$NetBSD: memmove.c,v 1.1.56.1 2014/08/19 23:45:14 tls Exp $	*/

#define MEMMOVE
#include "bcopy.c"

#if defined(__ARM_EABI__)
__strong_alias(__aeabi_memmove, memmove)
__strong_alias(__aeabi_memmove4, memmove)
__strong_alias(__aeabi_memmove8, memmove)
#endif
