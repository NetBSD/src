/*	$NetBSD: locore_subr.S,v 1.19 2003/08/12 13:51:52 scw Exp $	*/

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
 * If we're using the asm version of memcpy() in /sys/lib/libkern/arch/sh5,
 * then copyin(), copyout() and kcopy() can take advantage of it.
 */
#define	_COPYINOUT_USE_MEMCPY

/*
 * ##########
 *
 * This is not a standalone file.
 * To use it, put #include <sh5/sh5/locore_subr.S> in your port's locore.S
 *
 * ##########
 */
	_TEXT_SECTION
	_ALIGN_TEXT

/******************************************************************************
 *
 * Set up a basic environment in preparation for calling C code
 *
 * To be called early on from port-specific locore.S, after the MMU is
 * enabled, but before pmap_bootstrap().
 *
 * On entry, r2 must point to the physical address of KSEG0.
 */
Lsh5_init_env:
	/*
	 * Fix up the vector table.
	 */
	LEA(_C_LABEL(sh5_vector_table), r0)
	putcon  r0, vbr

	/*
	 * Arrange to catch debug/panic exceptions with the mmu off.
	 * Need to convert sh5_resvec_vector_table to a physical
	 * address.
	 */
	LEA(_C_LABEL(sh5_resvec_vector_table) - SH5_KSEG0_BASE, r0)
	add	r0, r2, r0		/* Convert to phys address */
	ori	r0, 1, r0		/* MMU off when handling exceptions */
	putcon  r0, resvec

	/*
	 * If we have an FPU, ensure it's quiescent
	 * XXX: The kernel currently requires an FPU for certain
	 * operations.
	 */
	getcon	sr, r0
	pta/u	1f, tr0
	shlri   r0, SH5_CONREG_SR_FD_SHIFT, r0
	andi	r0, 1, r0
	bne/u	r0, r63, tr0		/* No FPU or FPU disabled. Jump */
	LDC32(0x20000, r0)
	fmov.ls	r0, fr0			/* Set FPSCR.DN bit */
	fputscr	fr0
	fmov.qd	r63, dr0		/* Binary "Zero" to all FPU registers */
	fmov.qd	r63, dr2;  fmov.qd r63, dr4;  fmov.qd r63, dr6
	fmov.qd r63, dr8;  fmov.qd r63, dr10; fmov.qd r63, dr12
	fmov.qd	r63, dr14; fmov.qd r63, dr16; fmov.qd r63, dr18
	fmov.qd	r63, dr20; fmov.qd r63, dr22; fmov.qd r63, dr24
	fmov.qd	r63, dr26; fmov.qd r63, dr28; fmov.qd r63, dr30
	fmov.qd	r63, dr32; fmov.qd r63, dr34; fmov.qd r63, dr36
	fmov.qd	r63, dr38; fmov.qd r63, dr40; fmov.qd r63, dr42
	fmov.qd	r63, dr44; fmov.qd r63, dr46; fmov.qd r63, dr48
	fmov.qd	r63, dr50; fmov.qd r63, dr52; fmov.qd r63, dr54
	fmov.qd	r63, dr56; fmov.qd r63, dr58; fmov.qd r63, dr60
	fmov.qd	r63, dr62
1:

	/*
	 * Set up lwp0's pcb.
	 */
	LEA(Lcpu_info, r1)
	putcon	r1, kcr0		/* KCR0 always hold cpu_info */
	LEA(Lproc0paddr, r0)
	STPTR	r1, CI_CURPCB, r0	/* ci_curpcb = &proc0paddr */
	LEA(_C_LABEL(proc0paddr), r1)
	STPTR	r1, 0, r0
	movi	USPACE-(_PTRSZ*2), r1	/* Top of kernel stack, minus frame */
	add	r0, r1, r15		/* Set up the initial kernel stack */
	add	r0, r1, r14		/* Set up the initial frame pointer */

	/* Fix up lwp0's l_addr */
	LEA(_C_LABEL(lwp0), r3)
	STPTR	r3, L_ADDR, r0		/* lwp0->l_addr = &proc0paddr */

	/*
	 * Create a fake trapframe for proc0 which will be used when
	 * forking the initial process.
	 */
	movi	SZ_TRAPFRAME, r2
	sub	r15, r2, r2		/* r2 -> base of fake trapframe */

	/* Fix up lwp0->l_md.md_regs to point to the base of the trapframe */
	STPTR	r3, L_MD_REGS, r2

	/* Indicate that callee-saved registers are valid (albeit zeroed) */
	movi	SF_FLAGS_CALLEE_SAVED, r1
	st.q	r2, SF_FLAGS + (SZ_TRAPFRAME - SZ_INTRFRAME), r1

	/* Fix up a fake status register */
	LDUC32(SH5_CONREG_SR_MMU|SH5_CONREG_SR_MD, r0)
	st.q	r2, SF_SSR + (SZ_TRAPFRAME - SZ_INTRFRAME), r0

	/* Ditto for stack and frame pointers */
	st.q	r2, IF_R14 + (SZ_TRAPFRAME - SZ_INTRFRAME), r14
	st.q	r2, IF_R15 + (SZ_TRAPFRAME - SZ_INTRFRAME), r15

	/* Adjust stack */
	or	r2, r63, r15

	ptabs/l	r18, tr0
	blink	tr0, r63


/******************************************************************************
 *
 * void delay(u_int usecs);
 */
ENTRY(delay)
	ptabs/u	r18, tr1
	addz.l	r2, r63, r2
	beq/u	r2, r63, tr1
	pt/l	1f, tr0
	LEA(_C_LABEL(_sh5_delay_constant), r0)
	ld.l	r0, 0, r0
	mulu.l	r2, r0, r0
