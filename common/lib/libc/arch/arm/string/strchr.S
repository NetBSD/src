/* $NetBSD: strchr.S,v 1.4.28.2 2020/04/21 19:37:42 martin Exp $ */

#include <machine/asm.h>

STRONG_ALIAS(index, strchr)

#if defined(_STANDALONE) || (defined(__thumb__) && !defined(_ARM_ARCH_T2))
#include "strchr_naive.S"
#else
#include "strchr_arm.S"
#endif
