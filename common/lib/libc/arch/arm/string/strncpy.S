/* $NetBSD: strncpy.S,v 1.2.8.2 2013/02/25 00:23:59 tls Exp $ */

#define STRNCPY

#if defined(_STANDALONE)
#include "strcpy_naive.S"
#else
#include "strcpy_arm.S"
#endif
