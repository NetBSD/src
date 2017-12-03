/*	$NetBSD: exception_vector.S,v 1.49.14.1 2017/12/03 11:36:42 jdolecek Exp $	*/

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

#include "opt_cputype.h"
#include "opt_ddb.h"
#include "opt_ptrace.h"

#include "assym.h"

#include <sh3/param.h>
#include <sh3/locore.h>
#include <sh3/exception.h>
#include <sh3/ubcreg.h>
#include <sh3/pte.h>
#include <sh3/mmu_sh3.h>
#include <sh3/mmu_sh4.h>

/* 
 * Align vectors more strictly here (where we don't really care) so
 * that .align 5 (i.e. 32B cache line) before data block does the
 * right thing w.r.t. final destinations after vectors are copied.
 */
#define _ALIGN_TEXT	.align 5
#include <sh3/asm.h>

__KERNEL_RCSID(0, "$NetBSD: exception_vector.S,v 1.49.14.1 2017/12/03 11:36:42 jdolecek Exp $")


/*
 * Exception vectors.
 * The following routines are copied to vector addresses.
 *	sh_vector_generic:	VBR + 0x100
 *	sh_vector_tlbmiss:	VBR + 0x400
 *	sh_vector_interrupt:	VBR + 0x600
 */

#define VECTOR_END_MARKER(sym)			\
		.globl	_C_LABEL(sym);		\
	_C_LABEL(sym):


/*
 * LINTSTUB: Var: char sh_vector_generic[1];
 *
 * void sh_vector_generic(void);
 *	Copied to VBR+0x100.  This code should be position independent
 *	and maximum 786 bytes long (== 0x400 - 0x100).
 */
NENTRY(sh_vector_generic)
	__EXCEPTION_ENTRY
	/* Identify exception cause */
	MOV	(EXPEVT, r0)
	mov.l	@r0, r0
	mov.l	r0, @(TF_EXPEVT, r14)	/* tf->tf_expevt = EXPEVT */
	/* Get curlwp */
	mov.l	.Lg_curlwp, r1
	mov.l	@r1, r4			/* 1st arg */
	/* Get TEA */
	MOV	(TEA, r1)
	mov.l	@r1, r6			/* 3rd arg */
	/* Check TLB exception or not */
	mov.l	.Lg_TLB_PROT_ST, r1
	cmp/hi	r1, r0
	bt	1f

	/* tlb_exception(curlwp, tf, TEA); */
	__INTR_MASK_EXCEPTION_UNBLOCK(r0, r1, r3)
	mov.l	.Lg_tlb_exception, r0
	jsr	@r0
	 mov	r14, r5			/* 2nd arg */
	bra	.Lg_return_from_exception
	 nop

	/* general_exception(curlwp, tf, TEA); */
1:	mov	r4, r8
#if defined(PTRACE_HOOKS) || defined(DDB)
	mov	#0, r2
	MOV	(BBRA, r1)
	mov.l	r2, @(TF_UBC, r14)	/* clear tf->tf_ubc */
	mov.w	r2, @r1			/* disable UBC channel A */
#endif
	__INTR_MASK_EXCEPTION_UNBLOCK(r0, r1, r3)
	mov.l	.Lg_general_exception, r0
	jsr	@r0
	 mov	r14, r5			/* 2nd arg */

	/* Check for ASTs on exit to user mode. */
	mov	r8, r4
	mov.l	.Lg_ast, r0
	jsr	@r0
	 mov	r14, r5

#if defined(PTRACE_HOOKS) || defined(DDB)
	mov.l	@(TF_UBC, r14), r2
	tst	r2, r2			! single-step == 0?
	bt	.Lg_return_from_exception

	!! We are returning from DDB to do single step.  Channel A in
	!! UBC is already rigged, we just need to enable it.
	MOV	(BBRA, r3)
	MOV	(BARA, r5)
	__EXCEPTION_BLOCK(r0, r1)
	mov.l	@(TF_SPC, r14), r4
	mov.l	r4, @r5			! BARA = tf->tf_spc
	mov.w	r2, @r3			! BBRA = tf->tf_ubc
#endif /* PTRACE || DDB */
.Lg_return_from_exception:
	__EXCEPTION_RETURN

	.align	5
