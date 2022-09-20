/* $NetBSD: locore.S,v 1.25 2022/09/20 07:18:23 skrll Exp $ */

/*-
 * Copyright (c) 2014, 2022 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry, and by Nick Hudson.
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

#include "opt_console.h"
#include "opt_riscv_debug.h"

#include <machine/asm.h>
#include "assym.h"

	.globl	_C_LABEL(exception_userexit)
	.globl	_C_LABEL(cpu_Debugger_insn)

#if defined(VERBOSE_INIT_RISCV)

#define VPRINTS(string)		\
	call	locore_prints	; \
	.asciz string		; \
	.align 3		; \

#define VPRINTX(regno)		\
	mv	a0, regno	; \
	call	locore_printx

#define VPRINTXNL(regno)	\
	mv	a0, regno	; \
	call	locore_printxnl

/* Need to turn relaxation off for VPRINTS */
	.option norelax

#else
#define VPRINTS(string)		/* nothing */
#define VPRINTX(regno)		/* nothing */
#define VPRINTXNL(regno)	/* nothing */
#endif

#if VM_MIN_KERNEL_ADDRESS != VM_KERNEL_BASE
#error VM_MIN_KERNEL_ADDRESS assumed to match VM_KERNEL_BASE
#endif

/*
 * Entry point where.
 *    a0 is hartid
 *    a1 is pointer to dtb (PA)
 */
ENTRY_NP(start)
	csrw	sie, zero		// disable interrupts
	csrw	sip, zero		// clear any pending

	li	s0, SR_FS
	csrc	sstatus, s0		// disable FP

	/*
	 * atomically swap a non-zero value into hart_boot.  If we see zero
	 * we won in race to become BP.
	 */
	li	s1, 1
	la	s0, hart_boot

	amoswap.w s0, s1, (s0)
	bnez	s0, mpentry
	/*
	 * The BP only executes from here on.
	 */
	mv	s0, a0			// copy hartid
	mv	s1, a1			// copy dtb PA

	/* set the stack pointer for boot */
	PTR_LA	s8, _C_LABEL(bootstk)
	mv	sp, s8

	VPRINTS("\n------------\nNetBSD start\n\n")
	VPRINTS("sp:      ")
	VPRINTXNL(sp)

	VPRINTS("pc:      ")
	auipc	a0, 0
	VPRINTXNL(a0)

	VPRINTS("hart:    ")
	VPRINTXNL(s0)

	VPRINTS("dtb:     ")
	VPRINTXNL(s1)

	/*
	 * Calculate the difference between the VA and PA for start and
	 * keep in s8.  Store this in kern_vtopdiff once the MMU is on.
	 */
	PTR_LA	s11, start
	PTR_L	s8, .Lstart

	sub	s8, s8, s11

	/*
	 * Our load address is not fixed, but our VA is.  We need to construct
	 * an initial PDETAB.
	 */

	li	s10, PAGE_SIZE
	li	s9, USPACE

	PTR_LA	s5, _C_LABEL(lwp0uspace)
	PTR_LA	s6, _C_LABEL(bootstk)

	// The space for the inital page table is included in the kernel
	// .bss size calculation so we know the space exists.

	li	a1, 0
	PTR_LA	s2, _C_LABEL(l1_pte)
	mv	s4, s2			// last page table
#ifdef _LP64
	PTR_LA	s3, _C_LABEL(l2_pte)	// s3 = second PDE page (RV64 only)
	mv	s4, s3			// last page table
#ifdef notyet
	PTR_LA	s4, _C_LABEL(l3_pte)
#endif
#endif
	PTR_LA	s7, _C_LABEL(mmutables_end)


	// s2	L1 PDE (SV32:4MiB megapages, SV{39,48}: 2MiB megapages)
	// s3	L2 PDE (_LP64 SV39 only)
	// s4	L3 PDE (_LP64 SV48 only)
	// s5	lwp0uspace
	// s6	bootstk
	// s7   end of memory to clear

	VPRINTS("l1:      ")
	VPRINTXNL(s2)
#ifdef _LP64
	VPRINTS("l2:      ")
	VPRINTXNL(s3)
#ifdef notyet
	VPRINTS("l3:      ")
	VPRINTXNL(s4)
