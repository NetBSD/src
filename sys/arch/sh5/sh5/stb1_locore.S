/*	$NetBSD: stb1_locore.S,v 1.1 2002/07/05 13:32:07 scw Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * ##########
 *
 * This is not a standalone file.
 * To use it, put #include <sh5/sh5/stb1_locore.S> in your port's locore.S
 *
 * ##########
 */

/******************************************************************************
 *
 * This sub-routine is called very early on from a port's locore.S with
 * the MMU switched off and the TLB/Cache in an unknown state.
 *
 * It is our responsibility to initialise the TLB/Cache, set up a
 * wired TLB pair to map the kernel to its proper virtual address
 * space, and finally to enable the MMU.
 *
 * One entry, r2 holds the physical address of the base of the kernel.
 * This *MUST* be an exact multiple of SH5_KSEG0_SIZE.
 * r18 holds the return address. Note that the caller must ensure that
 * the return address is valid for the newly re-mapped kernel. Basically,
 * don't generate a return address using PC-relative addressing.
 *
 * On exit, although the MMU is enabled and the kernel is running mapped,
 * the pmap module has not yet been boot-strapped. Port-specific code
 * must *NOT* access any memory/device address outside the 512MB KSEG0
 * virtual address space until pmap_bootstrap has been called.
 */
Lsh5_stb1_init:
	addz.l	r2, r63, r2		/* Clear the upper 32-bits of r2 */
	pta/u	1f, tr0			/* Pre-load branches */
	LDC32(0x00800000 + (16 * 63), r0)	/* Top of DTLB */
	movi	(16 * 63), r1		/* Top of ITLB */
1:	putcfg	r0, 0, r63		/* Invalidate DTLB entry */
	putcfg	r0, 1, r63
	addi	r0, -16, r0		/* Next DTLB slot */
	putcfg	r1, 0, r63		/* Invalidate ITLB entry */
	putcfg	r1, 1, r63
	addi	r1, -16, r1		/* Next ITLB slot */
	bne/l	r1, r63, tr0		/* Back until all checked */

	/* Invalidate [DI]TLB#0 too */
	putcfg	r0, 0, r63
	putcfg	r1, 0, r63

	/* Fix up KSEG0 physical page and attributes in ITLB */
	movi	SH5_PTEL_CB_NOCACHE | SH5_PTEL_SZ_512MB | SH5_PTEL_PR_X, r3
	or	r2, r3, r3
	putcfg	r1, 1, r3

	/* Ditto for DTLB */
	movi	SH5_PTEL_CB_NOCACHE | SH5_PTEL_SZ_512MB | SH5_PTEL_PR_R | SH5_PTEL_PR_W, r3
	or	r2, r3, r3
	putcfg	r0, 1, r3

	/* Fix up the virtual page, shared and valid bits */
#ifndef _LP64
	LDSC32(SH5_KSEG0_BASE | SH5_PTEH_SH | SH5_PTEH_V, r2)
#else
	LDSC64(SH5_KSEG0_BASE | SH5_PTEH_SH | SH5_PTEH_V, r2)
#endif
	putcfg	r0, 0, r2
	putcfg	r1, 0, r2

	/* XXX: Cache */

	/*
	 * The TLB is primed, the cache is clean. All we have to do now
	 * is enable the MMU and return to the caller.
	 */
	getcon	sr, r0
	LDUC32(SH5_CONREG_SR_ASID_MASK << SH5_CONREG_SR_ASID_SHIFT, r1)
	andc	r0, r1, r1
	LDUC32(SH5_CONREG_SR_INIT|SH5_CONREG_SR_IMASK_ALL, r0)
	or	r1, r0, r0
	putcon	r0, ssr
	putcon	r18, spc
	synco
	rte				/* Enable MMU and return to caller */
	nop
	nop
	nop
	nop
	nop


/******************************************************************************/

#define	TLBCOOKIE_IDX_SHIFT_D	0
#define	TLBCOOKIE_IDX_SHIFT_I	5

#define	TLBCOOKIE_IDX_MASK	0x0f
#define	TLBCOOKIE_IDX_VALID	0x10


/******************************************************************************
 *
 * void _sh5_stb1_tlbinv(pteh_t pteh, pteh_t mask)
 *
 * Search the TLB for an entry which matches the constraints passed in
 * in `pteh' and `mask'.
 *
 * If found, invalidate the TLB entry.
 */
