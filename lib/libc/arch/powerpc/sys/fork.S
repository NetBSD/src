/*	$NetBSD: fork.S,v 1.2 1998/05/25 15:28:03 ws Exp $	*/

/*
 * pid = fork();
 *
 * r4 == 0 in parent process, 1 in child process.
 * r3 == pid of child in parent, pid of parent in child.
 */

#include "SYS.h"

SYSCALL(fork)
	addi	4,4,-1		# from 1 to 0 in child, 0 to -1 in parent
	and	3,3,4		# return 0 in child, pid in parent
	blr
