/* $NetBSD: strncpy.S,v 1.2.4.2 2013/02/07 07:06:02 matt Exp $ */

#define STRNCPY

#if defined(_STANDALONE)
#include "strcpy_naive.S"
#else
#include "strcpy_arm.S"
#endif