ENTRY(_sh5_stb1_tlbinv)
	getcon	sr, r5
	LDC32(SH5_CONREG_SR_BL, r4)
	ori	r2, SH5_PTEH_V, r2	/* Only check valid entries */
	ori	r3, SH5_PTEH_V, r3
	or	r4, r5, r4		/* Block exceptions */
	putcon	r4, sr

	ptabs/u	r18, tr1		/* Get return address to tr1 */

	/* Check the DTLB first */
	pta/l	1f, tr0
	LDC32(0x00800010, r0)		/* DTLB Base */
	movi	63, r1

1:	getcfg	r0, 0, r6		/* Fetch DTLB.PTEH entry */
	and	r6, r3, r7		/* Keep interesting bits in r7 */
	xor	r7, r2, r7		/* r7 <- 0 if we found a match */
	cmveq	r7, r63, r6		/* r6 <- 0 if we found a match */
	putcfg	r0, 0, r6		/* If matched, zap the DTLB entry */
	cmveq	r7, r63, r1		/* If matched, terminate loop early */
	addi	r1, -1, r1		/* Decrement loop counter */
	addi	r0, 16, r0		/* Next DTLB entry */
	bge/l	r1, r63, tr0		/* Back if not done and not found */

	/* Now check the ITLB */
	pta/l	2f, tr0
	movi	16, r1			/* ITLB Base */
	movi	63, r1

2:	getcfg	r0, 0, r6		/* Fetch ITLB.PTEH */
	and	r6, r3, r7		/* Keep interesting bits */
	xor	r7, r2, r7		/* r7 <- 0 if we found a match */
	cmveq	r7, r63, r6		/* Ditto for r6 */
	putcfg	r0, 0, r6		/* If matched, zap the ITLB entry */
	cmveq	r7, r63, r1		/* If matched, terminate loop early */
	addi	r1, -1, r1		/* Decrement loop counter */
	addi	r0, 16, r0		/* Next ITLB entry */
	bge/l	r1, r63, tr0		/* Back if not done and not found */

	putcon	r5, sr			/* Re-enable exceptions */
	blink	tr1, r63


/******************************************************************************
 *
 * void _sh5_stb1_tlbinv_cookie(pteh_t pteh, u_int tlbcookie)
 *
 * Callable from C code.
 *
 *  r2 - pteh
 *  r3 - tlbcookie
 *
 * Search the TLB for the mapping identified by `pteh' and invalidate
 * it if found. Use `tlbcookie' as a hint for where in the TLB the mapping
 * may be.
 *
 * Must also flush/invalidate the instruction/data cache before removing
 * the mapping.
 */
ENTRY(_sh5_stb1_tlbinv_cookie)
	ptabs/l	r18, tr0		/* Get return address to tr0 */
	beq/l	r3, r63, tr0		/* Just return if pteh isn't in TLB */

	/*
	 * Looks like the PTEH may be stashed somewhere in the TLB.
	 * Fortunately, we have a hint which narrows it down
	 * to one of four TLB entries in both instruction and data
	 * TLBs. This avoids searching all 64 entries in both TLBs.
	 */
	ori	r2, SH5_PTEH_V, r2	/* Only check valid entries */

	pta/u	1f, tr1			/* Pre-load jump targets */
	pta/u	2f, tr2
	pta/u	4f, tr3
	pta/u	5f, tr4

	/* Data TLB index valid?  */
	andi	r3, TLBCOOKIE_IDX_VALID, r0
	beq/u	r0, r63, tr4		/* Nope. Go check instruction TLB */

	/* Yup. Shoot it down */
	andi	r3, TLBCOOKIE_IDX_MASK, r0
	shlli	r0, 6, r0		/* Convert to TLB index */
	LDC32(0x00800000, r1)		/* DTLB base */
	add	r0, r1, r0		/* Point to required group of four */

1:	getcfg	r0, 0, r1		/* Fetch TLB.PTEH entry #0 */
	beq/u	r1, r2, tr2		/* Jump if we found it */
	addi	r0, 16, r0
	getcfg	r0, 0, r1		/* Fetch TLB.PTEH entry #1 */
	beq/u	r1, r2, tr2		/* Jump if we found it */
	addi	r0, 16, r0
	getcfg	r0, 0, r1		/* Fetch TLB.PTEH entry #2 */
	beq/u	r1, r2, tr2		/* Jump if we found it */
	addi	r0, 16, r0
	getcfg	r0, 0, r1		/* Fetch TLB.PTEH entry #3 */
	beq/u	r1, r2, tr2		/* Jump if we found it */

	/*
	 * Huh. It wasn't there after all...
	 * Perhaps it was evicted by the TLB miss code.
	 */
	blink	tr4, r63

	/* Found the required TLB entry. Disable exceptions and check again */
