/* $NetBSD: locore.S,v 1.42 2023/06/12 19:04:14 skrll Exp $ */

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
#include "opt_multiprocessor.h"
#include "opt_riscv_debug.h"

#include <machine/asm.h>
#include "assym.h"

#define BOOT_AP_STACKSIZE	1024	/* size of temporary stack for APs */
#define NBBY_SHIFT		3	/* log2(8 bits per byte) */

#define PRINTS(string)		\
	call	locore_prints	; \
	.asciz string		; \
	.align 3		; \


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

	mv	s10, a0			// copy hartid
	mv	s11, a1			// copy dtb PA

	/* set the stack pointer for boot */
	PTR_LA	t0, _C_LABEL(bootstk)
	mv	sp, t0

	VPRINTS("\n------------\nNetBSD start\n\n")
	VPRINTS("sp:      ")
	VPRINTXNL(sp)

	VPRINTS("pc:      ")
	auipc	a0, 0
	VPRINTXNL(a0)

	VPRINTS("hart:    ")
	VPRINTXNL(s10)

	VPRINTS("dtb:     ")
	VPRINTXNL(s11)

	/*
	 * Calculate the difference between the VA and PA for start and
	 * keep in s8.  Store this in kern_vtopdiff once the MMU is on.
	 */
	PTR_LA	t0, start
	PTR_L	s8, .Lstart

	sub	s8, s8, t0

	PTR_LA	s5, _C_LABEL(lwp0uspace)
	PTR_LA	s6, _C_LABEL(bootstk)

	/*
	 * Our load address is not fixed, but our VA is.  We need to construct
	 * an initial PDETAB.
	 *
	 * The space for the inital page table is included in the kernel
	 * .bss size calculation so we know the space exists.
	 */

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
	// s5	lwp0uspace
	// s6	bootstk
	// s7   end of memory to clear

	VPRINTS("l1:      ")
	VPRINTXNL(s2)
#ifdef _LP64
	VPRINTS("l2:      ")
	VPRINTXNL(s3)
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

	li	s7, PTE_V		// page table pointer {X,W,R} = {0,0,0}

	// We allocated the kernel first PDE page so let's insert in the
	// page table.

	// Need to setup tables so that for
	// sv32 : s2
	// sv39 : s3 -> s2

#ifdef _LP64
	VPRINTS("l2pde:   ")
	srli	t0, s2, (PGSHIFT - PTE_PPN_SHIFT)
	or	t0, t0, s7		// Assumes s2[11:0] == 0
#if ((VM_MIN_KERNEL_ADDRESS >> XSEGSHIFT) & (NPDEPG - 1)) * SZREG
	li	t1, ((VM_MIN_KERNEL_ADDRESS >> XSEGSHIFT) & (NPDEPG - 1)) * SZREG
	add	t1, t1, s3
	REG_S	t0, 0(t1)

	VPRINTX(t1)
#else
	REG_S	t0, 0(s3)

	VPRINTX(s3)
#endif

	VPRINTS(":  ")
	VPRINTXNL(t0)
	VPRINTS("\n\r")
#endif // _LP64

	// kernel VA
	li	t1,  ((VM_MIN_KERNEL_ADDRESS >> SEGSHIFT) & (NPDEPG - 1)) * SZREG
	add	s9, s2, t1

#if PGSHIFT < PTE_PPN_SHIFT
#error Code assumes PGSHIFT is greater than PTE_PPN_SHIFT
#endif

	li	s5, (VM_KERNEL_SIZE >> SEGSHIFT)		// # of megapages
	li	s6, (NBSEG >> (PGSHIFT - PTE_PPN_SHIFT))	// load for ease
	li	s7, PTE_KERN | PTE_HARDWIRED | PTE_R | PTE_W | PTE_X

	//
	// Fill in the PDEs for kernel.
	//
	PTR_LA	s0, start
	srli	s0, s0, SEGSHIFT	// round down to NBSEG, and shift in
	slli	s0, s0, (SEGSHIFT - PGSHIFT + PTE_PPN_SHIFT)	// ... to PPN
	or	s0, s0, s7
