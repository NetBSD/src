/*	$NetBSD: subr.s,v 1.39 2000/05/26 00:36:51 thorpej Exp $	   */

/*
 * Copyright (c) 1994 Ludd, University of Lule}, Sweden.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *     This product includes software developed at Ludd, University of Lule}.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <machine/asm.h>

#include "assym.h"
#include "opt_ddb.h"
#include "opt_multiprocessor.h"
#include "opt_compat_netbsd.h"
#include "opt_compat_ibcs2.h"
#ifdef COMPAT_IBCS2
#include <compat/ibcs2/ibcs2_syscall.h>
#endif
#include "opt_compat_ultrix.h"
#ifdef COMPAT_ULTRIX
#include <compat/ultrix/ultrix_syscall.h>
#endif

#define JSBENTRY(x)	.globl x ; .align 2 ; x :

		.text

/*
 * First entry routine from boot. This should be in a file called locore.
 */
ASENTRY(start, 0)
	bisl3	$0x80000000,r9,_esym		# End of loaded code
	pushl	$0x1f0000			# Push a nice PSL
	pushl	$to				# Address to jump to
	rei					# change to kernel stack
to:	movw	$0xfff,_panic			# Save all regs in panic
	addl3	_esym,$0x3ff,r0			# Round symbol table end
	bicl3	$0x3ff,r0,_proc0paddr		# save proc0 uarea pointer
	bicl3	$0x80000000,_proc0paddr,r0	# get phys proc0 uarea addr
	mtpr	r0,$PR_PCBB			# Save in IPR PCBB
	addl3	$USPACE,_proc0paddr,r0		# Get kernel stack top
	mtpr	r0,$PR_KSP			# put in IPR KSP
	movl	r0,_Sysmap			# SPT start addr after KSP

# Set some registers in known state
	movl	_proc0paddr,r0
	clrl	P0LR(r0)
	clrl	P1LR(r0)
	mtpr	$0,$PR_P0LR
	mtpr	$0,$PR_P1LR
	movl	$0x80000000,r1
	movl	r1,P0BR(r0)
	movl	r1,P1BR(r0)
	mtpr	r1,$PR_P0BR
	mtpr	r1,$PR_P1BR
	clrl	IFTRAP(r0)
	mtpr	$0,$PR_SCBB

# Copy the RPB to its new position
#if defined(COMPAT_14)
	tstl	(ap)				# Any arguments?
	bneq	1f				# Yes, called from new boot
	movl	r11,_boothowto			# Howto boot (single etc...)
#	movl	r10,_bootdev			# uninteresting, will complain
	movl	r8,_avail_end			# Usable memory (from VMB)
	clrl	-(sp)				# Have no RPB
	brb	2f
#endif

1:	pushl	4(ap)				# Address of old rpb
2:	calls	$1,_start			# Jump away.
	/* NOTREACHED */


/*
 * Signal handler code.
 */

		.globl	_sigcode,_esigcode
_sigcode:	pushr	$0x3f
		subl2	$0xc,sp
		movl	0x24(sp),r0
		calls	$3,(r0)
		popr	$0x3f
		chmk	$SYS___sigreturn14
		chmk	$SYS_exit
		halt	
		.align	2
_esigcode:

#ifdef COMPAT_IBCS2
		.globl	_ibcs2_sigcode,_ibcs2_esigcode
_ibcs2_sigcode:	pushr	$0x3f
		subl2	$0xc,sp
		movl	0x24(sp),r0
		calls	$3,(r0)
		popr	$0x3f
		chmk	$SYS___sigreturn14
		chmk	$SYS_exit
		halt	
		.align	2
_ibcs2_esigcode:
#endif /* COMPAT_IBCS2 */

#ifdef COMPAT_ULTRIX
		.globl	_ultrix_sigcode,_ultrix_esigcode
_ultrix_sigcode:	pushr	$0x3f
		subl2	$0xc,sp
		movl	0x24(sp),r0
		calls	$3,(r0)
		popr	$0x3f
		chmk	$ULTRIX_SYS_sigreturn
		chmk	$SYS_exit
		halt	
		.align	2
_ultrix_esigcode:
#endif

		.globl	_idsptch, _eidsptch
_idsptch:	pushr	$0x3f
		.word	0x9f16
		.long	_cmn_idsptch
		.long	0
		.long	0
_eidsptch:

_cmn_idsptch:
		movl	(sp)+,r0
		pushl	4(r0)
		calls	$1,*(r0)
		popr	$0x3f
		rei

ENTRY(badaddr,0)			# Called with addr,b/w/l
		mfpr	$0x12,r0
		mtpr	$0x1f,$0x12
		movl	4(ap),r2	# First argument, the address
		movl	8(ap),r1	# Sec arg, b,w,l
		pushl	r0		# Save old IPL
		clrl	r3
		movab	4f,_memtest	# Set the return address

		caseb	r1,$1,$4	# What is the size
1:		.word	1f-1b		
		.word	2f-1b
		.word	3f-1b		# This is unused
		.word	3f-1b
		
1:		movb	(r2),r1		# Test a byte
		brb	5f

