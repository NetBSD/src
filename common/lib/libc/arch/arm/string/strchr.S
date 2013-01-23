/* $NetBSD: strchr.S,v 1.1 2013/01/23 05:46:17 matt Exp $ */

#if defined(_STANDALONE)
#include "strchr_naive.S"
#else
#include "strchr_arm.S"
#endif
