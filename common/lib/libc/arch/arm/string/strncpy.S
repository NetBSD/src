/* $NetBSD: strncpy.S,v 1.2.2.3 2014/05/22 11:26:28 yamt Exp $ */

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