#endif
#endif

	VPRINTS("uspace:  ")
	VPRINTXNL(s5)
	VPRINTS("bootstk: ")
	VPRINTXNL(s6)

	VPRINTS("vtopdiff:")
	VPRINTXNL(s8)

	VPRINTS("\n\r")

	VPRINTS("bss:     ")
	PTR_LA	a0, _C_LABEL(__bss_start)
	VPRINTX(a0)
	VPRINTS(" - ")
	VPRINTXNL(s7)

	VPRINTS("\n\r")

	// a0	start of memory to clear
	// a1	end of memory to clear
	PTR_LA	a0, _C_LABEL(__bss_start)
	mv	a1, s7

	call	clear_bss		// zero through kernel_end (inc. stack)

	li	s7, PTE_KERN		// for megapages

	// We allocated the kernel first PDE page so let's insert in the
	// page table.

	// Need to setup tables so that for
	// sv32 : s2
	// sv39 : s3 -> s2
	// sv48 : s4 -> s3 -> s2

#ifdef _LP64
	srli	t0, s2, (PGSHIFT - PTE_PPN_SHIFT)
	or	t0, t0, s7		// Assumes s2[11:0] == 0
#if ((VM_MIN_KERNEL_ADDRESS >> XSEGSHIFT) & (NPDEPG - 1)) * SZREG
	li	t1, ((VM_MIN_KERNEL_ADDRESS >> XSEGSHIFT) & (NPDEPG - 1)) * SZREG
#ifdef notyet
	add	t1, t1, s4
#else
	add	t1, t1, s3
#endif
	REG_S	t0, 0(t1)

	VPRINTX(t1)
#else
#ifdef notyet
	REG_S	t0, 0(s4)
#else
	REG_S	t0, 0(s3)
#endif

	VPRINTX(s3)
#endif
#endif

	VPRINTS(": ")
	VPRINTXNL(t0)
	VPRINTS("\n\r")

#if PGSHIFT < PTE_PPN_SHIFT
#error Code assumes PGSHIFT is greater than PTE_PPN_SHIFT
#endif

	li	s5, (VM_KERNEL_SIZE >> SEGSHIFT)		// # of megapages
	li	s6, (NBSEG >> (PGSHIFT - PTE_PPN_SHIFT))	// load for ease
	li	s7, PTE_KERN | PTE_R | PTE_W | PTE_X

	//
	// Fill in the PDEs for kernel.
	//
	PTR_LA	s0, start
	srli	s0, s0, (PGSHIFT - PTE_PPN_SHIFT)
	or	s0, s0, s7
.Lfill:
	VPRINTS("kern ")
	VPRINTX(s2)
	VPRINTS(": ")
	VPRINTXNL(s0)

	REG_S	s0, 0(s2)		// store PDE
	add	s0, s0, s6		// advance PA in PDE to next segment
	add	s2, s2, SZREG		// advance to next PDE slot
	addi	s5, s5, -1		// count down segment
	bnez	s5, .Lfill		// loop if more

	li	s7, PTE_KERN | PTE_R | PTE_W

	// DTB physical address
	mv	s0, s1
	srli	s0, s0, SEGSHIFT	// round down to NBSEG, and shift in
	slli	s0, s0, (SEGSHIFT - PGSHIFT + PTE_PPN_SHIFT)	// ... to PPN
	or	s0, s0, s7

	VPRINTS("dtb  ")
	VPRINTX(s2)
	VPRINTS(": ")
	VPRINTXNL(s0)

	REG_S	s0, 0(s2)
	add	s2, s2, SZREG		// advance to next PDE slot

#ifdef CONSADDR
	ld	s0, .Lconsaddr
	srli	s0, s0, SEGSHIFT	// round down to NBSEG, and shift in
	slli	s0, s0, (SEGSHIFT - PGSHIFT + PTE_PPN_SHIFT)	// ... to PPN
	or	s0, s0, s7

	VPRINTS("cons ")
	VPRINTX(s2)
	VPRINTS(": ")
	VPRINTXNL(s0)

	REG_S	s0, 0(s2)
	add	s2, s2, SZREG		// advance to next PDE slot
#endif

	li	a0, 'P'
	call	_C_LABEL(uartputc)

	/* Set supervisor trap vector base register */
	PTR_LA	t0, .Lmmu_on
	add	t0, t0, s8
	csrw	stvec, t0

	/* Set supervisor address translation and protection register */
	srli	t1, s4, PGSHIFT
