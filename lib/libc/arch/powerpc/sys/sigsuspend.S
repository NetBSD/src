/*	$NetBSD: sigsuspend.S,v 1.6 1998/12/02 01:01:04 thorpej Exp $	*/

#include "SYS.h"

WARN_REFERENCES(sigsuspend, \
    "warning: reference to compatibility sigsuspend(); include <signal.h> for correct reference")

ENTRY(sigsuspend)
	lwz	3,0(3)			# indirect to mask arg
	li	0,SYS_compat_13_sigsuspend13
	sc
	b	PIC_PLT(_ASM_LABEL(cerror))	# always terminates with EINTR
