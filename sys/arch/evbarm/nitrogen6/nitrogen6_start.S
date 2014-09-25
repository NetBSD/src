/*	$NetBSD: nitrogen6_start.S,v 1.1 2014/09/25 05:05:28 ryo Exp $	*/

/*-
 * Copyright (c) 2014 Ryo Shimizu <ryo@nerv.org>
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

#include "opt_imx.h"
#include "opt_cpuoptions.h"
#include "opt_cputypes.h"
#include "opt_multiprocessor.h"
#include "opt_arm_debug.h"

#include <arm/asm.h>
#include <arm/armreg.h>
#include <arm/cortex/scu_reg.h>
#include "assym.h"

RCSID("$NetBSD: nitrogen6_start.S,v 1.1 2014/09/25 05:05:28 ryo Exp $")

#ifndef CONADDR
#define CONADDR		0x021e8000
#endif
#define COM_MULT	4

#define BOOT_MEMSIZE	256	/* temporary for boot up kernel */
#define TEMP_L1_TABLE	(KERNEL_BASE - KERNEL_BASE_VOFFSET +		\
			 BOOT_MEMSIZE * 0x100000 - L1_TABLE_SIZE)

#define MD_CPU_HATCH    _C_LABEL(imx6_cpu_hatch)


#ifdef VERBOSE_INIT_ARM
# define XPUTC_COM	1
# define XPUTC(n)	mov r0, n; bl xputc
# define XPUTC2(n)	mov r0, n; blx r11
# define PRINT(str)	bl xprint; .ascii str, "\0"; .align 2
# define PRINT_R0	bl print_r0
#else
# define XPUTC(n)
# define XPUTC2(n)
# define PRINT(str)
# define PRINT_R0
#endif /* VERBOSE_INIT_ARM */

/*
 * Kernel start routine for NITROGEN6X boards.
 * At this point, this code has been loaded into SDRAM
 * and the MMU is off
 */
	.section .start,"ax",%progbits

	.global	_C_LABEL(nitrogen6_start)
_C_LABEL(nitrogen6_start):
	/*
	 * Save any arguments u-boot passed us.
	 */
	movw	r4, #:lower16:uboot_args
	movt	r4, #:upper16:uboot_args
#if KERNEL_BASE_VOFFSET != 0
	sub	r4, r4, #KERNEL_BASE_VOFFSET
#endif
	stmia	r4, {r0-r3}


#ifdef VERBOSE_INIT_ARM
	PRINT("  PC=")
	mov	r0, pc
	PRINT_R0

	PRINT("  SP=")
	mov	r0, sp
	PRINT_R0

	PRINT("CPSR=")
	mrs	r0, cpsr
	PRINT_R0
#endif /* VERBOSE_INIT_ARM */

	/* set temporary stack */
	movw	sp, #:lower16:tmpstack
	movt	sp, #:upper16:tmpstack

	PRINT("<cortex_init>")
	bl	cortex_init
	PRINT("</cortex_init>\r\n")

	/*
	 * Set up a preliminary mapping in the MMU to allow us to run
	 * at KERNEL_BASE with caches on.
	 */
	PRINT("<mmu_init_table>")
	movw	r1, #:lower16:mmu_init_table
	movt	r1, #:upper16:mmu_init_table
	movw	r0, #:lower16:TEMP_L1_TABLE
	movt	r0, #:upper16:TEMP_L1_TABLE
	bl	arm_boot_l1pt_init
	PRINT("</mmu_init_table>\r\n")

	/*
	 * init the CPU TLB, Cache, MMU.
	 */
	PRINT("<arm_cpuinit>")
#ifdef VERBOSE_INIT_ARM
	adr	r11, xputc
#endif
	movw	r0, #:lower16:TEMP_L1_TABLE
	movt	r0, #:upper16:TEMP_L1_TABLE
	bl	arm_cpuinit
	PRINT("</arm_cpuinit>\r\n")

