/* $NetBSD: strcat.S,v 1.1.14.2 2014/05/22 11:26:28 yamt Exp $ */

#if defined(_STANDALONE)
#include "strcat_naive.S"
#else
#include "strcat_arm.S"
#endif
