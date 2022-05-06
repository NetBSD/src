/*	$NetBSD: locore.S,v 1.86 2022/05/06 06:09:50 ryo Exp $	*/

/*
 * Copyright (c) 2017 Ryo Shimizu <ryo@nerv.org>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "opt_arm_debug.h"
#include "opt_console.h"
#include "opt_cpuoptions.h"
#include "opt_ddb.h"
#include "opt_fdt.h"
#include "opt_kasan.h"
#include "opt_multiprocessor.h"

#include <aarch64/asm.h>
#include <aarch64/hypervisor.h>
#include "assym.h"

RCSID("$NetBSD: locore.S,v 1.86 2022/05/06 06:09:50 ryo Exp $")

#ifdef AARCH64_DEVICE_MEM_STRONGLY_ORDERED
#define	MAIR_DEVICE_MEM		MAIR_DEVICE_nGnRnE
#else
#define	MAIR_DEVICE_MEM		MAIR_DEVICE_nGnRE
#endif
#define	MAIR_DEVICE_MEM_SO	MAIR_DEVICE_nGnRnE

/*#define DEBUG_LOCORE			// debug print */
/*#define DEBUG_LOCORE_PRINT_LOCK	// avoid mixing AP's output */

#define LOCORE_EL2

#define BOOT_AP_STACKSIZE	256	/* size of temporally stack for APs */
#define PMAPBOOT_PAGEALLOCMAX	(1024 * 1024)	/* reserved size from _end[] */

#if (defined(VERBOSE_INIT_ARM) || defined(DEBUG_LOCORE)) && defined(EARLYCONS)
#if !defined(CONSADDR)
#error CONSADDR required with EARLYCONS
#endif
#define VERBOSE_LOCORE
#endif

#ifdef VERBOSE_LOCORE
#define VPRINT(string)		PRINT(string)
#else
#define VPRINT(string)
#endif

/* DPRINTREG macro use x19 internally. x0-x15 may be broken */
#if (defined(DEBUG_LOCORE) && defined(EARLYCONS))
#define DPRINT(string)		PRINT(string)
#define DPRINTREG(str, reg)	mov x19,reg; PRINT(str); mov x0,x19; bl print_x0
#define DPRINTSREG(str, reg)	mrs x19,reg; PRINT(str); mov x0,x19; bl print_x0
#else
#define DPRINT(string)
#define DPRINTREG(str, reg)
#define DPRINTSREG(str, reg)
#endif

#define PRINT(string)	bl xprint; .asciz string; .align 2


	.text
	.align	3
ASENTRY_NP(aarch64_start)
	/* keep lr & sp for return to bootloader if possible */
	mov	x27, lr
	mov	x28, sp

	/* set stack pointer for boot */
	adrl	x0, bootstk
	mov	sp, x0

	bl	clear_bss

	PRINT("boot NetBSD/aarch64\n")

	bl	1f
1:	DPRINTREG("PC               = ", lr)
	DPRINTREG("SP               = ", sp)
	mrs	x20, CurrentEL
	lsr	x20, x20, #2
	DPRINTREG("CurrentEL        = ", x20)
	cmp	x20, #2
	bcc	1f
	/* EL2 registers can be accessed in EL2 or higher */
	DPRINTSREG("SCTLR_EL2        = ", sctlr_el2)
	DPRINTSREG("HCR_EL2          = ", hcr_el2)
1:
	DPRINTSREG("SPSR_EL1         = ", spsr_el1)
	DPRINTSREG("CNTFREQ_EL0      = ", cntfrq_el0)
	DPRINTSREG("SCTLR_EL1        = ", sctlr_el1)
	DPRINTSREG("MIDR_EL1         = ", midr_el1)
	DPRINTSREG("MPIDR_EL1        = ", mpidr_el1)
	DPRINTSREG("ID_AA64MPFR0_EL1 = ", id_aa64pfr0_el1)
	DPRINTSREG("ID_AA64MPFR1_EL1 = ", id_aa64pfr1_el1)
	DPRINTSREG("ID_AA64ISAR0_EL1 = ", id_aa64isar0_el1)
	DPRINTSREG("ID_AA64ISAR1_EL1 = ", id_aa64isar1_el1)
	DPRINTSREG("ID_AA64MMFR0_EL1 = ", id_aa64mmfr0_el1)
	DPRINTSREG("ID_AA64MMFR1_EL1 = ", id_aa64mmfr1_el1)

#ifdef LOCORE_EL2
	VPRINT("Drop to EL1...")
# include <aarch64/aarch64/locore_el2.S>
	VPRINT("OK\n")
	mrs	x20, CurrentEL
	lsr	x20, x20, #2
	DPRINTREG("CurrentEL        = ", x20)