REG_SYMBOL(EXPEVT)
.Lg_curlwp:		.long	_C_LABEL(curlwp)
REG_SYMBOL(TEA)
.Lg_TLB_PROT_ST:	.long	EXPEVT_TLB_PROT_ST
.Lg_tlb_exception:	.long	_C_LABEL(tlb_exception)
.Lg_general_exception:	.long	_C_LABEL(general_exception)
.Lg_ast:		.long	_C_LABEL(ast)
REG_SYMBOL(BBRA)
REG_SYMBOL(BARA)

/* LINTSTUB: Var: char sh_vector_generic_end[1]; */
VECTOR_END_MARKER(sh_vector_generic_end)
	SET_ENTRY_SIZE(sh_vector_generic)


#ifdef SH3
/*
 * LINTSTUB: Var: char sh3_vector_tlbmiss[1];
 *
 * TLB miss vector.  We run through the fast path first, checking if
 * there's a valid mapping in curlwp or kernel pmap.  We do fast path
 * with exceptions disabled, so no P3 addresses please (including no
 * kernel stack, as we cannot wire TLB entries on sh3).  We can only
 * use BANK1 registers, and of those r6 and r7 are already taken.
 *
 * If we don't find a valid mapping in the fast path, we do context
 * save and call tlb exception handler.
 *
 * Copied to VBR+0x400.  This code should be position independent
 * and maximum 512 bytes long (== 0x600 - 0x400).
 */
NENTRY(sh3_vector_tlbmiss)
	mov	#(SH3_PTEH & 0xff), r4
	mov.l	.L3_VPN_cleanup, r0
	mov.l	@r4, r5
	and	r0, r5		! trim vpn to 4K page boundary
	!! For the duration of fast path we keep
	!! r4: SH3_PTEH - other PTE regs are addressable as @(offset, r4)
	!! r5: { VPN, ASID } that caused the miss

	cmp/pz	r5		! user space address?
	bt/s	.L3_user_va
	 mov	r5, r2		! copy of vpn to compute indices into ptd/ptp

	!! kernel space address, use pmap_kernel(), adjust vpn for indexing
	!! see __pmap_kpte_lookup
.L3_kernel_va:
	mov.l	.L3_VM_MIN_KERNEL_ADDRESS, r0
	mov.l	.L3_kernptd,  r1 ! pmap_kernel()->pm_ptp
	bra	.L3_fetch_pte
	 sub	r0, r2		! vpn -= VM_MIN_KERNEL_ADDRESS

	!! user space address, use curlwp's pmap
.L3_user_va:
	mov.l	.L3_curptd,  r1	! curlwp->...->pm_ptp

	!! see __pmap_pte_lookup
.L3_fetch_pte:
	mov.l	@r1, r3		! fetch ptd

	!! r2: vpn, prepared for indexing into ptd
	!! r3: pt_entry_t **ptd => pt_entry_t *ptp => pt_entry_t pte
#ifdef DEBUG
	tst	r3, r3		! ptd == NULL  - cannot happen
	bt/s	.L3_call_tlb_exception
#endif
	 mov	#-22, r1	! __PMAP_PTP_SHIFT

	!! __PMAP_PTP_INDEX(vpn)
	mov	r2, r0
	shld	r1, r0		! vpn >> __PMAP_PTP_SHIFT
	mov.l	.L3_ptp_index_mask, r1
	and	r1, r0		! ... & (__PMAP_PTP_N - 1)
	shll2	r0		! array index -> array offset
	mov.l	@(r0, r3), r3	! ptp = ptd[idx]
	tst	r3, r3		! if (ptp == NULL)
	bt/s	.L3_call_tlb_exception
	 mov	#-(PGSHIFT - 2), r1

	!! __PMAP_PTP_OFSET(vpn) - except we pre-shift 2 bits left to
	!! get the array offset directly, as we know bits 10 and 11
	!! are zero (we cleaned them in r5 to get 4K aligned VPN)
	shld	r1, r2		! vpn >> (PGSHIFT - 2)
	mov.l	.L3_ptp_offset_mask, r0
	and	r2, r0		! ... & ((__PMAP_PTP_PG_N - 1) << 2)
	mov.l	@(r0, r3), r3	! pte = ptp[idx]


	!! r3: pte
	!! r4: SH3_PTEH
	!! r5: { VPN, ASID }

	mov.l	.L3_PG_V, r0
	tst	r0, r3		! if ((pte & PG_V) == 0)
	bt/s	.L3_call_tlb_exception
	 nop

	mov.l	.L3_PG_HW_BITS, r1
	cmp/pz	r5		! user space address?
	and	r1, r3		! pte &= PG_HW_BITS
	bf/s	.L3_load_kernel
	 mov.l	r3, @(0x04, r4)	! *SH3_PTEL = pte

	!! load mapping for a user space page
	!! we reload PTEH to enter VPN aligned to 4K page boundary
