/*	$NetBSD: locore.S,v 1.18 2018/08/10 21:06:42 ryo Exp $	*/

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

#include "opt_cpuoptions.h"
#include "opt_multiprocessor.h"
#include "opt_ddb.h"
#include "opt_arm_debug.h"

#include <aarch64/asm.h>
#include <aarch64/hypervisor.h>
#include "assym.h"

RCSID("$NetBSD: locore.S,v 1.18 2018/08/10 21:06:42 ryo Exp $")

/* #define DEBUG_LOCORE */
/* #define DEBUG_MMU */

#if (defined(VERBOSE_INIT_ARM) || defined(DEBUG_LOCORE)) && defined(EARLYCONS)
#define VERBOSE_LOCORE
#endif

#define LOCORE_EL2

/* attributes are defined in MAIR_EL1 */
#define L2_BLKPAG_ATTR_NORMAL_WB	LX_BLKPAG_ATTR_INDX_0
#define L2_BLKPAG_ATTR_NORMAL_NC	LX_BLKPAG_ATTR_INDX_1
#define L2_BLKPAG_ATTR_NORMAL_WT	LX_BLKPAG_ATTR_INDX_2
#define L2_BLKPAG_ATTR_DEVICE_MEM	LX_BLKPAG_ATTR_INDX_3

#define PRINT(string)	bl xprint;.asciz string;.align 2

#ifdef VERBOSE_LOCORE
#define VERBOSE(string)	PRINT(string)
#else
#define VERBOSE(string)
#endif

/* load far effective address (pc relative) */
.macro	ADDR, reg, addr
	adrp	\reg, \addr
	add	\reg, \reg, #:lo12:\addr
.endm

ENTRY_NP(aarch64_start)
	/* Zero the BSS. The size must be aligned 16, usually it should be. */
	ADDR	x0, __bss_start__
	ADDR	x1, __bss_end__
	b	2f
1:	stp	xzr, xzr, [x0], #16
2:	cmp	x0, x1
	b.lo	1b

	/* set stack pointer for boot */
	ADDR	x0, bootstk
	mov	sp, x0

#ifdef DEBUG_LOCORE
	PRINT("PC               = ")
	bl	1f
1:	mov	x0, lr
	bl	print_x0

	PRINT("SP               = ")
	bl	1f
1:	mov	x0, sp
	bl	print_x0

	PRINT("CurrentEL        = ")
	mrs	x0, CurrentEL
	lsr	x0, x0, #2
	bl	print_x0

	cmp	x0, #2
	bne	1f

	/* EL2 registers can be accessed in EL2 or higher */
	PRINT("SCTLR_EL2        = ")
	mrs	x0, sctlr_el2
	bl	print_x0

	PRINT("HCR_EL2          = ")
	mrs	x0, hcr_el2
	bl	print_x0
1:

	PRINT("CNTFREQ_EL0      = ")
	mrs	x0, cntfrq_el0
	bl	print_x0

	PRINT("DAIF             = ")
	mrs	x0, daif
	bl	print_x0

	PRINT("MPIDR_EL1        = ")
	mrs	x0, mpidr_el1
	bl	print_x0

	PRINT("L2CTLR_EL1       = ")
	mrs	x0, s3_1_c11_c0_2
	bl	print_x0

	PRINT("ID_AA64MPFR0_EL1 = ")
	mrs	x0, id_aa64pfr0_el1
	bl	print_x0

	PRINT("ID_AA64MPFR1_EL1 = ")
	mrs	x0, id_aa64pfr1_el1
	bl	print_x0

	PRINT("ID_AA64ISAR0_EL1 = ")
	mrs	x0, id_aa64isar0_el1
	bl	print_x0

	PRINT("ID_AA64ISAR1_EL1 = ")
	mrs	x0, id_aa64isar1_el1
	bl	print_x0


	PRINT("ID_AA64MMFR0_EL1 = ")
	mrs	x0, id_aa64mmfr0_el1
	bl	print_x0

	PRINT("ID_AA64MMFR1_EL1 = ")
	mrs	x0, id_aa64mmfr1_el1
	bl	print_x0