2:	getcon	sr, r4			/* Grab SR */
	LDC32(SH5_CONREG_SR_BL, r1)
	or	r4, r1, r1
	putcon	r1, sr			/* Disable ALL exceptions */
	getcfg	r0, 0, r1		/* Fetch TLB.PTEH entry again */
	bne/u	r1, r2, tr3		/* It changed under our noses */

	/*
	 * Ok, we're about to invalidate a TLB entry.
	 * Before we can do this, we *must* purge the operand cache
	 * and invalidate the instruction cache over the whole page.
	 *
	 * At this point, the following registers are live:
	 *
	 *  r0 - The TLB configuration register to be invalidated
	 *  r1 - Available
	 *  r2 - The PTEH value matched in the TLB. Contains the EPN.
	 *  r3 - TLB Cookie
	 *  r4 - Saved SR
	 */

	/*
	 * XXX
	 * We should add a debug/diagnostic to ensure we're not about
	 * to invalidate [DI]TLB#0, with disasterous consequences...
	 * XXX
	 */

	putcfg	r0, 0, r63		/* Invalidate it */

4:	putcon	r4, sr			/* Re-enable exceptions */
	blink	tr4, r63

	/*
	 * Check the instruction TLB if necessary
	 */
5:	shlri	r3, TLBCOOKIE_IDX_SHIFT_I, r3
	andi	r3, TLBCOOKIE_IDX_VALID, r0
	beq/u	r0, r63, tr0		/* Nope. Just return. */

	/* Yup. Shoot it down */
	andi	r3, TLBCOOKIE_IDX_MASK, r0 /* Fetch ITLB index */
	shlli	r0, 6, r0		/* Point to required group of four */
	ptabs/l	r18, tr4		/* Return to caller when done */


/******************************************************************************
 *
 * void __sh5_stb1_tlbinv_all(void)
 *
 * Callable from C code.
 *
 * Invalidate all instruction and data TLB entries (except [DI]TLB#0,
 * which maps the kernel in KSEG0).
 */
ENTRY(_sh5_stb1_tlbinv_all)
	pta/u	1f, tr0			/* Pre-load branches */
	ptabs/u	r18, tr1		/* Get return address to tr1 */
	LDC32(0x00800000 + (16 * 63), r0)	/* DTLB Top */
	movi	(16 * 63), r1		/* ITLB Top */
1:	putcfg	r0, 0, r63		/* Invalidate DTLB entry */
	addi	r0, -16, r0		/* Next DTLB slot */
	putcfg	r1, 0, r63		/* Invalidate ITLB entry */
	addi	r1, -16, r1		/* Next ITLB slot */
	bne/l	r1, r63, tr0		/* Back until all checked */
	blink	tr1, r63


/******************************************************************************
 *
 * __sh5_stb1_tlbload(r0 = PTEH, r1 = PTEL, r2 = Return Address)
 *
 * Load the appropriate TLB with the PTEL and PTEH values.
 *
 * r3, tr0, and tr1 are available.
 * r1 and r2 need not be preserved.
 *
 * Can return an optional 10-bit "cookie" in r0, which will be passed to
 * the TLB invalidate function, above, when the PTE is to be purged from
 * the TLB. The cookie need not be returned if the ASID field of PTEH is
 * zero.
 *
 * NOT callable by C code.
 *
 * This subroutine will be called from within the generic TLB miss
 * handler. As such, it is executing on the TLB miss stack with all
 * exceptions disabled. Therefore, don't touch anything which might
 * cause *another* TLB miss!
 *
 * The subroutine is free to jump to ASENTRY(Ltlbmiss_dotrap) if the
 * PTE's protection bits are incompatible with type of TLB miss.
 * For example, an Instruction TLB miss for a non-executable page.
 */
