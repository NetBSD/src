/*	$NetBSD: subr.S,v 1.3 2002/03/31 00:11:13 matt Exp $	   */

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
#include "opt_lockdebug.h"
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

#ifdef	KERNEL_LOADABLE_BY_MOP
/*
 * This is a little tricky. The kernel is not loaded at the correct 
 * address, so the kernel must first be relocated, then copied, then
 * jump back to the correct address.
 */
/* Copy routine */
cps:
2:	movb	(%r0)+,(%r1)+
	cmpl	%r0,%r7
	bneq	2b

3:	clrb	(%r1)+
	incl	%r0
	cmpl	%r0,%r6
	bneq	3b
	clrl	-(%sp)
	movl	%sp,%ap
	movl	$_cca,%r7
	movl	%r8,(%r7)
	movpsl	-(%sp)
	pushl	%r2
	rei
cpe:

/* Copy the copy routine */
1:	movab	cps,%r0
	movab	cpe,%r1
	movl	$0x300000,%sp
	movl	%sp,%r3
4:	movb	(%r0)+,(%r3)+
	cmpl	%r0,%r1
	bneq	4b
	movl	%r7,%r8
/* Ok, copy routine copied, set registers and rei */
	movab	_edata,%r7
	movab	_end,%r6
	movl	$0x80000000,%r1
	movl	$0x80000200,%r0
	subl3	$0x200,%r6,%r9
	movab	2f,%r2
	subl2	$0x200,%r2
	movpsl	-(%sp)
	pushab	4(%sp)
	rei

/*
 * First entry routine from boot. This should be in a file called locore.
 */
JSBENTRY(start)
	brb	1b				# Netbooted starts here
#else
ASENTRY(start, 0)
#endif
2:	bisl3	$0x80000000,%r9,_C_LABEL(esym)	# End of loaded code
	pushl	$0x1f0000			# Push a nice PSL
	pushl	$to				# Address to jump to
	rei					# change to kernel stack
to:	movw	$0xfff,_C_LABEL(panic)		# Save all regs in panic
	cmpb	(%ap),$3			# symbols info present?
	blssu	3f				# nope, skip
	bisl3	$0x80000000,8(%ap),_C_LABEL(symtab_start)
						#   save start of symtab
	movl	12(%ap),_C_LABEL(symtab_nsyms)	#   save number of symtab
	bisl3	$0x80000000,%r9,_C_LABEL(symtab_end)
						#   save end of symtab
3:	addl3	_C_LABEL(esym),$0x3ff,%r0	# Round symbol table end
	bicl3	$0x3ff,%r0,_C_LABEL(proc0paddr)	# save proc0 uarea pointer
	bicl3	$0x80000000,_C_LABEL(proc0paddr),%r0 # get phys proc0 uarea addr
	mtpr	%r0,$PR_PCBB			# Save in IPR PCBB
	addl3	$USPACE,_C_LABEL(proc0paddr),%r0	# Get kernel stack top
	mtpr	%r0,$PR_KSP			# put in IPR KSP
	movl	%r0,_C_LABEL(Sysmap)		# SPT start addr after KSP
	movl	_C_LABEL(proc0paddr),%r0		# get PCB virtual address
	movab	IFTRAP(%r0),4(%r0)		# Save trap address in ESP
	mtpr	4(%r0),$PR_ESP			# Put it in ESP also

# Set some registers in known state
	movl	_C_LABEL(proc0paddr),%r0
	clrl	P0LR(%r0)
	clrl	P1LR(%r0)
	mtpr	$0,$PR_P0LR
	mtpr	$0,$PR_P1LR
	movl	$0x80000000,%r1
	movl	%r1,P0BR(%r0)
	movl	%r1,P1BR(%r0)
	mtpr	%r1,$PR_P0BR
	mtpr	%r1,$PR_P1BR
	clrl	IFTRAP(%r0)
	mtpr	$0,$PR_SCBB

