/*      $NetBSD: subr.s,v 1.22 1998/01/18 22:06:01 ragge Exp $     */

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

#define	JSBENTRY(x)	.globl x ; .align 2 ; x :

		.text

		.globl	_sigcode,_esigcode
_sigcode:	pushr	$0x3f
		subl2	$0xc,sp
		movl	0x24(sp),r0
		calls	$3,(r0)
		popr	$0x3f
		chmk	$SYS_sigreturn
		halt	
		.align	2
_esigcode:

		.globl	_idsptch, _eidsptch
_idsptch:	pushr	$0x3f
		pushl	$1
		nop
		calls	$1, *$0x12345678
		popr	$0x3f
		rei
_eidsptch:

ENTRY(badaddr,0)			# Called with addr,b/w/l
		mfpr	$0x12,r0
		mtpr	$0x1f,$0x12
		movl	4(ap),r2 	# First argument, the address
		movl	8(ap),r1 	# Sec arg, b,w,l
		pushl	r0		# Save old IPL
		clrl	r3
		movl	$4f,_memtest	# Set the return adress

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

#ifdef DDB
/*
 * DDB is the only routine that uses setjmp/longjmp.
 */
	.globl	_setjmp, _longjmp
_setjmp:.word	0
	movl	4(ap), r0
	movl	8(fp), (r0)
	movl	12(fp),	4(r0)
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
1:	extzv	$2,$6,P_PRIORITY(r0),r1	# get priority
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
1:	tstl	_whichqs		# Anything ready to run?
	beql	1b			# no, continue to loop
	brb	Swtch			# Yes, goto switch again.

#
# cpu_switch, cpu_exit and the idle loop implemented in assembler 
# for efficiency. r0 contains pointer to last process.
#

JSBENTRY(Swtch)
	clrl	_curproc		# Stop process accounting
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
	clrl	_want_resched		# we are now changing process
	movl	r2,_curproc		# set new process running
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
	movl	4(ap),r6	# Process pointer in r0
	pushl	P_VMSPACE(r6)	# free current vm space
	calls	$1,_vmspace_free
	mtpr	$0x18,$PR_IPL	# Block almost everything
	addl3	$512,_scratch,sp # Change stack, we will free it now
	pushl	$USPACE		# stack size
	pushl	P_ADDR(r6)	# pointer to stack space
	pushl	_kernel_map	# the correct vm map
	calls	$3,_kmem_free
	clrl	r0		# No process to switch from
	bicl3	$0xc0000000,_scratch,r1
	mtpr	r1,$PR_PCBB
	brw	Swtch


#
# copy/fetch/store routines. 
#

	.globl	_copyin, _copyout
_copyout:
_copyin:.word 0
	moval	1f,*pcbtrap
	movl	4(ap),r1
	movl	8(ap),r2
	movc3	12(ap),(r1), (r2)
1:	clrl	*pcbtrap
	ret

_copystr:	.globl	_copystr
_copyinstr:	.globl	_copyinstr
_copyoutstr:	.globl	_copyoutstr
	.word	0
	movl	4(ap),r4	# from
	movl	8(ap),r5	# to
	movl	12(ap),r2	# len
	movl	16(ap),r3	# copied

	moval	2f,*pcbtrap

/*
 * This routine consists of two parts: One is for MV2 that doesn't have
 * locc in hardware, the other is a fast version with locc. But because
 * locc only handles <64k strings, we default to the slow version if the
 * string is longer.
 */
	cmpl	_vax_cputype,$VAX_TYP_UV2
	bneq	4f		# Check if locc emulated

9:	movl	r2,r0
7:	movb	(r4)+,(r5)+
	beql	6f
	sobgtr	r0,7b
	brb 1f

6:	tstl	r3
	beql	5f
	incl	r2
	subl3	r0,r2,(r3)
5:	clrl	r0
	clrl	*pcbtrap
	ret

4:	cmpl	r2,$65535	# maxlen < 64k?
	blss	8f		# then use fast code.

	locc	$0,$65535,(r4)	# is strlen < 64k?
	beql	9b		# No, use slow code
	subl3	r0,$65535,r1	# Get string len
	brb	0f		# do the copy

8:	locc	$0,r2,(r4)	# check for null byte
	beql	1f

	subl3	r0,r2,r1	# Calculate len to copy
0:	incl	r1		# Copy null byte also
	tstl	r3
	beql	3f
	movl	r1,(r3)		# save len copied
3:	movc3	r1,(r4),(r5)
	brb	2f

1:	movl	$ENAMETOOLONG,r0
2:	clrl	*pcbtrap
	ret

ENTRY(subyte,0)
	moval	1f,*pcbtrap
	movl	4(ap),r0
	movb	8(ap),(r0)
	clrl	r1
1:	clrl	*pcbtrap
	movl	r1,r0
	ret

ENTRY(suword,0)
	moval	1f,*pcbtrap
	movl	4(ap),r0
	movl	8(ap),(r0)
	clrl	r1
1:	clrl	*pcbtrap
	movl	r1,r0
	ret

ENTRY(suswintr,0)
	moval	1f,*pcbtrap
	movl	4(ap),r0
	movw	8(ap),(r0)
	clrl	r1
1:	clrl	*pcbtrap
	movl	r1,r0
	ret

ENTRY(fuswintr,0)
	moval	1f,*pcbtrap
	movl    4(ap),r0
	movzwl	(r0),r1
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
	movl    4(ap),r1
	movl    8(ap),r3
	movl	12(ap),r6
	jbr 2f
1:	subl2   r0,r6
	movc3   r0,(r1),(r3)
2:	movzwl  $65535,r0
	cmpl    r6,r0
	jgtr    1b
	movc3   r6,(r1),(r3)
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
