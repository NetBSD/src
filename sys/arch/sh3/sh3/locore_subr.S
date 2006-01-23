/*	$NetBSD: locore_subr.S,v 1.27 2006/01/23 22:14:59 uwe Exp $	*/

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

#include "opt_compat_netbsd.h"
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

__KERNEL_RCSID(0, "$NetBSD: locore_subr.S,v 1.27 2006/01/23 22:14:59 uwe Exp $")


/*
 * LINTSTUB: include <sys/types.h>
 * LINTSTUB: include <sys/lwp.h>
 * LINTSTUB: include <sh3/locore.h>
 */

	.text
	.align 5	/* align cache line size (32B) */
/*
 * LINTSTUB: Func: int cpu_switch(struct lwp *l, struct lwp *XXX_IGNORED)
 *	Find a runnable lwp and switch to it.  Wait if necessary.
 */
ENTRY(cpu_switch)
	/* Save current lwp's context to switchframe */
	mov.l	.L_SF,	r0
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

.L_find_and_switch:
	/* Search next lwp. cpu_switch_search may or may not sleep. */
	mov.l	.L_cpu_switch_search, r0
	jsr	@r0
	 mov	r4,	r8	/* save old lwp */

	/* Skip context switch if same lwp. */
	cmp/eq	r8,	r0
	bt/s	1f
	 mov	r0,	r4	/* new lwp */

.L_doswitch:
	/* Setup kernel stack */
	mov.l	.L_SF,	r0
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
	 mov	r4,	r8	/* save new lwp */
	mov	r8,	r4
	__EXCEPTION_UNBLOCK(r0, r1)
	/* Now OK to use kernel stack. */

	/* Return 1 indicating "we switched". */
	bra	2f
	 mov	#1, r2

1:	/* Return 0 indicating "didn't switch". */
	mov	#0, r2

	/* Restore new lwp's context from switchframe */
	/* NOTE: r2 has return value! */
2:	mov.l	.L_SF,	r0
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

	/* r2 has the return value; stuff it into r0 now. */
	rts
	 mov	r2, r0
	.align	2
.L_SF:			.long	(L_MD_PCB)
.L_cpu_switch_search:	.long	_C_LABEL(cpu_switch_search)
FUNC_SYMBOL(switch_resume)



/*
 * LINTSTUB: Func: void cpu_switchto(struct lwp *current, struct lwp *next)
 *	Switch to the specified next LWP.
 */
ENTRY(cpu_switchto)
	/* Save current lwp's context to switchframe. */
	mov.l	.L_SFp,	r0
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

	/*
	 * curlwp = NULL;
	 * XXX Is this necessary?  We know we won't go idle.
	 */
	mov.l	.L_curlwp, r0
	mov	#0, r1
	mov.l	r1, @r0

	/* old and new lwps are already in r4, r5 */
	mov.l	.L_cpu_switch_prepare, r0
	jsr	@r0
	 nop

	/*
	 * Put the incoming LWP in r4 and jump into the middle
	 * of cpu_switch(), and let it do the work to restore the
	 * incoming LWP's context.
	 */
	bra	.L_doswitch
	 mov	r0, r4

	.align	2
.L_SFp:			.long	(L_MD_PCB)
.L_curlwp:		.long	_C_LABEL(curlwp)
.L_cpu_switch_prepare:	.long	_C_LABEL(cpu_switch_prepare)


/*
 * LINTSTUB: Func: void cpu_exit(struct lwp *l)
 *	Just fall-through to the switch_exit below.
 */
ENTRY(cpu_exit)
	mov.l	.L_switch_exit_lwp_exit2, r5	/* exit_func to call */
	/* FALLTHROUGH */

/*
 * LINTSTUB: Func: void switch_exit(struct lwp *l, void (*exit_func)(struct lwp *))
 *	Called only from cpu_exit(l).  Before we call exit_func to
 *	free lwp's resources (including kernel stack) we need to
 *	switch to the lwp0's kernel stack.  Then we jump into the
 *	middle of cpu_switch to find and switch to a new lwp.
 */
