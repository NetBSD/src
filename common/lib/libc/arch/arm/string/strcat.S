/* $NetBSD: strcat.S,v 1.1.38.2 2020/04/21 19:37:42 martin Exp $ */

#if defined(_STANDALONE)
#include "strcat_naive.S"
#else
#include "strcat_arm.S"
#endif
