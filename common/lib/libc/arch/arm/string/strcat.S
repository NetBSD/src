/* $NetBSD: strcat.S,v 1.1.2.2 2013/02/07 07:06:00 matt Exp $ */

#if defined(_STANDALONE)
#include "strcat_naive.S"
#else
#include "strcat_arm.S"
#endif
