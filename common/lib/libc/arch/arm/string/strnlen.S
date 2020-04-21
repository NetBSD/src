/* $NetBSD: strnlen.S,v 1.3.28.2 2020/04/21 19:37:42 martin Exp $ */

#include <arm/cdefs.h>		/* for _ARM_ARCH_T2 */

#define	STRNLEN

#if defined(_STANDALONE) || (defined(__thumb__) && !defined(_ARM_ARCH_T2))
#include "strlen_naive.S"
#else
#include "strlen_arm.S"
#endif