ALTENTRY(switch_exit)
	mov.l	.L_switch_exit_lwp0_pcb, r1
	mov.l	.L_switch_exit_curpcb, r0
	mov.l	@r1, r1
	mov.l	r1, @r0			/* curpcb = lwp0.l_md.md_pcb */

	mov.l	@(SF_R7_BANK, r1), r0	/* stack top */
	mov.l	@(SF_R6_BANK, r1), r2	/* current frame */
	mov.l	@(SF_R15, r1), r3	/* current stack */

	/* switch to lwp0's kernel stack */
	__EXCEPTION_BLOCK(r1, r6)
	ldc	r0, r7_bank
	ldc	r2, r6_bank
	mov	r3, r15
	__EXCEPTION_UNBLOCK(r0, r1)

	/* safe to call (*exit_func)(l); now */
	jsr	@r5
	 nop			/* lwp is already in r4 */

	/* proceed to cpu_switch */
	bra	.L_find_and_switch
	 mov	#0, r4		/* no "previous" lwp */

	.align 2
.L_switch_exit_lwp_exit2:
	.long	_C_LABEL(lwp_exit2)
.L_switch_exit_lwp0_pcb:
	.long	_C_LABEL(lwp0) + L_MD_PCB
.L_switch_exit_curpcb:
	.long	_C_LABEL(curpcb)


#ifdef SH3
/*
 * LINTSTUB: Func: void sh3_switch_resume(struct lwp *l)
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
	mov.l	.L_UPTE, r0
	add	r4,	r0
	mov.l	.L_curupte, r1
	mov.l	r0,	@r1
	rts
	 nop
	.align	2
.L_UPTE:		.long	L_MD_UPTE
.L_curupte:		.long	_C_LABEL(curupte)
	SET_ENTRY_SIZE(sh3_switch_resume)
#endif /* SH3 */


#ifdef SH4
/*
 * LINTSTUB: Func: void sh4_switch_resume(struct lwp *l)
 *	Wire u-area. invalidate TLB entry for kernel stack to prevent
 *	TLB multiple hit.
 */
NENTRY(sh4_switch_resume)
	mov.l	.L_UPTE__sh4, r0
	add	r0,	r4	/* l->l_md.md_upte */
	mov	#UPAGES,r3
	mov	#1,	r2
	mov.l	@r4,	r0	/* if (l->l_md.md_upte[0].addr == 0) return; */
	tst	r0,	r0
	bt	2f

	/* Save old ASID and set ASID to zero */
	xor	r0,	r0
	mov.l	.L_4_PTEH, r1
	mov.l	@r1,	r7
	mov.l	r0,	@r1

	mov.l	.L_VPN_MASK, r6
	mov.l	.L_4_UTLB_AA_A, r5

	/* TLB address array must be accessed via P2. Setup jump address. */
	mova	1f,	r0
	mov.l	.L_P2BASE, r1
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
	mov.l	.L_4_PTEH, r0
	mov.l	r7,	@r0
	mova	2f,	r0
	jmp	@r0		/* run P1 */
	 nop
	.align	2
2:	rts			/* mova target must be 4byte alignment */
	 nop
	.align	2
.L_UPTE__sh4:		.long	L_MD_UPTE
.L_4_PTEH:		.long	SH4_PTEH
.L_4_UTLB_AA_A:		.long	(SH4_UTLB_AA | SH4_UTLB_A)
.L_4_ITLB_AA:		.long	SH4_ITLB_AA
.L_VPN_MASK:		.long	0xfffff000
.L_P2BASE:		.long	0xa0000000
	SET_ENTRY_SIZE(sh4_switch_resume)
#endif /* SH4 */