2:		movw	(r2),r1		# Test a word
		brb	5f

3:		movl	(r2),r1		# Test a long
		brb	5f

4:		incl	r3		# Got machine chk => addr bad
5:		mtpr	(sp)+,$0x12
		movl	r3,r0
		ret

# Have bcopy and bzero here to be sure that system files that not gets
# macros.h included will not complain.
#if 0
ENTRY(bcopy,0)
	movl	4(ap), r0
	movl	8(ap), r1
	movl	0xc(ap), r2
	movc3	r2, (r0), (r1)
	ret

ENTRY(bzero,0)
	movl	4(ap), r0
	movl	8(ap), r1
	movc5	$0, (r0), $0, r1, (r0)
	ret
#endif
#ifdef DDB
/*
 * DDB is the only routine that uses setjmp/longjmp.
 */
	.globl	_setjmp, _longjmp
_setjmp:.word	0
	movl	4(ap), r0
	movl	8(fp), (r0)
	movl	12(fp), 4(r0)
	movl	16(fp), 8(r0)
	addl3	fp,$28,12(r0)
	clrl	r0
	ret

_longjmp:.word	0
	movl	4(ap), r1
	movl	8(ap), r0
	movl	(r1), ap
	movl	4(r1), fp
	movl	12(r1), sp
	jmp	*8(r1)
#endif

#
# setrunqueue/remrunqueue fast variants.
#

JSBENTRY(Setrq)
#ifdef DIAGNOSTIC
	tstl	4(r0)	# Check that process actually are off the queue
	beql	1f
	pushab	setrq
	calls	$1,_panic
setrq:	.asciz	"setrunqueue"
#endif
1:	extzv	$2,$6,P_PRIORITY(r0),r1 # get priority
	movaq	_qs[r1],r2		# get address of queue
	insque	(r0),*4(r2)		# put proc last in queue
	bbss	r1,_whichqs,1f		# set queue bit.
1:	rsb

JSBENTRY(Remrq)
	extzv	$2,$6,P_PRIORITY(r0),r1
#ifdef DIAGNOSTIC
	bbs	r1,_whichqs,1f
	pushab	remrq
	calls	$1,_panic
remrq:	.asciz	"remrunqueue"
#endif
1:	remque	(r0),r2
	bneq	1f		# Not last process on queue
	bbsc	r1,_whichqs,1f
1:	clrl	4(r0)		# saftey belt
	rsb

#
# Idle loop. Here we could do something fun, maybe, like calculating
# pi or something.
#
idle:	mtpr	$0,$PR_IPL		# Enable all types of interrupts
1:	movab	_uvm,r0
	tstl	UVM_PAGE_IDLE_ZERO(r0)
	beql	2f
#if 0
	calls	$0,_uvm_pageidlezero
#endif
2:	tstl	_whichqs		# Anything ready to run?
	beql	1b			# no, continue to loop
	brb	Swtch			# Yes, goto switch again.

#
# cpu_switch, cpu_exit and the idle loop implemented in assembler 
# for efficiency. r0 contains pointer to last process.
#

JSBENTRY(Swtch)
#if defined(MULTIPROCESSOR)
	pushl	r0
	calls	$0,*_vax_curcpu		# Get ptr to this cpu_info struct
	clrl	CI_CURPROC(r0)		# Stop process accounting
	movl	(sp)+,r0
#else
	clrl	_curproc		# Stop process accounting
#endif
	mtpr	$0x1f,$PR_IPL		# block all interrupts
	ffs	$0,$32,_whichqs,r3	# Search for bit set
	beql	idle			# no bit set, go to idle loop

	movaq	_qs[r3],r1		# get address of queue head
	remque	*(r1),r2		# remove proc pointed to by queue head
#ifdef DIAGNOSTIC
	bvc	1f			# check if something on queue
	pushab	noque
	calls	$1,_panic
noque:	.asciz	"swtch"
#endif
1:	bneq	2f			# more processes on queue?
	bbsc	r3,_whichqs,2f		# no, clear bit in whichqs
2:	clrl	4(r2)			# clear proc backpointer
	movb	$SONPROC,P_STAT(r2)	# p->p_stat = SONPROC;
#if defined(MULTIPROCESSOR)
	pushl	r0
	calls	$0,*_vax_curcpu		# Get ptr to this cpu_info struct
	movl	r2,CI_CURPROC(r0)	# set new process running
	clrl	CI_WANT_RESCHED(r0)	# we are now changing process
	movl	(sp)+,r0
#else
	movl	r2,_curproc		# set new process running
	clrl	_want_resched		# we are now changing process
#endif
	cmpl	r0,r2			# Same process?
	bneq	1f			# No, continue
	rsb
1:	movl	P_ADDR(r2),r0		# Get pointer to new pcb.
	addl3	r0,$IFTRAP,pcbtrap	# Save for copy* functions.

#
# Nice routine to get physical from virtual adresses.
#
	extzv	$9,$21,r0,r1		# extract offset
	movl	*_Sysmap[r1],r2		# get pte
	ashl	$9,r2,r3		# shift to get phys address.

