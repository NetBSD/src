/*	$NetBSD: sigsuspend.S,v 1.4 1998/10/03 12:30:39 tsubai Exp $	*/

#include "SYS.h"

ENTRY(sigsuspend)
	lwz	3,0(3)			# indirect to mask arg
	li	0,SYS_compat_13_sigsuspend13
	sc
	b	_ASM_LABEL(cerror)	# always terminates with EINTR