/*
 * LINTSTUB: Func: int _cpu_intr_raise(int s)
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
	SET_ENTRY_SIZE(_cpu_intr_raise)


/*
 * LINTSTUB: Func: int _cpu_intr_suspend(void)
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
	SET_ENTRY_SIZE(_cpu_intr_suspend)



/*
 * LINTSTUB: Func: int _cpu_intr_resume(int s)
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
	SET_ENTRY_SIZE(_cpu_intr_resume)


/*
 * LINTSTUB: Func: int _cpu_exception_suspend(void)
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
	SET_ENTRY_SIZE(_cpu_exception_suspend)


/*
 * LINTSTUB: Func: void _cpu_exception_resume(int s)
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
	SET_ENTRY_SIZE(_cpu_exception_resume)


/*
 * LINTSTUB: Func: void _cpu_spin(uint32_t count)
 *	Loop for 'count' * 10 cycles.
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
	SET_ENTRY_SIZE(_cpu_spin)


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
	SET_ENTRY_SIZE(proc_trampoline)


#ifdef COMPAT_16
/*
 * LINTSTUB: Var: char sigcode[1]
 *	Signal trampoline. copied to top of user stack.
 *
 *	The kernel arranges for the signal handler to be invoked directly.
 *	This trampoline is used only to perform the return.
 *
 *	On entry, the stack looks like this:
 *
 *	sp->	sigcontext structure
 */
NENTRY(sigcode)
	mov	r15, r4			/* get pointer to sigcontext */
	mov.l	.L_SYS___sigreturn14, r0
	trapa	#0x80			/* and call sigreturn() */
	mov.l	.L_SYS_exit, r0
	trapa	#0x80			/* exit if sigreturn fails */
	/* NOTREACHED */

	.align	2
.L_SYS___sigreturn14:	.long	SYS_compat_16___sigreturn14
.L_SYS_exit:		.long	SYS_exit

/* LINTSTUB: Var: char esigcode[1] */
.globl	_C_LABEL(esigcode)
_C_LABEL(esigcode):
	SET_ENTRY_SIZE(sigcode)
#endif /* COMPAT_16 */

/*
 * LINTSTUB: Func: void savectx(struct pcb *pcb)
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
	SET_ENTRY_SIZE(savectx)


/*
 * LINTSTUB: Func: int copyout(const void *ksrc, void *udst, size_t len)
 *	Copy len bytes into the user address space.
 */
ENTRY(copyout)
	mov.l	r14,	@-r15
	sts.l	pr,	@-r15
	mov	r15,	r14

	mov	#EFAULT, r0		/* assume there was a problem */
	mov	r4,	r3
	mov	r5,	r2
	mov	r5,	r4
	add	r6,	r2
	cmp/hs	r5,	r2		/* bomb if uaddr+len wraps */
	bf	2f
	mov.l	.L_copyout_VM_MAXUSER_ADDRESS, r1
	cmp/hi	r1,	r2		/* bomb if uaddr isn't in user space */
	bt	2f

	mov.l	.L_copyout_curpcb, r1	/* set fault hander */
	mov.l	@r1,	r2
	mov.l	.L_copyout_onfault, r1
	mov.l	r1,	@(PCB_ONFAULT,r2)
	mov.l	.L_copyout_memcpy, r1
	jsr	@r1			/* memcpy(uaddr, kaddr, len) */
	 mov	r3,	r5

	mov	#0,	r0
1:
	mov.l	.L_copyout_curpcb, r1	/* clear fault handler */
	mov.l	@r1,	r2
	mov	#0,	r1
	mov.l	r1,	@(PCB_ONFAULT,r2)
2:
	mov	r14,	r15
	lds.l	@r15+,	pr
	rts
	 mov.l	@r15+,	r14

3:
	bra	1b
	 mov	#EFAULT, r0

	.align 2
.L_copyout_onfault:
	.long	3b
.L_copyout_VM_MAXUSER_ADDRESS:
	.long	VM_MAXUSER_ADDRESS
.L_copyout_curpcb:
	.long	_C_LABEL(curpcb)
.L_copyout_memcpy:
	.long	_C_LABEL(memcpy)
	SET_ENTRY_SIZE(copyout)


