/* $NetBSD: strncpy.S,v 1.2 2013/01/10 02:13:49 matt Exp $ */

#define STRNCPY

#if defined(_STANDALONE)
#include "strcpy_naive.S"
#else
#include "strcpy_arm.S"
#endif
