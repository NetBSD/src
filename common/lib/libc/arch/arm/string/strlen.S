/* $NetBSD: strlen.S,v 1.3 2013/08/19 01:08:29 matt Exp $ */

#if defined(_STANDALONE) || defined(__thumb__)
#include "strlen_naive.S"
#else
#include "strlen_arm.S"
#endif
