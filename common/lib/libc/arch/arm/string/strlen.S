/* $NetBSD: strlen.S,v 1.2.8.2 2013/02/25 00:23:58 tls Exp $ */

#ifdef _STANDALONE
#include "strlen_naive.S"
#else
#include "strlen_arm.S"
#endif
