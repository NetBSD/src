/* $NetBSD: strlcat.S,v 1.4 2017/01/14 03:00:13 christos Exp $ */

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
