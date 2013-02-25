/* $NetBSD: strcpy.S,v 1.4.8.2 2013/02/25 00:23:57 tls Exp $ */

#if defined(_STANDALONE)
#include "strcpy_naive.S"
#else
#include "strcpy_arm.S"
#endif
