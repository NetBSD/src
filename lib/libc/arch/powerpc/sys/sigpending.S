/*	$NetBSD: sigpending.S,v 1.5 1998/11/24 11:14:57 tsubai Exp $	*/

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
	b	PIC_PLT(_ASM_LABEL(cerror))