#ifdef _LP64
	li	t0, SATP_MODE_SV39
#else
	li	t0, SATP_MODE_SV32
#endif
	or	t0, t0, t1
	sfence.vma
	csrw	satp, t0

	.align 2
.Lmmu_on:
	// MMU is on!
	csrw	sscratch, zero		// zero in sscratch to mark kernel

	li	a0, 'M'
	call	_C_LABEL(uartputc)	// uartputs doesn't use stack
	li	a0, '\n'
	call	_C_LABEL(uartputc)	// uartputs doesn't use stack
	li	a0, '\r'
	call	_C_LABEL(uartputc)	// uartputs doesn't use stack

	PTR_LA	tp, _C_LABEL(lwp0)	// put curlwp in tp

	.global vstart
vstart:

	/* Set supervisor trap vector base register */
	PTR_LA	a0, _C_LABEL(cpu_exception_handler)
	csrw	stvec, a0

	PTR_LA	s2, bootstk		// top of lwp0uspace
	PTR_S	s2, L_PCB(tp)		// set uarea of lwp (already zeroed)
	addi	sp, s2, -TF_LEN		// switch to new stack
	PTR_S	sp, L_MD_UTF(tp)	// store pointer to empty trapframe

	PTR_LA	t1, _C_LABEL(kernel_pmap_store)
	add	t2, s4, s8 		// PA -> VA
	srli	t3, s4, PGSHIFT
	PTR_S	t2, PM_MD_PDETAB(t1)	// VA of kernel PDETAB
	PTR_S	t3, PM_MD_PPN(t1)	// PPN of kernel PDETAB

	/*
	 * Store kern_vtopdiff (the difference between the physical
	 * and virtual address of the "start" symbol).
	 */
	PTR_LA	s11, _C_LABEL(kern_vtopdiff)
	PTR_S	s8, 0(s11)	/* kern_vtopdiff = start(virt) - start(phys) */

#if notyet
	mv	a0, s1			// dtb
	call	_C_LABEL(init_mmu)
#endif

	li	t0, VM_MIN_KERNEL_ADDRESS + VM_KERNEL_SIZE
	li	t1, NBSEG - 1
	and	t1, s1, t1
	or	t0, t0, t1

	/* Set the global pointer */
	.option push
	.option norelax
	lla	gp, __global_pointer$
	.option pop

	// Now we should ready to start initializing the kernel.
	mv	a0, s0			// hartid
	mv	a1, t0			// vdtb
	//mv	a1, s1			// dtb (physical)
	call	_C_LABEL(init_riscv)	// do MD startup
	tail	_C_LABEL(main)		// and transfer to main
	/* No return from main */
END(start)


ENTRY(mpentry)
1:
	wfi
	j	1b
END(mpentry)


	.align 3
.Lstart:
#ifdef _LP64
	.quad	start
#else
	.word	start
#endif


#ifdef CONSADDR
	.align 3
.Lconsaddr:
#ifdef _LP64
	.quad	CONSADDR
#else
	.word	CONSADDR
#endif
#endif


ENTRY_NP(uartputc)
#ifdef EARLYCONS
	tail	___CONCAT(EARLYCONS, _platform_early_putchar)
#else
#define SBI_LEGACY_CONSOLE_PUTCHAR        1
	li	a7, SBI_LEGACY_CONSOLE_PUTCHAR
	ecall
	ret
#endif
END(uartputc)


ENTRY_NP(clear_bss)
	bgeu	a0, a1, 1f
2:
	sb	zero, 0(a0)
	addi	a0, a0, 1
	bne	a1, a0, 2b
1:
	ret
END(clear_bss)


#if defined(VERBOSE_INIT_RISCV)
ENTRY_NP(locore_prints)
	addi	sp, sp, -(SZREG * 2)
	REG_S	s0, (0 * SZREG)(sp)
	mv	s0, ra
1:
	lbu	a0, 0(s0)
	beqz	a0, 2f

	call	uartputc

	addi	s0, s0, 1
	j	1b
2:
	addi	s0, s0, 8	// s0 points to the null terminator
	andi	ra, s0, -8

	REG_L	s0, (0 * SZREG)(sp)
	addi	sp, sp, (SZREG * 2)
	ret

END(locore_prints)