1:	addi	r0, -1, r0
	bgt/l	r0, r63, tr0
	blink	tr1, r63


/******************************************************************************
 *
 * int setjmp(label_t *jmpbuf);
 */
#define _O(n)	((n)*8)
ENTRY(setjmp)
	st.q	r2, _O(0), r10
	st.q	r2, _O(1), r11
	st.q	r2, _O(2), r12
	st.q	r2, _O(3), r13
	st.q	r2, _O(4), r14
	st.q	r2, _O(5), r15
	st.q	r2, _O(6), r18
	st.q	r2, _O(7), r28
	st.q	r2, _O(8), r29
	st.q	r2, _O(9), r30
	st.q	r2, _O(10), r31
	st.q	r2, _O(11), r32
	st.q	r2, _O(12), r33
	st.q	r2, _O(13), r34
	st.q	r2, _O(14), r35
	st.q	r2, _O(15), r44
	st.q	r2, _O(16), r45
	st.q	r2, _O(17), r46
	st.q	r2, _O(18), r47
	st.q	r2, _O(19), r48
	st.q	r2, _O(20), r49
	st.q	r2, _O(21), r50
	st.q	r2, _O(22), r51
	st.q	r2, _O(23), r52
	st.q	r2, _O(24), r53
	st.q	r2, _O(25), r54
	st.q	r2, _O(26), r55
	st.q	r2, _O(27), r56
	st.q	r2, _O(28), r57
	st.q	r2, _O(29), r58
	st.q	r2, _O(30), r59
	ptabs/l	r18, tr0
	gettr	tr5, r0
	st.q	r2, _O(31), r0
	gettr	tr6, r0
	st.q	r2, _O(32), r0
	gettr	tr7, r0
	st.q	r2, _O(33), r0
	movi	0, r2
	blink	tr0, r63

/******************************************************************************
 *
 * void longjmp(label_t *jmpbuf);
 */
ENTRY(longjmp)
	ld.q	r2, _O(0), r10
	ld.q	r2, _O(1), r11
	ld.q	r2, _O(2), r12
	ld.q	r2, _O(3), r13
	ld.q	r2, _O(4), r14
	ld.q	r2, _O(5), r15
	ld.q	r2, _O(6), r18
	ld.q	r2, _O(7), r28
	ld.q	r2, _O(8), r29
	ld.q	r2, _O(9), r30
	ld.q	r2, _O(10), r31
	ld.q	r2, _O(11), r32
	ld.q	r2, _O(12), r33
	ld.q	r2, _O(13), r34
	ld.q	r2, _O(14), r35
	ld.q	r2, _O(15), r44
	ld.q	r2, _O(16), r45
	ld.q	r2, _O(17), r46
	ld.q	r2, _O(18), r47
	ld.q	r2, _O(19), r48
	ld.q	r2, _O(20), r49
	ld.q	r2, _O(21), r50
	ld.q	r2, _O(22), r51
	ld.q	r2, _O(23), r52
	ld.q	r2, _O(24), r53
	ld.q	r2, _O(25), r54
	ld.q	r2, _O(26), r55
	ld.q	r2, _O(27), r56
	ld.q	r2, _O(28), r57
	ld.q	r2, _O(29), r58
	ld.q	r2, _O(30), r59
	ptabs/l	r18, tr0
	ld.q	r2, _O(31), r0
	ptabs/u	r0, tr5
	ld.q	r2, _O(32), r0
	ptabs/u	r0, tr6
	ld.q	r2, _O(33), r0
	ptabs/u	r0, tr7
	movi	1, r2
	blink	tr0, r63



/******************************************************************************
 *
 * int __cpu_intr_suspend(void)
 */
ENTRY_NOPROFILE(_cpu_intr_suspend)
	getcon	sr, r3
	ptabs/l	r18, tr0
	andi	r3, SH5_CONREG_SR_IMASK_ALL, r2
	ori	r3, SH5_CONREG_SR_IMASK_ALL, r3
	shlri	r2, SH5_CONREG_SR_IMASK_SHIFT, r2
	putcon	r3, sr
	blink	tr0, r63


/******************************************************************************
 *
 * void __cpu_intr_set(int spl)
 */
ENTRY_NOPROFILE(_cpu_intr_set)
	getcon	sr, r4
	ptabs/l	r18, tr0
	shlli	r2, SH5_CONREG_SR_IMASK_SHIFT, r2
	movi	SH5_CONREG_SR_IMASK_ALL, r3
	and	r2, r3, r2
	andc	r4, r3, r4
	or	r4, r2, r4
	putcon	r4, sr
	blink	tr0, r63


/******************************************************************************
 *
 * void splx(int spl)
 *
 * As above, but deal with pending software interrupts if
 * the lowered spl permits them.
 */
ENTRY_NOPROFILE(splx)
	ptabs/l	r18, tr0
	pta/u	Lcheck_softint_lower, tr1
	getcon	sr, r4			/* Fetch current interrupt mask */
	LEA(_C_LABEL(ssir), r0)		/* Get bitmap of pending soft ints */
	movi	SH5_CONREG_SR_IMASK_ALL, r3
	shlli	r2, SH5_CONREG_SR_IMASK_SHIFT, r2
	andc	r4, r3, r4		/* Clear out current mask */
	and	r2, r3, r2		/* Ensure supplied mask is valid */
	or	r4, r2, r2		/* r2 == new mask */
	ld.l	r0, 0, r1
	bne/u	r1, r63, tr1		/* Go deal with them if any pending */
	putcon	r2, sr			/* Restore interrupt level */
	blink	tr0, r63


/******************************************************************************
 *
 * int splraise(int ipl);
 */
