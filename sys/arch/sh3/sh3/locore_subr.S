/*	$NetBSD: locore_subr.S,v 1.11 2002/06/23 18:35:07 thorpej Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "opt_cputype.h"
#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "assym.h"

#include <sys/syscall.h>	/* SYS___sigreturn14, SYS_exit */
#include <sh3/asm.h>
#include <sh3/locore.h>
#include <sh3/param.h>		/* UPAGES */
#include <sh3/mmu_sh3.h>
#include <sh3/mmu_sh4.h>

	.text
	.align 5	/* align cache line size (32B) */
/*
 * void cpu_switch(struct proc *):
 *	Find a runnable process and switch to it.  Wait if necessary.
 */
ENTRY(cpu_switch)
	/* Save current process's context to switchframe */
	mov.l	_L.SF,	r0
	mov.l	@(r0, r4), r1
	add	#SF_SIZE, r1
	stc.l	r7_bank,@-r1
	stc.l	sr,	@-r1
	stc.l	r6_bank,@-r1
	sts.l	pr,	@-r1
	mov.l	r8,	@-r1
	mov.l	r9,	@-r1
	mov.l	r10,	@-r1
	mov.l	r11,	@-r1
	mov.l	r12,	@-r1
	mov.l	r13,	@-r1
	mov.l	r14,	@-r1
	mov.l	r15,	@-r1

	/* Search next process. cpu_switch_search may or may not sleep. */
	mov.l	_L.cpu_switch_search, r0
	jsr	@r0
	 mov	r4,	r8	/* save old proc */

	/* Skip context switch if same process. */
	cmp/eq	r8,	r0
	bt/s	1f
	 mov	r0,	r4	/* new proc */

	/* Setup kernel stack */
	mov.l	_L.SF,	r0
	mov.l	@(r0, r4), r1		/* switch frame */
	mov.l	@(SF_R7_BANK, r1), r0	/* stack top */
	mov.l	@(SF_R6_BANK, r1), r2	/* current frame */
	mov.l	@(SF_R15, r1), r3	/* current stack */
	/* During kernel stack switching, all interrupts are disabled. */
	__EXCEPTION_BLOCK(r1, r5)
	/* switch to new kernel stack */
	ldc	r0,	r7_bank
	ldc	r2,	r6_bank
	mov	r3,	r15

	/* Wire u-area */
	MOV	(switch_resume, r0)
	jsr	@r0
	 mov	r4,	r8	/* save new proc */
	mov	r8,	r4
	__EXCEPTION_UNBLOCK(r0, r1)
	/* Now OK to use kernel stack. */

	/* Restore new process's context from switchframe */
1:	mov.l	_L.SF,	r0
	mov.l	@(r0, r4), r1
	add	#4,	r1		/* r15 already restored */
	mov.l	@r1+,	r14
	mov.l	@r1+,	r13
	mov.l	@r1+,	r12
	mov.l	@r1+,	r11
	mov.l	@r1+,	r10
	mov.l	@r1+,	r9
	mov.l	@r1+,	r8
	lds.l	@r1+,	pr
	add	#4,	r1		/* r6_bank already restored */
	ldc.l	@r1+,	sr
	rts
	 nop
	.align	2
_L.SF:			.long	(P_MD + MD_PCB)
_L.cpu_switch_search:	.long	_C_LABEL(cpu_switch_search)
FUNC_SYMBOL(switch_resume)

#ifdef SH3
/*
 * void sh3_switch_resume(sturct proc *p)
 *	Set current u-area PTE array to curupte.
 *	No need to flush any entries. it is depended on u-area mapping is
 *	wired, and its mapping never cause modified/reference fault.
 *	u-area TLB fault is only covered by TLB miss exception.
 *	When the situation that "VPN match but not Valid" occur, SH3 jump to
 *	"generic exception" handler instead of TLB miss exception.
 *	But NetBSD/sh3 code doesn't handle it. As the result, it causes
 *	hard reset. (never can access kernel stack).
 */
NENTRY(sh3_switch_resume)
	mov.l	_L.UPTE, r0
	add	r4,	r0
	mov.l	_L.curupte, r1
	mov.l	r0,	@r1
	rts
	 nop
	.align	2
_L.curupte:		.long	_C_LABEL(curupte)
#endif /* SH3 */

#ifdef SH4
/*
 * void sh4_switch_resume(sturct proc *p)
 *	Wire u-area. invalidate TLB entry for kernel stack to prevent
 *	TLB multiple hit.
 */