ENTRY_NP(locore_printx)
	addi	sp, sp, -(SZREG * 4)
	REG_S	s0, (0 * SZREG)(sp)
	REG_S	s1, (1 * SZREG)(sp)
	REG_S	s2, (2 * SZREG)(sp)
	REG_S	ra, (3 * SZREG)(sp)

	mv	s1, a0		// our print value
	li	s2, 10

	li	a0, '0'
	call	uartputc
	li	a0, 'x'
	call	uartputc

	// Word size in bits
	li	s0, (SZREG * 8)
1:
	addi	s0, s0, -4	// nibble shift

	srl	a0, s1, s0	// extract ...
	andi	a0, a0, 0xf

	bltu	a0, s2, 2f
	addi	a0, a0, ('a' - '0' - 10)
2:	addi	a0, a0, '0'

	call	uartputc

	beqz	s0, 3f

	and	a0, s0, (16 - 1)
	bnez	a0, 1b

	li	a0, '_'
	call	uartputc

	j	1b

3:
	REG_L	s0, (0 * SZREG)(sp)
	REG_L	s1, (1 * SZREG)(sp)
	REG_L	s2, (2 * SZREG)(sp)
	REG_L	ra, (3 * SZREG)(sp)
	addi	sp, sp, (SZREG * 4)
	ret
END(locore_printx)


ENTRY_NP(locore_printxnl)
	addi	sp, sp, -(SZREG * 2)
	REG_S	ra, (1 * SZREG)(sp)

	call	locore_printx
	li	a0, '\n'
	call	uartputc

	li	a0, '\r'
	call	uartputc

	REG_L	ra, (1 * SZREG)(sp)
	addi	sp, sp, (SZREG * 2)

	ret
END(locore_printxnl)
#endif	/* VERBOSE_INIT_RISCV */


	.data
	.align	2
hart_boot:
	.word	0

	.section "_init_memory", "aw", %nobits
	.align PGSHIFT
	.global _C_LABEL(lwp0uspace)
_C_LABEL(lwp0uspace):
	.space	UPAGES * PAGE_SIZE
bootstk:

	/*
	 * Allocate some memory after the kernel image for stacks and
	 * bootstrap L1PT
	 */
	.align PGSHIFT

	.section "_init_memory", "aw", %nobits
	.align PGSHIFT
mmutables_start:
	.global _C_LABEL(l1_pte)
l1_pte:
	.space PAGE_SIZE
#ifdef _LP64
	.global _C_LABEL(l2_pte)
l2_pte:
	.space PAGE_SIZE
#ifdef notyet
l3_pte:
	.space PAGE_SIZE
#endif
#endif
mmutables_end:


//
// struct lwp *cpu_switchto(struct lwp *oldl, struct lwp *newl, bool returning);
//
ENTRY_NP(cpu_switchto)
	addi	sp, sp, -TF_LEN		// allocate trapframe

	REG_S	ra, TF_RA(sp)		// save return address
	REG_S	s0, TF_S0(sp)		// save callee saved address
	REG_S	s1, TF_S1(sp)		// save callee saved address
	REG_S	s2, TF_S2(sp)		// save callee saved address
	REG_S	s3, TF_S3(sp)		// save callee saved address
	REG_S	s4, TF_S4(sp)		// save callee saved address
	REG_S	s5, TF_S5(sp)		// save callee saved address
	REG_S	s6, TF_S6(sp)		// save callee saved address
	REG_S	s7, TF_S7(sp)		// save callee saved address
	REG_S	s8, TF_S8(sp)		// save callee saved address
	REG_S	s9, TF_S9(sp)		// save callee saved address
	REG_S	s10, TF_S10(sp)		// save callee saved address
	REG_S	s11, TF_S11(sp)		// save callee saved address
	csrr	t4, sstatus		// get status for intr state
	REG_S	t4, TF_SR(sp)		// save it

	REG_S	sp, L_MD_KTF(a0)	// record trapframe pointer

	csrrci	t0, sstatus, SR_SIE	// # disable interrupts

	mv	tp, a1			// # put the new lwp in thread pointer

	PTR_L	t1, L_CPU(tp)		// # get curcpu
	PTR_S	tp, CI_CURLWP(t1)	// # update curcpu with the new curlwp

	REG_L	sp, L_MD_KTF(tp)	// # load its kernel stack pointer
	REG_L	t4, TF_SR(sp)		// # fetch status register
	csrw	sstatus, t4		// # restore it (and interrupts?)

	REG_L	s0, TF_S0(sp)		// restore callee saved
	REG_L	s1, TF_S1(sp)		// restore callee saved
	REG_L	s2, TF_S2(sp)		// restore callee saved
	REG_L	s3, TF_S3(sp)		// restore callee saved
	REG_L	s4, TF_S4(sp)		// restore callee saved
	REG_L	s5, TF_S5(sp)		// restore callee saved
	REG_L	s6, TF_S6(sp)		// restore callee saved
	REG_L	s7, TF_S7(sp)		// restore callee saved
	REG_L	s8, TF_S8(sp)		// restore callee saved
	REG_L	s9, TF_S9(sp)		// restore callee saved
	REG_L	s10, TF_S10(sp)		// restore callee saved
	REG_L	s11, TF_S11(sp)		// restore callee saved

	REG_L	ra, TF_RA(sp)		// restore return address

	addi	sp, sp, TF_LEN		// remove trapframe

	//	a0 = oldl
	//	a1 = curcpu()
	//	tp = newl

	ret