ENTRY_NOPROFILE(splraise)
	ptabs/l	r18, tr0
	movi	SH5_CONREG_SR_IMASK_ALL, r1
	getcon	sr, r3			/* Fetch current mask */
	andi	r2, 15, r0		/* Ensure "ipl" is within range */
	and	r1, r3, r2		/* Get current mask to r2 */
	shlri	r2, SH5_CONREG_SR_IMASK_SHIFT, r2	/* Shift to low bits */
	bge/u	r2, r0, tr0		/* Return if current mask >= ipl */
	shlli	r0, SH5_CONREG_SR_IMASK_SHIFT, r0	/* Create new mask */
	andc	r3, r1, r3		/* Zap mask bits in current SR */
	or	r3, r0, r3		/* r3 == new SR */
	putcon	r3, sr
	blink	tr0, r63



/******************************************************************************
 *
 * int copyout(const void *kaddr, void *uaddr, size_t len)
 *                         r2           r3            r4
 */
ENTRY(copyout)
#ifdef DDB
	LINK_FRAME(0)
#else
	add	r18, r63, r61
#endif
	movi	EFAULT, r60		/* Assume there was a problem */
	cmveq	r4, r63, r60		/* But return success if len == 0 */
	pta/u	Lcopyexit, tr1
	bge/u	r63, r4, tr1		/* Return EFAULT if len <= 0 */

#ifndef _LP64
	LDSC32(VM_MAX_ADDRESS, r0)
#else
	LDSC64(VM_MAX_ADDRESS, r0)
#endif
	bgtu/u	r3, r0, tr1		/* Bomb if uaddr isn't in user space */
	add	r4, r3, r1		/* Check end of destination buffer */
	bgtu/u	r1, r0, tr1		/* Bomb if not in user space */
	bgtu/u	r3, r1, tr1		/* Also bomb if uaddr+len wraps */

#ifndef _LP64
	LDSC32(SH5_KSEG0_BASE, r0)
#else
	LDSC64(SH5_KSEG0_BASE, r0)
#endif
	bgtu/u	r0, r2, tr1		/* Bomb if kaddr isn't a kernel addr */
	add	r4, r2, r1		/* Check end of source buffer */
	bgtu/u	r0, r1, tr1		/* Bomb if not in kernel space */
	bgtu/u	r2, r1, tr1		/* Also bomb if kaddr+len wraps */

	pta/u	Lcopyerr, tr2
	getcon	kcr0, r0		/* Fetch cpu_info */
	gettr	tr2, r1
	LDPTR	r0, CI_CURPCB, r0
	STPTR	r0, PCB_ONFAULT, r1	/* Set pcb_onfault to Lcopyerr */

#ifdef _COPYINOUT_USE_MEMCPY
	/*
	 * Note: This assumes we're using the asm version of memcpy()
	 */
	LEAF(_C_LABEL(memcpy), r25)
	ptabs/l	r25, tr0
	add	r2, r63, r0
	add	r3, r63, r2
	add	r0, r63, r3
	blink	tr0, r18
#else
	movi	0, r1
	pta/l	1f, tr1
1:	ldx.b	r2, r1, r0		/* Copy (XXX: SLOWWWWWWW!!) */
	stx.b	r3, r1, r0
	addi	r1, 1, r1
	bgt/l	r4, r1, tr1
#endif

	pta/l	Lcopyerr, tr2
	movi	0, r60			/* No error! */
	blink	tr2, r63



/******************************************************************************
 *
 * int copyin(const void *uaddr, void *kaddr, size_t len)
 *                        r2           r3            r4
 */
ENTRY(copyin)
#ifdef DDB
	LINK_FRAME(0)
#else
	add	r18, r63, r61
#endif
	movi	EFAULT, r60		/* Assume there was a problem */
	cmveq	r4, r63, r60		/* But return success if len == 0 */
	pta/u	Lcopyexit, tr1
	bge/u	r63, r4, tr1		/* Return EFAULT if len <= 0 */

#ifndef _LP64
	LDSC32(VM_MAX_ADDRESS, r0)
#else
	LDSC64(VM_MAX_ADDRESS, r0)
#endif
	bgtu/u	r2, r0, tr1		/* Bomb if uaddr isn't in user space */
	add	r4, r2, r1		/* Check end of source buffer */
	bgtu/u	r1, r0, tr1		/* Bomb if not in user space */
	bgtu/u	r2, r1, tr1		/* Also bomb if uaddr+len wraps */

#ifndef _LP64
	LDSC32(SH5_KSEG0_BASE, r0)
#else
	LDSC64(SH5_KSEG0_BASE, r0)
#endif
	bgtu/u	r0, r3, tr1		/* Bomb if kaddr isn't a kernel addr */
	add	r4, r3, r1		/* Check end of destination buffer */
	bgtu/u	r0, r1, tr1		/* Bomb if not in kernel space */
	bgtu/u	r3, r1, tr1		/* Also bomb if kaddr+len wraps */

	pta/u	Lcopyerr, tr1
	getcon	kcr0, r0		/* Fetch cpu_info */
	gettr	tr1, r1
	LDPTR	r0, CI_CURPCB, r0
	STPTR	r0, PCB_ONFAULT, r1	/* Set pcb_onfault to Lcopyerr */

#ifdef _COPYINOUT_USE_MEMCPY
	/*
	 * Note: This assumes we're using the asm version of memcpy()
	 */
	LEAF(_C_LABEL(memcpy), r25)
	ptabs/l	r25, tr0
	add	r2, r63, r0
	add	r3, r63, r2
	add	r0, r63, r3
	blink	tr0, r18
#else
	movi	0, r1
	pta/l	1f, tr1

1:	ldx.b	r2, r1, r0		/* Copy (XXX: SLOWWWWWWW!!) */
	stx.b	r3, r1, r0
	addi	r1, 1, r1
	bgt/l	r4, r1, tr1
