/*	$NetBSD: sigsuspend.S,v 1.8 2002/07/30 06:08:01 matt Exp $	*/

#include "SYS.h"

WARN_REFERENCES(sigsuspend, \
    "warning: reference to compatibility sigsuspend(); include <signal.h> for correct reference")

ENTRY(sigsuspend)
	lwz	%r3,0(%r3)			# indirect to mask arg
	li	%r0,SYS_compat_13_sigsuspend13
	sc
	b	PIC_PLT(_C_LABEL(__cerror))	# always terminates with EINTR