END(cpu_switchto)

ENTRY_NP(cpu_lwp_trampoline)
	mv	a1, tp			// get new lwp
	call	_C_LABEL(lwp_startup)	// call lwp startup

	mv	a0, s1			// get saved arg
	jalr	s0			// call saved func

	// If the saved func returns, we are returning to user land.
	j	_C_LABEL(exception_userexit)
END(cpu_lwp_trampoline)

ENTRY_NP(cpu_fast_switchto_cleanup)
	INT_L	t0, CI_MTX_COUNT(a1)	// get mutex count
	REG_L	ra, CALLFRAME_RA(sp)	// get return address
	REG_L	a0, CALLFRAME_S0(sp)	// get pinned LWP
	addi	t0, t0, 1		// increment mutex count
	INT_S	t0, CI_MTX_COUNT(a1)	// save it
	addi	sp, sp, CALLFRAME_SIZ	// remove callframe
#if IPL_SCHED != IPL_HIGH
	tail	_C_LABEL(splhigh)	// go back to IPL HIGH
#else
	ret				// just return
#endif
END(cpu_fast_switchto_cleanup)

//
// void cpu_fast_switchto(struct lwp *, int s);
//
ENTRY_NP(cpu_fast_switchto)
	addi	sp, sp, -(TF_LEN + CALLFRAME_SIZ)
	REG_S	a0, (TF_LEN + CALLFRAME_S0)(sp)
	REG_S	ra, (TF_LEN + CALLFRAME_RA)(sp)

	PTR_LA	t2, _C_LABEL(cpu_fast_switchto_cleanup)

	REG_S	t2, TF_RA(sp)		// return to someplace else
	REG_S	s0, TF_S0(sp)		// save callee saved register
	REG_S	s1, TF_S1(sp)		// save callee saved register
	REG_S	s2, TF_S2(sp)		// save callee saved register
	REG_S	s3, TF_S3(sp)		// save callee saved register
	REG_S	s4, TF_S4(sp)		// save callee saved register
	REG_S	s5, TF_S5(sp)		// save callee saved register
	REG_S	s6, TF_S6(sp)		// save callee saved register
	REG_S	s7, TF_S7(sp)		// save callee saved register
	REG_S	s8, TF_S8(sp)		// save callee saved register
	REG_S	s9, TF_S9(sp)		// save callee saved register
	REG_S	s10, TF_S10(sp)		// save callee saved register
	REG_S	s11, TF_S11(sp)		// save callee saved register
	csrr	t4, sstatus		// get status register (for intr state)
	REG_S	t4, TF_SR(sp)		// save it

	mv	s0, tp			// remember curlwp
	mv	s1, sp			// remember kernel stack

	csrrci	t0, sstatus, SR_SIE	// disable interrupts
	PTR_L	t1, L_CPU(tp)		// get curcpu()

	PTR_S	sp, L_MD_KTF(tp)	// save trapframe ptr in oldlwp
	mv	tp, a0			// set thread pointer to newlwp
	PTR_S	tp, CI_CURLWP(t1)	// update curlwp
	PTR_L	sp, L_MD_KTF(tp)	// switch to its stack
	csrw	sstatus, t0		// reenable interrupts
	call	_C_LABEL(softint_dispatch)
	csrrci	t0, sstatus, SR_SIE	// disable interrupts
	PTR_L	t1, L_CPU(tp)		// get curcpu() again
	mv	tp, s0			// return to pinned lwp
	PTR_S	tp, CI_CURLWP(t1)	// restore curlwp
	csrw	sstatus, t0		// reenable interrupts
	mv	sp, s1			// restore stack pointer

	REG_L	ra, (TF_RA + CALLFRAME_RA)(sp)	// get return address
	REG_L	s0, TF_S0(sp)		// restore register we used
	REG_L	s1, TF_S1(sp)		// restore register we used

	addi	sp, sp, TF_LEN+CALLFRAME_SIZ	// drop trapframe/callframe
	ret				// return
