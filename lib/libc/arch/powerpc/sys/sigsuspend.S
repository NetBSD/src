/*	$NetBSD: sigsuspend.S,v 1.2 1998/05/25 15:28:04 ws Exp $	*/

#include "SYS.h"

ENTRY(sigsuspend)
	lwz	3,0(3)			# indirect to mask arg
	li	0,SYS_sigsuspend
	sc
	b	cerror			# always terminates with EINTR