1:
	VPRINTS("kern:    ")
	VPRINTX(s9)
	VPRINTS(":  ")
	VPRINTXNL(s0)

	REG_S	s0, 0(s9)		// store PDE
	add	s0, s0, s6		// advance PA in PDE to next segment
	add	s9, s9, SZREG		// advance to next PDE slot
	addi	s5, s5, -1		// count down segment
	bnez	s5, 1b			// loop if more

	// DTB VA
	li	t1,  ((VM_KERNEL_DTB_BASE >> SEGSHIFT) & (NPDEPG - 1)) * SZREG
	add	s9, s2, t1

	li	s7, PTE_KERN | PTE_HARDWIRED | PTE_R | PTE_W

	//
	// Fill in the PDE for the DTB. Only do one - if any more are required
	// they will be mapped in later.
	//
	mv	s0, s11
	srli	s0, s0, SEGSHIFT	// round down to NBSEG, and shift in
	slli	s0, s0, (SEGSHIFT - PGSHIFT + PTE_PPN_SHIFT)	// ... to PPN
	or	s0, s0, s7

	VPRINTS("dtb:     ")
	VPRINTX(s9)
	VPRINTS(":  ")
	VPRINTXNL(s0)

	REG_S	s0, 0(s9)

#ifdef CONSADDR
	li	t1,  ((VM_KERNEL_IO_BASE >> SEGSHIFT) & (NPDEPG - 1)) * SZREG
	add	s9, s2, t1

	// Fill in the PDE for CONSADDR.
	PTR_L	t0, .Lconsaddr
	mv	s0, t0
	srli	s0, s0, SEGSHIFT	// round down to NBSEG, and shift in
	slli	s0, s0, (SEGSHIFT - PGSHIFT + PTE_PPN_SHIFT)	// ... to PPN
	or	s0, s0, s7

	VPRINTS("cons:    ")
	VPRINTX(s9)
	VPRINTS(":  ")
	VPRINTXNL(s0)

	REG_S	s0, 0(s9)
#endif

	li	a0, 'P'
	call	_C_LABEL(uartputc)

	/* Set supervisor trap vector base register */
	PTR_LA	t0, vstart
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
	.global vstart
vstart:
	// MMU is on!
	csrw	sscratch, zero		// zero in sscratch to mark kernel

#ifdef CONSADDR
	add	sp, sp, s8
#endif
	li	a0, 'M'
	call	_C_LABEL(uartputc)	// uartputs doesn't use stack
	li	a0, '\n'
	call	_C_LABEL(uartputc)	// uartputs doesn't use stack
	li	a0, '\r'
	call	_C_LABEL(uartputc)	// uartputs doesn't use stack

	PTR_LA	tp, _C_LABEL(lwp0)	// put curlwp in tp


	/* Set supervisor trap vector base register */
	PTR_LA	a0, _C_LABEL(cpu_exception_handler)
	csrw	stvec, a0

	PTR_LA	t0, bootstk		// top of lwp0uspace
	PTR_S	t0, L_PCB(tp)		// set uarea of lwp (already zeroed)
	addi	sp, t0, -TF_LEN		// switch to new stack
	PTR_S	sp, L_MD_UTF(tp)	// store pointer to empty trapframe

	PTR_LA	t1, _C_LABEL(kernel_pmap_store)
	add	t2, s4, s8 		// PA -> VA
	srli	t3, s4, PGSHIFT
	PTR_S	t2, PM_PDETAB(t1)	// VA of kernel PDETAB
	PTR_S	t3, PM_MD_PPN(t1)	// PPN of kernel PDETAB

	/*
	 * Store kern_vtopdiff (the difference between the physical
	 * and virtual address of the "start" symbol).
	 *
	 * XXX For some reason doing this store to the physical
	 * XXX address of kern_vtopdiff before the MMU is enabled
	 * XXX doesn't work on the AllWinner D1.
	 */
	PTR_LA	t0, _C_LABEL(kern_vtopdiff)
	PTR_S	s8, 0(t0)	/* kern_vtopdiff = start(virt) - start(phys) */

#if notyet
	mv	a0, s11			// dtb
	call	_C_LABEL(init_mmu)