#endif /* LOCORE_EL2 */


	bl	mmu_disable
	bl	init_sysregs
	bl	init_mmutable
	cbnz	x0, aarch64_fatal
	bl	save_ttbrs

	VPRINT("MMU Enable...")
	bl	mmu_enable
	VPRINT("OK\n")

	ldr	x20, =vstart	/* virtual address of vstart */
	DPRINTSREG("SPSR_EL1         = ", spsr_el1)
	DPRINTSREG("DAIF             = ", daif)
	DPRINTREG("vstart           = ", x20)
	br	x20		/* jump to the kernel virtual address */

aarch64_fatal:
	PRINT("fatal error occurred while booting\n")
	/* return to bootloader. if switched from EL2 to EL1, It might fail */
	mov	lr, x27
	mov	sp, x28
	ret

/*
 * vstart is in kernel virtual address
 */
vstart:
	DPRINTREG("PC               = ", x20)

	/* set exception vector */
	adrl	x0, _C_LABEL(el1_vectors)
	msr	vbar_el1, x0

	/* set lwp0 stack */
	adrl	x0, lwp0uspace
	add	x0, x0, #(UPAGES * PAGE_SIZE)
	sub	x0, x0, #TF_SIZE	/* lwp0space + USPACE - TF_SIZE */
	mov	sp, x0			/* define lwp0 ksp bottom */
	DPRINTREG("SP(lwp0,kvm)     = ", sp)

	/* lwp-private = NULL */
	msr	tpidr_el0, xzr
	msr	tpidrro_el0, xzr

	/* set curlwp() */
	adrl	x0, lwp0		/* curlwp is lwp0 */
	msr	tpidr_el1, x0
	DPRINTREG("curlwp           = ", x0);

	/* init HAFDBS if supported */
	mov	x0, #1
	bl	aarch64_hafdbs_init

	/* init PAN if supported */
	mov	x0, #1
	bl	aarch64_pan_init

	/* init PAC if supported */
	mov	x0, #1
	bl	aarch64_pac_init
	cbnz	w0, 1f			/* if (aarch64_pac_init() == 0) */
	mrs	x0, sctlr_el1
	ldr	x1, sctlr_pac
	orr	x0, x0, x1		/*  enable PAC */
	msr	sctlr_el1, x0
	isb
1:

	adrl	x19, cpu_info_store	/* curcpu (&cpu_info_store[0] */

	mov	x0, x19
	bl	cpu_setup_id

	/* set topology information */
	mov	x0, x19
	mrs	x1, mpidr_el1
	mov	x2, #0
	bl	arm_cpu_topology_set

	/* get and parse the cache configuration */
	mov	x0, x19
	bl	aarch64_getcacheinfo
	mov	x0, x19
	bl	aarch64_parsecacheinfo

#ifdef KASAN
	adrl	x0, lwp0uspace
	bl	_C_LABEL(kasan_early_init)
#endif

	mov	fp, #0			/* trace back starts here */
	PRINT("initarm\n")
	bl	_C_LABEL(initarm)	/* Off we go */

	PRINT("main\n")
	bl	_C_LABEL(main)		/* call main() */

	adr	x0, .Lmainreturned
	b	_C_LABEL(panic)
	/* NOTREACHED */
ASEND(aarch64_start)

.Lmainreturned:
	.asciz	"main() returned"
	.align 2


ASENTRY_NP(clear_bss)
	/* Zero the BSS. The size must be aligned 16, usually it should be. */
	adrl	x14, __bss_start__
	adrl	x15, __bss_end__
	b	2f
1:	stp	xzr, xzr, [x14], #16
2:	cmp	x14, x15
	b.lo	1b
	ret
ASEND(clear_bss)