#ifdef MULTIPROCESSOR
	movw	r1, #:lower16:IMX6_AIPS1_BASE+AIPS1_SRC_BASE
	movt	r1, #:upper16:IMX6_AIPS1_BASE+AIPS1_SRC_BASE

	/* disable core1,2,3 */
	ldr	r2, [r1, #SRC_SCR]
	and	r2, r2, #~(SRC_SCR_CORE1_ENABLE|	\
			   SRC_SCR_CORE2_ENABLE|	\
			   SRC_SCR_CORE3_ENABLE)
	str	r2, [r1, #SRC_SCR]

	/* get number of CPU core */
	mrc	p15, 4, r0, c15, c0, 0		/* read SCU base addr */
#ifdef __ARMEB__
	setend	le
#endif
	ldr	r0, [r0, #SCU_CFG]
#ifdef __ARMEB__
	setend	be
#endif
	and	r0, r0, #3	/* r0 = (num of core) - 1 */

	mov	r3, #0	/* flags for arm_cpu_hatched */

	cmp	r0, #0
	ble	9f

#ifdef VERBOSE_INIT_ARM
	/* wait to prevent the mixing of console output */
	mov	r2, #0x400000
1:	subs	r2, r2, #1
	bne	1b
#endif
	/* enable core1 */
	adr	r2, nitrogen6_mpstart
	str	r2, [r1, #SRC_GPR3]	/* set core1 entry address */
	ldr	r2, [r1, #SRC_SCR]
	orr	r2, r2, #SRC_SCR_CORE1_ENABLE	/* enable core1 */
	orr	r2, r2, #SRC_SCR_CORE1_RST	/* reset core1 */
	str	r2, [r1, #SRC_SCR]
	orr	r3, r3, #(1<<1)		/* cpu1 bit for arm_cpu_hatched */

	cmp	r0, #1
	ble	9f

#ifdef VERBOSE_INIT_ARM
	/* wait to prevent the mixing of console output */
	mov	r2, #0x400000
1:	subs	r2, r2, #1
	bne	1b
#endif
	/* enable core2 */
	adr	r2, nitrogen6_mpstart
	str	r2, [r1, #SRC_GPR5]	/* set core2 entry address */
	ldr	r2, [r1, #SRC_SCR]
	orr	r2, r2, #SRC_SCR_CORE2_ENABLE	/* enable core2 */
	orr	r2, r2, #SRC_SCR_CORE2_RST	/* reset core2 */
	str	r2, [r1, #SRC_SCR]
	orr	r3, r3, #(1<<2)		/* cpu2 bit for arm_cpu_hatched */

	cmp	r0, #2
	ble	9f
#ifdef VERBOSE_INIT_ARM
	/* wait to prevent the mixing of console output */
	mov	r2, #0x400000
1:	subs	r2, r2, #1
	bne	1b
#endif
	/* enable core3 */
	adr	r2, nitrogen6_mpstart
	str	r2, [r1, #SRC_GPR7]	/* set core3 entry address */
	ldr	r2, [r1, #SRC_SCR]
	orr	r2, r2, #SRC_SCR_CORE3_ENABLE	/* enable core3 */
	orr	r2, r2, #SRC_SCR_CORE3_RST	/* reset core3 */
	str	r2, [r1, #SRC_SCR]
	orr	r3, r3, #(1<<3)		/* cpu3 bit for arm_cpu_hatched */
9:

	/* wait hatched */
	movw	r2, #:lower16:arm_cpu_hatched
	movt	r2, #:upper16:arm_cpu_hatched
	mov	r1, #0x10000000	/* i = 0x10000000; do {          */
1:	dmb			/*   memory barrier             */
	ldr	r0, [r2]
	cmp	r0, r3		/*   if (arm_cpu_hatched == r3) */
	beq	.hatched	/*     goto .hatched;           */
	subs	r1, r1, #1
	bne	1b		/* } while (--i != 0);          */
.not_hatched:
	PRINT("\r\nnot hatched. arm_cpu_hatched=")
	PRINT_R0
	b	9f
.hatched:
	PRINT("\r\nMULTIPROCESSOR hatched!\r\n")
9:
#endif

	PRINT("jump to start()\r\n")
	movw	lr, #:lower16:start
	movt	lr, #:upper16:start
	bx	lr
	/* NOTREACHED */

nitrogen6_mpstart:
	/* invalidate cache */
	movw	ip, #:lower16:_C_LABEL(armv7_dcache_inv_all)
	movt	ip, #:upper16:_C_LABEL(armv7_dcache_inv_all)
#ifndef KERNEL_BASES_EQUAL
	sub	ip, ip, #KERNEL_BASE_VOFFSET
#endif
	blx	ip
	b	_C_LABEL(cortex_mpstart)

#include <arm/cortex/a9_mpsubr.S>

mmu_init_table:
	/* map VA==PA for IO+MEM (0x00000000-0x4fffffff) */
	MMU_INIT(0x00000000, 0x00000000,
	    0x50000000 / L1_S_SIZE,
	    L1_S_PROTO_armv7 | L1_S_APv7_KRW)

#if KERNEL_BASE_VOFFSET != 0
	MMU_INIT(KERNEL_BASE, KERNEL_BASE - KERNEL_BASE_VOFFSET,
	    (BOOT_MEMSIZE * L1_S_SIZE + L1_S_SIZE - 1) / L1_S_SIZE,
	    L1_S_PROTO_armv7 | L1_S_APv7_KRW | L1_S_CACHEABLE)
#endif

	/* Map the 2MB of primary peripherals (AIPS1 + AIPS2) */
	MMU_INIT(KERNEL_IO_IOREG_VBASE, IMX6_IOREG_PBASE,
	    (IMX6_IOREG_SIZE + L1_S_SIZE - 1) / L1_S_SIZE,
	    L1_S_PROTO_armv7 | L1_S_APv7_KRW)

	/* Map the 1MB of armcore peripherals */
	MMU_INIT(KERNEL_IO_ARMCORE_VBASE, IMX6_ARMCORE_PBASE,
	    (IMX6_ARMCORE_SIZE + L1_S_SIZE - 1) / L1_S_SIZE,
	    L1_S_PROTO_armv7 | L1_S_APv7_KRW)

	/* end of table */
	MMU_INIT(0, 0, 0, 0)

#ifdef VERBOSE_INIT_ARM
/*
 * xprint - print strings pointed by $PC(LR)
 *          and return to the end of string.
 *          all registers are saved.
 * e.g.)
 *    bl	xprint        <- call
 *    .ascii	"Hello\r\n\0" <- don't return here
 *    .align	2
 *    nop                     <- return to here
 */
xprint:
	stmfd	sp!, {r0, r1}
	mov	r1, lr
1:
	ldrb	r0, [r1], #1

	stmfd	sp!, {r0, r1, r2, r3}
	bl	xputc
	ldmfd	sp!, {r0, r1, r2, r3}

	cmp	r0, #0
	bne	1b

	add	r1, r1, #3
	bic	lr, r1, #3	/* lr = 4byte-aligned end of string */
	ldmfd	sp!, {r0, r1}
	bx	lr

/*
 * print_r0 - show r0 hexadecimal with CR/LF.
 *            all registers are saved.
 */
print_r0:
	stmfd	sp!, {r0, r3, lr}
	mov	r3, r0
	mov	r0, #'0'
	bl	debugputc
	mov	r0, #'x'
	bl	debugputc
	bl	print_r3
	mov	r0, #'\r'
	bl	debugputc
	mov	r0, #'\n'
	bl	debugputc
	ldmfd	sp!, {r0, r3, pc}

/*
 * print_r3 - show r3 hexadecimal without CR/LF nor prefix(0x).
 *            all registers are saved.
 */
print_r3:
	stmfd	sp!, {r0, r3-r6, lr}
	mov	r4, #28		/* num of shift. 28,24,20...8,4,0 */
	mov	r5, #0xf	/* mask */
1:
	and	r6, r5, r3, ROR r4
	cmp	r6, #10
	addlt	r0, r6, #'0'
	addge	r0, r6, #('a' - 0x0a)
	bl	debugputc
	subs	r4, r4, #4
	bge	1b
	ldmfd	sp!, {r0, r3-r6, pc}

/*
 * debugputc - putc r0. xputc() is defined in arm/cortex/a9_mpsubr.S
 *             all registers are saved.
 */
debugputc:
	stmfd	sp!, {r0, r1, r2, r3, lr}
	bl	xputc
	ldmfd	sp!, {r0, r1, r2, r3, pc}
#endif /* VERBOSE_INIT_ARM */

/*
 * temporary local stack
 */
	.align 8
	.space	1024
tmpstack:

END(_C_LABEL(nitrogen6_start))
