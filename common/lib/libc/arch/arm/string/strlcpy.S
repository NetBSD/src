/* $NetBSD: strlcpy.S,v 1.2 2013/01/10 02:13:49 matt Exp $ */

#define STRLCPY

#if defined(_STANDALONE)
#include "strcpy_naive.S"
#else
#include "strcpy_arm.S"
#endif
