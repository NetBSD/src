/* $NetBSD: strnlen.S,v 1.1.6.2 2013/02/25 00:23:59 tls Exp $ */

#define	STRNLEN

#ifdef _STANDALONE
#include "strlen_naive.S"
#else
#include "strlen_arm.S"
#endif
