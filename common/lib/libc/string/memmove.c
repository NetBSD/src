/*	$NetBSD: memmove.c,v 1.1.50.1 2014/05/22 11:26:30 yamt Exp $	*/

#define MEMMOVE
#include "bcopy.c"

#if defined(__ARM_EABI__)
__strong_alias(__aeabi_memmove, memmove)
__strong_alias(__aeabi_memmove4, memmove)
__strong_alias(__aeabi_memmove8, memmove)
#endif
