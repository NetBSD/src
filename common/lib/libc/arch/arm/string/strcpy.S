/* $NetBSD: strcpy.S,v 1.5.26.2 2020/04/21 19:37:42 martin Exp $ */

#if defined(_STANDALONE)
#ifdef __thumb__
#include "strcpy_thumb.S"
#else
#include "strcpy_naive.S"
#endif
#else
#include "strcpy_arm.S"
#endif
