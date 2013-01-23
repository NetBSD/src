/* $NetBSD: strcpy.S,v 1.4.2.2 2013/01/23 00:04:06 yamt Exp $ */

#if defined(_STANDALONE)
#include "strcpy_naive.S"
#else
#include "strcpy_arm.S"
#endif