init_sysregs:
	stp	x0, lr, [sp, #-16]!

	/* init debug registers */
	msr	mdscr_el1, xzr
	msr	oslar_el1, xzr

	/* Clear context id register */
	msr	contextidr_el1, xzr

	/* No trap system register access, and Trap FP/SIMD access */
	msr	cpacr_el1, xzr
	isb

	/* allow to read CNTVCT_EL0 and CNTFRQ_EL0 from EL0 */
	mrs	x0, cntkctl_el1
	orr	x0, x0, #CNTKCTL_EL0VCTEN
	msr	cntkctl_el1, x0

	/* any exception not masked */
	msr	daif, xzr

	ldp	x0, lr, [sp], #16
	ret


#ifdef MULTIPROCESSOR

#ifdef DEBUG_LOCORE
/*
 * atomic_ops doesn't work before MMU enabled, so using Peterson's algorithm.
 * this is only used to serialize debug print and avoid mixing output.
 * Not absolutely necessary.
 *
 * x27 for cpuindex.
 */
locore_lock_enter:
#ifdef DEBUG_LOCORE_PRINT_LOCK
	mov	x3, xzr			/* x3 = level */
levelloop:
	/* lock_level[] and lock_turn[] are always accessed via PA(devmap) */
	adrl	x0, kern_vtopdiff
	ldr	x0, [x0]
	ldr	x4, =lock_level
	sub	x4, x4, x0
	ldr	x5, =lock_turn
	sub	x5, x5, x0

	strh	w3, [x4, x27, lsl #1]	/* lock_level[i] = level */
	dsb	sy
	strh	w27, [x5, x3, lsl #1]	/* lock_turn[level] = i */
	dsb	sy
waitloop:
	dmb	sy
	ldrh	w0, [x5, x3, lsl #1]	/* lock_turn[level] == i ? */
	cmp	x27, x0
	bne	nextlevel

	mov	x2, xzr			/* k = 0 */
levelcheck:
	cmp	x2, x27
	beq	levelcheck_next

	dmb	sy
	ldrsh	w0, [x4, x2, lsl #1]	/* lock_level[k] >= level */
	cmp	w0, w3
	bge	waitloop
levelcheck_next:
	add	x2, x2, #1		/* k++ */
	cmp	x2, #MAXCPUS
	bne	levelcheck
nextlevel:
	add	x3, x3, #1
	cmp	x3, #(MAXCPUS - 1)
	bne	levelloop
#endif /* DEBUG_LOCORE_PRINT_LOCK */
	ret


locore_lock_exit:
#ifdef DEBUG_LOCORE_PRINT_LOCK
	/* lock_level[] and lock_turn[] are always accessed via PA(devmap) */
	adrl	x0, kern_vtopdiff
	ldr	x0, [x0]
	ldr	x1, =lock_level
	sub	x1, x1, x0
	mvn	x0, xzr
	strh	w0, [x1, x27, lsl #1]	/* lock_level[i] = -1 */
	dsb	sy
#endif /* DEBUG_LOCORE_PRINT_LOCK */
	ret


/* print "[CPU$x27] " (x27 for cpuindex) */
printcpu:
	stp	x0, lr, [sp, #-16]!
	PRINT("[CPU");			\
	mov	x0, x27;		\
	bl	_printdec_x0;		\
	PRINT("] ");			\
	ldp	x0, lr, [sp], #16
	ret

#define CPU_DPRINT(str)			\
	bl	locore_lock_enter;	\
	bl	printcpu;		\
	DPRINT(str);			\
	bl	locore_lock_exit

/*
 * CPU_DPRINTREG macro use x19 internally. x0-x15 may be broken.
 * x27 for cpuindex.
 */
#define CPU_DPRINTREG(str,reg)		\
	mov	x19, reg;		\
	bl	locore_lock_enter;	\
	bl	printcpu;		\
	PRINT(str);			\
	mov	x0, x19;		\
	bl	print_x0;		\
	bl	locore_lock_exit

#define CPU_DPRINTSREG(str, reg)	\
	mrs	x19, reg;		\
	CPU_DPRINTREG(str, x19)

#else /* DEBUG_LOCORE */

#define CPU_DPRINT(str)
#define CPU_DPRINTREG(str,reg)
#define CPU_DPRINTSREG(str, reg)

#endif /* DEBUG_LOCORE */

ENTRY_NP(cpu_mpstart)

	mrs	x8, CurrentEL
	lsr	x8, x8, #2
	cmp	x8, #0x2
	b.lo	1f

	mrs	x8, sctlr_el2
#ifdef __AARCH64EB__
	orr	x8, x8, #SCTLR_EE	/* set: Big Endian */
#else
	bic	x8, x8, #SCTLR_EE	/* clear: Little Endian */
#endif
	msr	sctlr_el2, x8
	isb
1:
	mrs	x8, sctlr_el1
#ifdef __AARCH64EB__
	orr	x8, x8, #(SCTLR_EE | SCTLR_E0E)	/* set: Big Endian */
#else
	bic	x8, x8, #(SCTLR_EE | SCTLR_E0E)	/* clear: Little Endian */
#endif
	msr	sctlr_el1, x8
	isb

	mrs	x3, mpidr_el1
	ldr	x0, =(MPIDR_AFF0 | MPIDR_AFF1 | MPIDR_AFF2 | MPIDR_AFF3)
	and	x3, x3, x0

	/*
	 * resolve own cpuindex. my mpidr is stored in
	 * extern uint64_t cpu_mpidr[MAXCPUS]
	 */
	adrl	x0, _C_LABEL(cpu_mpidr)
	mov	x1, xzr
1:
	add	x1, x1, #1
	cmp	x1, #MAXCPUS		/* cpuindex >= MAXCPUS ? */
	bge	toomanycpus
	ldr	x2, [x0, x1, lsl #3]	/* cpu_mpidr[cpuindex] */
	cmp	x2, x3			/* == mpidr_el1 & MPIDR_AFF ? */
	bne	1b

	mov	x27, x1			/* x27 = cpuindex */

	/*
	 * x27 = cpuindex
	 */

	/* set stack pointer for boot */
	mov	x1, #BOOT_AP_STACKSIZE
	mul	x1, x1, x27
	adrl	x0, bootstk
	add	sp, x0, x1  /* sp = bootstk + (BOOT_AP_STACKSIZE * cpuindex) */

	bl	1f
1:	CPU_DPRINTREG("PC               = ", lr)
	CPU_DPRINTREG("SP               = ", sp)
	mrs	x20, CurrentEL
	lsr	x20, x20, #2
	CPU_DPRINTREG("CurrentEL        = ", x20)
	cmp	x20, #2
	bcc	1f
	/* EL2 registers can be accessed in EL2 or higher */
	CPU_DPRINTSREG("SCTLR_EL2        = ", sctlr_el2)
	CPU_DPRINTSREG("HCR_EL2          = ", hcr_el2)
1:
	CPU_DPRINTSREG("SPSR_EL1         = ", spsr_el1)
	CPU_DPRINTSREG("SCTLR_EL1        = ", sctlr_el1)
	CPU_DPRINTSREG("MIDR_EL1         = ", midr_el1)
	CPU_DPRINTSREG("MPIDR_EL1        = ", mpidr_el1)

#ifdef LOCORE_EL2
	CPU_DPRINT("Drop to EL1...\n")
	bl	drop_to_el1
	CPU_DPRINT("Drop to EL1 OK\n")
	mrs	x20, CurrentEL
	lsr	x20, x20, #2
	CPU_DPRINTREG("CurrentEL        = ", x20)
#endif /* LOCORE_EL2 */

	bl	mmu_disable
	bl	init_sysregs

	CPU_DPRINT("MMU Enable...\n")
	bl	load_ttbrs
	bl	mmu_enable
	CPU_DPRINT("MMU Enable OK\n")

	/* jump to virtual address */
	ldr	x20, =mp_vstart
	br	x20

mp_vstart:
	hint	0x24		/* bti j */

	CPU_DPRINTREG("PC               = ", x20)
	CPU_DPRINTREG("SP               = ", sp)

	CPU_DPRINTSREG("TTBR0            = ", ttbr0_el1)
	CPU_DPRINTSREG("TTBR1            = ", ttbr1_el1)

	/* Set SP to VA */
	adrl	x0, kern_vtopdiff
	ldr	x0, [x0]
	add	sp, sp, x0

	/* disable TTBR0 - CPU_DPRINT{,REG,SREG} no longer work after this */
	mrs	x0, tcr_el1
	orr	x0, x0, #TCR_EPD0
	msr	tcr_el1, x0
	isb

	tlbi	vmalle1is
	dsb	ish
	isb

	/* set exception vector */
	adrl	x0, _C_LABEL(el1_vectors)
	msr	vbar_el1, x0

	/* lwp-private = NULL */
	msr	tpidr_el0, xzr
	msr	tpidrro_el0, xzr

	mov	x0, x27
	bl	cpu_init_secondary_processor

	/* x29 = __BIT(cpuindex % (sizeof(u_long) * NBBY)) */
	mov	x0, #1
	and	x2, x27, #63
	lsl	x29, x0, x2

	/* x28 = &arm_cpu_mbox[cpuindex / (sizeof(u_long) * NBBY)] */
	adrl	x0, _C_LABEL(arm_cpu_mbox)
	// Appease clang - mov	x1, x27, lsr #6
	orr	x1, xzr, x27, lsr #6
	add	x28, x0, x1, lsl #3

	/* wait for the mailbox start bit to become true */
1:
	ldar	x20, [x28]	/* matches cpu_boot_secondary_processors */
	tst	x20, x29
	bne	9f
	wfe
	b	1b
9:

	mov	x0, #CPU_INFO_SIZE
	mul	x0, x27, x0
	adrl	x1, _C_LABEL(cpu_info_store)
	add	x0, x0, x1		/* x0 = &cpu_info_store[cpuindex] */

	/*
	 * set curlwp (tpidr_el1 and curcpu()->ci_curlwp) now we know the
	 * idle lwp from curcpu()->ci_idlelwp
	 */
	ldr	x1, [x0, #CI_IDLELWP]	/* x0 = curcpu()->ci_idlelwp */
	msr	tpidr_el1, x1		/* tpidr_el1 = curlwp = x1 */
	str	x1, [x0, #CI_CURLWP]	/* curlwp is idlelwp */

	/* get my stack from lwp */
	ldr	x2, [x1, #L_PCB]	/* x2 = lwp_getpcb(idlelwp) */
	add	x2, x2, #(UPAGES * PAGE_SIZE)
	sub	sp, x2, #TF_SIZE	/* sp = pcb + USPACE - TF_SIZE */

	/* init HAFDBS if supported */
	mov	x0, #0
	bl	aarch64_hafdbs_init

	/* init PAN if supported */
	mov	x0, #0
	bl	aarch64_pan_init

	/* init PAC if supported */
	mov	x0, #0
	bl	aarch64_pac_init
	cbnz	w0, 1f			/* if (aarch64_pac_init() == 0) */
	mrs	x0, sctlr_el1
	ldr	x1, sctlr_pac
	orr	x0, x0, x1		/*  enable PAC */
	msr	sctlr_el1, x0
	isb
1:

	mov	fp, xzr			/* trace back starts here */
	mrs	x0, tpidr_el1		/* curlwp */
	ldr	x0, [x0, #L_CPU]	/* curlwp->l_cpu */
	bl	_C_LABEL(cpu_hatch)
	mov	x0, xzr
	b	_C_LABEL(idle_loop)	/* never to return */
END(cpu_mpstart)

toomanycpus:
	CPU_DPRINT("too many cpus, or MPIDR not exists in cpu_mpidr[]\n")
1:	wfi
	b	1b


#else /* MULTIPROCESSOR */

ENTRY_NP(cpu_mpstart)
1:	wfi
	b	1b
END(cpu_mpstart)

#endif /* MULTIPROCESSOR */


/*
 * xprint - print strings pointed by $PC(LR)
 *          and return to the end of string.
 *          "\n" will be replaced "\r\n"
 * e.g.)
 *    bl        xprint      <- call
 *    .ascii    "Hello\n\0" <- wouldn't return here
 *    .align    2
 *    nop                   <- return to here
 *
 */
xprint:
	mov	x0, lr
	bl	_C_LABEL(uartputs)
	add	x0, x0, #3
	bic	lr, x0, #3
	ret

/*
 * uartputs(str) - print strings with replacing "\n" to "\r\n".
 * returns the address after the end of the string. (x0 = next of '\0')
 */
ENTRY_NP(uartputs)
	stp	x19, lr, [sp, #-16]!
	mov	x19, x0
	ldrb	w0, [x19], #1
	cbz	w0, 9f
1:
	cmp	x0, #'\n'
	bne	2f
	mov	x0, #0x0d	/* '\r' */
	bl	uartputc
	mov	x0, #'\n'
2:
	bl	uartputc
	ldrb	w0, [x19], #1
	cbnz	w0, 1b
9:
	mov	x0, x19
	ldp	x19, lr, [sp], #16
	ret
END(uartputs)

/*
 * print x0 in 16 widths hexadecimal.
 *
 * x0 is preserved despite being caller saved.
 * other caller saved registers will be broken.
 */
_print_x0:
	stp	x0, lr, [sp, #-16]!
	stp	x20, x21, [sp, #-16]!

	mov	x21, x0		/* number to display */
	mov	x20, #60	/* num of shift */
1:
	ror	x0, x21, x20
	and	x0, x0, #0xf
	cmp	x0, #10
	blt	2f
	add	x0, x0, #('a' - 10 - '0')
2:	add	x0, x0, #'0'
	bl	uartputc
	subs	x20, x20, #4
	bge	1b

	ldp	x20, x21, [sp], #16
	ldp	x0, lr, [sp], #16
	ret

/*
 * print x0 in decimal.
 *
 * x0 is preserved despite being caller saved.
 * other caller saved registers will be broken.
 */
_printdec_x0:
	stp	x0, lr, [sp, #-(16+32)]!
	add	x8, sp, #(16+32)

	strb	wzr, [x8, #-1]!
1:
	mov	x10, #10
	udiv	x1, x0, x10	/* x1 = x0 / 10 */
	msub	x3, x1, x10, x0	/* x3 = x0 % 10 */
	mov	x0, x1

	add	x3, x3, #'0'
	strb	w3, [x8, #-1]!
	cbnz	x0, 1b

	mov	x0, x8
	bl	uartputs

	ldp	x0, lr, [sp], #(16+32)
	ret

/*
 * print x0 in 16 widths hexadecimal with crlf.
 *
 * x0 is preserved despite being caller saved.
 * other caller saved registers will be broken.
 */
print_x0:
	stp	x0, lr, [sp, #-16]!
	bl	_print_x0
	PRINT("\n")
	ldp	x0, lr, [sp], #16
	ret

#ifdef VERBOSE_LOCORE
/*
 * tinyprintf() supports only maximum 7 '%x', '%d' and '%s' formats.
 * width and any modifiers are ignored. '\n' will be replaced to '\r\n'.
 *
 * '%x' will be always expanded 16 widths hexadicimal.
 * e.g., tinyprintf("Hello %s %x\n", "World", 0x12345)
 * outputs "Hello World 0000000000012345\r\n"
 */
tinyprintf:
	stp	x0, lr, [sp, #-16]!
	stp	x19, x20, [sp, #-16]!
	stp	x7, x8, [sp, #-16]!
	stp	x5, x6, [sp, #-16]!
	stp	x3, x4, [sp, #-16]!
	stp	x1, x2, [sp, #-16]!

	mov	x20, xzr
	mov	x19, x0
	ldrb	w0, [x19], #1
	cbz	w0, tinyprintf_done

tinyprintf_loop:
	cmp	x0, #'\n'
	bne	1f
	/* '\n' -> '\r', '\n' */
	mov	x0, #0x0d	/* '\r' */
	bl	uartputc
	mov	x0, #'\n'
1:

	cmp	x0, #'%'
	bne	tinyprintf_putc
	cmp	x20, #8
	bcs	tinyprintf_putc

tinyprintf_fetch_fmt:
	ldrb	w9, [x19], #1
	cbz	w9, tinyprintf_done

	/* width and modifier are ignored */
	cmp	x9, #'h'
	beq	tinyprintf_fetch_fmt
	cmp	x9, #'l'
	beq	tinyprintf_fetch_fmt
	cmp	x9, #'j'
	beq	tinyprintf_fetch_fmt
	cmp	x9, #'t'
	beq	tinyprintf_fetch_fmt
	cmp	x9, #'z'
	beq	tinyprintf_fetch_fmt
	cmp	x9, #'0'
	bcc	1f
	cmp	x9, #'9'
	bls	tinyprintf_fetch_fmt
1:
	ldr	x0, [sp, x20, lsl #3]	/* get Nth argument */
	add	x20, x20, #1

	cmp	x9, #'x'
	bne	5f
	/* "%x" format */
	bl	_print_x0
	b	tinyprintf_next
5:
	cmp	x9, #'d'
	bne	5f
	/* "%d" format */
	bl	_printdec_x0
	b	tinyprintf_next
5:
	cmp	x9, #'s'
	bne	5f
	/* "%s" format */
	bl	_C_LABEL(uartputs)
	b	tinyprintf_next
5:

tinyprintf_putc:
	bl	uartputc
tinyprintf_next:
	ldrb	w0, [x19], #1
	cbnz	w0, tinyprintf_loop

tinyprintf_done:
	mov	x0, x19

	ldp	x1, x2, [sp], #16
	ldp	x3, x4, [sp], #16
	ldp	x5, x6, [sp], #16
	ldp	x7, x8, [sp], #16
	ldp	x19, x20, [sp], #16
	ldp	x0, lr, [sp], #16
	ret
#endif /* VERBOSE_LOCORE */


save_ttbrs:
	/* save ttbr[01]_el1 for AP */
	mrs	x0, ttbr0_el1
	mrs	x1, ttbr1_el1
	adrl	x2, ttbr_save
	stp	x0, x1, [x2]
	ret

load_ttbrs:
	/* load ttbr[01]_el1 */
	adrl	x2, ttbr_save
	ldp	x0, x1, [x2]
	msr	ttbr0_el1, x0
	msr	ttbr1_el1, x1
	ret


init_mmutable:
	stp	x26, lr, [sp, #-16]!

	/* first allocated page must be kernel l0pt = ARM_BOOTSTRAP_LxPT */
	bl	pmapboot_pagealloc
	cbz	x0, init_mmutable_error
	msr	ttbr1_el1, x0

	bl	pmapboot_pagealloc
	cbz	x0, init_mmutable_error
	msr	ttbr0_el1, x0

	DPRINTSREG("TTBR0            = ", ttbr0_el1)
	DPRINTSREG("TTBR1            = ", ttbr1_el1)

#ifdef VERBOSE_LOCORE
	adr	x26, tinyprintf
#else
	mov	x26, xzr
#endif

	/*
	 * void
	 * pmapboot_enter(
	 *     x0: vaddr_t va,
	 *     x1: paddr_t pa,
	 *     x2: psize_t size,
	 *     x3: psize_t blocksize,  // L[123]_SIZE
	 *     x4: pt_entry_t attr,    // pte attributes. LX_BLKPAG_*
	 *     x5: void (*pr)(const char *, ...)
	 *  );
	 */

#ifdef CONSADDR
	VPRINT("Creating identity mapping for CONSADDR\n")
	ldr	x0, =CONSADDR			/* va = CONADDR (physical) */
	mov	x1, x0				/* pa = va */
	mov	x2, #L2_SIZE			/* size */
	mov	x3, #L2_SIZE			/* blocksize */
	mov	x4, #LX_BLKPAG_ATTR_DEVICE_MEM | LX_BLKPAG_AP_RW
	orr	x4, x4, #LX_BLKPAG_UXN | LX_BLKPAG_PXN	/* attr */
	mov	x5, x26				/* pr func */
	bl	pmapboot_enter
#endif

	/* identity mapping for kernel image */
	VPRINT("Creating identity mapping for kernel image\n")
	adrl	x0, start			/* va = start (physical) */

	mov	x1, x0				/* pa = va */
	adrl	x2, _end
	sub	x2, x2, x1			/* size = _end - start */
	add	x2, x2, #PMAPBOOT_PAGEALLOCMAX	/* for pmapboot_pagealloc() */
	mov	x3, #L2_SIZE			/* blocksize */
	mov	x4, #LX_BLKPAG_ATTR_NORMAL_WB | LX_BLKPAG_AP_RW	/* attr */
	orr	x4, x4, #LX_BLKPAG_UXN
	mov	x5, x26				/* pr func */
	bl	pmapboot_enter

#ifdef FDT
	VPRINT("Creating identity mapping for FDT\n")
	adrl	x8, _C_LABEL(fdt_addr_r)
	ldr	x8, [x8]

	mov	x0, x8				/* va */
	mov	x1, x8				/* pa */
	mov	x2, #L2_SIZE			/* size */
	mov	x3, #L2_SIZE			/* blocksize */
	mov	x4, #LX_BLKPAG_ATTR_NORMAL_WB | LX_BLKPAG_AP_RW
	orr	x4, x4, #LX_BLKPAG_UXN | LX_BLKPAG_PXN	/* attr */
	mov	x5, x26				/* pr func */
	bl	pmapboot_enter
#endif

	VPRINT("Creating KVA=PA tables\n")
	ldr	x0, =start			/* va */
	adrl	x1, start			/* pa = start (physical) */
	adrl	x2, _end
	sub	x2, x2, x1			/* size = _end - start */
	mov	x3, #L2_SIZE			/* blocksize */
	mov	x4, #LX_BLKPAG_ATTR_NORMAL_WB | LX_BLKPAG_AP_RW	/* attr */
	orr	x4, x4, #LX_BLKPAG_UXN
	mov	x5, x26				/* pr func */
	bl	pmapboot_enter

	VPRINT("OK\n");
	mov	x0, xzr
	b	init_mmutable_done
init_mmutable_error:
	mvn	x0, xzr
init_mmutable_done:
	ldp	x26, lr, [sp], #16
	ret

mmu_disable:
	dsb	sy
	mrs	x0, sctlr_el1
	bic	x0, x0, SCTLR_M		/* clear MMU enable bit */
	msr	sctlr_el1, x0
	isb
	ret

mmu_enable:
	dsb	sy

	/* Invalidate all TLB */
	dsb	ishst
#ifdef MULTIPROCESSOR
	tlbi	vmalle1is
#else
	tlbi	vmalle1
#endif
	dsb	ish
	isb

	ldr	x0, mair_setting
	msr	mair_el1, x0
	isb

	/* TCR_EL1:IPS[34:32] = AA64MMFR0:PARange[3:0] */
	ldr	x0, tcr_setting
	mrs	x1, id_aa64mmfr0_el1
	bfi	x0, x1, #32, #3
	msr	tcr_el1, x0

	/*
	 * configure SCTLR
	 */
	mrs	x0, sctlr_el1
	ldr	x1, sctlr_clear
	bic	x0, x0, x1
	ldr	x1, sctlr_pac	/* disable PAC */
	bic	x0, x0, x1
	ldr	x1, sctlr_set
	orr	x0, x0, x1

	msr	sctlr_el1, x0	/* enabling MMU! */
	isb

	ret


	.align 3
mair_setting:
	.quad (						\
	    __SHIFTIN(MAIR_NORMAL_WB, MAIR_ATTR0) |	\
	    __SHIFTIN(MAIR_NORMAL_NC, MAIR_ATTR1) |	\
	    __SHIFTIN(MAIR_NORMAL_WT, MAIR_ATTR2) |	\
	    __SHIFTIN(MAIR_DEVICE_MEM, MAIR_ATTR3) |	\
	    __SHIFTIN(MAIR_DEVICE_MEM_SO, MAIR_ATTR4))

#define VIRT_BIT	48

#ifdef MULTIPROCESSOR
#define TCR_SHAREABLE	(TCR_SH0_INNER | TCR_SH1_INNER)
#else
#define TCR_SHAREABLE	(TCR_SH0_NONE | TCR_SH1_NONE)
#endif

tcr_setting:
	.quad (						\
	    __SHIFTIN(64 - VIRT_BIT, TCR_T1SZ) |	\
	    __SHIFTIN(64 - VIRT_BIT, TCR_T0SZ) |	\
	    TCR_AS64K |					\
	    TCR_TG1_4KB | TCR_TG0_4KB |			\
	    TCR_ORGN0_WB_WA |				\
	    TCR_IRGN0_WB_WA |				\
	    TCR_ORGN1_WB_WA |				\
	    TCR_IRGN1_WB_WA) | TCR_SHAREABLE


#ifdef AARCH64_ALIGNMENT_CHECK
#define SCTLR_A_CONFIG		SCTLR_A
#else
#define SCTLR_A_CONFIG		0
#endif

#ifdef AARCH64_EL0_STACK_ALIGNMENT_CHECK
#define SCTLR_SA0_CONFIG	SCTLR_SA0
#else
#define SCTLR_SA0_CONFIG	0
#endif

#ifdef AARCH64_EL1_STACK_ALIGNMENT_CHECK
#define SCTLR_SA_CONFIG		SCTLR_SA
#else
#define SCTLR_SA_CONFIG		0
#endif


sctlr_set:
	.quad ( \
	    SCTLR_LSMAOE |  /* Load/Store Multiple Atomicity and Ordering */ \
	    SCTLR_nTLSMD |  /* no Trap Load/Store Multiple to Device */ \
	    SCTLR_UCI |     /* Enables EL0 DC {CVAU,CIVAC,CVAC}, IC IVAU */ \
	    SCTLR_SPAN |    /* This field resets to 1 */ \
	    SCTLR_UCT |     /* Enables EL0 access to the CTR_EL0 */ \
	    SCTLR_nTWE |    /* EL0 WFE non-trapping */ \
	    SCTLR_nTWI |    /* EL0 WFI non-trapping */ \
	    SCTLR_DZE |     /* Enables access to the DC ZVA instruction */ \
	    SCTLR_I |       /* Instruction cache enable */ \
	    SCTLR_SED |     /* SETEND instruction disable */ \
	    SCTLR_C |       /* Cache enable */ \
	    SCTLR_M |       /* MMU Enable */ \
	    SCTLR_SA0_CONFIG | \
	    SCTLR_SA_CONFIG | \
	    SCTLR_A_CONFIG | \
	    0)
sctlr_clear:
	.quad ( \
	    SCTLR_IESB |    /* Enable Implicit ErrorSynchronizationBarrier */ \
	    SCTLR_WXN |     /* Write permission implies Execute Never (W^X) */ \
	    SCTLR_UMA |     /* EL0 Controls access to interrupt masks */ \
	    SCTLR_ITD |     /* IT instruction disable */ \
	    SCTLR_nAA |     /* ? */ \
	    SCTLR_CP15BEN | /* CP15 barrier enable */ \
	    SCTLR_SA0 |     /* Enable EL0 stack alignment check */ \
	    SCTLR_SA |      /* Enable SP alignment check */ \
	    SCTLR_A |       /* Alignment check enable */ \
	    0)
sctlr_pac:
	.quad ( \
	    SCTLR_EnIA |    /* PACIA (APIAKey_EL1) instruction enable */ \
	    SCTLR_EnIB |    /* PACIB (APIBKey_EL1) instruction enable */ \
	    SCTLR_EnDA |    /* PACDA (APDAKey_EL1) instruction enable */ \
	    SCTLR_EnDB |    /* PACDB (APDBKey_EL1) instruction enable */ \
	    0)

.L_devmap_addr:
	.quad	VM_KERNEL_IO_ADDRESS

	.data

#ifdef DEBUG_LOCORE_PRINT_LOCK
	.align 2
lock_level:
	.fill	MAXCPUS, 2, -1
lock_turn:
	.fill	(MAXCPUS - 1), 2, -1
#endif /* DEBUG_LOCORE_PRINT_LOCK */

	.align 3
ttbr_save:
	.space	8 * 2

	.bss

	.align PGSHIFT
	.global _C_LABEL(lwp0uspace)
_C_LABEL(lwp0uspace):
	.space	UPAGES * PAGE_SIZE
bootstk:

#ifdef MULTIPROCESSOR
	.space	BOOT_AP_STACKSIZE * (MAXCPUS - 1)
#endif

	.section ".init_pagetable", "aw", %nobits
	.align PGSHIFT
	.global ARM_BOOTSTRAP_LxPT
ARM_BOOTSTRAP_LxPT:
l0pt_kern:

	.section "_init_memory", "aw", %nobits
	.align PGSHIFT

	/* None currently */