# Copy the RPB to its new position
#if defined(COMPAT_14)
	tstl	(%ap)				# Any arguments?
	bneq	1f				# Yes, called from new boot
	movl	%r11,_C_LABEL(boothowto)		# Howto boot (single etc...)
#	movl	%r10,_C_LABEL(bootdev)		# uninteresting, will complain
	movl	%r8,_C_LABEL(avail_end)		# Usable memory (from VMB)
	clrl	-(%sp)				# Have no RPB
	brb	2f
#endif

1:	pushl	4(%ap)				# Address of old rpb
2:	calls	$1,_C_LABEL(_start)		# Jump away.
	/* NOTREACHED */


/*
 * Signal handler code.
 */

	.align	2
	.globl	_C_LABEL(sigcode),_C_LABEL(esigcode)
_C_LABEL(sigcode):
	pushr	$0x3f
	subl2	$0xc,%sp
	movl	0x24(%sp),%r0
	calls	$3,(%r0)
	popr	$0x3f
	chmk	$SYS___sigreturn14
	chmk	$SYS_exit
	halt	
_C_LABEL(esigcode):

#ifdef COMPAT_IBCS2
	.align	2
	.globl	_C_LABEL(ibcs2_sigcode),_C_LABEL(ibcs2_esigcode)
_C_LABEL(ibcs2_sigcode):
	pushr	$0x3f
	subl2	$0xc,%sp
	movl	0x24(%sp),%r0
	calls	$3,(%r0)
	popr	$0x3f
	chmk	$SYS___sigreturn14
	chmk	$SYS_exit
	halt	
_C_LABEL(ibcs2_esigcode):
#endif /* COMPAT_IBCS2 */

#ifdef COMPAT_ULTRIX
	.align	2
	.globl	_C_LABEL(ultrix_sigcode),_C_LABEL(ultrix_esigcode)
_C_LABEL(ultrix_sigcode):
	pushr	$0x3f
	subl2	$0xc,%sp
	movl	0x24(%sp),%r0
	calls	$3,(%r0)
	popr	$0x3f
	chmk	$ULTRIX_SYS_sigreturn
	chmk	$SYS_exit
	halt	
_C_LABEL(ultrix_esigcode):
#endif

	.align	2
	.globl	_C_LABEL(idsptch), _C_LABEL(eidsptch)
_C_LABEL(idsptch):	pushr	$0x3f
	.word	0x9f16		# jsb to absolute address
	.long	_C_LABEL(cmn_idsptch)	# the absolute address
	.long	0		# the callback interrupt routine
	.long	0		# its argument
	.long	0		# ptr to correspond evcnt struct
_C_LABEL(eidsptch):

_C_LABEL(cmn_idsptch):
	movl	(%sp)+,%r0	# get pointer to idspvec
	movl	8(%r0),%r1	# get evcnt pointer
	beql	1f		# no ptr, skip increment
	incl	EV_COUNT(%r1)	# increment low longword
	adwc	$0,EV_COUNT+4(%r1) # add any carry to hi longword
1:	pushl	4(%r0)		# push argument
	calls	$1,*(%r0)	# call interrupt routine
	popr	$0x3f		# pop registers
	rei			# return from interrut

ENTRY(badaddr,0)			# Called with addr,b/w/l
	mfpr	$PR_IPL,%r0	# splhigh()
	mtpr	$IPL_HIGH,$PR_IPL
	movl	4(%ap),%r2	# First argument, the address
	movl	8(%ap),%r1	# Sec arg, b,w,l
	pushl	%r0		# Save old IPL
	clrl	%r3
	movab	4f,_C_LABEL(memtest)	# Set the return address

	caseb	%r1,$1,$4	# What is the size
1:	.word	1f-1b		
	.word	2f-1b
	.word	3f-1b		# This is unused
	.word	3f-1b
		
1:	movb	(%r2),%r1		# Test a byte
	brb	5f

2:	movw	(%r2),%r1		# Test a word
	brb	5f

3:	movl	(%r2),%r1		# Test a long
	brb	5f

4:	incl	%r3		# Got machine chk => addr bad
5:	mtpr	(%sp)+,$PR_IPL
	movl	%r3,%r0
	ret

#ifdef DDB
/*
 * DDB is the only routine that uses setjmp/longjmp.
 */
	.globl	_C_LABEL(setjmp), _C_LABEL(longjmp)
_C_LABEL(setjmp):.word	0
	movl	4(%ap), %r0
	movl	8(%fp), (%r0)
	movl	12(%fp), 4(%r0)
	movl	16(%fp), 8(%r0)
	moval	28(%fp),12(%r0)
	clrl	%r0
	ret

_C_LABEL(longjmp):.word	0
	movl	4(%ap), %r1
	movl	8(%ap), %r0
	movl	(%r1), %ap
	movl	4(%r1), %fp
	movl	12(%r1), %sp
	jmp	*8(%r1)
#endif

#
# setrunqueue/remrunqueue fast variants.
#

JSBENTRY(Setrq)
#ifdef DIAGNOSTIC
	tstl	4(%r0)	# Check that process actually are off the queue
	beql	1f
	pushab	setrq
	calls	$1,_C_LABEL(panic)
setrq:	.asciz	"setrunqueue"
#endif
1:	extzv	$2,$6,P_PRIORITY(%r0),%r1		# get priority
	movaq	_C_LABEL(sched_qs)[%r1],%r2	# get address of queue
	insque	(%r0),*PH_RLINK(%r2)		# put proc last in queue
	bbss	%r1,_C_LABEL(sched_whichqs),1f	# set queue bit.
1:	rsb

JSBENTRY(Remrq)
	extzv	$2,$6,P_PRIORITY(%r0),%r1
#ifdef DIAGNOSTIC
	bbs	%r1,_C_LABEL(sched_whichqs),1f
	pushab	remrq
	calls	$1,_C_LABEL(panic)
remrq:	.asciz	"remrunqueue"
#endif
1:	remque	(%r0),%r2
	bneq	2f			# Not last process on queue
	bbsc	%r1,_C_LABEL(sched_whichqs),2f
2:	clrl	P_BACK(%r0)		# saftey belt
	rsb

#
# Idle loop. Here we could do something fun, maybe, like calculating
# pi or something.
#
idle:
#if defined(LOCKDEBUG)
	calls	$0,_C_LABEL(sched_unlock_idle)
#elif defined(MULTIPROCESSOR)
	clrl	_C_LABEL(sched_lock)	# release sched lock
#endif
	mtpr	$1,$PR_IPL 		# IPL cannot be 0 because we are
					# running on the interrupt stack
					# and may get interrupts

1:	tstl	_C_LABEL(sched_whichqs)	# Anything ready to run?
	beql	1b			# no, run the idle loop again.
/* Now try the test the long way */
	mtpr	$IPL_HIGH,$PR_IPL	# block all types of interrupts
#if defined(LOCKDEBUG)
	calls	$0,_C_LABEL(sched_lock_idle)
#elif defined(MULTIPROCESSOR)
3:	bbssi	$0,_C_LABEL(sched_lock),3b	# acquire sched lock
#endif
	brb	lp			# check sched_whichqs again

#
# cpu_switch, cpu_exit and the idle loop implemented in assembler 
# for efficiency. %r6 contains pointer to last process.  This is
# called at IPL_HIGH.
#

JSBENTRY(Swtch)
	mfpr	$PR_SSP,%r1		# Get ptr to this cpu_info struct
	clrl	CI_CURPROC(%r1)		# Stop process accounting
	svpctx				# Save context if another CPU
					# get control first (must be on
					# the interrupt stack when idling)


lp:	ffs	$0,$32,_C_LABEL(sched_whichqs),%r3 # Search for bit set
	beql	idle			# no bit set, go to idle loop

	movaq	_C_LABEL(sched_qs)[%r3],%r1	# get address of queue head
	remque	*(%r1),%r2		# remove proc pointed to by queue head
					# proc ptr is now in %r2
#ifdef DIAGNOSTIC
	bvc	1f			# check if something on queue
	pushab	noque
	calls	$1,_C_LABEL(panic)
#endif

1:	bneq	2f			# more processes on queue?
	bbsc	%r3,_C_LABEL(sched_whichqs),2f	# no, clear bit in whichqs
2:	clrl	P_BACK(%r2)		# clear proc backpointer
	mfpr	$PR_SSP,%r1		# Get ptr to this cpu_info struct
	/* p->p_cpu initialized in fork1() for single-processor */
#if defined(MULTIPROCESSOR)
	movl	%r1,P_CPU(%r2)		# p->p_cpu = curcpu();
#endif
	movb	$SONPROC,P_STAT(%r2)	# p->p_stat = SONPROC;
	movl	%r2,CI_CURPROC(%r1)	# set new process running
	clrl	CI_WANT_RESCHED(%r1)	# we are now changing process
	movl	P_ADDR(%r2),%r0		# Get pointer to new pcb.
	addl3	%r0,$IFTRAP,%r1		# Save for copy* functions.
	mtpr	%r1,$PR_ESP		# Use ESP as CPU-specific pointer
	movl	%r1,ESP(%r0)		# Must save in PCB also.
	mfpr	$PR_SSP,%r1		# New process must inherit cpu_info
	movl	%r1,SSP(%r0)		# Put it in new PCB

#
# Nice routine to get physical from virtual adresses.
#
	extzv	$9,$21,%r0,%r1		# extract offset
	ashl	$9,*_C_LABEL(Sysmap)[%r1],%r3

	mtpr	%r3,$PR_PCBB
	ldpctx
#if defined(LOCKDEBUG)
	calls	$0,_C_LABEL(sched_unlock_idle)
#elif defined(MULTIPROCESSOR)
	clrl	_C_LABEL(sched_lock)	# clear sched lock
#endif
	rei

#if defined(MULTIPROCESSOR)
	.align 2
	.globl	_C_LABEL(tramp)	# used to kick off multiprocessor systems.
_C_LABEL(tramp):
	ldpctx
	rei
#endif

#
# the last routine called by a process.
#

ENTRY(cpu_exit,0)
	movl	4(%ap),%r6	# Process pointer in %r6
	mtpr	$IPL_CLOCK,$PR_IPL # Block almost everything
	mfpr	$PR_SSP,%r7	# get cpu_info ptr
	movl	CI_EXIT(%r7),%r8	# scratch page address
	movab	512(%r8),%sp	# change stack
	bicl2	$0xc0000000,%r8	# get physical address
	mtpr	%r8,$PR_PCBB	# new PCB
	mtpr	%r7,$PR_SSP	# In case...
	pushl	%r6
	calls	$1,_C_LABEL(exit2)	# release last resources.
	mtpr	$IPL_HIGH,$PR_IPL	# block all types of interrupts
#if defined(LOCKDEBUG)
	calls	$0,_C_LABEL(sched_lock_idle)
#elif defined(MULTIPROCESSOR)
1:	bbssi	$0,_C_LABEL(sched_lock),1b	# acquire sched lock
#endif
	clrl	%r6
	brw	Swtch

#
# copy/fetch/store routines. 
#

ENTRY(copyout, 0)
	movl	8(%ap),%r2
	blss	3f		# kernel space
	movl	4(%ap),%r1
	brb	2f

ENTRY(copyin, 0)
	movl	4(%ap),%r1
	blss	3f		# kernel space
	movl	8(%ap),%r2
2:	mfpr	$PR_ESP,%r3
	movab	1f,(%r3)
	movc3	12(%ap),(%r1),(%r2)
1:	mfpr	$PR_ESP,%r3
	clrl	(%r3)
	ret

3:	mnegl	$1,%r0
	ret

