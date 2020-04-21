/* $NetBSD: strlcat.S,v 1.4.14.2 2020/04/21 19:37:42 martin Exp $ */

#include <machine/asm.h>

#ifdef _LIBC
WEAK_ALIAS(strlcat, _strlcat)
#define strlcat _strlcat
#endif

#if defined(_STANDALONE) && 0	/* arm version is always smaller */
#include "strlcat_naive.S"
#else
#include "strlcat_arm.S"
#endif
