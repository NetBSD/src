/* $NetBSD: strlcpy.S,v 1.4.8.3 2014/08/19 23:45:12 tls Exp $ */

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
