/*	$NetBSD: sigpending.S,v 1.2 1998/05/25 15:28:03 ws Exp $	*/

#include "SYS.h"

ENTRY(sigpending)
	mr	5,3			# save pointer
	li	0,SYS_sigpending
	sc				# sigpending()
	bso	1f
	stw	3,0(5)			# store return value
	li	3,0			# and return 0
	blr
1:
	b	cerror