#endif

	li	t0, VM_MIN_KERNEL_ADDRESS + VM_KERNEL_SIZE
	li	t1, NBSEG - 1
	and	t1, s11, t1
	or	t0, t0, t1

	/* Set the global pointer */
	.option push
	.option norelax
	lla	gp, __global_pointer$
	.option pop

	// Now we should ready to start initializing the kernel.
	mv	a0, s10			// hartid
	mv	a1, s11			// dtb (physical)

	li	s0, 0			// zero frame pointer
	call	_C_LABEL(init_riscv)	// do MD startup
	tail	_C_LABEL(main)		// and transfer to main
	/* No return from main */
END(start)


#if defined(MULTIPROCESSOR)

// a0 is hartid
// a1 is the cookie from sbi_hart_start
ENTRY(cpu_mpstart)
	mv	s10, a0			// copy hartid
	mv	s11, a1			// copy sbi_hart_start cookie

	/*
	 * resolve our cpuindex. each hartid is stored in
	 * extern unsigned long cpu_hartid[MAXCPUS]
	 */
	PTR_LA	t0, _C_LABEL(cpu_hartid)
	li	t6, MAXCPUS
	li	t1, 0
1:
	addi	t1, t1, 1
	bgeu	t1, t6, toomanyharts	/* cpuindex >= MAXCPUS ? */

	slli	t2, t1, LONG_SCALESHIFT
	add	t2, t0, t2
	LONG_L	t3, 0(t2)		/* cpu_hartid[cpuindex] */
	bne	t3, s10, 1b

	mv	s9, t1			/* s9 = cpuindex */

	/*
	 * s9 = cpuindex
	 */

	/* set stack pointer for boot */
	li	t1, BOOT_AP_STACKSIZE		// XXXNH do a shift
	mul	t1, s9, t1
	PTR_LA	t0, _C_LABEL(bootstk)
	/* sp = bootstk + (BOOT_AP_STACKSIZE * cpuindex) */
	add	sp, t0, t1


	/*
	 * Calculate the difference between the VA and PA for start and
	 * keep in s8.
	 */
	PTR_LA	t0, start
	PTR_L	s8, .Lstart

	sub	s8, s8, t0

#ifdef _LP64
	PTR_LA	s4, _C_LABEL(l2_pte)
#else
	PTR_LA	s4, _C_LABEL(l1_pte)
#endif

	// s4 is satp address....
	// s8 is kern_vtopdiff
	//

	/* Set supervisor trap vector base register */
	PTR_LA	t0, vmpstart
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
	.global vmpstart
vmpstart:
	// MMU is on!
	csrw	sscratch, zero		// zero in sscratch to mark kernel

	/* Set the global pointer */
	.option push
	.option norelax
	lla	gp, __global_pointer$
	.option pop

	/* Set SP to VA */
	add	sp, sp, s8

	/* Set supervisor trap vector base register with ipi_handler */
	PTR_LA	a0, _C_LABEL(ipi_handler)
	csrw	stvec, a0
	csrsi	sie, SIE_SSIE
	csrsi	sstatus, SR_SIE		// enable interrupts

	li	tp, 0
	mv	a0, s9
	call	_C_LABEL(cpu_init_secondary_processor)

	/* t3 = __BIT(cpuindex % (sizeof(u_long) * NBBY)) */
	li	t3, 1
	andi	t0, s9, (1U << (LONG_SCALESHIFT + NBBY_SHIFT)) - 1
	sll	t3, t3, t0

	/* t4 = &riscv_cpu_mbox[cpuindex / (sizeof(u_long) * NBBY)] */
	PTR_LA	t0, _C_LABEL(riscv_cpu_mbox)
	srli	t1, s9, LONG_SCALESHIFT + NBBY_SHIFT
	slli	t1, t1, LONG_SCALESHIFT
	add	t4, t0, t1

	/* wait for the mailbox start bit to become true */
1:
	fence	rw, r		/* matches cpu_boot_secondary_processors */
	LONG_L	t0, 0(t4)
	and	t0, t0, t3
	bne	t0, zero, 9f
	wfi
	j	1b
