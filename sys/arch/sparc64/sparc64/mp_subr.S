/*	$NetBSD: mp_subr.S,v 1.7 2014/09/24 18:32:10 palle Exp $	*/

/*
 * Copyright (c) 2006-2010 Matthew R. Green
 * Copyright (c) 1996-2002 Eduardo Horvath
 * Copyright (c) 1996 Paul Kranenburg
 * Copyright (c) 1996
 * 	The President and Fellows of Harvard College.
 *	All rights reserved.
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.
 *	All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
 *	This product includes software developed by Harvard University.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 *	This product includes software developed by Harvard University.
 *	This product includes software developed by Paul Kranenburg.
 * 4. Neither the name of the University nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 *	@(#)locore.s	8.4 (Berkeley) 12/10/93
 */

#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_multiprocessor.h"
#include "opt_compat_netbsd.h"
#include "opt_compat_netbsd32.h"
#include "opt_lockdebug.h"

#include "assym.h"
#include <machine/param.h>
#include <sparc64/sparc64/intreg.h>
#include <sparc64/sparc64/timerreg.h>
#include <machine/ctlreg.h>
#include <machine/psl.h>
#include <machine/signal.h>
#include <machine/trap.h>
#include <machine/frame.h>
#include <machine/pte.h>
#include <machine/pmap.h>
#include <machine/intr.h>
#include <machine/asm.h>
#include <machine/locore.h>
#include <sys/syscall.h>
#ifdef SUN4V
#include <machine/hypervisor.h>
#endif	
#include "ksyms.h"

	.register	%g2,#scratch
	.register	%g3,#scratch

#define BLOCK_SIZE SPARC64_BLOCK_SIZE
#define BLOCK_ALIGN SPARC64_BLOCK_ALIGN

#if defined(MULTIPROCESSOR)
/*
 * IPI handler to do nothing, but causes rescheduling..
 * void sparc64_ipi_nop(void *);
 */
ENTRY(sparc64_ipi_nop)
	ba,a	ret_from_intr_vector
	 nop

/*
 * IPI handler to halt the CPU.  Just calls the C vector.
 * void sparc64_ipi_halt(void *);
 */
ENTRY(sparc64_ipi_halt)
	call	_C_LABEL(sparc64_ipi_halt_thiscpu)
	 clr	%g4
	sir

/*
 * IPI handler to pause the CPU.  We just trap to the debugger if it
 * is configured, otherwise just return.
 */
ENTRY(sparc64_ipi_pause)
#if defined(DDB)
	.global sparc64_ipi_pause_trap_point
sparc64_ipi_pause_trap_point:
	ta	1
	 nop
#endif
	ba,a	ret_from_intr_vector
	 nop

/*
 * Increment IPI event counter, defined in machine/{cpu,intr}.h.
 */
#define IPIEVC_INC(n,r1,r2)						\
	sethi	%hi(CPUINFO_VA+CI_IPIEVC+EVC_SIZE*n), r2;		\
	ldx	[r2 + %lo(CPUINFO_VA+CI_IPIEVC+EVC_SIZE*n)], r1;	\
	inc	r1;							\
	stx	r1, [r2 + %lo(CPUINFO_VA+CI_IPIEVC+EVC_SIZE*n)]

/*
 * void sparc64_ipi_flush_pte_us(void *);
 * void sparc64_ipi_flush_pte_usiii(void *);
 *
 * IPI handler to flush single pte.  We enter here with %tl already 1
 * and PSTATE_IE already disabled, so there's no need to do it again.
 *
 * On entry:
 *	%g2 = vaddr_t va
 *	%g3 = int ctx
 */