/*
 * LINTSTUB: Func: int copyin(const void *usrc, void *kdst, size_t len)
 *	Copy len bytes from the user address space.
 */
ENTRY(copyin)
	mov.l	r14,	@-r15
	sts.l	pr,	@-r15
	mov	r15,	r14

	mov	#EFAULT, r0		/* assume there was a problem */
	mov	r4,	r3
	mov	r5,	r4
	mov	r3,	r2
	add	r6,	r2
	cmp/hs	r3,	r2		/* bomb if uaddr+len wraps */
	bf	2f
	mov.l	.L_copyin_VM_MAXUSER_ADDRESS, r1
	cmp/hi	r1,	r2		/* bomb if uaddr isn't in user space */
	bt	2f

	mov.l	.L_copyin_curpcb, r1	/* set fault hander */
	mov.l	@r1,	r2
	mov.l	.L_copyin_onfault, r1
	mov.l	r1,	@(PCB_ONFAULT,r2)
	mov.l	.L_copyin_memcpy, r1
	jsr	@r1			/* memcpy(kaddr, uaddr, len) */
	 mov	r3,	r5

	mov	#0,	r0
1:
	mov.l	.L_copyin_curpcb, r1	/* clear fault hander */
	mov.l	@r1,	r2
	mov	#0,	r1
	mov.l	r1,	@(PCB_ONFAULT,r2)
2:
	mov	r14,	r15
	lds.l	@r15+,	pr
	rts
	 mov.l	@r15+,	r14

3:
	bra	1b
	 mov	#EFAULT, r0

	.align 2
.L_copyin_onfault:
	.long	3b
.L_copyin_VM_MAXUSER_ADDRESS:
	.long	VM_MAXUSER_ADDRESS
.L_copyin_curpcb:
	.long	_C_LABEL(curpcb)
.L_copyin_memcpy:
	.long	_C_LABEL(memcpy)
	SET_ENTRY_SIZE(copyin)


/*
 * LINTSTUB: Func: int copyoutstr(const void *ksrc, void *udst, size_t maxlen, size_t *lencopied)
 *	Copy a NUL-terminated string, at most maxlen characters long,
 *	into the user address space.  Return the number of characters
 *	copied (including the NUL) in *lencopied.  If the string is
 *	too long, return ENAMETOOLONG; else return 0 or EFAULT.
 */
ENTRY(copyoutstr)
	mov.l	r8,	@-r15

	mov	#EFAULT, r3		/* assume there was a problem */
	mov	r4,	r8
	mov.l	.L_copyoutstr_curpcb, r1	/* set fault handler */
	mov.l	@r1,	r2
	mov.l	.L_copyoutstr_onfault, r1
	mov.l	r1,	@(PCB_ONFAULT,r2)
	mov.l	.L_copyoutstr_VM_MAXUSER_ADDRESS, r1
	mov	r1,	r0
	sub	r5,	r0
	cmp/hi	r6,	r0		/* don't beyond user space */
	bf	2f
	bra	2f
	 mov	r6,	r0

	.align 2
1:
	mov.b	@r4+,	r1		/* copy str */
	mov.b	r1,	@r5
	extu.b	r1,	r1
	add	#1,	r5
	tst	r1,	r1
	bf	2f
	bra	3f
	 mov	#0,	r3
	.align 2
2:
	add	#-1,	r0
	cmp/eq	#-1,	r0
	bf	1b
	mov.l	.L_copyoutstr_VM_MAXUSER_ADDRESS, r1
	cmp/hs	r1,	r5
	bt	3f
	mov	#ENAMETOOLONG, r3

3:
	tst	r7,	r7		/* set lencopied if needed */
	bt	4f
	mov	r4,	r1
	sub	r8,	r1
	mov.l	r1,	@r7
4:
	mov.l	.L_copyoutstr_curpcb, r1	/* clear fault handler */
	mov.l	@r1,	r2
	mov	#0,	r1
	mov.l	r1,	@(PCB_ONFAULT,r2)

	mov	r3,	r0
	rts
	 mov.l	@r15+,	r8

