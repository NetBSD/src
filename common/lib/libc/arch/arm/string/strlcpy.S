/* $NetBSD: strlcpy.S,v 1.3 2013/01/10 02:14:42 matt Exp $ */

#define STRLCPY

#if defined(_STANDALONE) || 1
#include "strcpy_naive.S"
#else
#include "strcpy_arm.S"
#endif