#endif
	movi	0, r60			/* No error! */

Lcopyerr:
	getcon	kcr0, r0		/* Refetch cpu_info */
	LDPTR	r0, CI_CURPCB, r0
	STPTR	r0, PCB_ONFAULT, r63	/* Clear pcb_onfault */
Lcopyexit:
#ifdef DDB
	UNLINK_FRAME(0)
	ptabs/l	r18, tr0
#else
	ptabs/l	r61, tr0
#endif
	or	r60, r63, r2		/* Copy error code */
	blink	tr0, r63



/******************************************************************************
 *
 * int copyoutstr(const void *kaddr, void *uaddr, size_t len, size_t *done)
 *                            r2           r3            r4           r5
 */
ENTRY(copyoutstr)
#ifdef DDB
	LINK_FRAME(0)
#else
	ptabs/u	r18, tr0
#endif
	pta/u	Lcopystrerr, tr3
	movi	EFAULT, r6		/* Assume the worst */
	movi	0, r7
	bge/u	r63, r4, tr3		/* Return EFAULT if len <= 0 */

#ifndef _LP64
	LDSC32(VM_MAX_ADDRESS, r0)
#else
	LDSC64(VM_MAX_ADDRESS, r0)
#endif
	bgtu/u	r3, r0, tr3		/* Bomb if uaddr isn't in user space */

#ifndef _LP64
	LDSC32(SH5_KSEG0_BASE, r0)
#else
	LDSC64(SH5_KSEG0_BASE, r0)
#endif
	bgtu/u	r0, r2, tr3		/* Bomb if kaddr isn't a kernel addr */

	getcon	kcr0, r0		/* Fetch cpu_info */
	gettr	tr3, r1
	LDPTR	r0, CI_CURPCB, r0
	STPTR	r0, PCB_ONFAULT, r1	/* Set pcb_onfault to Lcopystrerr */

	pta/u	2f, tr2			/* Success path */
	pta/l	1f, tr1			/* Loop */

1:	ldx.b	r2, r7, r0
	stx.b	r3, r7, r0
	addi	r7, 1, r7
	beq/u	r4, r7, tr2		/* Reached limit set by "len"? */
	bne/l	r0, r63, tr1		/* Back until we find the NUL byte */

2:	movi	ENAMETOOLONG, r6	/* Assume failed */
	cmveq	r0, r63, r6		/* Success if we didn't exceed len */
	blink	tr3, r63


/******************************************************************************
 *
 * int copyinstr(const void *uaddr, void *kaddr, size_t len, size_t *done)
 *                           r2           r3            r4           r5
 */
ENTRY(copyinstr)
#ifdef DDB
	LINK_FRAME(0)
#else
	ptabs/u	r18, tr0
#endif
	pta/u	Lcopystrerr, tr3
	movi	EFAULT, r6		/* Assume the worst */
	movi	0, r7
	bge/u	r63, r4, tr3		/* Return EFAULT if len <= 0 */

#ifndef _LP64
	LDSC32(VM_MAX_ADDRESS, r0)
#else
	LDSC64(VM_MAX_ADDRESS, r0)
#endif
	bgtu/u	r2, r0, tr3		/* Bomb if uaddr isn't in user space */

#ifndef _LP64
	LDSC32(SH5_KSEG0_BASE, r0)
#else
	LDSC64(SH5_KSEG0_BASE, r0)
#endif
	bgtu/u	r0, r3, tr3		/* Bomb if kaddr isn't a kernel addr */
	add	r4, r3, r1		/* Check end of destination buffer */
	bgtu/u	r0, r1, tr3		/* Bomb if not in kernel space */
	bgtu/u	r3, r1, tr3		/* Also bomb if kaddr+len wraps */

	getcon	kcr0, r0		/* Fetch cpu_info */
	gettr	tr3, r1
	LDPTR	r0, CI_CURPCB, r0
	STPTR	r0, PCB_ONFAULT, r1	/* Set pcb_onfault to Lcopystrerr */

	pta/u	2f, tr2			/* Success path */
	pta/l	1f, tr1			/* Loop */

1:	ldx.b	r2, r7, r0
	stx.b	r3, r7, r0
	addi	r7, 1, r7
	beq/u	r4, r7, tr2		/* Reached limit set by "len"? */
	bne/l	r0, r63, tr1		/* Back until we find the NUL byte */

2:	movi	ENAMETOOLONG, r6	/* Assume failed */
	cmveq	r0, r63, r6		/* Success if we didn't exceed len */

Lcopystrerr:
	getcon	kcr0, r0		/* Refetch cpu_info */
	LDPTR	r0, CI_CURPCB, r0
	STPTR	r0, PCB_ONFAULT, r63	/* Clear pcb_onfault */
	or	r6, r63, r2		/* Grab return code */
#ifdef DDB
	UNLINK_FRAME(0)
	ptabs/l	r18, tr0
#endif
	beq/u	r5, r63, tr0		/* Just return if "done" == NULL */
	STPTR	r5, 0, r7		/* NOTE: sizeof(size_t) = sizeof(ptr) */
	blink	tr0, r63



/******************************************************************************
 *
 * int copystr(const void *ksrc, void *kdst, size_t len, size_t *done)
 *                         r2           r3            r4           r5
 */
ENTRY(copystr)
	ptabs/u	r18, tr0
	movi	ENAMETOOLONG, r6	/* Assume failed */
	bge/u	r63, r4, tr0		/* Return ENAMETOOLONG if len <= 0 */
	movi	0, r1
	pta/u	2f, tr2			/* Success path */
	pta/l	1f, tr1			/* Loop */
