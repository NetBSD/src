/*	$NetBSD: locore.S,v 1.13.2.1 2014/08/20 00:03:02 tls Exp $	*/

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

#include "opt_cputypes.h"
#include "assym.h"
#include <sys/syscall.h>
#include <sys/errno.h>
#include <machine/asm.h>
#include <machine/frame.h>
#if defined(CPU_XSCALE_PXA250) || defined(CPU_XSCALE_PXA270)
#include <arm/armreg.h>
#endif


/* What size should this really be ? It is only used by init_arm() */
#define INIT_ARM_STACK_SIZE	2048

/*
 * This is for kvm_mkdb, and should be the address of the beginning
 * of the kernel text segment (not necessarily the same as kernbase).
 */

ENTRY_NP(kernel_text)

ASENTRY_NP(start)
	/* Put the processer in SVC mode */
	mov	r5, sp
	mrs	r4, cpsr
	bic	r4, r4, #(PSR_MODE)
	orr	r4, r4, #(PSR_SVC32_MODE)
	msr	cpsr_all, r4
	mov	sp, r5

	/* Disable PID virtual address mapping */
	mov	r4, #0
	mcr	p15, 0, r4, c13, c0, 0

#if defined(CPU_XSCALE_PXA250) || defined(CPU_XSCALE_PXA270)
/*
 * CPWAIT -- Canonical method to wait for CP15 update.
 * NOTE: Clobbers the specified temp reg.
 * copied from arm/arm/cpufunc_asm_xscale.S
 * XXX: better be in a common header file.
 */
#define	CPWAIT_BRANCH							 \
	sub	pc, pc, #4

#define	CPWAIT(tmp)							 \
	mrc	p15, 0, tmp, c2, c0, 0	/* arbitrary read of CP15 */	;\
	mov	tmp, tmp		/* wait for it to complete */	;\
	CPWAIT_BRANCH			/* branch to next insn */

	/*
	 *  Kernel is loaded in SDRAM (0xa0200000..), and is expected to run
	 *  in VA 0xc0200000..  
	 */

	/* build page table from scratch */
	adr	r9, .Lstartup_pagetable
	ldr	r9, [r9]
	adr	r4, .Lmmu_init_table
	b	3f

2:
	str	r8, [r9, r7]
	add	r7, r7, #4
	add	r8, r8, #(L1_S_SIZE)
	subs	r6, r6, #1
	bne	2b
3:
	ldmia	r4!, {r6,r7,r8}		/* # of sections, VA, PA|attr */
	cmp	r6, #0
	bne	2b

	/* Set translation table */
	mcr	p15, 0, r9, c2, c0, 0
	mcr	p15, 0, r9, c8, c7, 0	/* flush I/D-TLBs */
	CPWAIT(r9)

	/* Set the Domain Access register.  Very important! */
	mov	r9, #((DOMAIN_CLIENT << (PMAP_DOMAIN_KERNEL*2)) | DOMAIN_CLIENT)
	mcr	p15, 0, r9, c3, c0, 0

	/* Enable MMU */
	mrc	p15, 0, r9, c1, c0, 0
	orr	r9, r9, #CPU_CONTROL_SYST_ENABLE
	orr	r9, r9, #CPU_CONTROL_MMU_ENABLE
	mcr	p15, 0, r9, c1, c0, 0
	CPWAIT(r9)

	/* Jump to kernel code in TRUE VA */
	ldr	r9, .Lhpc_start
	mov	pc, r9

.Lhpc_start:
	.word	hpc_start

#define MMU_INIT(va,pa,n_sec,attr)					\
	.word	(n_sec);						\
	.word	4 * ((va) >> L1_S_SHIFT);				\
	.word	(pa)|(attr);

.Lstartup_pagetable:
	.word	0xa0004000

