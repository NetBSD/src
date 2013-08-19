/* $NetBSD: strnlen.S,v 1.2 2013/08/19 01:08:29 matt Exp $ */

#define	STRNLEN

#if defined(_STANDALONE) || defined(__thumb__)
#include "strlen_naive.S"
#else
#include "strlen_arm.S"
#endif
