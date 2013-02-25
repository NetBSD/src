/* $NetBSD: strcat.S,v 1.1.6.2 2013/02/25 00:23:57 tls Exp $ */

#if defined(_STANDALONE)
#include "strcat_naive.S"
#else
#include "strcat_arm.S"
#endif
