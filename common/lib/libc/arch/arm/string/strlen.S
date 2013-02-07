/* $NetBSD: strlen.S,v 1.2.4.2 2013/02/07 07:06:01 matt Exp $ */

#ifdef _STANDALONE
#include "strlen_naive.S"
#else
#include "strlen_arm.S"
#endif
