/*	$NetBSD: sigprocmask.S,v 1.5 1998/11/24 11:14:57 tsubai Exp $	*/

#include "SYS.h"

/*
 * sigprocmask(int how, sigset_t *set, sigset_t *oset)
 */
ENTRY(sigprocmask)
	or.	4,4,4		# set == NULL?
	li	6,1		# how = SIG_BLOCK
	beq	1f
	lwz	4,0(4)		# if not, replace it in r4 with *set
	mr	6,3
1:
	mr	3,6		# ... using sigprocmask(SIG_BLOCK)
	li	0,SYS_compat_13_sigprocmask13
	sc
	bso	3f
	or.	5,5,5		# check to see of oset requested
	beq	2f		# if oset != NULL,
	stw	3,0(5)		#    *oset = oldmask
2:
	li	3,0
	blr			# in any case, return 0
3:
	b	PIC_PLT(_ASM_LABEL(cerror))