5:
	bra	4b
	 mov	#EFAULT, r3

	.align 2
.L_copyoutstr_onfault:
	.long	5b
.L_copyoutstr_VM_MAXUSER_ADDRESS:
	.long	VM_MAXUSER_ADDRESS
.L_copyoutstr_curpcb:
	.long	_C_LABEL(curpcb)
	SET_ENTRY_SIZE(copyoutstr)


/*
 * LINTSTUB: Func: int copyinstr(const void *src, void *dst, size_t maxlen, size_t *lencopied)
 *	Copy a NUL-terminated string, at most maxlen characters long,
 *	from the user address space.  Return the number of characters
 *	copied (including the NUL) in *lencopied.  If the string is
 *	too long, return ENAMETOOLONG; else return 0 or EFAULT.
 */
ENTRY(copyinstr)
	mov.l	r8,	@-r15
	mov	#EFAULT, r3		/* assume there was a problem */
	mov	r4,	r8
	mov.l	.L_copyinstr_curpcb, r1	/* set fault handler */
	mov.l	@r1,	r2
	mov.l	.L_copyinstr_onfault, r1
	mov.l	r1,	@(PCB_ONFAULT,r2)

	mov.l	.L_copyinstr_VM_MAXUSER_ADDRESS, r1
	mov	r1,	r0
	sub	r5,	r0
	cmp/hi	r6,	r0		/* don't beyond user space */
	bf	2f
	bra	2f
	 mov	r6,	r0

	.align 2
1:
	mov.b	@r4+,	r1		/* copy str */
	mov.b	r1,	@r5
	extu.b	r1,	r1
	add	#1,	r5
	tst	r1,	r1
	bf	2f
	bra	3f
	 mov	#0,	r3
	.align 2
2:
	add	#-1,	r0
	cmp/eq	#-1,	r0
	bf	1b
	mov.l	.L_copyinstr_VM_MAXUSER_ADDRESS, r1
	cmp/hs	r1,	r5
	bt	3f
	mov	#ENAMETOOLONG, r3

3:
	tst	r7,	r7		/* set lencopied if needed */
	bt	4f
	mov	r4,	r1
	sub	r8,	r1
	mov.l	r1,	@r7
4:
	mov.l	.L_copyinstr_curpcb, r1	/* clear fault handler */
	mov.l	@r1,	r2
	mov	#0,	r1
	mov.l	r1,	@(PCB_ONFAULT,r2)

	mov	r3,	r0
	rts
	 mov.l	@r15+,	r8

5:
	bra	4b
	 mov	#EFAULT, r3

	.align 2
.L_copyinstr_onfault:
	.long	5b
.L_copyinstr_VM_MAXUSER_ADDRESS:
	.long	VM_MAXUSER_ADDRESS
.L_copyinstr_curpcb:
	.long	_C_LABEL(curpcb)
	SET_ENTRY_SIZE(copyinstr)



/*
 * LINTSTUB: Func: long	fuword(const void *uaddr)
 *	Fetch an int from the user address space.
 */
ENTRY(fuword)
	mov.l	.L_fuword_VM_MAXUSER_ADDRESS, r1
	cmp/hi	r1,	r4		/* bomb if uaddr isn't in user space */
	bt/s	2f
	 mov	#-1,	r0
	mov.l	.L_fuword_curpcb, r1	/* set fault handler */
	mov.l	@r1,	r2
	mov.l	.L_fuword_onfault, r1
	mov.l	r1,	@(PCB_ONFAULT,r2)
	mov.l	@r4,	r0		/* fetch the value */
1:
	mov.l	.L_fuword_curpcb, r1	/* clear fault handler */
	mov.l	@r1,	r2
	mov	#0,	r1
	mov.l	r1,	@(PCB_ONFAULT,r2)
2:
	rts
	 nop

3:
	bra	1b
	 mov	#-1,	r0

	.align 2
.L_fuword_onfault:
	.long	3b
.L_fuword_VM_MAXUSER_ADDRESS:
	.long	VM_MAXUSER_ADDRESS - 4 /*sizeof(long)*/