#
# Do the actual process switch. pc + psl are already on stack, from
# the calling routine.
#
	svpctx
	mtpr	r3,$PR_PCBB
	ldpctx
	rei

#
# the last routine called by a process.
#

ENTRY(cpu_exit,0)
	movl	4(ap),r6	# Process pointer in r6
	mtpr	$0x18,$PR_IPL	# Block almost everything
	addl3	$512,_scratch,sp # Change stack, and schedule it to be freed

	pushl	r6		# exit2(p)
	calls	$1,_exit2

	clrl	r0		# No process to switch from
	bicl3	$0xc0000000,_scratch,r1
	mtpr	r1,$PR_PCBB
	brw	Swtch


#
# copy/fetch/store routines. 
#

ENTRY(copyout, 0)
	movl	8(ap),r2
	blss	3f		# kernel space
	movl	4(ap),r1
	brb	2f

ENTRY(copyin, 0)
	movl	4(ap),r1
	blss	3f		# kernel space
	movl	8(ap),r2
2:	movab	1f,*pcbtrap
	movc3	12(ap),(r1),(r2)
1:	clrl	*pcbtrap
	ret

3:	mnegl	$1,r0
	ret

ENTRY(kcopy,0)
	movl	*pcbtrap,-(sp)
	movab	1f,*pcbtrap
	movl	4(ap),r1
	movl	8(ap),r2
	movc3	12(ap),(r1), (r2)
	clrl	r1
1:	movl	(sp)+,*pcbtrap
	movl	r1,r0
	ret

/*
 * copy{in,out}str() copies data from/to user space to/from kernel space.
 * Security checks:
 *	1) user space address must be < KERNBASE
 *	2) the VM system will do the checks while copying
 */
ENTRY(copyinstr, 0)
	tstl	4(ap)		# kernel address?
	bgeq	8f		# no, continue
6:	movl	$EFAULT,r0
	movl	16(ap),r2
	beql	7f
	clrl	(r2)
7:	ret

ENTRY(copyoutstr, 0)
	tstl	8(ap)		# kernel address?
	bgeq	8f		# no, continue
	brb	6b		# yes, return EFAULT

ENTRY(copystr,0)
8:	movl	4(ap),r5	# from
	movl	8(ap),r4	# to
	movl	12(ap),r3	# len
	movl	16(ap),r2	# copied
	clrl	r0
	movab	3f,*pcbtrap	# XXX - MULTIPROCESSOR

	tstl	r3		# any chars to copy?
	bneq	1f		# yes, jump for more
0:	tstl	r2		# save copied len?
	beql	2f		# no
	subl3	4(ap),r5,(r2)	# save copied len
2:	ret

1:	movb	(r5)+,(r4)+	# copy one char
	beql	0b		# jmp if last char
	sobgtr	r3,1b		# copy one more
	movl	$ENAMETOOLONG,r0 # inform about too long string
	brb	0b		# out of chars

3:	clrl	*pcbtrap	# XXX - MULTIPROCESSOR
	brb	0b

ENTRY(subyte,0)
	movl	4(ap),r0
	blss	3f		# illegal space
	movb	8(ap),(r0)
	movab	1f,*pcbtrap
	clrl	r1
1:	clrl	*pcbtrap
	movl	r1,r0
	ret

ENTRY(suword,0)
	movl	4(ap),r0
	blss	3f		# illegal space
	movl	8(ap),(r0)
	movab	1f,*pcbtrap
	clrl	r1
1:	clrl	*pcbtrap
	movl	r1,r0
	ret

ENTRY(suswintr,0)
	movl	4(ap),r0
	blss	3f		# illegal space
	movw	8(ap),(r0)
	movab	1f,*pcbtrap
	clrl	r1
1:	clrl	*pcbtrap
	movl	r1,r0
	ret

3:	mnegl	$1,r0
	ret

ENTRY(fuswintr,0)
	movl	4(ap),r0
	blss	3b
	movzwl	(r0),r1
	movab	1f,*pcbtrap
1:	clrl	*pcbtrap
	movl	r1,r0
	ret

#
# data department
#
	.data

_memtest:	.long 0 ; .globl _memtest	# Memory test in progress.
pcbtrap:	.long 0x800001fc; .globl pcbtrap	# Safe place

/*
 * Copy/zero more than 64k of memory (as opposite of bcopy/bzero).
 */
ENTRY(blkcpy,R6)
	movl	4(ap),r1
	movl	8(ap),r3
	movl	12(ap),r6
	jbr 2f
1:	subl2	r0,r6
	movc3	r0,(r1),(r3)
2:	movzwl	$65535,r0
	cmpl	r6,r0
	jgtr	1b
	movc3	r6,(r1),(r3)
	ret

ENTRY(blkclr,R6)
	movl	4(ap), r3
	movl	8(ap), r6
	jbr	2f
1:	subl2	r0, r6
	movc5	$0,(r3),$0,r0,(r3)
2:	movzwl	$65535,r0
	cmpl	r6, r0
	jgtr	1b
	movc5	$0,(r3),$0,r6,(r3)
	ret
