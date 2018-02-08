/*	$NetBSD: locore_subr.S,v 1.56 2018/02/08 09:05:18 dholland Exp $	*/

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
#include "opt_modular.h"
#include "assym.h"

#include <sys/syscall.h>	/* SYS___sigreturn14, SYS_exit */
#include <sh3/asm.h>
#include <sh3/locore.h>
#include <sh3/param.h>		/* UPAGES */
#include <sh3/mmu_sh3.h>
#include <sh3/mmu_sh4.h>

__KERNEL_RCSID(0, "$NetBSD: locore_subr.S,v 1.56 2018/02/08 09:05:18 dholland Exp $")


/*
 * LINTSTUB: include <sys/types.h>
 * LINTSTUB: include <sys/lwp.h>
 * LINTSTUB: include <sh3/locore.h>
 */


/*
 * Save processor state to pcb->pcb_sf switchframe.
 * Note that offsetof(struct pcb, pcb_sf) is zero.
 */
#define SAVEPCB_AND_JUMP(pcb, jump)			  \
	add	#SF_SIZE, pcb				; \
	stc.l	r7_bank, @-pcb				; \
	stc.l	r6_bank, @-pcb				; \
	mov.l	r15, @-pcb				; \
	mov.l	r14, @-pcb				; \
	mov.l	r13, @-pcb				; \
	mov.l	r12, @-pcb				; \
	mov.l	r11, @-pcb				; \
	mov.l	r10, @-pcb				; \
	mov.l	r9, @-pcb				; \
	mov.l	r8, @-pcb				; \
	sts.l	pr, @-pcb				; \
	stc.l	sr, @-pcb				; \
	jump						; \
	 stc.l	gbr, @-pcb

/* Hide ugly empty argument if we don't need the jump */
#define SAVEPCB(pcb) \
	SAVEPCB_AND_JUMP(pcb, /* no jump */)


	.text
	.align 5	/* align cache line size (32B) */
/*
 * LINTSTUB: Func: lwp_t *cpu_switchto(lwp_t *olwp, lwp_t *nlwp, bool returning)
 *	Switch from olwp to nlwp.
 *	If returning is true, we do the fast softint dance
 *	 and can skip user-space related activities (pmap, ras, etc...)
 *	Return olwp (in the nlwp context).
 */
ENTRY(cpu_switchto)
	tst	r4, r4			! olwp can be NULL
	bt	.L_saved

	!! save old lwp's context to switchframe
	mov.l	@(L_MD_PCB, r4), r1	! olwp->l_md.md_pcb
	SAVEPCB(r1)
.L_saved:
	!! free to use callee-save registers now

	mov.l	.L_curlwp, r2
	mov.l	.L_curpcb, r3
	mov.l	@(L_MD_PCB, r5), r10	! nlwp->l_md.md_pcb
	!tst	r6, r6			! "full" switch?
	mov.l	r5, @r2			! curlwp = nlwp;
	!bt/s	.L_prepare_switch
	 mov.l	r10, @r3		! curpcb = nlwp->l_md.md_pcb;

	!mov	r5, r8		! preserve nlwp
	!bra	.L_restore_nlwp
	! mov	r4, r9		! preserve olwp

.L_prepare_switch:
	!! arguments to cpu_switch_prepare are already in the right registers
	mov.l	.L_cpu_switch_prepare, r0
	mov	r5, r8		! preserve nlwp
	jsr	@r0
	 mov	r4, r9		! preserve olwp

.L_restore_nlwp:
	!! restore new lwp's context from switchframe
	!! r10 is nlwp->l_md.md_pcb == &nlwp->l_md.md_pcb->pcb_sf

	!! setup nlwp's kernel stack first
	mov.l	@(SF_R7_BANK, r10), r0	! kernel stack bottom
	mov.l	@(SF_R6_BANK, r10), r2	! current trapframe
	mov.l	@(SF_R15, r10), r3	! current kernel sp

	!! while switching kernel stack, all exceptions must be disabled
	__EXCEPTION_BLOCK(r1, r11)	! saves SR in r11
	ldc	r0, r7_bank
	ldc	r2, r6_bank
	mov	r3, r15

#if !defined(P1_STACK) && defined(SH4)
	!! wire u-area in TLB
	MOV	(switch_resume, r0)
	jsr	@r0
	 mov	r8, r4		! nlwp
#endif
	!! safe to use nlwp's kernel stack now
	ldc	r11, sr		! __EXCEPTION_UNBLOCK

	!! finish restoring new lwp's context from switchframe
	!! sf_r15, sf_r6_bank, sf_r7_bank are already restored
	mov	r10, r1		! &nlwp->l_md.md_pcb->pcb_sf
	mov	r9, r0		! return olwp (we are about to clobber r9)
	ldc.l	@r1+, gbr
	ldc.l	@r1+, sr
	lds.l	@r1+, pr
	mov.l	@r1+, r8
	mov.l	@r1+, r9
	mov.l	@r1+, r10
	mov.l	@r1+, r11
	mov.l	@r1+, r12
	mov.l	@r1+, r13
	rts
	 mov.l	@r1+, r14

	.align	2
