/*	$NetBSD: sigprocmask.S,v 1.7.14.1 2004/07/23 13:56:02 tron Exp $	*/

#include "SYS.h"

WARN_REFERENCES(sigprocmask, \
    "warning: reference to compatibility sigprocmask(); include <signal.h> for correct reference")

/*
 * sigprocmask(int how, sigset_t *set, sigset_t *oset)
 */
ENTRY(sigprocmask)
	or.	%r4,%r4,%r4	# set == NULL?
	li	%r6,1		# how = SIG_BLOCK
	beq	1f
	lwz	%r4,0(%r4)	# if not, replace it in r4 with *set
	mr	%r6,%r3
1:
	mr	%r3,%r6		# ... using sigprocmask(SIG_BLOCK)
	li	%r0,SYS_compat_13_sigprocmask13
	sc
	bso	3f
	or.	%r5,%r5,%r5	# check to see of oset requested
	beq	2f		# if oset != NULL,
	stw	%r3,0(%r5)	#    *oset = oldmask
2:
	li	%r3,0
	blr			# in any case, return 0
3:
	b	PIC_PLT(_C_LABEL(__cerror))
