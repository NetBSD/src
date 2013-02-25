/* $NetBSD: strchr.S,v 1.2.6.2 2013/02/25 00:23:57 tls Exp $ */

#include <machine/asm.h>

STRONG_ALIAS(index, strchr)

#if defined(_STANDALONE)
#include "strchr_naive.S"
#else
#include "strchr_arm.S"
#endif