.L3_load_user:
	mov.l	r5, @r4		! *SH3_PTEH = { VPN, ASID }
	ldtlb			! needs 2 insns padding before RTE
	nop
	nop
	rte
	 nop

	!! load mapping for a kernel space page
	!! we need to temporary set ASID to 0
.L3_load_kernel:
	mov.l	.L3_clear_ASID, r1
	and	r5, r1		! *SH3_PTEH & ~SH3_PTEH_ASID_MASK
	mov.l	r1, @r4		! *SH3_PTEH = { VPN, ASID = 0 }
	ldtlb
	mov.l	r5, @r4		! restore ASID
	nop
	rte
	 nop


	!! if we haven't found a valid mapping in the fast path
	!!     tlb_exception(curlwp, trapframe, tea)
.L3_call_tlb_exception:
	__EXCEPTION_ENTRY
	mov.l	.L3_SH3_EXPEVT, r2
	mov.l	.L3_curlwp, r1
	mov	#(SH3_TEA & 0xff), r0
	mov.l	@r2, r2			! *SH3_EXPEVT
	mov.l	@r0, r6			! arg3: va = *SH3_TEA
	mov.l	@r1, r4			! arg1: curlwp
	__INTR_MASK_EXCEPTION_UNBLOCK(r0, r1, r3)
	mov.l	.L3_tlb_exception, r0
	mov.l	r2, @(TF_EXPEVT, r14)	! tf->tf_expevt = EXPEVT
	jsr	@r0
	 mov	r14, r5			! arg2: trapframe
	__EXCEPTION_RETURN

	.align	4
.L3_VPN_cleanup:		.long	~0x00000c00
.L3_curptd:			.long	_C_LABEL(curptd)
.L3_kernptd:			.long	_C_LABEL(__pmap_kernel)
.L3_VM_MIN_KERNEL_ADDRESS:	.long	VM_MIN_KERNEL_ADDRESS
.L3_ptp_index_mask:		.long	0x1ff
.L3_ptp_offset_mask:		.long	0x3ff << 2
.L3_PG_HW_BITS:			.long	PG_HW_BITS
.L3_PG_V:			.long	PG_V
.L3_clear_ASID:			.long	~SH3_PTEH_ASID_MASK
.L3_SH3_EXPEVT:			.long	SH3_EXPEVT
.L3_curlwp:			.long	_C_LABEL(curlwp)
.L3_tlb_exception:		.long	_C_LABEL(tlb_exception)

/* LINTSTUB: Var: char sh3_vector_tlbmiss_end[1]; */
VECTOR_END_MARKER(sh3_vector_tlbmiss_end)
	SET_ENTRY_SIZE(sh3_vector_tlbmiss)

#endif /* SH3 */


#ifdef SH4
/*
 * LINTSTUB: Var: char sh4_vector_tlbmiss[1];
 *
 * TLB miss vector.  We run through the fast path first, checking if
 * there's a valid mapping in curlwp or kernel pmap.  We do fast path
 * with exceptions disabled, so no P3 addresses please (though we can
 * use kernel stack if need be, as its TLB entries are wired).  We can
 * only use BANK1 registers, and of those r6 and r7 are already taken.
 *
 * If we don't find a valid mapping in the fast path, we do context
 * save and call tlb exception handler.
 *
 * Copied to VBR+0x400.  This code should be relocatable
 * and maximum 512 bytes long (== 0x600 - 0x400).
 */
NENTRY(sh4_vector_tlbmiss)
	mov.l	.L4_SH4_PTEH, r4
	mov.l	.L4_VPN_cleanup, r0
	mov.l	@r4, r5
	and	r0, r5		! trim vpn to 4K page boundary
	!! For the duration of fast path we keep
	!! r4: SH4_PTEH - other PTE regs are addressable as @(offset, r4)
	!! r5: { VPN, ASID } that caused the miss

	cmp/pz	r5		! user space address?
	bt/s	.L4_user_va
	 mov	r5, r2		! copy of vpn to compute indices into ptd/ptp

	!! kernel space address, use pmap_kernel(), adjust vpn for indexing
	!! see __pmap_kpte_lookup
