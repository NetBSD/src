/*	$NetBSD: sigpending.S,v 1.8 2002/07/30 06:08:01 matt Exp $	*/

#include "SYS.h"

WARN_REFERENCES(sigpending, \
    "warning: reference to compatibility sigpending(); include <signal.h> for correct reference")

ENTRY(sigpending)
	mr	%r5,%r3			# save pointer
	li	%r0,SYS_compat_13_sigpending13
	sc				# sigpending()
	bso	1f
	stw	%r3,0(%r5)		# store return value
	li	%r3,0			# and return 0
	blr
1:
	b	PIC_PLT(_C_LABEL(__cerror))
