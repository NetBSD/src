/* $NetBSD: strcpy.S,v 1.3 2013/01/08 13:21:05 matt Exp $ */

#ifdef _STANDALONE
#include "strcpy_naive.S"
#else
#include "strcpy_arm.S"
#endif