NENTRY(sh4_switch_resume)
	mov.l	_L.UPTE,r0
	add	r0,	r4	/* p->p_md.md_upte */
	mov	#UPAGES,r3
	mov	#1,	r2
	mov.l	@r4,	r0	/* if (p->p_md.md_upte[0].addr == 0) return; */
	tst	r0,	r0
	bt	2f

	/* Save old ASID and set ASID to zero */
	xor	r0,	r0
	mov.l	_L.4_PTEH, r1
	mov.l	@r1,	r7
	mov.l	r0,	@r1

	mov.l	_L.VPN_MASK, r6
	mov.l	_L.4_UTLB_AA_A, r5

	/* TLB address array must be accessed via P2. Setup jump address. */
	mova	1f,	r0
	mov.l	_L.P2BASE, r1
	or	r1,	r0
	jmp	@r0		/* run P2 */
	 nop

	/* Probe VPN match TLB entry and invalidate it. */
	.align	2		/* mova target must be 4byte alignment */
1:	mov.l	@(4, r4), r0
	and	r6,	r0
	mov.l	r0,	@r5	/* clear D, V */

	/* Wire u-area TLB entry */
	/* Address array */
	mov.l	@r4+,	r0	/* addr */
	mov.l	@r4+,	r1	/* data */
	mov.l	r1,	@r0	/* *addr = data */

	/* Data array */
	mov.l	@r4+,	r0	/* addr */
	mov.l	@r4+,	r1	/* data */
	mov.l	r1,	@r0	/* *addr = data */
	cmp/eq	r2,	r3
	bf/s	1b
	 add	#1,	r2

	/* restore ASID */
	mov.l	_L.4_PTEH, r0
	mov.l	r7,	@r0
	mova	2f,	r0
	jmp	@r0		/* run P1 */
	 nop
	.align	2
2:	rts			/* mova target must be 4byte alignment */
	 nop
	.align	2
_L.4_PTEH:		.long	SH4_PTEH
_L.4_UTLB_AA_A:		.long	(SH4_UTLB_AA | SH4_UTLB_A)
_L.4_ITLB_AA:		.long	SH4_ITLB_AA
_L.VPN_MASK:		.long	0xfffff000
_L.P2BASE:		.long	0xa0000000
#endif /* SH4 */
_L.UPTE:		.long	(P_MD + MD_UPTE)

/*
 * int _cpu_intr_raise(int s):
 *	raise SR.IMASK to 's'. if current SR.IMASK is greater equal 's',
 *	nothing to do. returns previous SR.IMASK.
 */
NENTRY(_cpu_intr_raise)
	stc	sr,	r2
	mov	#0x78,	r1
	mov	r2,	r0
	shll	r1		/* r1 = 0xf0 */
	and	r1,	r0	/* r0 = SR & 0xf0 */
	cmp/ge	r4,	r0	/* r0 >= r4 ? T = 1 */
	bt/s	1f
	 not	r1,	r1	/* r1 = 0xffffff0f */
	and	r1,	r2	/* r2 = SR & ~0xf0 */
	or	r2,	r4	/* r4 = (SR & ~0xf0) | s */
	ldc	r4,	sr	/* SR = r4 (don't move to delay slot) */
1:	rts
	 nop	/* return (SR & 0xf0) */

/*
 * int _cpu_intr_suspend(void):
 *	Mask all external interrupt. Returns previous SR.IMASK.
 */
NENTRY(_cpu_intr_suspend)
	stc	sr,	r0	/* r0 = SR */
	mov	#0x78,	r1
	shll	r1		/* r1 = 0x000000f0 */
	mov	r0,	r2	/* r2 = SR */
	or	r1,	r2	/* r2 |= 0x000000f0 */
	ldc	r2,	sr	/* SR = r2 */
	rts
	 and	r1,	r0	/* r0 = SR & 0x000000f0 */

/*
 * int _cpu_intr_resume(int s):
 *	Set 's' to SR.IMASK. Returns previous SR.IMASK.
 */
NENTRY(_cpu_intr_resume)
	stc	sr,	r0	/* r0 = SR */
	mov	#0x78,	r2
	shll	r2		/* r2 = 0x000000f0 */
	not	r2,	r1	/* r1 = 0xffffff0f */
	and	r0,	r1	/* r1 = (SR & ~0xf0) */
	or	r1,	r4	/* r4 = (SR & ~0xf0) | level */
	ldc	r4,	sr	/* SR = r0 (don't move to delay slot) */
	rts
	 and	r2,	r0	/* return (SR & 0xf0) */

/*
 * u_int32_t _cpu_exception_suspend(void):
 *	Block exception (SR.BL). if external interrupt raise, pending interrupt.
 *	if exception occur, jump to 0xa0000000 (hard reset).
 */
NENTRY(_cpu_exception_suspend)
	stc	sr,	r0	/* r0 = SR */
	mov	#0x10,	r1
	swap.b	r1,	r1
	mov	r0,	r2	/* r2 = r0 */
	swap.w	r1,	r1	/* r1 = 0x10000000 */
	or	r1,	r2	/* r2 |= 0x10000000 */
	ldc	r2,	sr	/* SR = r2 */
	rts
	 and	r1,	r0	/* r0 &= 0x10000000 */

