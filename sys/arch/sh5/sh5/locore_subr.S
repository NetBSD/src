/*	$NetBSD: locore_subr.S,v 1.1 2002/07/05 13:32:05 scw Exp $	*/

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
 * To use it, put #include <sh5/sh5/locore_subr.S> in your port's locore.S
 *
 * ##########
 */

	.section .text..SHmedia32,"ax"
	.align 3

/******************************************************************************
 *
 * Set up a basic environment in preparation for calling C code
 *
 * To be called early on from port-specific locore.S, after the MMU is
 * enabled, but before pmap_bootstrap().
 */
Lsh5_init_env:
	/*
	 * Fix up the vector table.
	 * Note: For now, we don't touch RESVEC on the assumption that
	 * a ROM-based debugger is being used.
	 */
	LEA(_C_LABEL(sh5_vector_table), r0)
	putcon  r0, vbr
	putcon  r0, resvec

	/*
	 * Set up proc0's pcb.
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

	/* Fix up proc0's p_addr */
	LEA(_C_LABEL(proc0), r3)
	STPTR	r3, P_ADDR, r0		/* proc0->p_addr = &proc0paddr */

	/*
	 * Create a fake trapframe for proc0 which will be used when
	 * forking the initial process.
	 */
	movi	SZ_TRAPFRAME, r2
	sub	r15, r2, r2		/* r2 -> base of fake trapframe */

	/* Fix up proc0->p_md.md_regs to point to the base of the trapframe */
	STPTR	r3, P_MD_REGS, r2

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
	ori	r3, SH5_CONREG_SR_IMASK_IPL15, r3
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
	getcon	sr, r4
	LEAF(Lcheck_softint_loweripl, r0)
	ptabs/l	r0, tr0
	movi	SH5_CONREG_SR_IMASK_ALL, r3
	shlli	r2, SH5_CONREG_SR_IMASK_SHIFT, r2
	andc	r4, r3, r4
	and	r2, r3, r2
	or	r4, r2, r2
	blink	tr0, r63


/******************************************************************************
 *
 * int splraise(int ipl);
 */
ENTRY_NOPROFILE(splraise)
	ptabs/l	r18, tr0
	movi	SH5_CONREG_SR_IMASK_ALL, r1
	getcon	sr, r3
	andi	r2, 15, r0		/* Ensure "ipl" is within range */
	and	r1, r3, r2		/* Get old mask to r2 */
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
	pta/l	_C_LABEL(copyin), tr0
	or	r2, r63, r0		/* Swap r2 and r3 */
	or	r3, r63, r2
	or	r0, r63, r2
	blink	tr0, r63


/******************************************************************************
 *
 * int copyin(const void *uaddr, void *kaddr, size_t len)
 *                        r2           r3            r4
 */
ENTRY(copyin)
	ptabs/l	r18, tr0
	bgt/u	r63, r4, tr0		/* Just return if len == 0 */
	pta/u	Lcopyexit, tr1
	movi	EFAULT, r5		/* Assume there was a problem */

#ifndef _LP64
	LDSC32(VM_MAX_ADDRESS, r0)
#else
	LDSC64(VM_MAX_ADDRESS, r0)
#endif
	bgtu/u	r2, r0, tr1		/* Bomb if uaddr isn't in user space */
	add	r4, r2, r1		/* Check end of destination buffer */
	bgtu/u	r1, r0, tr1		/* Bomb if not in user space */
	bgtu/u	r2, r1, tr1		/* Also bomb if uaddr+len wraps */

#ifndef _LP64
	LDSC32(VM_MIN_KERNEL_ADDRESS, r0)
#else
	LDSC64(VM_MIN_KERNEL_ADDRESS, r0)
#endif
	bgtu/u	r0, r3, tr1		/* Bomb if kaddr isn't a kernel addr */
	add	r4, r3, r1		/* Check end of source buffer */
	bgtu/u	r0, r1, tr1		/* Bomb if not in kernel space */
	bgtu/u	r3, r1, tr1		/* Also bomb if kaddr+len wraps */

	pta/u	Lcopyerr, tr1
	getcon	kcr0, r0		/* Fetch cpu_info */
	gettr	tr1, r1
	LDPTR	r0, CI_CURPCB, r0
	STPTR	r0, PCB_ONFAULT, r1	/* Set pcb_onfault to Lcopyerr */

	movi	0, r1
	pta/l	1f, tr1

