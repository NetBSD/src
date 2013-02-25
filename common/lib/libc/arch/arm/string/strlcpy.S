/* $NetBSD: strlcpy.S,v 1.4.8.2 2013/02/25 00:23:58 tls Exp $ */

#define STRLCPY

#if defined(_STANDALONE)
#include "strcpy_naive.S"
#else
#include "strcpy_arm.S"
#endif