.L4_kernel_va:
	mov.l	.L4_VM_MIN_KERNEL_ADDRESS, r0
	mov.l	.L4_kernptd,  r1 ! pmap_kernel()->pm_ptp
	bra	.L4_fetch_pte
	 sub	r0, r2		! vpn -= VM_MIN_KERNEL_ADDRESS

	!! user space address, use curlwp's pmap
.L4_user_va:
	mov.l	.L4_curptd,  r1	! curlwp->...->pm_ptp

	!! see __pmap_pte_lookup
.L4_fetch_pte:
	mov.l	@r1, r3		! fetch ptd

	!! r2: vpn, prepared for indexing into ptd
	!! r3: pt_entry_t **ptd => pt_entry_t *ptp => pt_entry_t pte
#ifdef DEBUG
	tst	r3, r3		! ptd == NULL  - cannot happen
	bt/s	.L4_call_tlb_exception
#endif
	 mov	#-22, r1	! __PMAP_PTP_SHIFT

	!! __PMAP_PTP_INDEX(vpn)
	mov	r2, r0
	shld	r1, r0		! vpn >> __PMAP_PTP_SHIFT
	mov.l	.L4_ptp_index_mask, r1
	and	r1, r0		! ... & (__PMAP_PTP_N - 1)
	shll2	r0		! array index -> array offset
	mov.l	@(r0, r3), r3	! ptp = ptd[idx]
	tst	r3, r3		! if (ptp == NULL)
	bt/s	.L4_call_tlb_exception
	 mov	#-(PGSHIFT - 2), r1

	!! __PMAP_PTP_OFSET(vpn) - except we pre-shift 2 bits left to
	!! get the array offset directly, as we know bits 10 and 11
	!! are zero (we cleaned them in r5 to get 4K aligned VPN)
	shld	r1, r2		! vpn >> (PGSHIFT - 2)
	mov.l	.L4_ptp_offset_mask, r0
	and	r2, r0		! ... & ((__PMAP_PTP_PG_N - 1) << 2)
	mov.l	@(r0, r3), r3	! pte = ptp[idx]


	!! r3: pte
	!! r4: SH4_PTEH
	!! r5: { VPN, ASID }

	mov.l	.L4_PG_V, r0
	tst	r0, r3		! if ((pte & PG_V) == 0)
	bt/s	.L4_call_tlb_exception
	 mov	r3, r0		! prepare PCMCIA SA bits for SH4_PTEA

	mov.l	.L4_PG_HW_BITS, r1
	shlr8	r0
	and	r1, r3		! pte &= PG_HW_BITS
	shlr	r0		! pte >> _PG_PCMCIA_SHIFT
	cmp/pz	r5		! user space address?
	and	#SH4_PTEA_SA_MASK, r0
	mov.l	r3, @(0x04, r4)	! *SH4_PTEL = pte
	bf/s	.L4_load_kernel
	 mov.l	r0, @(0x34, r4)	! *SH4_PTEA = PCMCIA space attrs

	!! load mapping for a user space page
	!! we reload PTEH to enter VPN aligned to 4K page boundary
.L4_load_user:
	mov.l	r5, @r4		! *SH4_PTEH = { VPN, ASID }
	ldtlb			! needs 1 insn padding before RTE
	nop
	rte
	 nop

	!! load mapping for a kernel space page
	!! we need to temporary set ASID to 0
.L4_load_kernel:
	mov.l	.L4_clear_ASID, r1
	and	r5, r1		! *SH4_PTEH & ~SH4_PTEH_ASID_MASK
	mov.l	r1, @r4		! *SH4_PTEH = { VPN, ASID = 0 }
	ldtlb
	mov.l	r5, @r4		! restore ASID
	rte
	 nop


	!! if we haven't found a valid mapping in the fast path
	!!     tlb_exception(curlwp, trapframe, tea)
.L4_call_tlb_exception:
	__EXCEPTION_ENTRY
	mov.l	.L4_SH4_PTEH, r0
	mov.l	.L4_curlwp, r1
	mov.l	@(0x24, r0), r2		! *SH4_EXPEVT
	mov.l	@(0x0c, r0), r6		! arg3: va = *SH4_TEA
	mov.l	@r1, r4			! arg1: curlwp
	__INTR_MASK_EXCEPTION_UNBLOCK(r0, r1, r3)
	mov.l	.L4_tlb_exception, r0
	mov.l	r2, @(TF_EXPEVT, r14)	! tf->tf_expevt = EXPEVT
	jsr	@r0
	 mov	r14, r5			! arg2: trapframe
	__EXCEPTION_RETURN

	.align	5