END(cpu_fast_switchto)

// RISCV only has a simple exception handler handles both synchronous traps
// and interrupts.
ENTRY_NP(cpu_exception_handler)
	csrrw	tp, sscratch, tp	// swap scratch and thread pointer
	beqz	tp, .Lexception_kernel	//   tp == 0, already on kernel stack
	//
	// The exception happened while user code was executing.  We need to
	// get the pointer to the user trapframe from the LWP md area.  Then we
	// save t1 and tp so we have a register to work with and to get curlwp
	// into tp.  We also save the saved SP into the trapframe.
	// Upon entry on an exception from user, sscratch will contain curlwp.
	//
	REG_S	sp, L_MD_USP(tp)	// save user stack pointer temporarily
	PTR_L	sp, L_MD_UTF(sp)	// trapframe pointer loaded
	REG_S	t1, TF_T1(sp)		// save t1
	REG_L	t1, L_MD_USP(tp)	// get user stack pointer
	REG_S	t1, TF_SP(sp)		// save thread pointer in trapframe
	csrrw	t1, sscratch, zero	// swap saved thread pointer with 0
	REG_L	t1, TF_TP(sp)		// save thread pointer in trapframe
	li	t1, 0			// indicate user exception
	j	.Lexception_common

	//
	// The exception happened while we were already in the kernel.  That
	// means tp already has curlwp and sp has the kernel stack pointer so
	// just need to restore it and then adjust it down for space for the
	// trap frame.  We save t1 so we can use it the original sp into the
	// trapframe for use by the exception exiting code.
	//
.Lexception_kernel:
	csrrw	tp, sscratch, zero	// get back our thread pointer
	addi	sp, sp, -TF_LEN		// allocate stack frame
	REG_S	t1, TF_T1(sp)		// save t1
	addi	t1, sp, TF_LEN
	REG_S	t1, TF_SP(sp)		// save SP
	li	t1, 1			// indicate kernel exception

.Lexception_common:
	// Now we save all the temporary registers into the trapframe since
	// they will most certainly be changed.
	REG_S	ra, TF_RA(sp)		// save return address
	REG_S	gp, TF_GP(sp)		// save gp
	REG_S	a0, TF_A0(sp)		// save a0
	REG_S	a1, TF_A1(sp)		// save a1
	REG_S	a2, TF_A2(sp)		// save a2
	REG_S	a3, TF_A3(sp)		// save a3
	REG_S	a4, TF_A4(sp)		// save a4
	REG_S	a5, TF_A5(sp)		// save a5
	REG_S	a6, TF_A6(sp)		// save a6
	REG_S	a7, TF_A7(sp)		// save a7
	REG_S	t0, TF_T0(sp)		// save t0
					// t1 is already saved
	REG_S	t2, TF_T2(sp)		// save t2
	REG_S	t3, TF_T3(sp)		// save t3
	REG_S	t4, TF_T4(sp)		// save t4
	REG_S	t5, TF_T5(sp)		// save t5
	REG_S	t6, TF_T6(sp)		// save t6

	// Now we get the
	mv	a0, sp			// trapframe pointer
	csrr	a1, sepc		// get exception pc
	csrr	a2, sstatus		// get status
	csrr	a3, scause		// get cause

	REG_S	a1, TF_PC(sp)
	INT_S	a2, TF_SR(sp)
	INT_S	a3, TF_CAUSE(sp)	// save cause

	// Now we've saved the trapfame, the cause is still in a3.

	bltz	a3, intr_handler	// MSB is set if interrupt

	// stval is only relevant for non-interrupts
	csrr	a4, stval		// get stval
	REG_S	a4, TF_TVAL(sp)

	beqz	t1, trap_user		// this was a user trap
	// This was a kernel exception
	call	_C_LABEL(cpu_trap)	// just call trap to handle it
