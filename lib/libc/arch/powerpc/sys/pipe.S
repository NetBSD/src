/*	$NetBSD: pipe.S,v 1.3 1998/05/25 15:35:06 ws Exp $	*/

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
	b	_ASM_LABEL(cerror)