ENTRY_NOPROFILE(_sh5_stb1_tlbload)
	LEAF(Ltlbmiss_dotrap, r3)
	ptabs/u	r2, tr1			/* Stash the return address */
	ptabs/u	r3, tr0
	addz.l	r1, r63, r1		/* Ensure upper 32-bit of PTEL clear */

	/* For "write" misses, verify the page is actually writable */

	getcon	expevt, r2		/* Test for Write TLB Miss.. */
	addi	r2, -T_WTLBMISS, r2	/* r2 == 0 for write miss */
	andi	r1, SH5_PTEL_PR_W, r3	/* r3 == `W' bit of PTEL */
	or	r2, r3, r3		/* r3 == 0 if WR to RO page */
	beq/u	r3, r63, tr0		/* Read-only page. Trap it. */

	/* Determine which TLB to load (Data or Instruction) */

	getcon	expevt, r2		/* Test for Instruction TLB miss */
	movi	T_ITLBMISS, r3
	sub	r2, r3, r2		/* r2 == 0 for ITLB miss */
	LDC32(0x00800000, r3)		/* Presume DLTB miss */
	cmveq	r2, r63, r3		/* If it was the ITLB, r3 = 0 */

	/* Assume DTLB */

	pta/l	2f, tr0
	movi	SH5_PTEL_PR_X, r2
	bne/l	r3, r63, tr0

	/* It's the ITLB. Ensure the Execute bit is set for ITLB misses */

1:	andi	r1, SH5_PTEL_PR_X, r2
	pta/l	50f, tr0
	bne/l	r2, r63, tr0

	LEAF(Ltlbmiss_dotrap, r0)
	ptabs/l	r0, tr0
	blink	tr0, r63

50:
	movi	SH5_PTEL_PR_R | SH5_PTEL_PR_W, r2

	/*
	 * r0 == PTEH
	 * r1 == PTEL
	 * r2 == Mask of protection bits to clear out of PTEL
	 * r3 == 0x00800000 for DTLB, or 0x00000000 for ITLB
	 */

2:	andc	r1, r2, r1		/* Clear unwanted bits of PTEL */
	ori	r0, SH5_PTEH_V, r0	/* Ensure mapping is Valid */
	getcon	ctc, r2			/* Fetch free-running counter */
	pta/l	3f, tr0
	andi	r2, 0x3f, r2		/* Generate random TLB index */
	bne/l	r2, r63, tr0		/* Don't trash KSEG0 TLB slot! */
	addi	r2, 1, r2		/* FIX ME! Overloads TLB slot 1 */
3:	shlli	r2, 4, r2		/* Index *= 16 */
	add	r2, r3, r2		/* Point to target TLB slot */

	/* r2 == pseudo-random [ID]TLB index */

	putcfg	r2, 0, r63		/* Make sure PTEH.V is clear */
	putcfg	r2, 1, r1		/* Store PTEL */
	putcfg	r2, 0, r0		/* Store PTEH */

	/* Generate TLB cookie, if necessary */

	movi	(SH5_PTEH_ASID_MASK << SH5_PTEH_ASID_SHIFT), r3
	and	r0, r3, r3
	beq/u	r3, r63, tr1

	shlri	r2, 6, r0			/* Get TLB index/4 */
	andi	r0, TLBCOOKIE_IDX_MASK, r0	/* Ditch unwanted bits */
	ori	r0, TLBCOOKIE_IDX_VALID, r0	/* Mark it as valid */
	movi	0, r3				/* Test which TLB was used */
	shori	65535, r3
	andc	r2, r3, r3
	bne/l	r3, r63, tr1			/* Just return if DTLB */
	shlli	r0, TLBCOOKIE_IDX_SHIFT_I, r0	/* Convert to ITLB cookie */
	blink	tr1, r63


/******************************************************************************
 *
 * void _sh5_stb1_cache_purge(vaddr_t start, vsize_t len)
 */
ENTRY(_sh5_stb1_cache_purge)
	pta/l	1f, tr0
	ptabs/l	r18, tr1
1:	ocbp	r2, 0
	addi	r2, 32, r2
	addi	r3, -32, r3
	bgt/l	r3, r63, tr0
	synci
	blink	tr1, r63

/******************************************************************************
 *
 * void _sh5_stb1_cache_invalidate(vaddr_t start, vsize_t len)
 */
ENTRY(_sh5_stb1_cache_invalidate)
	pta/l	1f, tr0
	ptabs/l	r18, tr0
1:	ocbi	r2, 0
	addi	r2, 32, r2
	addi	r3, -32, r3
	bgt/l	r3, r63, tr0
	synci
	blink	tr0, r63
