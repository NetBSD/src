/*	$NetBSD: compat_sigsuspend13.S,v 1.1 2006/02/18 00:21:28 matt Exp $	*/

#include "SYS.h"

WARN_REFERENCES(sigsuspend, \
    "warning: reference to compatibility sigsuspend(); include <signal.h> for correct reference")

ENTRY(sigsuspend)
	lwz	%r3,0(%r3)			# indirect to mask arg
	li	%r0,SYS_compat_13_sigsuspend13
	sc
	b	PIC_PLT(_C_LABEL(__cerror))	# always terminates with EINTR
