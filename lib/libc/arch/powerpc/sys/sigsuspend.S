/*	$NetBSD: sigsuspend.S,v 1.3 1998/05/25 15:35:06 ws Exp $	*/

#include "SYS.h"

ENTRY(sigsuspend)
	lwz	3,0(3)			# indirect to mask arg
	li	0,SYS_sigsuspend
	sc
	b	_ASM_LABEL(cerror)	# always terminates with EINTR