.L4_SH4_PTEH:			.long	SH4_PTEH
.L4_VPN_cleanup:		.long	~0x00000c00
.L4_curptd:			.long	_C_LABEL(curptd)
.L4_kernptd:			.long	_C_LABEL(__pmap_kernel)
.L4_VM_MIN_KERNEL_ADDRESS:	.long	VM_MIN_KERNEL_ADDRESS
.L4_ptp_index_mask:		.long	0x1ff
.L4_ptp_offset_mask:		.long	0x3ff << 2
.L4_PG_HW_BITS:			.long	PG_HW_BITS
.L4_PG_V:			.long	PG_V
.L4_clear_ASID:			.long	~SH4_PTEH_ASID_MASK
.L4_curlwp:			.long	_C_LABEL(curlwp)
.L4_tlb_exception:		.long	_C_LABEL(tlb_exception)

/* LINTSTUB: Var: char sh4_vector_tlbmiss_end[1]; */
VECTOR_END_MARKER(sh4_vector_tlbmiss_end)
	SET_ENTRY_SIZE(sh4_vector_tlbmiss)

#endif /* SH4 */


/*
 * LINTSTUB: Var: char sh_vector_interrupt[1];
 *
 * void sh_vector_interrupt(void);
 *	Copied to VBR+0x600.  This code should be position independent.
 */
NENTRY(sh_vector_interrupt)
	__EXCEPTION_ENTRY
	!! arguments for intc_intr(): for struct clockframe
	stc	ssr, r4
	stc	spc, r5
	stc	r0_bank, r6		! ssp
	__INTR_MASK_EXCEPTION_UNBLOCK(r0, r1, r3)

	mov.l	.Li_ci_idepth, r8	! callee-saved
	mov.l	.Li_intc_intr, r0
	mov.l	@r8, r9			! callee-saved
	mov	#0, r1
	add	#1, r9			! curcpu()->ci_idepth++
	mov.l	r1, @(TF_EXPEVT, r14)	! tf->tf_expevt = 0 (for debug)?
	jsr	@r0			! intc_intr(ssr, spc, ssp)
	 mov.l	r9, @r8

	cmp/pl	r9			! curcpu()->ci_idepth > 0
	add	#-1, r9			! curcpu()->ci_idepth--
	bt/s	.Li_return_to_kernel	! returning from a nested interrupt
	 mov.l	r9, @r8

	mov.l	@(TF_SSR, r14), r2
	mov.l	.Li_PSL_MD, r1
	tst	r1, r2			! tf->tf_ssr & PSL_MD == 0 ?
	bt	.Li_return_to_user

.Li_return_to_kernel:
	!! Check for interrupted kernel RAS when returning to kernel
	mov.l	@(TF_SPC, r14), r2
	mov.l	.Li_ras_start, r3
	cmp/hi	r3, r2			! spc > _lock_cas_ras_start ?
	bf	.Li_return_from_interrupt

	mov.l	.Li_ras_end, r1
	cmp/hs	r1, r2			! spc >= _lock_cas_ras_end ?
	bt	.Li_return_from_interrupt

	bra	.Li_return_from_interrupt
	 mov.l	r3, @(TF_SPC, r14)	! spc = _lock_cas_ras_start

.Li_return_to_user:
	/* Check for ASTs on exit to user mode. */
	mov.l	.Li_ast, r0
	mov.l	.Li_curlwp, r1
	mov	r14, r5		/* 2nd arg */
	jsr	@r0
	 mov.l	@r1, r4		/* 1st arg */

.Li_return_from_interrupt:
	__EXCEPTION_RETURN

	.align	5
.Li_ci_idepth:		.long	_C_LABEL(cpu_info_store) + CI_IDEPTH
.Li_intc_intr:		.long	_C_LABEL(intc_intr)
.Li_PSL_MD:		.long	0x40000000 /* PSL_MD */
.Li_ras_start:		.long	_C_LABEL(_lock_cas_ras_start)
.Li_ras_end:		.long	_C_LABEL(_lock_cas_ras_end)
.Li_ast:		.long	_C_LABEL(ast)
.Li_curlwp:		.long	_C_LABEL(curlwp)


/* LINTSTUB: Var: char sh_vector_interrupt_end[1]; */
VECTOR_END_MARKER(sh_vector_interrupt_end)
	SET_ENTRY_SIZE(sh_vector_interrupt)