.L_fuword_curpcb:
	.long	_C_LABEL(curpcb)
	SET_ENTRY_SIZE(fuword)


/*
 * LINTSTUB: Func: int fusword(const void *uaddr)
 *	Fetch a short from the user address space.
 */
ENTRY(fusword)
	mov.l	.L_fusword_VM_MAXUSER_ADDRESS, r1
	cmp/hi	r1,	r4		/* bomb if uaddr isn't user space */
	bt/s	2f
	 mov	#-1,	r0
	mov.l	.L_fusword_curpcb, r1	/* set fault handler */
	mov.l	@r1,	r2
	mov.l	.L_fusword_onfault, r1
	mov.l	r1,	@(PCB_ONFAULT,r2)
	mov.w	@r4,	r1		/* fetch the value */
	exts.w	r1,	r0
1:
	mov.l	.L_fusword_curpcb, r1	/* clear fault handler */
	mov.l	@r1,	r2
	mov	#0,	r1
	mov.l	r1,	@(PCB_ONFAULT,r2)
2:
	rts
	 nop

3:
	bra	1b
	 mov	#-1,	r0

	.align 2
.L_fusword_onfault:
	.long	3b
.L_fusword_VM_MAXUSER_ADDRESS:
	.long	VM_MAXUSER_ADDRESS - 2 /*sizeof(short)*/
.L_fusword_curpcb:
	.long	_C_LABEL(curpcb)
	SET_ENTRY_SIZE(fusword)


/*
 * LINTSTUB: Func: int fuswintr(const void *uaddr)
 *	Fetch a short from the user address space.
 *	Can be called during an interrupt.
 */
ENTRY(fuswintr)
	mov.l	.L_fuswintr_VM_MAXUSER_ADDRESS, r1
	cmp/hi	r1,	r4		/* bomb if uaddr isn't user space */
	bt/s	2f
	 mov	#-1,	r0
	mov.l	.L_fuswintr_curpcb, r1	/* set fault handler */
	mov.l	@r1,	r2
	mov.l	.L_fuswintr_onfault, r1
	mov.l	r1,	@(PCB_ONFAULT,r2)
	mov	#1,	r1		/* set faultbail */
	mov.l	r1,	@(PCB_FAULTBAIL,r2)
	mov.w	@r4,	r1		/* fetch the value */
	exts.w	r1,	r0
1:
	mov.l	.L_fuswintr_curpcb, r1	/* clear fault handler and faultbail */
	mov.l	@r1,	r2
	mov	#0,	r1
	mov.l	r1,	@(PCB_ONFAULT,r2)
	mov.l	r1,	@(PCB_FAULTBAIL,r2)
2:
	rts
	 nop

3:
	bra	1b
	 mov	#-1,	r0

	.align 2
.L_fuswintr_onfault:
	.long	3b
.L_fuswintr_VM_MAXUSER_ADDRESS:
	.long	VM_MAXUSER_ADDRESS - 2 /*sizeof(short)*/
.L_fuswintr_curpcb:
	.long	_C_LABEL(curpcb)
	SET_ENTRY_SIZE(fuswintr)


/*
 * LINTSTUB: Func: int fubyte(const void *uaddr)
 *	Fetch a byte from the user address space.
 */
ENTRY(fubyte)
	mov.l	.L_fubyte_VM_MAXUSER_ADDRESS, r1
	cmp/hi	r1,	r4		/* bomb if uaddr isn't in user space */
	bt/s	2f
	 mov	#-1,	r0
	mov.l	.L_fubyte_curpcb, r1	/* set fault handler */
	mov.l	@r1,	r2
	mov.l	.L_fubyte_onfault, r1
	mov.l	r1,	@(PCB_ONFAULT,r2)
	mov.b	@r4,	r1		/* fetch the value */
	exts.b	r1,	r0
1:
	mov.l	.L_fubyte_curpcb, r1		/* clear fault handler */
	mov.l	@r1,	r2
	mov	#0,	r1
	mov.l	r1,	@(PCB_ONFAULT,r2)
