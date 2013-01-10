/* $NetBSD: strlcpy.S,v 1.4 2013/01/10 06:52:35 matt Exp $ */

#define STRLCPY

#if defined(_STANDALONE)
#include "strcpy_naive.S"
#else
#include "strcpy_arm.S"
#endif
