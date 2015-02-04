/*	$NetBSD: pipe.S,v 1.1.64.1 2015/02/04 22:00:24 martin Exp $	*/

#include "SYS.h"

#ifdef WEAK_ALIAS
WEAK_ALIAS(pipe, _pipe)
#endif

ENTRY(_pipe)
	mr	%r5,%r3		# save pointer
	_DOSYSCALL(pipe)	# assume, that r5 is kept
	bso	1f
	stint	%r3,0(%r5)	# success, store fds
	stint	%r4,4(%r5)
	li	%r3,0
	blr			# and return 0
1:
	BRANCH_TO_CERROR()
END(_pipe)
