/* $NetBSD: strlen.S,v 1.2.8.3 2014/08/19 23:45:12 tls Exp $ */

#include <arm/cdefs.h>

#if defined(_STANDALONE) || (defined(__thumb__) && !defined(_ARM_ARCH_T2))
#include "strlen_naive.S"
#else
#include "strlen_arm.S"
#endif