1:	ldx.b	r2, r1, r0
	stx.b	r3, r1, r0
	addi	r1, 1, r1
	beq/u	r4, r1, tr2		/* Reached limit set by "len"? */
	bne/l	r0, r63, tr1		/* Back until we find the NUL byte */
2:	cmveq	r0, r63, r6		/* Success if we didn't exceed len */
	or	r6, r63, r2		/* Grab return code */
	beq/u	r5, r63, tr0		/* Just return if "done" == NULL */
	STPTR	r5, 0, r1		/* NOTE: sizeof(size_t) = sizeof(ptr) */
	blink	tr0, r63



/******************************************************************************
 *
 * int kcopy(const void *src, void *dst, size_t len)
 *                       r2         r3          r4
 */
ENTRY(kcopy)
#ifdef DDB
	LINK_FRAME(0)
#else
	add	r18, r63, r61
#endif
	movi	EFAULT, r60		/* Assume there was a problem */
	cmveq	r4, r63, r60		/* But return success if len == 0 */
	pta/u	Lkcopyexit, tr1
	bge/u	r63, r4, tr1		/* Return EFAULT if len <= 0 */

#ifndef _LP64
	LDSC32(SH5_KSEG0_BASE, r0)
#else
	LDSC64(SH5_KSEG0_BASE, r0)
#endif
	bgtu/u	r0, r2, tr1		/* Bomb if src not kernel addr */
	add	r4, r2, r1		/* Check end of src buffer */
	bgtu/u	r0, r1, tr1		/* Bomb if not in kernel space */
	bgtu/u	r2, r1, tr1		/* Also bomb if src+len wraps */

	bgtu/u	r0, r3, tr1		/* Bomb if dst not kernel addr */
	add	r4, r3, r1		/* Check end of dst buffer */
	bgtu/u	r0, r1, tr1		/* Bomb if not in kernel space */
	bgtu/u	r3, r1, tr1		/* Also bomb if dst+len wraps */

	pta/u	Lkcopyerr, tr1
	getcon	kcr0, r0		/* Fetch cpu_info */
	LDPTR	r0, CI_CURPCB, r0
	gettr	tr1, r1
	LDPTR	r0, PCB_ONFAULT, r6	/* Preserve current pcb_onfault */
	STPTR	r0, PCB_ONFAULT, r1	/* Set pcb_onfault to Lcopyerr */

#ifdef _COPYINOUT_USE_MEMCPY
	/*
	 * Note: This assumes we're using the asm version of memcpy()
	 */
	LEAF(_C_LABEL(memcpy), r25)
	ptabs/l	r25, tr0
	add	r2, r63, r0
	add	r3, r63, r2
	add	r0, r63, r3
	blink	tr0, r18
#else
	movi	0, r1
	pta/l	1f, tr1

1:	ldx.b	r2, r1, r0		/* Copy (XXX: SLOWWWWWWW!!) */
	stx.b	r3, r1, r0
	addi	r1, 1, r1
	bgt/l	r4, r1, tr1
#endif

	movi	0, r60			/* No error! */

Lkcopyerr:
	getcon	kcr0, r0		/* Refetch cpu_info */
	LDPTR	r0, CI_CURPCB, r0
	STPTR	r0, PCB_ONFAULT, r6	/* Restore pcb_onfault */
Lkcopyexit:
#ifdef DDB
	UNLINK_FRAME(0)
	ptabs/l	r18, tr0
#else
	ptabs/l	r61, tr0
#endif
	or	r60, r63, r2		/* Copy error code */
	blink	tr0, r63


/******************************************************************************
 *
 * int subyte(void *base, int c)
 * int suibyte(void *base, int c)
 *
 * Store byte `c' to user address space `base', returning EFAULT on error.
 */
ENTRY(subyte)
ALTENTRY(suibyte)
#ifdef DDB
	LINK_FRAME(0)
#else
	ptabs/l	r18, tr0
#endif
	pta/u	Lsubexit, tr1
	movi	EFAULT, r5		/* Assume there was a problem */
#ifndef _LP64
	LDSC32(VM_MAX_ADDRESS, r0)
#else
	LDSC64(VM_MAX_ADDRESS, r0)
#endif
	bgtu/u	r2, r0, tr1		/* Bomb if base isn't in user space */
	pta/u	Lsuberr, tr1
	getcon	kcr0, r0		/* Fetch cpu_info */
	gettr	tr1, r1
	LDPTR	r0, CI_CURPCB, r0
	LDPTR	r0, PCB_ONFAULT, r8
	STPTR	r0, PCB_ONFAULT, r1	/* Set pcb_onfault to Lcopyerr */
	st.b	r2, 0, r3
	movi	0, r5			/* No error! */
Lsuberr:
	getcon	kcr0, r0		/* Refetch cpu_info */
	LDPTR	r0, CI_CURPCB, r0
	STPTR	r0, PCB_ONFAULT, r8	/* Clear pcb_onfault */
Lsubexit:
#ifdef DDB
	UNLINK_FRAME(0)
	ptabs/l	r18, tr0
#endif
	or	r5, r63, r2		/* Copy error code */
	blink	tr0, r63


/******************************************************************************
 *
 * int susword(void *base, short c)
 * int suisword(void *base, short c)
 *
 * Store u_int16_t `c' to user address space `base', returning EFAULT on error.
 */
ENTRY(susword)
ALTENTRY(suisword)
#ifdef DDB
	LINK_FRAME(0)
#else
	ptabs/l	r18, tr0
#endif
	pta/u	Lsusexit, tr1
	movi	EFAULT, r5		/* Assume there was a problem */
#ifndef _LP64
	LDSC32(VM_MAX_ADDRESS, r0)
#else
	LDSC64(VM_MAX_ADDRESS, r0)