#endif


#ifdef LOCORE_EL2
	VERBOSE("Drop to EL1...")
# include <aarch64/aarch64/locore_el2.S>
	VERBOSE("OK\r\n")
#ifdef DEBUG_LOCORE
	PRINT("CurrentEL        = ")
	mrs	x0, CurrentEL
	lsr	x0, x0, #2
	bl	print_x0
#endif /* DEBUG_LOCORE */
#endif /* LOCORE_EL2 */

#ifdef DEBUG_LOCORE
	PRINT("DAIF             = ")
	mrs	x0, daif
	bl	print_x0
#endif

	bl	mmu_disable

	bl	init_sysregs

	bl	arm_boot_l0pt_init

	VERBOSE("MMU Enable...")
	bl	mmu_enable
	VERBOSE("OK\r\n")

	/* set exception vector */
	ldr	x2, =el1_vectors	/* el1_vectors is in kva */
	msr	vbar_el1, x2

#ifdef DEBUG_LOCORE
	PRINT("SPSR_EL1        = ")
	mrs	x0, spsr_el1
	bl	print_x0

	PRINT("DAIF            = ")
	mrs	x0, daif
	bl	print_x0

	PRINT("VSTART          = ")
	ldr	x0, =vstart	/* virtual address of vstart */
	bl	print_x0
#endif

	ldr	x0, =vstart	/* virtual address of vstart */
	br	x0		/* jump to the kernel virtual address */

/*
 * vstart is in kernel virtual address
 */
vstart:
	ADDR	x0, lwp0uspace
	add	x0, x0, #(UPAGES * PAGE_SIZE)
	sub	x0, x0, #TF_SIZE	/* lwp0space + USPACE - TF_SIZE */
	mov	sp, x0			/* define lwp0 ksp bottom */

#ifdef DEBUG_LOCORE
	PRINT("VSP             = ")
	mov	x0, sp
	bl	print_x0
#endif

	msr	tpidr_el0, xzr		/* tpidr_el0 (for TLS) = NULL */
	ADDR	x0, cpu_info_store	/* cpu_info_store is cpu_info[0] */
	msr	tpidr_el1, x0		/* curcpu is cpu_info[0] */

	mov	fp, #0			/* trace back starts here */
	PRINT("initarm\r\n")
	bl	_C_LABEL(initarm)	/* Off we go */

	PRINT("main\r\n")
	bl	_C_LABEL(main)		/* call main() */

	adr	x0, .Lmainreturned
	b	_C_LABEL(panic)
	/* NOTREACHED */
END(aarch64_start)

.Lmainreturned:
	.asciz	"main() returned"

	.align 3
	.text

#ifdef MULTIPROCESSOR

#if defined(VERBOSE_LOCORE) || defined(DEBUG_LOCORE)
/*
 * print "[CPU$x27] " (x27 as cpuid)
 * XXX: max 4 digit
 */
