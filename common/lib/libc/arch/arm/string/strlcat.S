/* $NetBSD: strlcat.S,v 1.1.2.2 2013/02/07 07:06:01 matt Exp $ */

#include <machine/asm.h>

#ifdef _LIBC
WEAK_ALIAS(strlcat, _strlcat)
#include "namespace.h"
#endif

#ifdef _STANDALONE
#include "strlcat_naive.S"
#else
#include "strlcat_arm.S"
#endif