#endif
	bgtu/u	r2, r0, tr1		/* Bomb if base isn't in user space */
	pta/u	Lsuserr, tr1
	getcon	kcr0, r0		/* Fetch cpu_info */
	gettr	tr1, r1
	LDPTR	r0, CI_CURPCB, r0
	LDPTR	r0, PCB_ONFAULT, r8
	STPTR	r0, PCB_ONFAULT, r1	/* Set pcb_onfault to Lcopyerr */
	st.w	r2, 0, r3
	movi	0, r5			/* No error! */
Lsuserr:
	getcon	kcr0, r0		/* Refetch cpu_info */
	LDPTR	r0, CI_CURPCB, r0
	STPTR	r0, PCB_ONFAULT, r8	/* Clear pcb_onfault */
Lsusexit:
#ifdef DDB
	UNLINK_FRAME(0)
	ptabs/l	r18, tr0
#endif
	or	r5, r63, r2		/* Copy error code */
	blink	tr0, r63


/******************************************************************************
 *
 * int suword(void *base, long)
 * int suiword(void *base, long)
 *
 * Store long `c' to user address space `base', returning EFAULT on error.
 */
ENTRY(suword)
ALTENTRY(suiword)
#ifdef DDB
	LINK_FRAME(0)
#else
	ptabs/l	r18, tr0
#endif
	pta/u	Lsuwexit, tr1
	movi	EFAULT, r5		/* Assume there was a problem */
#ifndef _LP64
	LDSC32(VM_MAX_ADDRESS, r0)
#else
	LDSC64(VM_MAX_ADDRESS, r0)
#endif
	bgtu/u	r2, r0, tr1		/* Bomb if base isn't in user space */
	pta/u	Lsuwerr, tr1
	getcon	kcr0, r0		/* Fetch cpu_info */
	gettr	tr1, r1
	LDPTR	r0, CI_CURPCB, r0
	LDPTR	r0, PCB_ONFAULT, r8
	STPTR	r0, PCB_ONFAULT, r1	/* Set pcb_onfault to Lcopyerr */
	STPTR	r2, 0, r3
	movi	0, r5			/* No error! */
Lsuwerr:
	getcon	kcr0, r0		/* Refetch cpu_info */
	LDPTR	r0, CI_CURPCB, r0
	STPTR	r0, PCB_ONFAULT, r8	/* Clear pcb_onfault */
Lsuwexit:
#ifdef DDB
	UNLINK_FRAME(0)
	ptabs/l	r18, tr0
#endif
	or	r5, r63, r2		/* Copy error code */
	blink	tr0, r63


/******************************************************************************
 *
 * int fubyte(void *base)
 * int fuibyte(void *base)
 *
 * Fetch a byte from user address space `base', returning -1 on error.
 */
ENTRY(fubyte)
ALTENTRY(fuibyte)
#ifdef DDB
	LINK_FRAME(0)
#else
	ptabs/l	r18, tr0
#endif
	pta/u	Lfubexit, tr1
	movi	-1, r5			/* Assume there was a problem */
#ifndef _LP64
	LDSC32(VM_MAX_ADDRESS, r0)
#else
	LDSC64(VM_MAX_ADDRESS, r0)
#endif
	bgtu/u	r2, r0, tr1		/* Bomb if base isn't in user space */
	pta/u	Lfuberr, tr1
	getcon	kcr0, r0		/* Fetch cpu_info */
	gettr	tr1, r1
	LDPTR	r0, CI_CURPCB, r0
	LDPTR	r0, PCB_ONFAULT, r8
	STPTR	r0, PCB_ONFAULT, r1	/* Set pcb_onfault to Lcopyerr */
	ld.ub	r2, 0, r5
Lfuberr:
	getcon	kcr0, r0		/* Refetch cpu_info */
	LDPTR	r0, CI_CURPCB, r0
	STPTR	r0, PCB_ONFAULT, r8	/* Clear pcb_onfault */
Lfubexit:
#ifdef DDB
	UNLINK_FRAME(0)
	ptabs/l	r18, tr0
#endif
	or	r5, r63, r2		/* Copy result */
	blink	tr0, r63


/******************************************************************************
 *
 * int fusword(void *base)
 * int fuisword(void *base)
 *
 * Fetch a u_int16_t from user address space `base', returning -1 on error.
 */
ENTRY(fusword)
ALTENTRY(fuisword)
#ifdef DDB
	LINK_FRAME(0)
#else
	ptabs/l	r18, tr0
#endif
	pta/u	Lfusexit, tr1
	movi	-1, r5			/* Assume there was a problem */
#ifndef _LP64
	LDSC32(VM_MAX_ADDRESS, r0)
#else
	LDSC64(VM_MAX_ADDRESS, r0)
#endif
	bgtu/u	r2, r0, tr1		/* Bomb if base isn't in user space */
	pta/u	Lfuserr, tr1
	getcon	kcr0, r0		/* Fetch cpu_info */
	gettr	tr1, r1
	LDPTR	r0, CI_CURPCB, r0
	LDPTR	r0, PCB_ONFAULT, r8
	STPTR	r0, PCB_ONFAULT, r1	/* Set pcb_onfault to Lcopyerr */
	ld.uw	r2, 0, r5
Lfuserr:
	getcon	kcr0, r0		/* Refetch cpu_info */
	LDPTR	r0, CI_CURPCB, r0
	STPTR	r0, PCB_ONFAULT, r8	/* Clear pcb_onfault */
Lfusexit:
#ifdef DDB
	UNLINK_FRAME(0)
	ptabs/l	r18, tr0
#endif
	or	r5, r63, r2		/* Copy result */
	blink	tr0, r63


/******************************************************************************
 *
 * long fuword(void *base)
 * long fuiword(void *base)
 *
 * Fetch a long from user address space `base', returning -1 on error.
 */
