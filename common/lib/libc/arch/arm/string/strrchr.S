/* $NetBSD: strrchr.S,v 1.2.6.2 2013/02/25 00:23:59 tls Exp $ */

#include <machine/asm.h>

STRONG_ALIAS(rindex, strrchr)  

#if defined(_STANDALONE)
#include "strrchr_naive.S"
#else
#include "strrchr_arm.S"
#endif
