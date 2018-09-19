/*	$NetBSD: altera_start.S,v 1.1 2018/09/19 17:31:38 aymeric Exp $ */

#include "opt_arm_debug.h"
#include "opt_fdt.h"

#include <arm/armreg.h>
#include <arm/asm.h>

#include <arm/altera/cycv_reg.h>
#include <evbarm/altera/platform.h>

#include "assym.h"

RCSID("$NetBSD: altera_start.S,v 1.1 2018/09/19 17:31:38 aymeric Exp $")

#define MD_CPU_HATCH _C_LABEL(arm_fdt_cpu_hatch)

#define INIT_MEMSIZE 128

#define TEMP_L1_TABLE (KERNEL_BASE - KERNEL_BASE_VOFFSET + \
				INIT_MEMSIZE * L1_S_SIZE - L1_TABLE_SIZE)

#ifdef VERBOSE_INIT_ARM

#ifndef CONADDR
#define CONADDR 0xFFC02000
#endif
#define COM_MULT 4
#ifdef __ARMEB__
#define COM_BSWAP
#endif

#define XPUTC_COM
#define XPUTC(x)	mov r0, x; bl xputc
#else
#define XPUTC(x)

#endif /* VERBOSE_INIT_ARM */

	.section .start, "ax", %progbits

	.global	_C_LABEL(nanosoc_start)
_C_LABEL(nanosoc_start):
#ifdef __ARMEB__
	setend	be
#endif

	mov	r9, #KERNEL_BASE_VOFFSET

	cpsid	if, #PSR_SVC32_MODE

	movw	r4, #:lower16:uboot_args
	movt	r4, #:upper16:uboot_args
	sub	r4, r4, r9
	stmia	r4, {r0-r3}

#ifdef FDT
	/*
	 * ARM boot protocol has FDT address in r2
	 */
	movw	r4, #:lower16:fdt_addr_r
	movt	r4, #:upper16:fdt_addr_r
	sub	r4, r4, r9
	str	r2, [r4]
#endif

	bl	cortex_init

	movw	r0, #:lower16:TEMP_L1_TABLE
	movt	r0, #:upper16:TEMP_L1_TABLE
	movw	r1, #:lower16:.Lmmu_init_table
	movt	r1, #:upper16:.Lmmu_init_table
	bl	arm_boot_l1pt_init

	XPUTC(#'D')

	movw	r0, #:lower16:TEMP_L1_TABLE
	movt	r0, #:upper16:TEMP_L1_TABLE
#ifdef KERNEL_BASES_EQUAL
	bl	arm_cpuinit
#else
#ifdef VERBOSE_INIT_ARM
	adr	r11, xputc
#endif
	movw	lr, #:lower16:1f
	movt	lr, #:upper16:1f
	b	arm_cpuinit
	.pushsection .start, "ax", %progbits
1:
#endif
/*
	XPUTC2(#'Z')
*/

	b	start
	/* NOTREACHED */

#ifdef KERNEL_BASES_EQUAL
	.popsection
#endif

#include <arm/cortex/a9_mpsubr.S>

.Lmmu_init_table:
	/* Map reset vector to bootstrap secondary cpu */
	MMU_INIT(0, 0, 1, L1_S_PROTO_armv7 | L1_S_APv7_KRW | L1_S_CACHEABLE)

	MMU_INIT(KERNEL_BASE, KERNEL_BASE - KERNEL_BASE_VOFFSET, INIT_MEMSIZE,
		L1_S_PROTO_armv7 | L1_S_APv7_KRW | L1_S_CACHEABLE)

#if KERNEL_BASE_VOFFSET != 0
	/* Map KERNEL_BASE VA to SDRAM PA, write-back cacheable, shareable */
	MMU_INIT(KERNEL_BASE - KERNEL_BASE_VOFFSET,
		KERNEL_BASE - KERNEL_BASE_VOFFSET, INIT_MEMSIZE,
		L1_S_PROTO_armv7 | L1_S_APv7_KRW | L1_S_CACHEABLE)
#endif

	/* Map peripherals */
	MMU_INIT(CYCV_PERIPHERAL_VBASE, CYCV_PERIPHERAL_BASE,
		(CYCV_PERIPHERAL_SIZE + L1_S_SIZE - 1) / L1_S_SIZE,
		L1_S_PROTO_armv7 | L1_S_APv7_KRW | L1_S_V6_XN)

	/* end of table */
	MMU_INIT(0, 0, 0, 0)

END(_C_LABEL(nanosoc_start))