printcpu:
	stp	x0, lr, [sp, #-16]!
	stp	x25, x26, [sp, #-16]!
	PRINT("[CPU")
	mov	x26, x27		/* n = cpuid */
	mov	x25, xzr		/* zeropad = 0 */
	mov	x1, #1000
	udiv	x0, x26, x1		/* x0 = n / 1000 */
	msub	x26, x0, x1, x26	/* n %= 1000 */
	cbz	x0, 1f			/* if (x0 == 0) goto 1f */
	add	x0, x0, #'0'
	bl	uartputc
	mov	x25, #1			/* zeropad = 1 */
1:
	mov	x1, #100
	udiv	x0, x26, x1		/* x0 = n / 100 */
	msub	x26, x0, x1, x26	/* n %= 100 */
	adds	x25, x25, x0		/* if ((zeropad + x0) == 0) */
	beq	1f			/*   goto 1f */
	add	x0, x0, #'0'
	bl	uartputc
	mov	x25, #1			/* zeropad = 1 */
1:
	mov	x1, #10
	udiv	x0, x26, x1		/* x0 = n / 10 */
	msub	x26, x0, x1, x26	/* n %= 10 */
	adds	x25, x25, x0		/* if ((zeropad + x0) == 0) */
	beq	1f			/*   goto 1f */
	add	x0, x0, #'0'
	bl	uartputc
1:
	add	x0, x26, #'0'
	bl	uartputc
	PRINT("] ")
	ldp	x25, x26, [sp], #16
	ldp	x0, lr, [sp], #16
	ret
#define PRINTCPU()	bl	printcpu
#else
#define PRINTCPU()
#endif /* VERBOSE_LOCORE || DEBUG_LOCORE */

#ifdef VERBOSE_LOCORE
#define VERBOSE_PRINTCPU()	PRINTCPU()
#else
#define VERBOSE_PRINTCPU()
#endif

ENTRY_NP(aarch64_mpstart)
ENTRY_NP(cortex_mpstart)	/* compat arm */
	/*
	 * XXX:
	 *  cpuid(index) is read from MPIDR_EL1.AFF0. AFF1,2,3 are ignored.
	 *  cpuid should be passed from primary processor...
	 */
	mrs	x27, mpidr_el1
	and	x27, x27, #MPIDR_AFF0	/* XXX: cpuid = mpidr_el1 & Aff0 */
	mov	x0, #1
	lsl	x28, x0, x27		/* x28 = 1 << cpuid */
	mov	x0, x28

	/* x27 = cpuid, x28 = (1 << cpuid) */

	/* set stack pointer for boot */
#define BOOT_STACKSIZE	256
	mov	x1, #BOOT_STACKSIZE
	mul	x1, x1, x27
	ADDR	x0, bootstk_cpus
	sub	sp, x0, x1	/* sp = bootstk_cpus - BOOT_STACKSIZE * cpuid */

#ifdef DEBUG_LOCORE
	PRINTCPU()
	PRINT("PC               = ")
	bl	1f
1:	mov	x0, lr
	bl	print_x0

	PRINTCPU()
	PRINT("SP               = ")
	bl	1f
1:	mov	x0, sp
	bl	print_x0

	PRINTCPU()
	PRINT("CurrentEL        = ")
	mrs	x0, CurrentEL
	lsr	x0, x0, #2
	bl	print_x0
#endif

#ifdef LOCORE_EL2
#ifdef DEBUG_LOCORE
	VERBOSE_PRINTCPU()
	VERBOSE("Drop to EL1...")
#endif
	bl	drop_to_el1
#ifdef DEBUG_LOCORE
	VERBOSE("OK\r\n")
#endif
#ifdef DEBUG_LOCORE
	PRINTCPU()
	PRINT("CurrentEL        = ")
	mrs	x0, CurrentEL
	lsr	x0, x0, #2
	bl	print_x0
#endif /* DEBUG_LOCORE */
#endif /* LOCORE_EL2 */

	bl	mmu_disable

	bl	init_sysregs

#ifdef DEBUG_LOCORE
	VERBOSE_PRINTCPU()
	VERBOSE("MMU Enable...")
#endif
	bl	mmu_enable
#ifdef DEBUG_LOCORE
	VERBOSE("OK\r\n")
#endif

	/* jump to virtual address */
	ldr	x0, =mp_vstart
	br	x0

mp_vstart:
	/* set exception vector */
	ADDR	x0, el1_vectors
	msr	vbar_el1, x0

#ifdef DEBUG_LOCORE
	PRINTCPU()
	PRINT("PC               = ")
	bl	1f
1:	mov	x0, lr
	bl	print_x0

	PRINTCPU()
	PRINT("arm_cpu_hatched  = ")
	ADDR	x0, _C_LABEL(arm_cpu_hatched)
	ldr	x0, [x0]
	bl	print_x0

	PRINTCPU()
	PRINT("my cpubit        = ")
	mov	x0, x28
	bl	print_x0
#endif

	ADDR	x0, _C_LABEL(cpus_midr)
	mrs	x1, midr_el1
	str	w1, [x0, x27, lsl #2]	/* cpu_midr[cpuid] = midr_el1 */

	ADDR	x0, _C_LABEL(cpus_mpidr)
	mrs	x1, mpidr_el1
	str	x1, [x0, x27, lsl #3]	/* cpu_mpidr[cpuid] = mpidr_el1 */


	/*
	 * atomic_or_32(&arm_cpu_hatched, 1 << cpuid)
	 * to tell my activity to primary processor.
	 */
	ADDR	x0, _C_LABEL(arm_cpu_hatched)
	mov	x1, x28
	bl	_C_LABEL(atomic_or_32)	/* hatched! */
	sev

#ifdef DEBUG_LOCORE
	PRINTCPU()
	PRINT("arm_cpu_hatched -> ")
	ADDR	x0, _C_LABEL(arm_cpu_hatched)
	ldr	x0, [x0]
	bl	print_x0
#endif

#ifdef DEBUG_LOCORE
	PRINTCPU()
	PRINT("Hatched.\r\n")
#endif

	/* wait for my bit of arm_cpu_mbox become true */
	ADDR	x1, _C_LABEL(arm_cpu_mbox)
1:
	dmb	sy
	ldr	x0, [x1]
	tst	x0, x28
	bne	9f
	wfe
	b	1b
9:

#ifdef DEBUG_LOCORE
	/* XXX: delay to prevent the mixing of console output */
	mov	x0, #0x4000000
	mul	x0, x0, x27	/* delay (cpuid * 0x4000000) */
1:	subs	x0, x0, #1
	bne	1b

	PRINTCPU()
	PRINT("MBOX received\r\n")

	PRINTCPU()
	PRINT("arm_cpu_mbox  = ")
	ADDR	x0, _C_LABEL(arm_cpu_mbox)
	ldr	x0, [x0]
	bl	print_x0
#endif

	msr	tpidr_el0, xzr		/* tpidr_el0 (for TLS) = NULL */

	/* fill my cpu_info */
	ADDR	x0, _C_LABEL(cpu_info)
	ldr	x0, [x0, x27, lsl #3]	/* x0 = cpu_info[cpuid] */
	msr	tpidr_el1, x0		/* tpidr_el1 = my cpu_info */

	ldr	x1, [x0, #CI_IDLELWP]	/* x1 = curcpu()->ci_data.cpu_idlelwp */
	str	x1, [x0, #CI_CURLWP]	/* curlwp is idlelwp */

	ldr	x2, [x1, #L_PCB]	/* x2 = lwp_getpcb(idlelwp) */
	add	x2, x2, #(UPAGES * PAGE_SIZE)
	sub	sp, x2, #TF_SIZE	/* sp = pcb + USPACE - TF_SIZE */


	mov	fp, xzr			/* trace back starts here */
	bl	_C_LABEL(cpu_hatch)
	mov	x0, xzr
	b	_C_LABEL(idle_loop)	/* never to return */
END(aarch64_mpstart)

#else /* MULTIPROCESSOR */

ENTRY_NP(aarch64_mpstart)
ENTRY_NP(cortex_mpstart)	/* compat arm */
1:	wfi
	b	1b
END(aarch64_mpstart)

#endif /* MULTIPROCESSOR */

/*
 * xprint - print strings pointed by $PC(LR)
 *          and return to the end of string.
 * e.g.)
 *    bl        xprint        <- call
 *    .ascii    "Hello\r\n\0" <- wouldn't return here
 *    .align    2
 *    nop                     <- return to here
 */
	.global xprint
xprint:
	mov	x11, lr
	mov	x12, x0
	ldrb	w0, [x11], #1
	cbz	w0, 2f

1:
	bl	uartputc
	ldrb	w0, [x11], #1
	cbnz	w0, 1b

2:
	add	x11, x11, #3
	bic	lr, x11, #3
	mov	x0, x12
	ret
END(xprint)

	.global _C_LABEL(uartputs)
_C_LABEL(uartputs):
	mov	x11, x0
	ldrb	w0, [x11], #1
	cbz	w0, 9f
1:	bl	uartputc
	ldrb	w0, [x11], #1
	cbnz	w0, 1b
9:
	mov	x0, x11
	ret
END(_C_LABEL(uartputs))

	.global _print_x0
_print_x0:
	stp	x0, lr, [sp, #-16]!
	stp	x4, x5, [sp, #-16]!
	stp	x6, x7, [sp, #-16]!

	mov	x7, x0		/* number to display */
	mov	x4, #60		/* num of shift */
	mov	x5, #0xf	/* mask */
1:
	ror	x0, x7, x4
	and	x0, x0, x5
	cmp	x0, #10
	blt	2f
	add	x0, x0, #('a' - 10 - '0')
2:	add	x0, x0, #'0'
	bl	uartputc
	subs	x4, x4, #4
	bge	1b

	ldp	x6, x7, [sp], #16
	ldp	x4, x5, [sp], #16
	ldp	x0, lr, [sp], #16
	ret
END(_print_x0)

	.global _C_LABEL(print_x0)
_C_LABEL(print_x0):
	stp	x0, lr, [sp, #-16]!
	bl	_print_x0
	PRINT("\r\n")
	ldp	x0, lr, [sp], #16
	ret
END(_C_LABEL(print_x0))

printn_x1:
	stp	x0, lr, [sp, #-16]!
	mov	x0, x1
	bl	_print_x0
	ldp	x0, lr, [sp], #16
	ret

print_x2:
	stp	x0, lr, [sp, #-16]!
	mov	x0, x2
	bl	_print_x0
	PRINT("\r\n")
	ldp	x0, lr, [sp], #16
	ret

arm_boot_l0pt_init:
	stp	x0, lr, [sp, #-16]!

	/* Clean the page table */
	ADDR	x0, mmutables_start
	ADDR	x1, mmutables_end
1:
	stp	xzr, xzr, [x0], #16
	stp	xzr, xzr, [x0], #16
	stp	xzr, xzr, [x0], #16
	stp	xzr, xzr, [x0], #16
	cmp	x0, x1
	b.lo	1b

	VERBOSE("Creating VA=PA tables\r\n")

	/* VA=PA table, link L0->L1 */
	ADDR	x0, ttbr0_l0table
	mov	x1, #0
	ADDR	x2, ttbr0_l1table
	bl	l0_settable

	/* VA=PA L1 blocks */
	ADDR	x0, ttbr0_l1table
	mov	x1, #0			/* VA */
	mov	x2, #0			/* PA */
	mov	x3, #L2_BLKPAG_ATTR_DEVICE_MEM
	mov	x4, #4			/* 4GB = whole 32bit */
	bl	l1_setblocks

	VERBOSE("Creating KSEG tables\r\n")

	/* KSEG table, link L0->L1 */
	ADDR	x0, ttbr1_l0table
	mov	x1, #AARCH64_KSEG_START
	ADDR	x2, ttbr1_l1table_kseg
	bl	l0_settable

	/* KSEG L1 blocks */
	ADDR	x0, ttbr1_l1table_kseg
	mov	x1, #AARCH64_KSEG_START
	mov	x2, #0
	mov	x3, #L2_BLKPAG_ATTR_NORMAL_WB
	orr	x3, x3, #(LX_BLKPAG_PXN|LX_BLKPAG_UXN)
	mov	x4, #Ln_ENTRIES		/* whole l1 table */
	bl	l1_setblocks

	VERBOSE("Creating KVA=PA tables\r\n")

	/* KVA=PA table, link L0->L1 */
	ADDR	x0, ttbr1_l0table
	mov	x1, #VM_MIN_KERNEL_ADDRESS
	ADDR	x2, ttbr1_l1table_kva
	bl	l0_settable

	/* KVA=PA table, link L1->L2 */
	ADDR	x0, ttbr1_l1table_kva
	mov	x1, #VM_MIN_KERNEL_ADDRESS
	ADDR	x2, ttbr1_l2table_kva
	bl	l1_settable

	/* KVA=PA L2 blocks */
	ADDR	x0, ttbr1_l2table_kva
	adr	x2, start		/* physical addr. before MMU */
	and	x2, x2, #L2_BLK_OA	/* L2 block size aligned (2MB) */
	mov	x1, #VM_MIN_KERNEL_ADDRESS
	mov	x3, #(L2_BLKPAG_ATTR_NORMAL_WB|LX_BLKPAG_UXN)

	/* kernelsize = _end - start */
	ldr	x1, =start
	ldr	x4, =_end
	sub	x4, x4, x1

	/* round up kernelsize to L2_SIZE (2MB) */
	add	x4, x4, #L2_SIZE
	sub	x4, x4, #1
	lsr	x4, x4, #L2_SHIFT
	bl	l2_setblocks

	/* map READONLY from VM_MIN_KERNEL_ADDRESS to __data_start */
	VERBOSE("Set kernel text/rodata READONLY\r\n")
	ldr	x3, =__data_start
	ands	x0, x3, #(L2_SIZE - 1)
	beq	1f
	ldr	x1, =_erodata
	and	x1, x1, #L2_ADDR_BITS	/* _erodata & L2_ADDR_BIT */
	and	x0, x3, #L2_ADDR_BITS	/* __data_start & L2_ADDR_BIT */
	cmp	x0, x1
	bne	1f
	/* __data_start and _erodata are in same L2 block */
	PRINT("Warning: data section not aligned on size of L2 block\r\n")
1:
	/* x3 = l2pde_index(__data_start) */
	and	x3, x3, #L2_ADDR_BITS
	lsr	x3, x3, #L2_SHIFT

	/* x2 = l2pde_inex(VM_MIN_KERNEL_ADDRESS) */
	mov	x2, #VM_MIN_KERNEL_ADDRESS
	and	x2, x2, #L2_ADDR_BITS
	lsr	x2, x2, #L2_SHIFT

	ADDR	x1, ttbr1_l2table_kva
	b	9f
1:
	ldr	x0, [x1, x2, lsl #3]	/* x0 = l2table[x2] */
	and	x0, x0, #~LX_BLKPAG_AP
	orr	x0, x0, #LX_BLKPAG_AP_RO
	str	x0, [x1, x2, lsl #3]	/* l2table[x2] = x0 */
	add	x2, x2, #1
9:
	cmp	x2, x3
	blo	1b


	/* add eXecute Never bit from _rodata to _end */
	VERBOSE("Set kernel rodata/data non-Executable\r\n")
	ldr	x0, =__rodata_start
	ands	x0, x0, #(L2_SIZE - 1)
	beq	1f
	PRINT("Warning: rodata section not aligned on size of L2 block\r\n")
1:
	/* x2 = l2pde_index(__rodata_start) */
	ldr	x2, =__rodata_start
	mov	x0, #(L2_SIZE - 1)
	add	x2, x2, x0		/* round block */
	and	x2, x2, #L2_ADDR_BITS
	lsr	x2, x2, #L2_SHIFT

	/* x3 = l2pde_inex(_end) */
	ldr	x3, =_end
	and	x3, x3, #L2_ADDR_BITS
	lsr	x3, x3, #L2_SHIFT

	ADDR	x1, ttbr1_l2table_kva
	b	9f
1:
	ldr	x0, [x1, x2, lsl #3]	/* x0 = l2table[x2] */
	orr	x0, x0, #(LX_BLKPAG_UXN|LX_BLKPAG_PXN)
	str	x0, [x1, x2, lsl #3]	/* l2table[x2] = x0 */
	add	x2, x2, #1
9:
	cmp	x2, x3			/* including the L2 block of _end[] */
	bls	1b


	VERBOSE("Creating devmap tables\r\n")
	/* devmap=PA table, link L1->L2 */
	ADDR	x0, ttbr1_l1table_kva
	ldr	x1, .L_devmap_addr
	ADDR	x2, ttbr1_l2table_devmap
	bl	l1_settable

	ldp	x0, lr, [sp], #16
	ret

	.align 3
.L_devmap_addr:
	.quad	VM_KERNEL_IO_ADDRESS

/*
 *	x0 = l0table
 *	x1 = vaddr
 *	x2 = l1table
 */
l0_settable:
	stp	x0, lr, [sp, #-16]!

	and	x2, x2, #~PAGE_MASK
	mov	x8, #L0_TABLE
	orr	x2, x2, x8
	and	x1, x1, #L0_ADDR_BITS
	lsr	x1, x1, #L0_SHIFT
	str	x2, [x0, x1, lsl #3]	/* l0table[x1] = x2 */

#ifdef DEBUG_MMU
	PRINT("L0 entry[")
	bl printn_x1
	PRINT("]=")
	bl print_x2
#endif

	ldp	x0, lr, [sp], #16
	ret

/*
 *	x0 = l1table
 *	x1 = vaddr
 *	x2 = paddr
 *	x3 = attr
 *	x4 = N entries
 */
l1_setblocks:
	stp	x0, lr, [sp, #-16]!

	and	x2, x2, #L1_ADDR_BITS
	mov	x8, #L1_BLOCK
	orr	x2, x2, x8
	orr	x2, x2, x3
	mov	x8, #(LX_BLKPAG_AF|LX_BLKPAG_AP_RW)
	orr	x2, x2, x8
#ifdef MULTIPROCESSOR
	orr	x2, x2, #LX_BLKPAG_SH_IS
#endif
	and	x1, x1, #L1_ADDR_BITS
	lsr	x1, x1, #L1_SHIFT
1:
	str	x2, [x0, x1, lsl #3]	/* l1table[x1] = x2 */
#ifdef DEBUG_MMU
	PRINT("L1 entry[")
	bl printn_x1
	PRINT("]=")
	bl print_x2
#endif
	mov	x3, #L1_SIZE
	add	x2, x2, x3
	add	x1, x1, #1
	subs	x4, x4, #1
	bne	1b

	ldp	x0, lr, [sp], #16
	ret

/*
 *	x0 = l1table
 *	x1 = vaddr
 *	x2 = l2table
 */
l1_settable:
	stp	x0, lr, [sp, #-16]!

	and	x2, x2, #~PAGE_MASK
	mov	x8, #L1_TABLE
	orr	x2, x2, x8
	and	x1, x1, #L1_ADDR_BITS
	lsr	x1, x1, #L1_SHIFT
	str	x2, [x0, x1, lsl #3]	/* l1table[x1] = x2 */

#ifdef DEBUG_MMU
	PRINT("L1 entry[")
	bl printn_x1
	PRINT("]=")
	bl print_x2
#endif

	ldp	x0, lr, [sp], #16
	ret

/*
 *	x0 = l2table
 *	x1 = vaddr
 *	x2 = paddr
 *	x3 = attr
 *	x4 = N entries
 */
l2_setblocks:
	stp	x0, lr, [sp, #-16]!

	and	x2, x2, #L2_BLOCK_MASK
	mov	x8, #L2_BLOCK
	orr	x2, x2, x8
	orr	x2, x2, x3
	mov	x8, #(LX_BLKPAG_AF|LX_BLKPAG_AP_RW)
	orr	x2, x2, x8
#ifdef MULTIPROCESSOR
	orr	x2, x2, #LX_BLKPAG_SH_IS
#endif
	and	x1, x1, #L2_ADDR_BITS
	lsr	x1, x1, #L2_SHIFT
1:
	str	x2, [x0, x1, lsl #3]	/* l2table[x1] = x2 */
#ifdef DEBUG_MMU
	PRINT("L2 entry[")
	bl printn_x1
	PRINT("]=")
	bl print_x2
#endif
	mov	x3, #L2_SIZE
	add	x2, x2, x3
	add	x1, x1, #1
	subs	x4, x4, #1
	bne	1b

	ldp	x0, lr, [sp], #16
	ret

init_sysregs:
	stp	x0, lr, [sp, #-16]!

	/* Disable debug event */
	msr	mdscr_el1, xzr

	/* Clear context id register */
	msr	contextidr_el1, xzr

	/* No trap system register access, and Trap FP/SIMD access */
	msr	cpacr_el1, xzr

	/* allow to read CNTVCT_EL0 and CNTFRQ_EL0 from EL0 */
	mrs	x0, cntkctl_el1
	orr	x0, x0, #CNTKCTL_EL0VCTEN
	msr	cntkctl_el1, x0

	/* any exception not masked */
	msr	daif, xzr

	ldp	x0, lr, [sp], #16
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

	ADDR	x0, ttbr0_l0table
	msr	ttbr0_el1, x0
	ADDR	x0, ttbr1_l0table
	msr	ttbr1_el1, x0
	isb

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
	ldr	x1, sctlr_set
	orr	x0, x0, x1

	ldr	x1, sctlr_ee
#ifdef __AARCH64EB__
	orr	x0, x0, x1	/* set: BigEndian */
#else
	bic	x0, x0, x1	/* clear: LittleEndian */
#endif
#ifdef MULTIPROCESSOR
	ldr	x1, tcr_setting_inner_shareable
	orr	x0, x0, x1
#endif
	msr	sctlr_el1, x0	/* enabling MMU! */
	isb

	ret

	.align 3
mair_setting:
	.quad (						\
	    __SHIFTIN(MAIR_NORMAL_WB, MAIR_ATTR0) |	\
	    __SHIFTIN(MAIR_NORMAL_NC, MAIR_ATTR1) |	\
	    __SHIFTIN(MAIR_NORMAL_WT, MAIR_ATTR2) |	\
	    __SHIFTIN(MAIR_DEVICE_nGnRnE, MAIR_ATTR3))

#define VIRT_BIT	48
tcr_setting:
	.quad (						\
	    __SHIFTIN(64 - VIRT_BIT, TCR_T1SZ) |	\
	    __SHIFTIN(64 - VIRT_BIT, TCR_T0SZ) |	\
	    TCR_AS64K |					\
	    TCR_TG1_4KB | TCR_TG0_4KB |			\
	    TCR_ORGN0_WB_WA |				\
	    TCR_IRGN0_WB_WA |				\
	    TCR_ORGN1_WB_WA |				\
	    TCR_IRGN1_WB_WA)
#ifdef MULTIPROCESSOR
tcr_setting_inner_shareable:
	.quad (TCR_SH0_INNER | TCR_SH1_INNER)
#endif


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


sctlr_ee:
	.quad (SCTLR_EE | SCTLR_EOE)	/* Endiannes of Exception and EL0 */
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
	    SCTLR_THEE |    /* T32EE is not implemented */ \
	    SCTLR_CP15BEN | /* CP15 barrier enable */ \
	    SCTLR_SA0 |     /* Enable EL0 stack alignment check */ \
	    SCTLR_SA |      /* Enable SP alignment check */ \
	    SCTLR_A |       /* Alignment check enable */ \
	    0)


	.bss

	.align PGSHIFT
	.global _C_LABEL(lwp0uspace)
_C_LABEL(lwp0uspace):
	.space	UPAGES * PAGE_SIZE
bootstk:

#ifdef MULTIPROCESSOR
	.space	BOOT_STACKSIZE * (MAXCPUS - 1)
bootstk_cpus:
#endif


	.align PGSHIFT
mmutables_start:
/*
 * PA == VA mapping using L1 1G block (whole 32bit)
 */
ttbr0_l0table:
	.space	PAGE_SIZE
ttbr0_l1table:
	.space	PAGE_SIZE

/*
 * KVA    => PA mapping using L2 2MB block (kernelsize, max 2MB*512=2Gbyte)
 * DEVMAP => PA mapping using L2 2MB block (devmap size, max 2MB*512=2Gbyte)
 * KSEG   => PA mapping using L1 1GB block * 512
 */
ttbr1_l0table:
	.space	PAGE_SIZE
ttbr1_l1table_kseg:
	.space	PAGE_SIZE
ttbr1_l1table_kva:
	.space	PAGE_SIZE
ttbr1_l2table_kva:
	.space	PAGE_SIZE
ttbr1_l2table_devmap:
	.space	PAGE_SIZE
mmutables_end:
