/*	$NetBSD: subr.S,v 1.33.18.1 2017/12/03 11:36:48 jdolecek Exp $	   */

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
#define SCBENTRY(name) \
	.text			; \
	.align 2		; \
	.globl __CONCAT(X,name)	; \
__CONCAT(X,name):

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
	bicl3	$0x3ff,%r0,%r1			# 
	movl	%r1,_C_LABEL(lwp0)+L_PCB	# lwp0 pcb, XXXuvm_lwp_getuarea
	bicl3	$0x80000000,%r1,%r0		# get phys lwp0 uarea addr
	mtpr	%r0,$PR_PCBB			# Save in IPR PCBB
	addl3	$USPACE,%r1,%r0			# Get kernel stack top
	mtpr	%r0,$PR_KSP			# put in IPR KSP
	movl	%r0,_C_LABEL(Sysmap)		# SPT start addr after KSP
	movl	_C_LABEL(lwp0)+L_PCB,%r0	# get PCB virtual address
	mfpr	$PR_PCBB,PCB_PADDR(%r0)		# save PCB physical address
	movab	PCB_ONFAULT(%r0),ESP(%r0)	# Save trap address in ESP
	mtpr	4(%r0),$PR_ESP			# Put it in ESP also

# Set some registers in known state
	movl	%r1,%r0				# get lwp0 pcb
	clrl	P0LR(%r0)
	clrl	P1LR(%r0)
	mtpr	$0,$PR_P0LR
	mtpr	$0,$PR_P1LR
	movl	$0x80000000,%r1
	movl	%r1,P0BR(%r0)
	movl	%r1,P1BR(%r0)
	mtpr	%r1,$PR_P0BR
	mtpr	%r1,$PR_P1BR
	clrl	PCB_ONFAULT(%r0)
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
	chmk	$SYS_compat_16___sigreturn14
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
	chmk	$SYS_compat_16___sigreturn14
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
_C_LABEL(idsptch):
	pushr	$0x3f
	.word	0x9f16		# jsb to absolute address
	.long	_C_LABEL(cmn_idsptch)	# the absolute address
	.long	0		# the callback interrupt routine
	.long	0		# its argument
	.long	0		# ptr to correspond evcnt struct
_C_LABEL(eidsptch):

_C_LABEL(cmn_idsptch):
#if defined(MULTIPROCESSOR) || defined(LOCKDEBUG)
	calls	$0,_C_LABEL(krnlock)
#endif
	movl	(%sp)+,%r0	# get pointer to idspvec
	mtpr	$IPL_VM,$PR_IPL	# Make sure we are at IPL_VM
	movl	8(%r0),%r1	# get evcnt pointer
	beql	1f		# no ptr, skip increment
	incl	EV_COUNT(%r1)	# increment low longword
	adwc	$0,EV_COUNT+4(%r1) # add any carry to hi longword
1:	mfpr	$PR_SSP, %r2	# get curlwp
	movl	L_CPU(%r2), %r2 # get curcpu
	incl	CI_NINTR(%r2)	# increment ci_data.cpu_nintr
	adwc	$0,(CI_NINTR+4)(%r2)
#if 0
	pushl	%r0
	movq	(%r0),-(%sp)
	pushab	2f
	calls	$3,_C_LABEL(printf)
	movl	(%sp)+,%r0
#endif
	pushl	4(%r0)		# push argument
	calls	$1,*(%r0)	# call interrupt routine
#if defined(MULTIPROCESSOR) || defined(LOCKDEBUG)
	calls	$0,_C_LABEL(krnunlock)
#endif
	popr	$0x3f		# pop registers
	rei			# return from interrut
#if 0
2:	.asciz	"intr %p(%p)\n"
#endif

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

#if defined(MULTIPROCESSOR)
	.align 2
	.globl	_C_LABEL(vax_mp_tramp)	# used to kick off multiprocessor systems.
_C_LABEL(vax_mp_tramp):
	ldpctx
	rei
#endif

	.globl	softint_cleanup,softint_exit,softint_process
	.type	softint_cleanup@function
	.type	softint_exit@function
	.type	softint_process@function
softint_cleanup:
	movl    L_CPU(%r0),%r1		/* get cpu_info */
	incl    CI_MTX_COUNT(%r1)	/* increment mutex count */
	clrl    L_CTXSWTCH(%r0)		/* clear l_ctxswtch of old lwp */
	movl	L_PCB(%r0),%r1		/* get PCB of softint LWP */
