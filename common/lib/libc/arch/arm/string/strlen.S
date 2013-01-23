/* $NetBSD: strlen.S,v 1.2.2.2 2013/01/23 00:04:06 yamt Exp $ */

#ifdef _STANDALONE
#include "strlen_naive.S"
#else
#include "strlen_arm.S"
#endif
