/*	$NetBSD: pipe.S,v 1.4 1998/11/24 11:14:57 tsubai Exp $	*/

#include "SYS.h"

ENTRY(pipe)
	mr	5,3		# save pointer
	li	0,SYS_pipe
	sc			# assume, that r5 is kept
	bso	1f
	stw	3,0(5)		# success, store fds
	stw	4,4(5)
	li	3,0
	blr			# and return 0
1:
	b	PIC_PLT(_ASM_LABEL(cerror))