softint_exit:
	popr	$0x3			/* restore r0 and r1 */
	rei				/* return from interrupt */

softint_process:
	/*
	 * R6 contains pinned LWP
	 * R7 contains ipl to dispatch with
	 */
	movq	%r6,-(%sp)		/* push old lwp and ipl onto stack */
	calls	$2,_C_LABEL(softint_dispatch) /* dispatch it */

	/* We can use any register because ldpctx will overwrite them */
	movl	L_PCB(%r6),%r3		/* get pcb */
	movab	softint_exit,PCB_PC(%r3)/* do a quick exit */
#ifdef MULTIPROCESSOR
	movl	L_CPU(%r6),%r8
	movl	%r6,CI_CURLWP(%r8)
#endif

	mtpr	PCB_PADDR(%r3),$PR_PCBB	/* restore PA of interrupted pcb */
	ldpctx				/* implictily updates curlwp */
	rei


softint_common:
	mfpr	$PR_IPL,%r1
	mtpr	$IPL_HIGH,$PR_IPL	/* we need to be at IPL_HIGH */
	movpsl	-(%sp)			/* add cleanup hook */
	pushab	softint_cleanup
	svpctx

	/* We can use any register because ldpctx will overwrite them */
	mfpr	$PR_SSP,%r6		/* Get curlwp */
	movl	L_CPU(%r6),%r8		/* get cpu_info */
	movl	CI_SOFTLWPS(%r8)[%r0],%r2 /* get softlwp to switch to */
	movl	L_PCB(%r2),%r3		/* Get pointer to its pcb. */
	movl	%r6,PCB_R6(%r3)		/* move old lwp into new pcb */
	movl	%r1,PCB_R7(%r3)		/* move IPL into new pcb */
#ifdef MULTIPROCESSOR
	movl	%r2,CI_CURLWP(%r8)	/* update ci_curlwp */
#endif

	/*
	 * Now reset the PCB since we no idea what state it was last in
	 */
	movab	(USPACE-TRAPFRAMELEN-CALLSFRAMELEN)(%r3),%r0
					/* calculate where KSP should be */
	movl	%r0,KSP(%r3)		/* save it as SP */
	movl	%r0,PCB_FP(%r3)		/* and as the FP too */
	movab	CA_ARGNO(%r0),PCB_AP(%r3) /* update the AP as well */
	movab	softint_process,PCB_PC(%r3) /* and where we will start */
	movl	$PSL_HIGHIPL,PCB_PSL(%r3) /* Needs to be running at IPL_HIGH */

	mtpr	PCB_PADDR(%r3),$PR_PCBB	/* set PA of new pcb */
	ldpctx				/* load it */
	rei				/* get off interrupt stack */

SCBENTRY(softclock)
	pushr	$0x3			/* save r0 and r1 */
	movl	$SOFTINT_CLOCK,%r0
	brb	softint_common

SCBENTRY(softbio)
	pushr	$0x3			/* save r0 and r1 */
	movl	$SOFTINT_BIO,%r0
	brb	softint_common

SCBENTRY(softnet)
	pushr	$0x3			/* save r0 and r1 */
	movl	$SOFTINT_NET,%r0
	brb	softint_common

SCBENTRY(softserial)
	pushr	$0x3			/* save r0 and r1 */
	movl	$SOFTINT_SERIAL,%r0
	brb	softint_common

/*
 * Helper routine for cpu_lwp_fork.  It get invoked by Swtchto.
 * It let's the kernel know the lwp is alive and then calls func(arg)
 * and possibly returns to sret.
 */
ENTRY(cpu_lwp_bootstrap, 0)
	movq	%r2,-(%sp)			/* save func & arg */
	movq	%r0,-(%sp)			/* push oldl/newl */
	calls	$2,_C_LABEL(lwp_startup)	/* startup the lwp */
	movl	(%sp)+,%r0			/* grab func */
	calls	$1,(%r0)			/* call it with arg */
	ret

/*
 * r1 = newlwp
 * r0 = oldlwp
 */
JSBENTRY(Swtchto)
	/* this pops the pc and psw from the stack and puts them in the pcb. */
	svpctx				# Now on interrupt stack

	/* We can know use any register because ldpctx will overwrite them */
	/* New LWP already in %r1 */
	movl	L_PCB(%r1),%r3		# Get pointer to new pcb.
	movl	%r0,PCB_R0(%r3)		# move r0 into new pcb (return value)
