/* $NetBSD: strchr.S,v 1.2.2.2 2013/02/07 07:06:00 matt Exp $ */

#include <machine/asm.h>

STRONG_ALIAS(index, strchr)

#if defined(_STANDALONE)
#include "strchr_naive.S"
#else
#include "strchr_arm.S"
#endif
