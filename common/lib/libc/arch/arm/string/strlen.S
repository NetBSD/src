/* $NetBSD: strlen.S,v 1.2.2.3 2014/05/22 11:26:28 yamt Exp $ */

#include <arm/cdefs.h>

#if defined(_STANDALONE) || (defined(__thumb__) && !defined(_ARM_ARCH_T2))
#include "strlen_naive.S"
#else
#include "strlen_arm.S"
#endif
