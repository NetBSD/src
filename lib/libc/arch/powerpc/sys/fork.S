/*	$NetBSD: fork.S,v 1.3.2.1 2002/08/01 03:28:02 nathanw Exp $	*/

/*
 * pid = fork();
 *
 * r4 == 0 in parent process, 1 in child process.
 * r3 == pid of child in parent, pid of parent in child.
 */

#include "SYS.h"

#ifdef WEAK_ALIAS
WEAK_ALIAS(fork, _fork)
#endif

_SYSCALL(_fork,fork)
	addi	%r4,%r4,-1	# from 1 to 0 in child, 0 to -1 in parent
	and	%r3,%r3,%r4	# return 0 in child, pid in parent
	blr