2:
	rts
	 nop

3:
	bra	1b
	 mov	#-1,	r0

	.align 2
.L_fubyte_onfault:
	.long	3b
.L_fubyte_VM_MAXUSER_ADDRESS:
	.long	VM_MAXUSER_ADDRESS - 1 /*sizeof(char)*/
.L_fubyte_curpcb:
	.long	_C_LABEL(curpcb)
	SET_ENTRY_SIZE(fubyte)


/*
 * LINTSTUB: Func: int suword(void *uaddr, long x)
 *	Store an int in the user address space.
 */
ENTRY(suword)
	mov.l	.L_suword_VM_MAXUSER_ADDRESS, r1
	cmp/hi	r1,	r4		/* bomb if uaddr isn't in user space */
	bt/s	2f
	 mov	#-1,	r0
	mov.l	.L_suword_curpcb, r1	/* set fault handler */
	mov.l	@r1,	r2
	mov.l	.L_suword_onfault, r1
	mov.l	r1,	@(PCB_ONFAULT,r2)
	mov.l	r5,	@r4		/* store the value */
	mov	#0,	r0
1:
	mov.l	.L_suword_curpcb, r1	/* clear fault handler */
	mov.l	@r1,	r2
	mov	#0,	r1
	mov.l	r1,	@(PCB_ONFAULT,r2)
2:
	rts
	 nop

3:
	bra	1b
	 mov	#-1,	r0

	.align 2
.L_suword_onfault:
	.long	3b
.L_suword_VM_MAXUSER_ADDRESS:
	.long	VM_MAXUSER_ADDRESS - 4 /*sizeof(long)*/
.L_suword_curpcb:
	.long	_C_LABEL(curpcb)
	SET_ENTRY_SIZE(suword)


/*
 * LINTSTUB: Func: int susword(void *uaddr, short x)
 *	Store a short in the user address space.
 */
ENTRY(susword)
	mov.l	.L_susword_VM_MAXUSER_ADDRESS, r1
	cmp/hi	r1,	r4		/* bomb if uaddr isn't in user space */
	bt/s	2f
	 mov	#-1,	r0
	mov.l	.L_susword_curpcb, r1	/* set fault handler */
	mov.l	@r1,	r2
	mov.l	.L_susword_onfault, r1
	mov.l	r1,	@(PCB_ONFAULT,r2)
	mov.w	r5,	@r4		/* store the value */
	mov	#0,	r0
1:
	mov.l	.L_susword_curpcb, r1	/* clear fault handler */
	mov.l	@r1,	r2
	mov	#0,	r1
	mov.l	r1,	@(PCB_ONFAULT,r2)
2:
	rts
	 nop

3:
	bra	1b
	 mov	#-1,	r0

	.align 2
.L_susword_onfault:
	.long	3b
.L_susword_VM_MAXUSER_ADDRESS:
	.long	VM_MAXUSER_ADDRESS - 2 /*sizeof(short)*/
.L_susword_curpcb:
	.long	_C_LABEL(curpcb)
	SET_ENTRY_SIZE(susword)


/*
 * LINTSTUB: Func: int suswintr(void *uaddr, short x)
 *	Store a short in the user address space.
 *	Can be called during an interrupt.
 */
ENTRY(suswintr)
	mov.l	.L_suswintr_VM_MAXUSER_ADDRESS, r1
	cmp/hi	r1,	r4		/* bomb if uaddr isn't in user space */
	bt/s	2f
	 mov	#-1,	r0
	mov.l	.L_suswintr_curpcb, r1	/* set fault handler */
	mov.l	@r1,	r2
	mov.l	.L_suswintr_onfault, r1
	mov.l	r1,	@(PCB_ONFAULT,r2)
	mov	#1,	r1		/* set faultbail */
	mov.l	r1,	@(PCB_FAULTBAIL,r2)
	mov.w	r5,	@r4		/* store the value */
	mov	#0,	r0