ENTRY(sparc64_ipi_flush_pte_us)
	srlx	%g2, PG_SHIFT4U, %g2		! drop unused va bits
	mov	CTX_SECONDARY, %g5
	sllx	%g2, PG_SHIFT4U, %g2
	ldxa	[%g5] ASI_DMMU, %g6		! Save secondary context
	sethi	%hi(KERNBASE), %g7
	membar	#LoadStore
	stxa	%g3, [%g5] ASI_DMMU		! Insert context to demap
	membar	#Sync
	or	%g2, DEMAP_PAGE_SECONDARY, %g2	! Demap page from secondary context only
	stxa	%g2, [%g2] ASI_DMMU_DEMAP	! Do the demap
	stxa	%g2, [%g2] ASI_IMMU_DEMAP	! to both TLBs
#ifdef TLB_FLUSH_LOWVA
	srl	%g2, 0, %g2			! and make sure it's both 32- and 64-bit entries
	stxa	%g2, [%g2] ASI_DMMU_DEMAP	! Do the demap
	stxa	%g2, [%g2] ASI_IMMU_DEMAP	! Do the demap
#endif
	flush	%g7
	stxa	%g6, [%g5] ASI_DMMU		! Restore secondary context
	membar	#Sync
	IPIEVC_INC(IPI_EVCNT_TLB_PTE,%g2,%g3)
	 
	ba,a	ret_from_intr_vector
	 nop

ENTRY(sparc64_ipi_flush_pte_usiii)
	andn	%g2, 0xfff, %g2			! drop unused va bits
	mov	CTX_PRIMARY, %g5
	ldxa	[%g5] ASI_DMMU, %g6		! Save primary context
	sethi	%hi(KERNBASE), %g7
	membar	#LoadStore
	stxa	%g3, [%g5] ASI_DMMU		! Insert context to demap
	membar	#Sync
	or	%g2, DEMAP_PAGE_PRIMARY, %g2
	stxa	%g2, [%g2] ASI_DMMU_DEMAP	! Do the demap
	stxa	%g2, [%g2] ASI_IMMU_DEMAP	! to both TLBs
#ifdef TLB_FLUSH_LOWVA
	srl	%g2, 0, %g2			! and make sure it's both 32- and 64-bit entries
	stxa	%g2, [%g2] ASI_DMMU_DEMAP	! Do the demap
	stxa	%g2, [%g2] ASI_IMMU_DEMAP	! Do the demap
#endif
	membar	#Sync
	flush	%g7
	stxa	%g6, [%g5] ASI_DMMU		! Restore primary context
	membar	#Sync
	flush	%g7
	IPIEVC_INC(IPI_EVCNT_TLB_PTE,%g2,%g3)

	ba,a	ret_from_intr_vector
	 nop

#ifdef SUN4V
ENTRY(sparc64_ipi_flush_pte_sun4v)
	mov	%o0, %g1		! save input
	mov	%o1, %g2
	mov	%o2, %g4
	mov	%g3, %o0		! vaddr
	mov	%g5, %o1		! ctx
	mov	MAP_DTLB|MAP_ITLB, %o2	! flags
	ta	ST_MMU_UNMAP_ADDR
	mov	%g1, %o0		! restore input
	mov	%g2, %o1
	mov	%g4, %o2

	retry
#endif	

/*
 * Secondary CPU bootstrap code.
 */
	.text
	.align 32
1:	rd	%pc, %l0
	LDULNG	[%l0 + (4f-1b)], %l1	! Load tlb slot count
	LDULNG	[%l0 + (7f-1b)], %g2	! Load cpu_args address.
	add	%l0, (6f-1b), %l2	! tlb slots
	ld	[%g2 + CBA_CPUTYP], %g3 ! Load cputype
	clr	%l3
2:	cmp	%l3, %l1
	be	CCCR, 3f
	 nop
	ldx	[%l2 + TTE_VPN], %l4
	ldx	[%l2 + TTE_DATA], %l5
#ifdef SUN4V	
	cmp	%g3, CPU_SUN4V
	bne,pt	%icc, 8f
	 nop
	! sun4v
	mov	%l4, %o0			! vaddr
	clr	%o1				! reserved
	mov	%l5, %o2			! tte
	mov	MAP_DTLB|MAP_ITLB, %o3		! flags
	mov	FT_MMU_MAP_PERM_ADDR, %o5	! hv fast trap function
	ta	ST_FAST_TRAP
	cmp	%o0, 0
	be,pt	%icc, 9f
	 nop
	sir					! crash if mapping fails
