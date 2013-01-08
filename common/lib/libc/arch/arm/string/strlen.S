/* $NetBSD: strlen.S,v 1.2 2013/01/08 13:21:05 matt Exp $ */

#ifdef _STANDALONE
#include "strlen_naive.S"
#else
#include "strlen_arm.S"
#endif
