/* $NetBSD: strrchr.S,v 1.1 2013/01/23 05:46:17 matt Exp $ */

#if defined(_STANDALONE)
#include "strrchr_naive.S"
#else
#include "strrchr_arm.S"
#endif
