/*	$NetBSD: pipe.S,v 1.5 1999/01/14 22:48:21 kleink Exp $	*/

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
	b	PIC_PLT(_C_LABEL(__cerror))
