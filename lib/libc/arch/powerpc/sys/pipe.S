/*	$NetBSD: pipe.S,v 1.2 1998/05/25 15:28:03 ws Exp $	*/

#include "SYS.h"

ENTRY(pipe)
	mr	5, 3		# save pointer
	li	0,SYS_pipe
	sc			# assume, that r5 is kept
	bso	1f
	stw	3,0(5)		# success, store fds
	stw	4,4(5)
	li	3,0
	blr			# and return 0
1:
	b	cerror
