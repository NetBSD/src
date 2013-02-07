/* $NetBSD: strnlen.S,v 1.1.2.2 2013/02/07 07:06:02 matt Exp $ */

#define	STRNLEN

#ifdef _STANDALONE
#include "strlen_naive.S"
#else
#include "strlen_arm.S"
#endif