1:	ldx.b	r2, r1, r0		/* Copy (XXX: SLOWWWWWWW!!) */
	stx.b	r3, r1, r0
	addi	r1, 1, r1
	bgt/l	r4, r1, tr1
	movi	0, r5			/* No error! */

Lcopyerr:
	getcon	kcr0, r0		/* Refetch cpu_info */
	LDPTR	r0, CI_CURPCB, r0
	STPTR	r0, PCB_ONFAULT, r63	/* Clear pcb_onfault */
Lcopyexit:
	or	r5, r63, r2		/* Copy error code */
	blink	tr0, r63



/******************************************************************************
 *
 * int copyoutstr(const void *kaddr, void *uaddr, size_t len, size_t *done)
 *                            r2           r3            r4           r5
 */
ENTRY(copyoutstr)
	pta/l	_C_LABEL(copyinstr), tr0
	or	r2, r63, r0		/* Swap r2 and r3 */
	or	r3, r63, r2
	or	r0, r63, r2
	blink	tr0, r63


/******************************************************************************
 *
 * int copyinstr(const void *uaddr, void *kaddr, size_t len, size_t *done)
 *                           r2           r3            r4           r5
 */
ENTRY(copyinstr)
	ptabs/u	r18, tr0
	pta/u	Lcopystrexit, tr3
	movi	EFAULT, r6		/* Assume the worst */
	movi	0, r7

#ifndef _LP64
	LDSC32(VM_MAX_ADDRESS, r0)
#else
	LDSC64(VM_MAX_ADDRESS, r0)
#endif
	bgtu/u	r2, r0, tr3		/* Bomb if uaddr isn't in user space */
	add	r4, r2, r1		/* Check end of destination buffer */
	bgtu/u	r1, r0, tr3		/* Bomb if not in user space */
	bgtu/u	r2, r1, tr3		/* Also bomb if uaddr+len wraps */

#ifndef _LP64
	LDSC32(VM_MIN_KERNEL_ADDRESS, r0)
#else
	LDSC64(VM_MIN_KERNEL_ADDRESS, r0)
#endif
	bgtu/u	r0, r3, tr3		/* Bomb if kaddr isn't a kernel addr */
	add	r4, r3, r1		/* Check end of source buffer */
	bgtu/u	r0, r1, tr3		/* Bomb if not in kernel space */
	bgtu/u	r3, r1, tr3		/* Also bomb if kaddr+len wraps */

	pta/u	Lcopystrexit, tr3
	getcon	kcr0, r0		/* Fetch cpu_info */
	gettr	tr3, r1
	LDPTR	r0, CI_CURPCB, r0
	STPTR	r0, PCB_ONFAULT, r1	/* Set pcb_onfault to Lcopystrerr */

	pta/u	2f, tr2			/* Success path */
	pta/l	1f, tr1			/* Loop */

1:	beq/u	r4, r7, tr2		/* Oops, ENAMETOOLONG */
	ldx.b	r2, r7, r0
	stx.b	r3, r7, r0
	addi	r7, 1, r7
	bne/l	r0, r63, tr1		/* Back until we find the NUL byte */

2:	sub	r4, r7, r0
	movi	ENAMETOOLONG, r6	/* Assume failed */
	cmvne	r0, r63, r6		/* Success if we didn't exceed len */

Lcopystrerr:
	getcon	kcr0, r0		/* Refetch cpu_info */
	LDPTR	r0, CI_CURPCB, r0
	STPTR	r0, PCB_ONFAULT, r63	/* Clear pcb_onfault */
Lcopystrexit:
	or	r6, r63, r2		/* Grab return code */
	beq/u	r5, r63, tr0		/* Just return if "done" == NULL */
	STPTR	r5, 0, r7		/* NOTE: sizeof(size_t) = sizeof(ptr) */
	blink	tr0, r63



/******************************************************************************
 *
 * int copystr(const void *uaddr, void *kaddr, size_t len, size_t *done)
 *                         r2           r3            r4           r5
 */
ENTRY(copystr)
	ptabs/u	r18, tr0
	movi	0, r1
	pta/u	2f, tr2			/* Success path */
	pta/l	1f, tr1			/* Loop */
1:	beq/u	r4, r1, tr2		/* Oops, ENAMETOOLONG */
	ldx.b	r2, r1, r0
	stx.b	r3, r1, r0
	addi	r1, 1, r1
	bne/l	r0, r63, tr1		/* Back until we find the NUL byte */