9:

	/* Set supervisor trap vector base register */
	PTR_LA	a0, _C_LABEL(cpu_exception_handler)
	csrw	stvec, a0

	li	t0, CI_SIZE
	mul	t0, s9, t0
	PTR_LA	t1, _C_LABEL(cpu_info_store)
	add	a0, t0, t1		/* a0 = &cpu_info_store[cpuindex] */

	/*
	 * set curlwp (tp and curcpu()->ci_curlwp) now we know the
	 * idle lwp from curcpu()->ci_idlelwp
	 */
	PTR_L	tp, CI_IDLELWP(a0)	/* tp = curcpu()->ci_idlelwp */
	PTR_S	tp, CI_CURLWP(a0)	/* curlwp is idlelwp */

	/* get my stack from lwp */
	PTR_L	t2, L_PCB(tp)		/* t2 = lwp_getpcb(idlelwp) */
	li	t3, UPAGES * PAGE_SIZE
	add	t2, t2, t3
	addi	sp, t2, -TF_LEN		/* sp = pcb + USPACE - TF_LEN */

	li	s0, 0			/* trace back starts here (fp = 0) */
	PTR_L	a0, L_CPU(tp)		/* curlwp->l_cpu */
	call	_C_LABEL(cpu_hatch)

	li	s0, 0			// zero frame pointer
	tail	idle_loop
	/* No return from idle_loop */
END(cpu_mpstart)


toomanyharts:
	PRINTS("too many harts, or hart id doens't exist in cpu_hart[]\n")
1:	wfi
	j	1b

/*
 * A very basic exception handler to just return when an IPI comes in during
 * AP bringup.
 *
 * The handler address needs to have bottom two bits as zero.
 */
	.align 2

ipi_handler:
	csrrw	tp, sscratch, tp	// swap scratch and thread pointer
	bnez	tp, 1f			//   tp != 0, something went wrong.

	csrr	tp, scause		// get cause
	bgez	tp, 2f			// MSB is set if interrupt

	csrw	sip, zero		// clear all interrupts

	csrrw	tp, sscratch, zero	// get back our thread pointer
	sret

1:
	wfi
	j	1b
2:
	wfi
	j	2b
#endif

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
#define	SBI_LEGACY_CONSOLE_PUTCHAR	1
	li	a7, SBI_LEGACY_CONSOLE_PUTCHAR
	ecall
	ret
#endif
END(uartputc)


ENTRY_NP(uartgetc)
#ifdef EARLYCONS
	li	a0, -1
#else
#define	SBI_LEGACY_CONSOLE_GETCHAR	2
	li	a7, SBI_LEGACY_CONSOLE_GETCHAR
	ecall
	ret
#endif


ENTRY_NP(clear_bss)
	bgeu	a0, a1, 1f
2:
	sb	zero, 0(a0)
	addi	a0, a0, 1
	bne	a1, a0, 2b
1:
	ret
END(clear_bss)


	.globl  _C_LABEL(cpu_Debugger_insn)
	.globl  _C_LABEL(cpu_Debugger_ret)

ENTRY_NP(cpu_Debugger)
cpu_Debugger_insn:
	ebreak
cpu_Debugger_ret:
	ret
END(cpu_Debugger)

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

	/*
	 * Allocate some memory after the kernel image for stacks and
	 * bootstrap L1PT
	 */

	.section "_init_memory", "aw", %nobits
	.align PGSHIFT
	.global _C_LABEL(lwp0uspace)
_C_LABEL(lwp0uspace):
	.space	UPAGES * PAGE_SIZE
bootstk:

#ifdef MULTIPROCESSOR
	.space	BOOT_AP_STACKSIZE * (MAXCPUS - 1)
#endif

	.section "_init_memory", "aw", %nobits
	.align PGSHIFT
mmutables_start:
bootstrap_pde:
	.global _C_LABEL(bootstrap_pde)
#ifdef _LP64
	.global _C_LABEL(l2_pte)
l2_pte:
	.space PAGE_SIZE
#endif
	.global _C_LABEL(l1_pte)
l1_pte:
	.space PAGE_SIZE
mmutables_end:

