/*	$NetBSD: sigsuspend.S,v 1.5 1998/11/24 11:14:57 tsubai Exp $	*/

#include "SYS.h"

ENTRY(sigsuspend)
	lwz	3,0(3)			# indirect to mask arg
	li	0,SYS_compat_13_sigsuspend13
	sc
	b	PIC_PLT(_ASM_LABEL(cerror))	# always terminates with EINTR
