/*	$NetBSD: sigsuspend.S,v 1.7 1999/01/14 22:48:22 kleink Exp $	*/

#include "SYS.h"

WARN_REFERENCES(sigsuspend, \
    "warning: reference to compatibility sigsuspend(); include <signal.h> for correct reference")

ENTRY(sigsuspend)
	lwz	3,0(3)			# indirect to mask arg
	li	0,SYS_compat_13_sigsuspend13
	sc
	b	PIC_PLT(_C_LABEL(__cerror))	# always terminates with EINTR