.L_curlwp:		.long	_C_LABEL(curlwp)
.L_curpcb:		.long	_C_LABEL(curpcb)
.L_cpu_switch_prepare:	.long	_C_LABEL(cpu_switch_prepare)
#ifdef SH4
FUNC_SYMBOL(switch_resume)
#endif
	SET_ENTRY_SIZE(cpu_switchto)


#ifdef SH3
/*
 * LINTSTUB: Func: void sh3_switch_resume(struct lwp *l)
 *     We only need this dummy sh3 version if both SH3 and SH4 are defined.
 */
NENTRY(sh3_switch_resume)
	rts
	 nop
	SET_ENTRY_SIZE(sh3_switch_resume)
#endif /* SH3 */


#ifdef SH4
/*
 * LINTSTUB: Func: void sh4_switch_resume(struct lwp *l)
 *	Wire u-area. invalidate TLB entry for kernel stack to prevent
 *	TLB multiple hit.
 */
NENTRY(sh4_switch_resume)
	add	#L_MD_UPTE, r4	! l->l_md.md_upte
	mov	#UPAGES, r3
	mov.l	@r4, r0		! if (l->l_md.md_upte[0].addr == 0) return;
	tst	r0, r0
	bt	2f

	/* Save old ASID and set ASID to zero */
	mov	#0, r0
	mov.l	.L_4_PTEH, r2
	mov.l	@r2, r7
	mov.l	r0, @r2

	mov.l	.L_VPN_MASK, r6
	mov.l	.L_4_UTLB_AA_A, r5

	/* TLB address array must be accessed via P2. Setup jump address. */
	mova	1f, r0
	mov.l	.L_P2BASE, r1
	or	r1, r0
	jmp	@r0		! run on P2
	 nop

	/* Probe VPN match TLB entry and invalidate it. */
	.align	2		! mova target must be 4byte aligned
1:	mov.l	@(4, r4), r0
	and	r6, r0
	mov.l	r0, @r5		! clear D, V

	/* Wire u-area TLB entry */
	/* Address array */
	mov.l	@r4+, r0	! addr
	mov.l	@r4+, r1	! data
	mov.l	r1, @r0		! *addr = data

	/* Data array */
	mov.l	@r4+, r0	! addr
	mov.l	@r4+, r1	! data
	mov.l	r1, @r0		! *addr = data
	dt	r3
	bf	1b

	/* restore ASID */
	mov.l	r7, @r2

2:	rts			! to the caller in P1
	 nop

	.align	2
.L_4_PTEH:		.long	SH4_PTEH
.L_4_UTLB_AA_A:		.long	(SH4_UTLB_AA | SH4_UTLB_A)
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
 * lwp_trampoline:
 *
 * cpu_lwp_fork() arranges for lwp_trampoline() to run when that
 * nascent lwp is selected by cpu_switchto().
 *
 * The switch frame will contain pointer to struct lwp of this lwp in
 * r10, a pointer to the function to call in r12, and an argument to
 * pass to it in r11 (we abuse the callee-saved registers).
 *
 * We enter lwp_trampoline as if we are "returning" from
 * cpu_switchto(), so r0 contains previous lwp (the one we are
 * switching from) that we pass to lwp_startup().
 *
 * After that the trampoline should call the function that is intended
 * to do some additional setup.  When the function returns, the
 * trampoline returns to the user mode.
 */
NENTRY(lwp_trampoline)
	mov.l	.L_lwp_startup, r1
	mov	r0, r4		/* previous lwp returned by cpu_switchto */
	jsr	@r1
	 mov	r10, r5		/* my struct lwp */
	jsr	@r12
	 mov	r11, r4
	__EXCEPTION_RETURN
	/* NOTREACHED */

	.align	2
.L_lwp_startup:		.long	_C_LABEL(lwp_startup)

	SET_ENTRY_SIZE(lwp_trampoline)


#if defined(COMPAT_16) || defined(MODULAR)
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
#endif /* COMPAT_16 || MODULAR */


/*
 * LINTSTUB: Func: void savectx(struct pcb *pcb)
 *	Save CPU state in pcb->pcb_sf
 */
ENTRY(savectx)
	SAVEPCB_AND_JUMP(r4, rts)
	SET_ENTRY_SIZE(savectx)


/*
 * LINTSTUB: Func: int copyout(const void *ksrc, void *udst, size_t len)
 *	Copy len bytes into the user address space.
 */