/*
 * void _cpu_exception_resume(u_int32_t s):
 *	restore 's' exception mask. (SR.BL)
 */
NENTRY(_cpu_exception_resume)
	stc	sr,	r0	/* r0 = SR */
	mov	#0x10,	r1
	swap.b	r1,	r1
	swap.w	r1,	r1
	not	r1,	r1	/* r1 = ~0x10000000 */
	and	r1,	r0	/* r0 &= ~0x10000000 */
	or	r4,	r0	/* r0 |= old SR.BL */
	ldc	r0,	sr	/* SR = r0 (don't move to delay slot) */
	rts
	 nop

/*
 * void _cpu_spin(u_int32_t count)
 *	loop 'count' * 10 cycle.
 * [...]
 * add    IF ID EX MA WB
 * nop       IF ID EX MA WB
 * cmp/pl       IF ID EX MA WB -  -
 * nop             IF ID EX MA -  -  WB
 * bt                 IF ID EX .  .  MA WB
 * nop                   IF ID -  -  EX MA WB
 * nop                      IF -  -  ID EX MA WB
 * nop                      -  -  -  IF ID EX MA WB
 * add                                  IF ID EX MA WB
 * nop                                     IF ID EX MA WB
 * cmp/pl                                     IF ID EX MA WB -  -
 * nop                                           IF ID EX MA -  - WB
 * bt                                               IF ID EX .  . MA
 * [...]
 */
	.align 5	/* align cache line size (32B) */
NENTRY(_cpu_spin)
1:	nop			/* 1 */
	nop			/* 2 */
	nop			/* 3 */
	add	#-1, r4		/* 4 */
	nop			/* 5 */
	cmp/pl	r4		/* 6 */
	nop			/* 7 */
	bt	1b		/* 8, 9, 10 */
	rts
	 nop

/*
 * proc_trapmpoline:
 *	Call the service funciton with one argument specified by the r12 and r11
 *	respectively. setted by cpu_fork().
 */
NENTRY(proc_trampoline)
	jsr	@r12
	 mov	r11,	r4
	__EXCEPTION_RETURN
	/* NOTREACHED */

/*
 * sigcode:
 *	Signal trampoline. copied to top of user stack.
 *
 *	On entry, the registers and stack look like this:
 *
 *	r4	signal number
 *	r5	signal specific code
 *	r6	pointer to sigcontext structure
 *	r7	address of handler
 *
 *	sp->	sigcontext structure
 */
NENTRY(sigcode)
	jsr	@r7			/* call handler */
	 nop
	mov	r15, r4			/* get pointer to sigcontext */
	mov.l	_L.SYS___sigreturn14, r0
	trapa	#0x80			/* and call sigreturn() */
	mov.l	_L.SYS_exit, r0
	trapa	#0x80			/* exit if sigreturn fails */
	.align	2
_L.SYS___sigreturn14:	.long	SYS___sigreturn14
_L.SYS_exit:		.long	SYS_exit
	.globl	_C_LABEL(esigcode)
_C_LABEL(esigcode):

/*
 * void savectx(struct pcb *pcb):
 *	save struct switchframe.
 */
ENTRY(savectx)
	add	#SF_SIZE, r4
	stc.l	r7_bank,@-r4
	stc.l	sr,	@-r4
	stc.l	r6_bank,@-r4
	sts.l	pr,	@-r4
	mov.l	r8,	@-r4
	mov.l	r9,	@-r4
	mov.l	r10,	@-r4
	mov.l	r11,	@-r4
	mov.l	r12,	@-r4
	mov.l	r13,	@-r4
	mov.l	r14,	@-r4
	mov.l	r15,	@-r4
	rts
	 nop

#if defined(DDB) || defined(KGDB)
/*
 * int setjmp(label_t *):
 */
ENTRY(setjmp)
	add	#4*9,	r4
	mov.l	r8,	@-r4
	mov.l	r9,	@-r4
	mov.l	r10,	@-r4
	mov.l	r11,	@-r4
	mov.l	r12,	@-r4
	mov.l	r13,	@-r4
	mov.l	r14,	@-r4
	mov.l	r15,	@-r4
	sts.l	pr,	@-r4
	rts
	 xor	r0, r0
/*
 * void longjmp(label_t *):
 */
ENTRY(longjmp)
	lds.l	@r4+,	pr
	mov.l	@r4+,	r15
	mov.l	@r4+,	r14
	mov.l	@r4+,	r13
	mov.l	@r4+,	r12
	mov.l	@r4+,	r11
	mov.l	@r4+,	r10
	mov.l	@r4+,	r9
	mov.l	@r4+,	r8
	rts
	 nop
#endif /* DDB || KGDB */
