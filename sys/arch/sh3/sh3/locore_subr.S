/*	$NetBSD: locore_subr.S,v 1.9 2002/04/28 17:10:38 uch Exp $	*/

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

#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_cputype.h"

#include "assym.h"

#include <sys/syscall.h>	/* SYS___sigreturn14, SYS_exit */
#include <sh3/asm.h>
#include <sh3/locore.h>
#include <sh3/mmu_sh3.h>	/* TTB */
#include <sh3/mmu_sh4.h>	/* TTB */
#include <sh3/param.h>		/* UPAGES */

	.text
	.align 5	/* align cache line size (32B) */
/*
 * void cpu_switch(struct proc *):
 *	Find a runnable process and switch to it.  Wait if necessary.
 */
ENTRY(cpu_switch)
	/* Save current process's context to switchframe */
	mov.l	_L.P_ADDR, r0
	mov.l	@(r0, r4), r1
	add	#SF_SIZE, r1
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

	/* Set curpcb */
	mov.l	_L.P_ADDR, r0
	mov.l	@(r0, r4), r1
	mov.l	_L.curpcb, r0
	mov.l	r1,	@r0	/* curpcb = curproc->p_addr */

	/* Switch address space. */
	mov	#PCB_PAGEDIRREG, r0
	mov.l	@(r0, r1), r0
	MOV	(TTB,	r1)
	mov.l	r0,	@r1	/* TTB = curpcb->pageDirReg */

	/* Wire kernel stack */
	MOV	(switch_resume, r0)
	jsr	@r0
	 nop

	/* Restore new process's context from switchframe */
1:	mov.l	_L.curpcb, r1
	mov.l	@r1,	r1
	mov.l	@r1+,	r15
	mov.l	@r1+,	r14
	mov.l	@r1+,	r13
	mov.l	@r1+,	r12
	mov.l	@r1+,	r11
	mov.l	@r1+,	r10
	mov.l	@r1+,	r9
	mov.l	@r1+,	r8
	lds.l	@r1+,	pr
	ldc.l	@r1+,	r6_bank	/* current fp */
	mov.l	@r1+,	r0
	ldc.l	@r1+,	r7_bank	/* kernel stack top */
	ldc	r0,	sr
	rts
	 nop
	.align	2
_L.P_ADDR:		.long	P_ADDR
_L.cpu_switch_search:	.long	_C_LABEL(cpu_switch_search)
_L.curpcb:		.long	_C_LABEL(curpcb)
REG_SYMBOL(TTB)
FUNC_SYMBOL(switch_resume)

#ifdef SH3
/*
 * void sh3_switch_resume(sturct proc *p)
 *	Zero clear all TLB. and
 *	Set current kernel stack PTE table.
 */
NENTRY(sh3_switch_resume)
	mov.l	_L.UPTE, r0
	add	r4,	r0
	mov.l	_L.curupte, r1
	mov.l	r0,	@r1

	mov.l	_L.MMUAA, r7
	mov.l	_L.MMUDA, r6
	mov	#3,	r5	/* max way # */
2:	mov	r5,	r3
	shll8	r3
	mov	#31,	r4	/* max entry # */
1:	mov	r4,	r2
	shll8	r2
	shll2	r2
	shll2	r2
	or	r3,	r2	/* r2 = way|entry */
	mov	r7,	r1
	or	r2,	r1	/* Address array */
	xor	r0,	r0
	mov.l	r0,	@r1
	mov	r6,	r1
	or	r2,	r1	/* Data array */
	mov.l	r0,	@r1
	cmp/gt	r0,	r4
	bt/s	1b
	 add	#-1,	r4
	cmp/gt	r0,	r5
	bt/s	2b
	 add	#-1,	r5
	rts
	 nop
	.align	2
_L.MMUCR:		.long	SH3_MMUCR
_L.MMUAA:		.long	SH3_MMUAA
_L.MMUDA:		.long	SH3_MMUDA
_L.curupte:		.long	_C_LABEL(curupte)
#endif /* SH3 */

#ifdef SH4
/*
 * void sh4_switch_resume(sturct proc *p)
 *	Invalidate all TLB
 *	Wire kernel stack.
 */
NENTRY(sh4_switch_resume)
	/* Invalidate ITLB */
	mov.l	_L.4_ITLB_AA, r0
	mov	#1,	r2
	xor	r1,	r1
	mov.l	r1,	@r0
	shll8	r2
	add	r2,	r0
	mov.l	r1,	@r0
	add	r2,	r0
	mov.l	r1,	@r0
	add	r2,	r0
	mov.l	r1,	@r0

	/* Invalidate UTLB */
	mov.l	_L.4_MMUCR, r0
	mov.l	@r0,	r1
	mov.l	_L.4_MMUCR_MASK, r2
	and	r2,	r1
	mov	#4,	r2	/* SH4_MMUCR_TI */
	or	r2,	r1
	mov.l	r1,	@r0

	/*
	 * Wire u-area
	 * for (i = 0; i < UPAGES * 2; i++)
	 *	_reg_write_4(p->p_md.md_upte[i].addr, p->p_md.md_upte[i].data);
	 * if u-area is P1, nothing to do.
	 */
	mov.l	_L.UPTE,r0
	add	r0,	r4	/* p->p_md.md_upte */
	mov	#UPAGES,r3
	mov	#1,	r2
	mov.l	@r4,	r0	/* if (p->p_md.md_upte[0].addr == 0) return; */
	tst	r0,	r0
	bt	2f
1:	/* Address array */
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
2:	rts
	 nop
	.align	2
_L.4_ITLB_AA:		.long	SH4_ITLB_AA
_L.4_MMUCR:		.long	SH4_MMUCR
_L.4_MMUCR_MASK:	.long	SH4_MMUCR_MASK
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
 *	respectively.
 */
NENTRY(proc_trampoline)
	jsr	@r12
	 mov	r11,	r4
	__EXCEPTION_RETURN
	/* NOTREACHED */

/*
 * sigcode:
 *	Signal trampoline. copied to top of user stack.
 */
NENTRY(sigcode)
	mov	r15,	r0
	mov.l	@r0,	r4
	add	#SIGF_HANDLER, r0
	mov.l	@r0,	r0
	jsr	@r0			/* (*sf_handler)(sf_signum) */
	 nop
	mov	r15,	r4
	add	#SIGF_SC, r4
	mov.l	_L.SYS___sigreturn14, r0
	trapa	#0x80			/* enter kernel with args on stack */
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