2:	sub	r4, r1, r0
	movi	ENAMETOOLONG, r6	/* Assume failed */
	cmvne	r0, r63, r6		/* Success if we didn't exceed len */
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
	ptabs/l	r18, tr0
	bgt/u	r63, r4, tr0		/* Just return if len == 0 */
	pta/u	Lkcopyexit, tr1
	movi	EFAULT, r5		/* Assume there was a problem */

#ifndef _LP64
	LDSC32(VM_MIN_KERNEL_ADDRESS, r0)
#else
	LDSC64(VM_MIN_KERNEL_ADDRESS, r0)
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

	movi	0, r1
	pta/l	1f, tr1

1:	ldx.b	r2, r1, r0		/* Copy (XXX: SLOWWWWWWW!!) */
	stx.b	r3, r1, r0
	addi	r1, 1, r1
	bgt/l	r4, r1, tr1
	movi	0, r5			/* No error! */
Lkcopyerr:
	getcon	kcr0, r0		/* Refetch cpu_info */
	LDPTR	r0, CI_CURPCB, r0
	STPTR	r0, PCB_ONFAULT, r6	/* Restore pcb_onfault */
Lkcopyexit:
	or	r5, r63, r2		/* Copy error code */
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
	ptabs/l	r18, tr0
	bgt/u	r63, r4, tr0		/* Just return if len == 0 */
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
	ptabs/l	r18, tr0
	bgt/u	r63, r4, tr0		/* Just return if len == 0 */
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
	or	r5, r63, r2		/* Copy error code */
	blink	tr0, r63


/******************************************************************************
 *
 * int suword(void *base, short c)
 * int suiword(void *base, short c)
 *
 * Store u_int32_t `c' to user address space `base', returning EFAULT on error.
 */
ENTRY(suword)
ALTENTRY(suiword)
	ptabs/l	r18, tr0
	bgt/u	r63, r4, tr0		/* Just return if len == 0 */
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
	st.l	r2, 0, r3
	movi	0, r5			/* No error! */
Lsuwerr:
	getcon	kcr0, r0		/* Refetch cpu_info */
	LDPTR	r0, CI_CURPCB, r0
	STPTR	r0, PCB_ONFAULT, r8	/* Clear pcb_onfault */
Lsuwexit:
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
	ptabs/l	r18, tr0
	bgt/u	r63, r4, tr0		/* Just return if len == 0 */
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
	movi	0, r3
	ld.b	r2, 0, r3
	or	r3, r63, r5
Lfuberr:
	getcon	kcr0, r0		/* Refetch cpu_info */
	LDPTR	r0, CI_CURPCB, r0
	STPTR	r0, PCB_ONFAULT, r8	/* Clear pcb_onfault */
Lfubexit:
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
	ptabs/l	r18, tr0
	bgt/u	r63, r4, tr0		/* Just return if len == 0 */
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
	movi	0, r3
	ld.w	r2, 0, r3
	or	r3, r63, r5
Lfuserr:
	getcon	kcr0, r0		/* Refetch cpu_info */
	LDPTR	r0, CI_CURPCB, r0
	STPTR	r0, PCB_ONFAULT, r8	/* Clear pcb_onfault */
Lfusexit:
	or	r5, r63, r2		/* Copy result */
	blink	tr0, r63


/******************************************************************************
 *
 * int fuword(void *base)
 * int fuiword(void *base)
 *
 * Fetch a u_int32_t from user address space `base', returning -1 on error.
 */
ENTRY(fuword)
ALTENTRY(fuiword)
	ptabs/l	r18, tr0
	bgt/u	r63, r4, tr0		/* Just return if len == 0 */
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
	ld.l	r2, 0, r3
	movi	0, r5
	shori	65535, r5
	shori	65535, r5
	and	r3, r5, r5
Lfuwerr:
	getcon	kcr0, r0		/* Refetch cpu_info */
	LDPTR	r0, CI_CURPCB, r0
	STPTR	r0, PCB_ONFAULT, r8	/* Clear pcb_onfault */
Lfuwexit:
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
	pta/l	2f, tr0

2:	ld.l	r1, 0, r2		/* Fetch "sync" value */
	synco
	synci
	bne/l	r2, r63, tr0		/* Back if not clear*/

	putcon	r18, spc		/* Prepare to return to caller */
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

/*
 * Reserve space in the BSS for proc0's PCB
 */
	.comm	Lproc0paddr,USPACE,16
	.comm	_C_LABEL(proc0paddr),8,8

/*
 * Reserve space in the BSS for cpu_info
 */
	.comm	Lcpu_info,SZ_CPU_INFO,8

