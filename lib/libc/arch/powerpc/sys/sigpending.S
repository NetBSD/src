/*	$NetBSD: sigpending.S,v 1.4 1998/10/03 12:30:38 tsubai Exp $	*/

#include "SYS.h"

ENTRY(sigpending)
	mr	5,3			# save pointer
	li	0,SYS_compat_13_sigpending13
	sc				# sigpending()
	bso	1f
	stw	3,0(5)			# store return value
	li	3,0			# and return 0
	blr
1:
	b	_ASM_LABEL(cerror)
