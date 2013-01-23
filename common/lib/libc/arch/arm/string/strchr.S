/* $NetBSD: strchr.S,v 1.2 2013/01/23 07:45:46 matt Exp $ */

#include <machine/asm.h>

STRONG_ALIAS(index, strchr)

#if defined(_STANDALONE)
#include "strchr_naive.S"
#else
#include "strchr_arm.S"
#endif
