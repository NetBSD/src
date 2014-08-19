/* $NetBSD: strnlen.S,v 1.1.6.3 2014/08/19 23:45:12 tls Exp $ */

#include <arm/cdefs.h>		/* for _ARM_ARCH_T2 */

#define	STRNLEN

#if defined(_STANDALONE) || (defined(__thumb__) && !defined(_ARM_ARCH_T2))
#include "strlen_naive.S"
#else
#include "strlen_arm.S"
#endif
