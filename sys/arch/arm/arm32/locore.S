/*	$NetBSD: locore.S,v 1.38 2017/10/14 15:35:02 skrll Exp $	*/

/*
 * Copyright (C) 1994-1997 Mark Brinicombe
 * Copyright (C) 1994 Brini
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
 *	This product includes software developed by Brini.
 * 4. The name of Brini may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BRINI ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL BRINI BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "assym.h"
#include <sys/syscall.h>
#include <sys/errno.h>
#include <arm/asm.h>
#include <arm/locore.h>

/* What size should this really be ? It is only used by init_arm() */
#define INIT_ARM_STACK_SIZE	2048

	RCSID("$NetBSD: locore.S,v 1.38 2017/10/14 15:35:02 skrll Exp $")

/*
 * This is for kvm_mkdb, and should be the address of the beginning
 * of the kernel text segment (not necessarily the same as kernbase).
 */

	.text
	.align	0

ENTRY_NP(kernel_text)

ASENTRY_NP(start)
	mrs	r1, cpsr		/* fetch CPSR value */
	msr	spsr_sx, r1		/* set SPSR[23:8] to known value */

	/*
	 * Get bss bounds (r1, r2), curlwp or curcpu (r8), and set initial
	 * stack.
	 */
	adr	r1, .Lstart
	ldmia	r1, {r1, r2, r8, sp}

#if defined(TPIDRPRW_IS_CURCPU) || defined(TPIDRPRW_IS_CURLWP)
	mcr	p15, 0, r8, c13, c0, 4
#endif
#if defined(TPIDRPRW_IS_CURLWP)
	ldr	r8, [r8, #L_CPU]	/* r8 needs curcpu in it */
#endif

	mov	r4, #0
#ifdef _ARM_ARCH_DWORD_OK
	mov	r5, #0
#endif
.L1:
#ifdef _ARM_ARCH_DWORD_OK
	strd	r4, r5, [r1], #0x0008	/* Zero the bss */
#else
	str	r4, [r1], #0x0004	/* Zero the bss */
#endif
	cmp	r1, r2
	blt	.L1

	mrc	p15, 0, r3, c0, c0, 0	/* get our cpuid and save it early */
	str	r3, [r8, #CI_ARM_CPUID]

	mov	fp, #0x00000000		/* trace back starts here */
	bl	_C_LABEL(initarm)	/* Off we go */

	/* init arm will return the new stack pointer. */
	mov	sp, r0

	mov	fp, #0x00000000		/* trace back starts here */
	mov	ip, sp
	push	{fp, ip, lr, pc}
	sub	fp, ip, #4

	bl	_C_LABEL(main)		/* call main()! */

	adr	r0, .Lmainreturned
	b	_C_LABEL(panic)
	/* NOTREACHED */

.Lstart:
	.word	_edata
	.word	_end
#if defined(TPIDRPRW_IS_CURLWP)
	.word	_C_LABEL(lwp0)
#else
	.word	_C_LABEL(cpu_info_store)
#endif
	.word	svcstk + INIT_ARM_STACK_SIZE

.Lmainreturned:
	.asciz	"main() returned"
	.align	0
ASEND(start)

	.bss
#ifdef __ARM_EABI__
	.align	3
#endif
svcstk:
	.space	INIT_ARM_STACK_SIZE

	.text
	.align	0

#ifndef OFW
	/* OFW based systems will used OF_boot() */

.Lcpufuncs:
	.word	_C_LABEL(cpufuncs)

ENTRY_NP(cpu_reset)
#ifdef _ARM_ARCH_6
	cpsid	if, #PSR_SVC32_MODE
#else
	mrs     r2, cpsr
	bic	r2, r2, #(PSR_MODE)
	orr     r2, r2, #(PSR_SVC32_MODE)
	orr	r2, r2, #(IF32_bits)
	msr     cpsr_c, r2
#endif

	ldr	r0, .Lcpufuncs
	mov	lr, pc
	ldr	pc, [r0, #CF_IDCACHE_WBINV_ALL]

	/*
	 * Load the virutal address of the MD reset function first.
	 */
	ldr	r4, .Lcpu_reset_address
	ldr	r4, [r4]
	cmp	r4, #0

	/*
	 * If virtual address is NULL, we must be using the physical address
	 */
	ldreq	r4, .Lcpu_reset_address_paddr
	ldreq	r4, [r4]

	/*
 	 * MMU & IDC off, 32 bit program & data space
	 * Hurl ourselves into the ROM
	 */
	mrc	p15, 0, r0, c1, c0, 0
	bic	r0, r0, #(CPU_CONTROL_MMU_ENABLE | CPU_CONTROL_DC_ENABLE)
	bic	r0, r0, #(CPU_CONTROL_IC_ENABLE)
	orr	r0, r0, #(CPU_CONTROL_32BP_ENABLE | CPU_CONTROL_32BD_ENABLE)
	mcr	p15, 0, r0, c1, c0, 0
	mcreq	p15, 0, r2, c8, c7, 0 	/* nail I+D TLB on ARMv4 and greater */
	mov     pc, r4

	/*
	 * cpu_reset_address contains the address to branch to, to complete
	 * the CPU reset after turning the MMU off
	 * This variable is provided by the hardware specific code
	 */
.Lcpu_reset_address:
	.word	_C_LABEL(cpu_reset_address)
.Lcpu_reset_address_paddr:
	.word	_C_LABEL(cpu_reset_address_paddr)
END(cpu_reset)
#endif	/* OFW */

/*
 * setjump + longjmp
 */
ENTRY(setjmp)
	stmia	r0, {r4-r14}
	mov	r0, #0x00000000
	RET
END(setjmp)

ENTRY(longjmp)
	ldmia	r0, {r4-r14}
	mov	r0, #0x00000001
	RET
END(longjmp)

	.data
	.global _C_LABEL(esym)
_C_LABEL(esym):	.word	_C_LABEL(end)

ENTRY_NP(abort)
	b	_C_LABEL(abort)
END(abort)

/*
 * Part of doing a system dump, we need to save a switchframe onto the
 * stack, then save the rest of the registers into the dumppcb.
 */
ENTRY(dumpsys)
	/* push registers onto stack */
	mov	ip, sp
	push	{r4-r7, ip, lr}

	/* fill in dumppcb */
	ldr	r0, .Ldumppcb

#ifndef __XSCALE__
        add     r2, r0, #(PCB_R8)
        stmia   r2, {r8-r13}
#else
        strd    r8, r9, [r0, #(PCB_R8)]
        strd    r10, r11, [r0, #(PCB_R10)]
        strd    r12, r13, [r0, #(PCB_R12)]
#endif

	bl	_C_LABEL(dodumpsys)

	/* unwind the stack */
	pop	{r4-r7, ip, pc}

.Ldumppcb:
	.word	_C_LABEL(dumppcb)
END(dumpsys)

/* End of locore.S */