1:
	mov.l	.L_suswintr_curpcb, r1	/* clear fault handler and faultbail */
	mov.l	@r1,	r2
	mov	#0,	r1
	mov.l	r1,	@(PCB_ONFAULT,r2)
	mov.l	r1,	@(PCB_FAULTBAIL,r2)
2:
	rts
	 nop

3:
	bra	1b
	 mov	#-1,	r0

	.align 2
.L_suswintr_onfault:
	.long	3b
.L_suswintr_VM_MAXUSER_ADDRESS:
	.long	VM_MAXUSER_ADDRESS - 2 /*sizeof(short)*/
.L_suswintr_curpcb:
	.long	_C_LABEL(curpcb)
	SET_ENTRY_SIZE(suswintr)


/*
 * LINTSTUB: Func: int subyte(void *uaddr, int x);
 *	Store a byte in the user address space.
 */
ENTRY(subyte)
	mov.l	.L_subyte_VM_MAXUSER_ADDRESS, r1
	cmp/hi	r1,	r4		/* bomb if uaddr isn't in user space */
	bt/s	2f
	 mov	#-1,	r0
	mov.l	.L_subyte_curpcb, r1	/* set fault handler */
	mov.l	@r1,	r2
	mov.l	.L_subyte_onfault, r1
	mov.l	r1,	@(PCB_ONFAULT,r2)
	mov.b	r5,	@r4		/* store the value */
	mov	#0,	r0
1:
	mov.l	.L_subyte_curpcb, r1	/* clear fault handler */
	mov.l	@r1,	r2
	mov	#0,	r1
	mov.l	r1,	@(PCB_ONFAULT,r2)
2:
	rts
	 nop

3:
	bra	1b
	 mov	#-1,	r0

	.align 2
.L_subyte_onfault:
	.long	3b
.L_subyte_VM_MAXUSER_ADDRESS:
	.long	VM_MAXUSER_ADDRESS - 1 /*sizeof(char)*/
.L_subyte_curpcb:
	.long	_C_LABEL(curpcb)
	SET_ENTRY_SIZE(subyte)


/*
 * LINTSTUB: Func: int kcopy(const void *src, void *dst, size_t len)
 */
ENTRY(kcopy)
	mov.l	r8,	@-r15
	mov.l	r14,	@-r15
	sts.l	pr,	@-r15
	mov	r15,	r14

	mov	r4,	r3
	mov.l	.L_kcopy_curpcb, r1
	mov.l	@r1,	r2
	mov.l	@(PCB_ONFAULT,r2) ,r8	/* save old fault handler */
	mov.l	.L_kcopy_onfault, r1
	mov.l	r1,	@(PCB_ONFAULT,r2) /* set fault handler */
	mov.l	.L_kcopy_memcpy, r1
	mov	r5,	r4
	jsr	@r1			/* memcpy(dst, src, len) */
	 mov	r3,	r5
	mov	#0,	r0
1:
	mov.l	.L_kcopy_curpcb, r1	/* restore fault handler */
	mov.l	@r1,	r2
	mov.l	r8,	@(PCB_ONFAULT,r2)

	mov	r14,	r15
	lds.l	@r15+,	pr
	mov.l	@r15+,	r14
	rts
	 mov.l	@r15+,	r8

2:
	bra	1b
	 mov	#EFAULT, r0

	.align 2
.L_kcopy_onfault:
	.long	2b
.L_kcopy_curpcb:
	.long	_C_LABEL(curpcb)
.L_kcopy_memcpy:
	.long	_C_LABEL(memcpy)
	SET_ENTRY_SIZE(kcopy)


#if defined(DDB) || defined(KGDB)

/*
 * LINTSTUB: Func: int setjmp(label_t *jmpbuf)
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
	SET_ENTRY_SIZE(setjmp)

/*
 * LINTSTUB: Func: void longjmp(label_t *jmpbuf)
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
	SET_ENTRY_SIZE(longjmp)

#endif /* DDB || KGDB */