ENTRY(copyout)
	mov.l	r14,	@-r15
	sts.l	pr,	@-r15
	mov	r15,	r14

	mov	r4,	r3
	mov	r5,	r2
	mov	r5,	r4
	add	r6,	r2
	cmp/hs	r5,	r2		/* bomb if uaddr+len wraps */
	bf	3f
	mov.l	.L_copyout_VM_MAXUSER_ADDRESS, r1
	cmp/hi	r1,	r2		/* bomb if uaddr isn't in user space */
	bt	3f

	mov.l	.L_copyout_curpcb, r1	/* set fault handler */
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
	bra	2b
	 mov	#EFAULT, r0

	.align 2
.L_copyout_onfault:
	.long	1b
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

	mov	r4,	r3
	mov	r5,	r4
	mov	r3,	r2
	add	r6,	r2
	cmp/hs	r3,	r2		/* bomb if uaddr+len wraps */
	bf	3f
	mov.l	.L_copyin_VM_MAXUSER_ADDRESS, r1
	cmp/hi	r1,	r2		/* bomb if uaddr isn't in user space */
	bt	3f

	mov.l	.L_copyin_curpcb, r1	/* set fault handler */
	mov.l	@r1,	r2
	mov.l	.L_copyin_onfault, r1
	mov.l	r1,	@(PCB_ONFAULT,r2)
	mov.l	.L_copyin_memcpy, r1
	jsr	@r1			/* memcpy(kaddr, uaddr, len) */
	 mov	r3,	r5

	mov	#0,	r0
1:
	mov.l	.L_copyin_curpcb, r1	/* clear fault handler */
	mov.l	@r1,	r2
	mov	#0,	r1
	mov.l	r1,	@(PCB_ONFAULT,r2)
2:
	mov	r14,	r15
	lds.l	@r15+,	pr
	rts
	 mov.l	@r15+,	r14

3:
	bra	2b
	 mov	#EFAULT, r0

	.align 2
.L_copyin_onfault:
	.long	1b
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

	mov	r4,	r8
	mov.l	.L_copyoutstr_curpcb, r1	/* set fault handler */
	mov.l	@r1,	r2
	mov.l	.L_copyoutstr_onfault, r1
	mov.l	r1,	@(PCB_ONFAULT,r2)
	mov.l	.L_copyoutstr_VM_MAXUSER_ADDRESS, r3
	cmp/hi	r3,	r5		/* bomb if udst isn't in user space */
	bt	5f
	mov	r3,	r0
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
	 mov	#0,	r0

	.align 2
2:
	add	#-1,	r0
	cmp/eq	#-1,	r0
	bf	1b
	cmp/hi	r3,	r5
	bf	6f
	mov	#0,	r0

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

	rts
	 mov.l	@r15+,	r8

5:
	bra	4b
	 mov	#EFAULT, r0

6:	bra	3b
	 mov	#ENAMETOOLONG, r0

	.align 2
.L_copyoutstr_onfault:
	.long	4b
.L_copyoutstr_VM_MAXUSER_ADDRESS:
	.long	VM_MAXUSER_ADDRESS
.L_copyoutstr_curpcb:
	.long	_C_LABEL(curpcb)
	SET_ENTRY_SIZE(copyoutstr)


/*
 * LINTSTUB: Func: int copyinstr(const void *usrc, void *kdst, size_t maxlen, size_t *lencopied)
 *	Copy a NUL-terminated string, at most maxlen characters long,
 *	from the user address space.  Return the number of characters
 *	copied (including the NUL) in *lencopied.  If the string is
 *	too long, return ENAMETOOLONG; else return 0 or EFAULT.
 */
ENTRY(copyinstr)
	mov.l	r8,	@-r15

	mov	r4,	r8
	mov.l	.L_copyinstr_curpcb, r1	/* set fault handler */
	mov.l	@r1,	r2
	mov.l	.L_copyinstr_onfault, r1
	mov.l	r1,	@(PCB_ONFAULT,r2)

	mov.l	.L_copyinstr_VM_MAXUSER_ADDRESS, r3
	cmp/hi	r3,	r4		/* bomb if usrc isn't in user space */
	bt	5f
	mov	r3,	r0
	sub	r4,	r0
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
	 mov	#0,	r0

	.align 2
2:
	add	#-1,	r0
	cmp/eq	#-1,	r0
	bf	1b
	cmp/hi	r3,	r4
	bf	6f
	mov	#0,	r0

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

	rts
	 mov.l	@r15+,	r8

5:
	bra	4b
	 mov	#EFAULT, r0

6:
	bra	3b
	 mov	#ENAMETOOLONG, r0

	.align 2
.L_copyinstr_onfault:
	.long	4b
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
	extu.w	r1,	r0
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
	extu.w	r1,	r0
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
	extu.b	r1,	r0
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

	.align 2
.L_kcopy_onfault:
	.long	1b
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
	 mov	#1, r0		/* return 1 from setjmp */
	SET_ENTRY_SIZE(longjmp)

#endif /* DDB || KGDB */
