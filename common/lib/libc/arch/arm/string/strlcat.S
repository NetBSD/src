/* $NetBSD: strlcat.S,v 1.1.6.3 2014/08/19 23:45:12 tls Exp $ */

#include <machine/asm.h>

#ifdef _LIBC
WEAK_ALIAS(strlcat, _strlcat)
#include "namespace.h"
#endif

#if defined(_STANDALONE) && 0	/* arm version is always smaller */
#include "strlcat_naive.S"
#else
#include "strlcat_arm.S"
#endif
