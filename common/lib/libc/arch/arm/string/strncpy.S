/* $NetBSD: strncpy.S,v 1.2.8.3 2014/08/19 23:45:12 tls Exp $ */

#define STRNCPY

#if defined(_STANDALONE)
#if defined(__thumb__)
#include "strcpy_thumb.S"
#else
#include "strcpy_naive.S"
#endif
#else
#include "strcpy_arm.S"
#endif
