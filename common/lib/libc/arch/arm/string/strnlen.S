/* $NetBSD: strnlen.S,v 1.3.4.2 2014/05/22 11:26:28 yamt Exp $ */

#include <arm/cdefs.h>		/* for _ARM_ARCH_T2 */

#define	STRNLEN

#if defined(_STANDALONE) || (defined(__thumb__) && !defined(_ARM_ARCH_T2))
#include "strlen_naive.S"
#else
#include "strlen_arm.S"
#endif