exception_kernexit:
	// If we got here, we are returning from a kernel exception (either a
	// trap or interrupt).  Simply return the volatile registers and the
	// exception PC and status, load the saved SP from the trapframe, and
	// return from the exception
	csrrci	zero, sstatus, SR_SIE	// disable interrupts

	REG_L	ra, TF_RA(sp)		// restore return address
	REG_L	gp, TF_GP(sp)		// restore gp
	REG_L	a0, TF_A0(sp)		// restore a0
	REG_L	a1, TF_A1(sp)		// restore a1
	REG_L	a2, TF_A2(sp)		// restore a2
	REG_L	a3, TF_A3(sp)		// restore a3
	REG_L	a4, TF_A4(sp)		// restore a4
	REG_L	a5, TF_A5(sp)		// restore a5
	REG_L	a6, TF_A6(sp)		// restore a6
	REG_L	a7, TF_A7(sp)		// restore a7
	REG_L	t2, TF_T2(sp)		// restore t2
	REG_L	t3, TF_T3(sp)		// restore t3
	REG_L	t4, TF_T4(sp)		// restore t4
	REG_L	t5, TF_T3(sp)		// restore t5
	REG_L	t6, TF_T4(sp)		// restore t6

	REG_L	t0, TF_PC(sp)		// fetch exception PC
	REG_L	t1, TF_SR(sp)		// fetch status

	csrw	sepc, t0		// restore exception PC
	csrw	sstatus, t1		// restore status

	REG_L	t0, TF_T0(sp)		// restore t0
	REG_L	t1, TF_T1(sp)		// restore t1
	REG_L	sp, TF_SP(sp)		// restore SP
	sret				// and we're done

trap_user:
	REG_S	s0, TF_S0(sp)		// only save from userland
	REG_S	s1, TF_S1(sp)		// only save from userland
	REG_S	s2, TF_S2(sp)		// only save from userland
	REG_S	s3, TF_S3(sp)		// only save from userland
	REG_S	s4, TF_S4(sp)		// only save from userland
	REG_S	s5, TF_S5(sp)		// only save from userland
	REG_S	s6, TF_S6(sp)		// only save from userland
	REG_S	s7, TF_S7(sp)		// only save from userland
	REG_S	s8, TF_S8(sp)		// only save from userland
	REG_S	s9, TF_S9(sp)		// only save from userland
	REG_S	s10, TF_S10(sp)		// only save from userland
	REG_S	s11, TF_S11(sp)		// only save from userland

	csrsi	sstatus, SR_SIE		// reenable interrupts

	li	t0, CAUSE_SYSCALL	// let's see if this was a syscall
	beq	a3, t0, trap_syscall	//   yes it was

	call	_C_LABEL(cpu_trap)	// nope, just a regular trap
_C_LABEL(exception_userexit):
	INT_L	t0, L_MD_ASTPENDING(tp)	// ast pending?
	bnez	t0, trap_doast		//   yes, handle it.
	csrrci	zero, sstatus, SR_SIE	// disable interrupts
	csrw	sscratch, tp		// show we are coming from userland
	REG_L	tp, TF_TP(sp)		// only restore from userland
	REG_L	s0, TF_S0(sp)		// only restore from userland
	REG_L	s1, TF_S1(sp)		// only restore from userland
	REG_L	s2, TF_S2(sp)		// only restore from userland
	REG_L	s3, TF_S3(sp)		// only restore from userland
	REG_L	s4, TF_S4(sp)		// only restore from userland
	REG_L	s5, TF_S5(sp)		// only restore from userland
	REG_L	s6, TF_S6(sp)		// only restore from userland
	REG_L	s7, TF_S7(sp)		// only restore from userland
	REG_L	s8, TF_S8(sp)		// only restore from userland
	REG_L	s9, TF_S9(sp)		// only restore from userland
	REG_L	s10, TF_S10(sp)		// only restore from userland
	REG_L	s11, TF_S11(sp)		// only restore from userland
	j	exception_kernexit

