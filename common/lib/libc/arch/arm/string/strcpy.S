/* $NetBSD: strcpy.S,v 1.4.4.2 2013/02/07 07:06:01 matt Exp $ */

#if defined(_STANDALONE)
#include "strcpy_naive.S"
#else
#include "strcpy_arm.S"
#endif