ENTRY(fuword)
ALTENTRY(fuiword)
#ifdef DDB
	LINK_FRAME(0)
#else
	ptabs/l	r18, tr0
#endif
	pta/u	Lfuwexit, tr1
	movi	-1, r5			/* Assume there was a problem */
#ifndef _LP64
	LDSC32(VM_MAX_ADDRESS, r0)
#else
	LDSC64(VM_MAX_ADDRESS, r0)
#endif
	bgtu/u	r2, r0, tr1		/* Bomb if base isn't in user space */
	pta/u	Lfuwerr, tr1
	getcon	kcr0, r0		/* Fetch cpu_info */
	gettr	tr1, r1
	LDPTR	r0, CI_CURPCB, r0
	LDPTR	r0, PCB_ONFAULT, r8
	STPTR	r0, PCB_ONFAULT, r1	/* Set pcb_onfault to Lcopyerr */
	LDPTR	r2, 0, r5		/* XXX: Should we really sign-extend? */
Lfuwerr:
	getcon	kcr0, r0		/* Refetch cpu_info */
	LDPTR	r0, CI_CURPCB, r0
	STPTR	r0, PCB_ONFAULT, r8	/* Clear pcb_onfault */
Lfuwexit:
#ifdef DDB
	UNLINK_FRAME(0)
	ptabs/l	r18, tr0
#endif
	or	r5, r63, r2		/* Copy result */
	blink	tr0, r63


/******************************************************************************
 *
 * int suswintr(void *base, short c)
 * int fuswintr(void *base)
 *
 * Interrupt-safe versions of the above.
 */
ENTRY(fuswintr)
ALTENTRY(suswintr)
	ptabs/l	r18, tr0
	movi	EFAULT, r2
	blink	tr0, r63



/******************************************************************************
 *
 * void pmap_asm_zero_page(vaddr_t kva)
 *
 * Fast, cache-friendly zero page function.
 */
ENTRY(pmap_asm_zero_page)
	ptabs/u	r18, tr0
	pta/l	1f, tr1
#ifndef _LP64
	add.l	r2, r63, r2		/* Ensure kva is sign-extended */
#endif
	movi	NBPG-128, r0
	add	r2, r0, r0		/* End of page (minus 4 cache-lines) */
	blink	tr1, r63

	/* Cache-align the loop */
	.balign	32
1:	st.q	r2, 0, r63
	st.q	r2, 8, r63
	st.q	r2, 16, r63
	st.q	r2, 24, r63
	alloco	r2, 128

	st.q	r2, 32, r63
	st.q	r2, 40, r63
	st.q	r2, 48, r63
	st.q	r2, 56, r63
	alloco	r2, 160

	st.q	r2, 64, r63
	st.q	r2, 72, r63
	st.q	r2, 80, r63
	st.q	r2, 88, r63
	alloco	r2, 192

	st.q	r2, 96, r63
	st.q	r2, 104, r63
	st.q	r2, 112, r63
	st.q	r2, 120, r63
	alloco	r2, 224

	addi	r2, 128, r2		/* Next block */
	bne/l	r2, r0, tr1		/* Back for the next one, until done */

	/* Finish the remaining block */
	st.q	r2, 0, r63
	st.q	r2, 8, r63
	st.q	r2, 16, r63
	st.q	r2, 24, r63
	st.q	r2, 32, r63
	st.q	r2, 40, r63
	st.q	r2, 48, r63
	st.q	r2, 56, r63
	st.q	r2, 64, r63
	st.q	r2, 72, r63
	st.q	r2, 80, r63
	st.q	r2, 88, r63
	st.q	r2, 96, r63
	st.q	r2, 104, r63
	st.q	r2, 112, r63
	st.q	r2, 120, r63
	blink	tr0, r63


/******************************************************************************
 *
 * void pmap_asm_copy_page(vaddr_t dst, vaddr_t src)
 *
 * Fast, cache-friendly copy page function.
 */
ENTRY(pmap_asm_copy_page)
	ptabs/u	r18, tr0
	pta/l	1f, tr1
#ifndef _LP64
	add.l	r2, r63, r2		/* Ensure src/dst are sign-extended */
	add.l	r3, r63, r3
#endif
	movi	NBPG-128, r0
	add	r2, r0, r0		/* End of page (minus 4 cache-lines) */
	blink	tr1, r63

	/* Cache-align the loop */
	.balign	32