ENTRY(kcopy,0)
	mfpr	$PR_ESP,%r3
	movl	(%r3),-(%sp)
	movab	1f,(%r3)
	movl	4(%ap),%r1
	movl	8(%ap),%r2
	movc3	12(%ap),(%r1), (%r2)
	clrl	%r1
1:	mfpr	$PR_ESP,%r3
	movl	(%sp)+,(%r3)
	movl	%r1,%r0
	ret

/*
 * copy{in,out}str() copies data from/to user space to/from kernel space.
 * Security checks:
 *	1) user space address must be < KERNBASE
 *	2) the VM system will do the checks while copying
 */
ENTRY(copyinstr, 0)
	tstl	4(%ap)		# kernel address?
	bgeq	8f		# no, continue
6:	movl	$EFAULT,%r0
	movl	16(%ap),%r2
	beql	7f
	clrl	(%r2)
7:	ret

ENTRY(copyoutstr, 0)
	tstl	8(%ap)		# kernel address?
	bgeq	8f		# no, continue
	brb	6b		# yes, return EFAULT

ENTRY(copystr,0)
8:	movl	4(%ap),%r5	# from
	movl	8(%ap),%r4	# to
	movl	12(%ap),%r3	# len
	movl	16(%ap),%r2	# copied
	clrl	%r0
	mfpr	$PR_ESP,%r1
	movab	3f,(%r1)

	tstl	%r3		# any chars to copy?
	bneq	1f		# yes, jump for more
0:	tstl	%r2		# save copied len?
	beql	2f		# no
	subl3	4(%ap),%r5,(%r2)	# save copied len
2:	ret

1:	movb	(%r5)+,(%r4)+	# copy one char
	beql	0b		# jmp if last char
	sobgtr	%r3,1b		# copy one more
	movl	$ENAMETOOLONG,%r0 # inform about too long string
	brb	0b		# out of chars

3:	mfpr	$PR_ESP,%r1
	clrl	(%r1)
	brb	0b

ENTRY(subyte,0)
	movl	4(%ap),%r0
	blss	3f		# illegal space
	mfpr	$PR_ESP,%r1
	movab	1f,(%r1)
	movb	8(%ap),(%r0)
	clrl	%r1
1:	mfpr	$PR_ESP,%r2
	clrl	(%r2)
	movl	%r1,%r0
	ret

ENTRY(suword,0)
	movl	4(%ap),%r0
	blss	3f		# illegal space
	mfpr	$PR_ESP,%r1
	movab	1f,(%r1)
	movl	8(%ap),(%r0)
	clrl	%r1
1:	mfpr	$PR_ESP,%r2
	clrl	(%r2)
	movl	%r1,%r0
	ret

ENTRY(suswintr,0)
	movl	4(%ap),%r0
	blss	3f		# illegal space
	mfpr	$PR_ESP,%r1
	movab	1f,(%r1)
	movw	8(%ap),(%r0)
	clrl	%r1
1:	mfpr	$PR_ESP,%r2
	clrl	(%r2)
	movl	%r1,%r0
	ret

3:	mnegl	$1,%r0
	ret

	.align	2
ALTENTRY(fusword)
ENTRY(fuswintr,0)
	movl	4(%ap),%r0
	blss	3b
	mfpr	$PR_ESP,%r1
	movab	1f,(%r1)
	movzwl	(%r0),%r1
1:	mfpr	$PR_ESP,%r2
	clrl	(%r2)
	movl	%r1,%r0
	ret

#if defined(MULTIPROCESSOR)

JSBENTRY(Slock)
1:	bbssi	$0,(%r1),1b
	rsb

JSBENTRY(Slocktry)
	clrl	%r0
	bbssi	$0,(%r1),1f
	incl	%r0
1:	rsb

JSBENTRY(Sunlock)
	bbcci	$0,(%r1),1f
1:	rsb

#endif

#
# data department
#
	.data

	.globl _C_LABEL(memtest)
_C_LABEL(memtest):		# memory test in progress
	.long 0

#ifdef __ELF__
	.section	.rodata
#endif
noque:	.asciz	"swtch"