8:
#endif	
	! sun4u
	wr	%g0, ASI_DMMU, %asi
	stxa	%l4, [%g0 + TLB_TAG_ACCESS] %asi
	stxa	%l5, [%g0] ASI_DMMU_DATA_IN
	wr	%g0, ASI_IMMU, %asi
	stxa	%l4, [%g0 + TLB_TAG_ACCESS] %asi
	stxa	%l5, [%g0] ASI_IMMU_DATA_IN
9:		
	membar	#Sync
	flush	%l4
	add	%l2, PTE_SIZE, %l2
	add	%l3, 1, %l3
	ba	%xcc, 2b
	 nop
3:	LDULNG	[%l0 + (5f-1b)], %l1	! Load function
	jmpl	%l1, %g0
	 nop

	.align PTRSZ
4:	ULONG	0x0
5:	ULONG	0x0
7:	ULONG	0x0
	_ALIGN
6:

#define DATA(name) \
        .data ; \
        .align PTRSZ ; \
        .globl  name ; \
name:

DATA(mp_tramp_code)
	POINTER	1b
DATA(mp_tramp_code_len)
	ULONG	6b-1b
DATA(mp_tramp_tlb_slots)
	ULONG	4b-1b
DATA(mp_tramp_func)
	ULONG	5b-1b
DATA(mp_tramp_ci)
	ULONG	7b-1b

	.text
	.align 32


/*
 * IPI handler to store the current FPU state.
 * void sparc64_ipi_save_fpstate(void *);
 *
 * On entry:
 *	%g2 = lwp
 */
ENTRY(sparc64_ipi_save_fpstate)
	sethi	%hi(FPLWP), %g1
	LDPTR	[%g1 + %lo(FPLWP)], %g3
	cmp	%g3, %g2
	bne,pn	CCCR, 7f		! skip if fplwp has changed

	 rdpr	%pstate, %g2		! enable FP before we begin
	rd	%fprs, %g5
	wr	%g0, FPRS_FEF, %fprs
	or	%g2, PSTATE_PEF, %g2
	wrpr	%g2, 0, %pstate

	LDPTR	[%g3 + L_FPSTATE], %g3
	stx	%fsr, [%g3 + FS_FSR]	! f->fs_fsr = getfsr();
	rd	%gsr, %g2		! Save %gsr
	st	%g2, [%g3 + FS_GSR]
#if FS_REGS > 0
	add	%g3, FS_REGS, %g3
#endif
#ifdef DIAGNOSTIC
	btst	BLOCK_ALIGN, %g3	! Needs to be re-executed
	bnz,pn	%icc, 6f		! Check alignment
#endif
	 st	%g0, [%g3 + FS_QSIZE - FS_REGS]	! f->fs_qsize = 0;
	btst	FPRS_DL|FPRS_DU, %g5	! Both FPU halves clean?
	bz,pt	%icc, 5f		! Then skip it

	 mov	CTX_PRIMARY, %g2
	ldxa	[%g2] ASI_DMMU, %g6
	membar	#LoadStore
	stxa	%g0, [%g2] ASI_DMMU	! Switch MMU to kernel primary context
	membar	#Sync

	btst	FPRS_DL, %g5		! Lower FPU clean?
	bz,a,pt	%icc, 1f		! Then skip it, but upper FPU not clean
	 add	%g3, 2*BLOCK_SIZE, %g3	! Skip a block

	stda	%f0, [%g3] ASI_BLK_P	! f->fs_f0 = etc;
	inc	BLOCK_SIZE, %g3
	stda	%f16, [%g3] ASI_BLK_P

	btst	FPRS_DU, %g5		! Upper FPU clean?
	bz,pt	%icc, 2f		! Then skip it
	 inc	BLOCK_SIZE, %g3