1:	
	ld.q	r3, 0, r4		/* Fetch 32-bytes at a time */
	ld.q	r3, 8, r5
	ld.q	r3, 16, r6
	ld.q	r3, 24, r7
	st.q	r2, 0, r4
	st.q	r2, 8, r5
	st.q	r2, 16, r6
	st.q	r2, 24, r7
	ld.q	r3, 128, r63		/* Pre-fetch next src cache-line */
	alloco	r2, 128			/* Allocate cache-line for next dst */

	ld.q	r3, 32, r4		/* Fetch 32-bytes at a time */
	ld.q	r3, 40, r5
	ld.q	r3, 48, r6
	ld.q	r3, 56, r7
	st.q	r2, 32, r4
	st.q	r2, 40, r5
	st.q	r2, 48, r6
	st.q	r2, 56, r7
	ld.q	r3, 160, r63		/* Pre-fetch next src cache-line */
	alloco	r2, 160			/* Allocate cache-line for next dst */

	ld.q	r3, 64, r4		/* Fetch 32-bytes at a time */
	ld.q	r3, 72, r5
	ld.q	r3, 80, r6
	ld.q	r3, 88, r7
	st.q	r2, 64, r4
	st.q	r2, 72, r5
	st.q	r2, 80, r6
	st.q	r2, 88, r7
	ld.q	r3, 192, r63		/* Pre-fetch next src cache-line */
	alloco	r2, 192			/* Allocate cache-line for next dst */

	ld.q	r3, 96, r4		/* Fetch 32-bytes at a time */
	ld.q	r3, 104, r5
	ld.q	r3, 112, r6
	ld.q	r3, 120, r7
	st.q	r2, 96, r4
	st.q	r2, 104, r5
	st.q	r2, 112, r6
	st.q	r2, 120, r7
	ld.q	r3, 224, r63		/* Pre-fetch next src cache-line */
	alloco	r2, 224			/* Allocate cache-line for next dst */

	addi	r2, 128, r2		/* Next dst block */
	addi	r3, 128, r3		/* Next src block */
	bne/l	r2, r0, tr1		/* Back for the next one, until done */

	/* Finish the remaining block */
	ld.q	r3, 0, r4
	ld.q	r3, 8, r5
	ld.q	r3, 16, r6
	ld.q	r3, 24, r7
	st.q	r2, 0, r4
	st.q	r2, 8, r5
	st.q	r2, 16, r6
	st.q	r2, 24, r7
	ld.q	r3, 32, r4
	ld.q	r3, 40, r5
	ld.q	r3, 48, r6
	ld.q	r3, 56, r7
	st.q	r2, 32, r4
	st.q	r2, 40, r5
	st.q	r2, 48, r6
	st.q	r2, 56, r7
	ld.q	r3, 64, r4
	ld.q	r3, 72, r5
	ld.q	r3, 80, r6
	ld.q	r3, 88, r7
	st.q	r2, 64, r4
	st.q	r2, 72, r5
	st.q	r2, 80, r6
	st.q	r2, 88, r7
	ld.q	r3, 96, r4
	ld.q	r3, 104, r5
	ld.q	r3, 112, r6
	ld.q	r3, 120, r7
	st.q	r2, 96, r4
	st.q	r2, 104, r5
	st.q	r2, 112, r6
	st.q	r2, 120, r7
	blink	tr0, r63



#include "dtfcons.h"
#if NDTFCONS > 0
/******************************************************************************
 *
 * void _dtf_trap(paddr_t fpfreg, paddr_t trapbuff, paddr_t _dtf_trap_frob)
 *
 * Frob the FPF register to initiate a DTF transaction with the debug host.
 * The MMU has to be disabled for the duration of the transaction, which is
 * why we have to do it in assembler.
 *
 * Returns only when the host acks the transaction, which may take some
 * time, depending on the requested operation.
 *
 * XXX: The DTF protocol would appear to only work with 32-bit phyisical
 * addresses...
 */
ENTRY(_dtf_trap)
	getcon	sr, r5			/* Save current SR in r5 */
	LDC32(SH5_CONREG_SR_BL|SH5_CONREG_SR_MD|SH5_CONREG_SR_IMASK_ALL, r0)
	ori	r5, SH5_CONREG_SR_IMASK_ALL, r1
	addz.l	r4, r63, r4		/* Clear upper 32-bits of frob addr */
	putcon	r1, sr			/* Ensure interrupts are clear */
	putcon	r4, spc			/* Phys address of _dtf_trap_frob() */
	putcon	r0, ssr			/* Prime the SR to use while frobbing */
	movi	-2304, r0		/* Some "magic number". Don't ask. */
	shlli	r0, 48, r0		/* r0 == 0xf700000000000000 */
	addz.l	r3, r63, r3		/* Clear upper 32-bits of trapbuff */
	addz.l	r2, r63, r2		/* Clear upper 32-bits of fpfreg */
	or	r3, r0, r0		/* r0 == value to write to fpfreg */
	add.l	r3, r63, r3		/* Sign-extend trapbuff address again */
	synco
	synci
	rte
	nop
	nop
	nop
	nop
	nop

ENTRY_NOPROFILE(_dtf_trap_frob)
	/*
	 * The MMU is off and exceptions/interrupts are blocked.
	 */
	ld.l	r3, 4, r1		/* Get pointer to dtf_params struct */
	synco
	synci
	st.q	r2, 0, r0		/* Frob the FPF register */
	synco
	synci
	pta/u	2f, tr0
	pta/u	3f, tr1
#ifndef SH5_SIM
	LDC32(0x100000, r0)
#else
	LDC32(0x4000, r0)
#endif

2:	ld.l	r1, 0, r2		/* Fetch "sync" value */
	synco
	synci
	beq/u	r2, r63, tr1		/* Exit if clear*/
	addi	r0, -1, r0
	bne/l	r0, r63, tr0		/* Back until timeout */

3:	putcon	r18, spc		/* Prepare to return to caller */
	putcon	r5, ssr			/* Original SR value */
	synco
	synci
	rte				/* Back to caller, with MMU enabled, */
	nop				/* and original SR restored */
	nop
	nop
	nop
	nop


	.comm	_C_LABEL(_dtf_buffer),NBPG,32
#endif	/* NDTFCONS */


/*
 * Reserve space in the BSS for lwp0's PCB
 */
	.comm	Lproc0paddr,USPACE,16
	.comm	_C_LABEL(proc0paddr),8,8

/*
 * Reserve space in the BSS for cpu_info
 */
	.comm	Lcpu_info,SZ_CPU_INFO,8


/*
 * Interrupt counters.
 * XXXSCW: These need to go away when userland tools (vmstat(8)) stop
 * grovelling for them.
 */
	.data
	.balign 4
#ifdef _LP64
#define	TYPEOF_INTRCNT	.quad
#else
#define	TYPEOF_INTRCNT	.long
#endif
GLOBAL(intrcnt)
	TYPEOF_INTRCNT	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
GLOBAL(eintrcnt)
