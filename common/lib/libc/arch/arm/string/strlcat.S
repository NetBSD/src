/* $NetBSD: strlcat.S,v 1.2.10.1 2017/03/20 06:51:38 pgoyette Exp $ */

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
