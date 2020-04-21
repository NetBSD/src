/* $NetBSD: strlcpy.S,v 1.5.26.2 2020/04/21 19:37:42 martin Exp $ */

#define STRLCPY

#if defined(_STANDALONE)
#if defined(__thumb__)
#include "strcpy_thumb.S"
#else
#include "strcpy_naive.S"
#endif
#else
#include "strcpy_arm.S"
#endif
