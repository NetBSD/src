/* $NetBSD: strcpy.S,v 1.4 2013/01/10 02:13:49 matt Exp $ */

#if defined(_STANDALONE)
#include "strcpy_naive.S"
#else
#include "strcpy_arm.S"
#endif