#ifdef MULTIPROCESSOR
	movl	L_CPU(%r0), %r8		/* get cpu_info of old lwp */
	movl	%r8, L_CPU(%r1)		/* update cpu_info of new lwp */
	movl	%r1,CI_CURLWP(%r8)	/* update ci_curlwp */
#endif

	mtpr	PCB_PADDR(%r3),$PR_PCBB	# set PA of new pcb
	mtpr	$IPL_HIGH,$PR_IPL	/* we need to be at IPL_HIGH */
	ldpctx				# load it
	/* r0 already has previous lwp */
	/* r1 already has this lwp */
	/* r2/r3 and r4/r5 restored */
	rei				/* get off interrupt stack */

#
# copy/fetch/store routines. 
#

ENTRY(copyout, 0)
	movl	8(%ap),%r3
	blss	3f		# kernel space
	movl	4(%ap),%r1
	brb	2f

ENTRY(copyin, 0)
	movl	4(%ap),%r1
	blss	3f		# kernel space
	movl	8(%ap),%r3
2:	mfpr	$PR_ESP,%r2
	movab	1f,(%r2)	# set pcb_onfault
4:	tstw	14(%ap)		# check if >= 64K
	bneq	5f
	movc3	12(%ap),(%r1),(%r3)
	clrl	%r0
1:	mfpr	$PR_ESP,%r2
	clrl	(%r2)		# clear pcb_onfault
	ret
5:	movc3	$0xfffc,(%r1),(%r3)
	subl2	$0xfffc,12(%ap)
	brb	4b

3:	movl	$EFAULT,%r0
	ret

ENTRY(kcopy,0)
	mfpr	$PR_ESP,%r3
	movl	(%r3),-(%sp)	# save current pcb_onfault
	movab	1f,(%r3)	# set pcb_onfault
	movl	4(%ap),%r1
	movl	8(%ap),%r2
	movc3	12(%ap),(%r1), (%r2)
	clrl	%r0
1:	mfpr	$PR_ESP,%r3
	movl	(%sp)+,(%r3)	# restore pcb_onfault
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
	movab	2f,(%r1)	# set pcb_onfault

	tstl	%r3		# any chars to copy?
	bneq	1f		# yes, jump for more
0:	tstl	%r2		# save copied len?
	beql	2f		# no
	subl3	4(%ap),%r5,(%r2)	# save copied len
2:	mfpr	$PR_ESP,%r1
	clrl	(%r1)		# clear pcb_onfault
	ret

1:	movb	(%r5)+,(%r4)+	# copy one char
	beql	0b		# jmp if last char
	sobgtr	%r3,1b		# copy one more
	movl	$ENAMETOOLONG,%r0 # inform about too long string
	brb	0b		# out of chars

ENTRY(subyte,0)
	movl	4(%ap),%r0
	blss	3f		# illegal space
	mfpr	$PR_ESP,%r1
	movab	1f,(%r1)	# set pcb_onfault
	movb	8(%ap),(%r0)
	clrl	%r0
1:	mfpr	$PR_ESP,%r1
	clrl	(%r1)		# clear pcb_onfault
	ret

ENTRY(suword,0)
	movl	4(%ap),%r0
	blss	3f		# illegal space
	mfpr	$PR_ESP,%r1
	movab	3f,(%r1)	# set pcb_onfault
	movl	8(%ap),(%r0)
	clrl	%r0
1:	mfpr	$PR_ESP,%r1
	clrl	(%r1)		# clear pcb_onfault
	ret

ENTRY(suswintr,0)
	movl	4(%ap),%r0
	blss	3f		# illegal space
	mfpr	$PR_ESP,%r1
	movab	3f,(%r1)	# set pcb_onfault
	movw	8(%ap),(%r0)
	clrl	%r0
1:	mfpr	$PR_ESP,%r1
	clrl	(%r1)		# clear pcb_onfault
	ret

3:	mnegl	$1,%r0
	brb	1b

	.align	2
ALTENTRY(fusword)
ENTRY(fuswintr,0)
	movl	4(%ap),%r2
	blss	3b
	mfpr	$PR_ESP,%r1
	movab	3b,(%r1)	# set pcb_onfault
	movzwl	(%r2),%r0
1:	mfpr	$PR_ESP,%r1
	clrl	(%r1)		# clear pcb_onfault
	ret

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

#
# data department
#
	.data

	.globl _C_LABEL(memtest)
_C_LABEL(memtest):		# memory test in progress
	.long 0
