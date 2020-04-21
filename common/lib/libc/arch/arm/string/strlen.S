/* $NetBSD: strlen.S,v 1.4.26.2 2020/04/21 19:37:42 martin Exp $ */

#include <arm/cdefs.h>

#if defined(_STANDALONE) || (defined(__thumb__) && !defined(_ARM_ARCH_T2))
#include "strlen_naive.S"
#else
#include "strlen_arm.S"
#endif
