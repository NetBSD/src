/* $NetBSD: strchr.S,v 1.3 2013/08/19 01:08:29 matt Exp $ */

#include <machine/asm.h>

STRONG_ALIAS(index, strchr)

#if defined(_STANDALONE) || defined(__thumb__)
#include "strchr_naive.S"
#else
#include "strchr_arm.S"
#endif
