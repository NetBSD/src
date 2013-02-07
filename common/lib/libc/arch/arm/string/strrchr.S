/* $NetBSD: strrchr.S,v 1.2.2.2 2013/02/07 07:06:02 matt Exp $ */

#include <machine/asm.h>

STRONG_ALIAS(rindex, strrchr)  

#if defined(_STANDALONE)
#include "strrchr_naive.S"
#else
#include "strrchr_arm.S"
#endif