trap_syscall:
	PTR_LA	ra, exception_userexit
	PTR_L	t0, L_PROC(tp)		// get proc struct
	PTR_L	t0, P_MD_SYSCALL(t0)	// get syscall address from proc
	jr	t0			// and jump to it

intr_usersave:
	REG_S	s0, TF_S0(sp)		// only save from userland
	REG_S	s1, TF_S1(sp)		// only save from userland
	REG_S	s2, TF_S2(sp)		// only save from userland
	REG_S	s3, TF_S3(sp)		// only save from userland
	REG_S	s4, TF_S4(sp)		// only save from userland
	REG_S	s5, TF_S5(sp)		// only save from userland
	REG_S	s6, TF_S6(sp)		// only save from userland
	REG_S	s7, TF_S7(sp)		// only save from userland
	REG_S	s8, TF_S8(sp)		// only save from userland
	REG_S	s9, TF_S9(sp)		// only save from userland
	REG_S	s10, TF_S10(sp)		// only save from userland
	REG_S	s11, TF_S11(sp)		// only save from userland
	PTR_LA	ra, exception_userexit
trap_doast:
	mv	a0, sp			// only argument is trapframe
	tail	_C_LABEL(cpu_ast)

intr_user:
	call	_C_LABEL(cpu_intr)	// handle interrupt
	INT_L	t0, L_MD_ASTPENDING(tp)	// get astpending
	bnez	t0, intr_usersave	//    if one is pending, deal with in

	csrw	sscratch, tp		// show we are coming from userland
	REG_L	tp, TF_TP(sp)		// restore thread pointer
	j	exception_kernexit	// do standard exception exit

intr_handler:
	beqz	t1, intr_user
	call	_C_LABEL(cpu_intr)
	j	exception_kernexit
END(cpu_exception_handler)

// int cpu_set_onfault(struct faultbuf *fb, register_t retval)
//
ENTRY_NP(cpu_set_onfault)
	REG_S	ra, FB_RA(a0)
	REG_S	s0, FB_S0(a0)
	REG_S	s1, FB_S1(a0)
	REG_S	s2, FB_S2(a0)
	REG_S	s3, FB_S3(a0)
	REG_S	s4, FB_S4(a0)
	REG_S	s5, FB_S5(a0)
	REG_S	s6, FB_S6(a0)
	REG_S	s7, FB_S7(a0)
	REG_S	s8, FB_S8(a0)
	REG_S	s9, FB_S9(a0)
	REG_S	s10, FB_S10(a0)
	REG_S	s11, FB_S11(a0)
	REG_S	sp, FB_SP(a0)
	REG_S	a1, FB_A0(a0)
	PTR_S	a0, L_MD_ONFAULT(tp)
	li	a0, 0
	ret
END(cpu_set_onfault)

ENTRY_NP(setjmp)
	REG_S	ra, FB_RA(a0)
	REG_S	s0, FB_S0(a0)
	REG_S	s1, FB_S1(a0)
	REG_S	s2, FB_S2(a0)
	REG_S	s3, FB_S3(a0)
	REG_S	s4, FB_S4(a0)
	REG_S	s5, FB_S5(a0)
	REG_S	s6, FB_S6(a0)
	REG_S	s7, FB_S7(a0)
	REG_S	s8, FB_S8(a0)
	REG_S	s9, FB_S9(a0)
	REG_S	s10, FB_S10(a0)
	REG_S	s11, FB_S11(a0)
	REG_S	sp, FB_SP(a0)
	li	a0, 0
	ret
END(setjmp)

ENTRY_NP(longjmp)
	REG_L	ra, FB_RA(a0)
	REG_L	s0, FB_S0(a0)
	REG_L	s1, FB_S1(a0)
	REG_L	s2, FB_S2(a0)
	REG_L	s3, FB_S3(a0)
	REG_L	s4, FB_S4(a0)
	REG_L	s5, FB_S5(a0)
	REG_L	s6, FB_S6(a0)
	REG_L	s7, FB_S7(a0)
	REG_L	s8, FB_S8(a0)
	REG_L	s9, FB_S9(a0)
	REG_L	s10, FB_S10(a0)
	REG_L	s11, FB_S11(a0)
	REG_L	sp, FB_SP(a0)
	mv	a0, a1
	ret
END(longjmp)

ENTRY_NP(cpu_Debugger)
cpu_Debugger_insn:
	sbreak
	ret
END(cpu_Debugger)
