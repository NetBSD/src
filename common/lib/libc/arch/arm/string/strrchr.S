/* $NetBSD: strrchr.S,v 1.3 2013/08/19 01:08:29 matt Exp $ */

#include <machine/asm.h>

STRONG_ALIAS(rindex, strrchr)  

#if defined(_STANDALONE) || defined(__thumb__)
#include "strrchr_naive.S"
#else
#include "strrchr_arm.S"
#endif