.Lmmu_init_table:	
	/* fill all table VA==PA */
	MMU_INIT(0x00000000, 0x00000000, 1<<(32-L1_S_SHIFT), L1_TYPE_S|L1_S_AP_KRW)
	/* map SDRAM VA==PA, WT cacheable */
	MMU_INIT(0xa0000000, 0xa0000000, 256, L1_TYPE_S|L1_S_C|L1_S_AP_KRW)
	/* map VA 0xc0000000..0xcfffffff to PA 0xa0000000..0xafffffff */
	MMU_INIT(0xc0000000, 0xa0000000, 256, L1_TYPE_S|L1_S_C|L1_S_AP_KRW)
	.word	0,0,0	/* end of table */

	.text
	.align	0
ASENTRY_NP(hpc_start)
#endif	/* CPU_XSCALE_PXA250 | CPU_XSCALE_PXA270 */

	adr	r5, .Lstart
	ldmia	r5, {r3, r4, sp}	/* Set initial stack and */
	sub	r4, r4, r3		/* get zero init data */

	mov	r5, #0
.L1:
	str	r5, [r3], #4		/* Zero the bss */
	subs	r4, r4, #4
	bgt	.L1

	mov	fp, #0x00000000		/* trace back starts here */
	bl	_C_LABEL(initarm)	/* Off we go */

	/* init arm will return the new stack pointer. */
	mov	sp, r0

	mov	fp, #0x00000000		/* trace back starts here */
	mov	ip, sp
	stmfd	sp!, {fp, ip, lr, pc}
	sub	fp, ip, #4

	/* Setup an initial trap frame for start_init to use */
	PUSHFRAME
	mov	r0, sp			/* parameter to main is trap frame */ 
	bl	_C_LABEL(main)		/* Lets light the flame and start her up */
	PULLFRAME			/* Pull the trap frame, now valid */
	movs	pc, lr			/* Exit to user process */
	/* Never gets here */
	b	.

.Lstart:
	.word	_edata
	.word	_end
	.word	svcstk + INIT_ARM_STACK_SIZE

	.bss
svcstk:
	.space	INIT_ARM_STACK_SIZE

	.text
	.align	0

.Lcpufuncs:	
	.word	_C_LABEL(cpufuncs)

ENTRY_NP(cpu_reset)
	mrs     r2, cpsr
	bic	r2, r2, #(PSR_MODE)
	orr     r2, r2, #(PSR_SVC32_MODE)
	orr	r2, r2, #(I32_bit | F32_bit)
	msr     cpsr_all, r2

	ldr	r4, .Lcpu_reset_address
	ldr	r4, [r4]

	ldr	r0, .Lcpufuncs
	mov	lr, pc
	ldr	pc, [r0, #CF_IDCACHE_WBINV_ALL]

	/*
 	 * MMU & IDC off, 32 bit program & data space
	 * Hurl ourselves into the ROM
	 */
	mov	r0, #(CPU_CONTROL_32BP_ENABLE | CPU_CONTROL_32BD_ENABLE)
	mcr     p15, 0, r0, c1, c0, 0
	mcr     p15, 0, r0, c8, c7, 0
	mov     pc, r4

	/*
	 * _cpu_reset_address contains the address to branch to, to complete
	 * the CPU reset after turning the MMU off
	 * This variable is provided by the hardware specific code
	 */

.Lcpu_reset_address:
	.word	_C_LABEL(cpu_reset_address)

/*
 * setjump + longjmp
 */
ENTRY(setjmp)
	stmia	r0, {r4-r14}
	mov	r0, #0x00000000
	mov	pc, lr

ENTRY(longjmp)
	ldmia	r0, {r4-r14}
	mov	r0, #0x00000001
	mov	pc, lr

	.data
	.global _C_LABEL(esym)
_C_LABEL(esym):	.word	_C_LABEL(end)

ENTRY_NP(abort)
	b	_C_LABEL(abort)

/*
 * part of doing a system dump, we need to save registers and cpsr onto the
 * stack, then save the rest of the registers into the dumppcb
 */
ENTRY(dumpsys)
	/* push registers onto stack */
	stmfd	sp!, {r0-r7, lr}
	
	/* push the status bits onto the stack */
	mrs	r0, cpsr
	stmfd	sp!, {r0}

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
	ldmfd	sp!, {r0}
	nop
	ldmfd	sp!, {r0-r7, pc}

.Ldumppcb:
	.word	_C_LABEL(dumppcb)

/* End of locore.S */
