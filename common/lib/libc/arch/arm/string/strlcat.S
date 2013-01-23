/* $NetBSD: strlcat.S,v 1.1 2013/01/23 07:56:09 matt Exp $ */

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
