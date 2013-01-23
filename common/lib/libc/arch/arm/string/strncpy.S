/* $NetBSD: strncpy.S,v 1.2.2.2 2013/01/23 00:04:06 yamt Exp $ */

#define STRNCPY

#if defined(_STANDALONE)
#include "strcpy_naive.S"
#else
#include "strcpy_arm.S"
#endif
