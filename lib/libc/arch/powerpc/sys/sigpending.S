/*	$NetBSD: sigpending.S,v 1.6 1998/12/02 01:01:04 thorpej Exp $	*/

#include "SYS.h"

WARN_REFERENCES(sigpending, \
    "warning: reference to compatibility sigpending(); include <signal.h> for correct reference")

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
