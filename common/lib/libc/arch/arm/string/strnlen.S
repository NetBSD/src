/* $NetBSD: strnlen.S,v 1.1 2013/01/23 07:56:09 matt Exp $ */

#define	STRNLEN

#ifdef _STANDALONE
#include "strlen_naive.S"
#else
#include "strlen_arm.S"
#endif
