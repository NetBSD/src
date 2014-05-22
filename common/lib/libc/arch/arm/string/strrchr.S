/* $NetBSD: strrchr.S,v 1.4.4.2 2014/05/22 11:26:28 yamt Exp $ */

#include <machine/asm.h>

STRONG_ALIAS(rindex, strrchr)  

#if defined(_STANDALONE) || (defined(__thumb__) && !defined(_ARM_ARCH_T2))
#include "strrchr_naive.S"
#else
#include "strrchr_arm.S"
#endif