1:
	stda	%f32, [%g3] ASI_BLK_P
	inc	BLOCK_SIZE, %g3
	stda	%f48, [%g3] ASI_BLK_P
2:
	membar	#Sync			! Finish operation so we can
	brz,pn	%g6, 5f			! Skip if context 0
	 nop
	stxa	%g6, [%g2] ASI_DMMU	! Restore primary context
	membar	#Sync
5:
	wr	%g0, FPRS_FEF, %fprs	! Mark FPU clean
	STPTR	%g0, [%g1 + %lo(FPLWP)]	! fplwp = NULL
7:
	IPIEVC_INC(IPI_EVCNT_FPU_SYNCH,%g2,%g3)
	ba,a	ret_from_intr_vector
	 nop

#ifdef DIAGNOSTIC
	!!
	!! Damn thing is *NOT* aligned on a 64-byte boundary
	!! 
6:
	wr	%g0, FPRS_FEF, %fprs
	! XXX -- we should panic instead of silently entering debugger
	ta	1
	 nop
	ba,a	ret_from_intr_vector
	 nop
#endif

/*
 * IPI handler to drop the current FPU state.
 * void sparc64_ipi_drop_fpstate(void *);
 *
 * On entry:
 *	%g2 = lwp
 */
ENTRY(sparc64_ipi_drop_fpstate)
	rdpr	%pstate, %g1
	wr	%g0, FPRS_FEF, %fprs
	or	%g1, PSTATE_PEF, %g1
	wrpr	%g1, 0, %pstate
	set	FPLWP, %g1
	CASPTRA	[%g1] ASI_N, %g2, %g0	! fplwp = NULL if fplwp == %g2
	membar	#Sync			! Should not be needed due to retry
	IPIEVC_INC(IPI_EVCNT_FPU_FLUSH,%g2,%g3)
	ba,a	ret_from_intr_vector
	 nop

/*
 * IPI handler to drop the current FPU state.
 * void sparc64_ipi_dcache_flush_page_usiii(paddr_t pa, int line_size)
 * void sparc64_ipi_dcache_flush_page_us(paddr_t pa, int line_size)
 *
 * On entry:
 *	%g2 = pa
 *	%g3 = line_size
 */
ENTRY(sparc64_ipi_dcache_flush_page_usiii)
	set	NBPG, %g1
	add	%g2, %g1, %g1	! end address

1:
	stxa	%g0, [%g2] ASI_DCACHE_INVALIDATE
	add	%g2, %g3, %g2
	cmp	%g2, %g1
	bl,pt	%xcc, 1b
	 nop

	sethi	%hi(KERNBASE), %g5
	flush	%g5
	membar	#Sync
	ba,a	ret_from_intr_vector
	 nop

ENTRY(sparc64_ipi_dcache_flush_page_us)
	mov	-1, %g1		! Generate mask for tag: bits [29..2]
	srlx	%g2, 13-2, %g5	! Tag is PA bits <40:13> in bits <29:2>
	clr	%g4
	srl	%g1, 2, %g1	! Now we have bits <29:0> set
	set	(2*NBPG), %g7
	ba,pt	%icc, 1f
	 andn	%g1, 3, %g1	! Now we have bits <29:2> set

	.align 8
1:
	ldxa	[%g4] ASI_DCACHE_TAG, %g6
	mov	%g4, %g2
	deccc	32, %g7
	bl,pn	%icc, 2f
	 inc	32, %g4

	xor	%g6, %g5, %g6
	andcc	%g6, %g1, %g0
	bne,pt	%xcc, 1b
	 membar	#LoadStore

	stxa	%g0, [%g2] ASI_DCACHE_TAG
	ba,pt	%icc, 1b
	 membar	#StoreLoad
2:

	sethi	%hi(KERNBASE), %g5
	flush	%g5
	membar	#Sync
	ba,a	ret_from_intr_vector
	 nop

#endif
