/* $NetBSD: asm.h,v 1.2.2.3 2020/04/21 18:42:02 martin Exp $ */

#ifndef _AARCH64_ASM_H_
#define _AARCH64_ASM_H_

#if defined(_KERNEL_OPT)
#include "opt_cpuoptions.h"
#endif

#include <arm/asm.h>

#ifdef __aarch64__

#ifdef __ASSEMBLER__
.macro	adrl 	reg, addr
	adrp	\reg, \addr
	add	\reg, \reg, #:lo12:\addr
.endm
#endif

#define	fp	x29
#define	lr	x30

/*
 * Add a speculation barrier after the 'eret'.
 * Some aarch64 cpus speculatively execute instructions after 'eret',
 * and this potentiates side-channel attacks.
 */
#define	ERET	\
	eret; dsb sy; isb

/*
 * ARMv8 options to be made available for the compiler to use. Should be
 * inserted at the beginning of the ASM files that need them.
 *
 * For now the only option is PAC, needed for the compiler to recognize
 * the key registers.
 */
#ifdef ARMV83_PAC
#define ARMV8_DEFINE_OPTIONS	\
	.arch armv8.3-a+pac
#else
#define ARMV8_DEFINE_OPTIONS	/* nothing */
#endif

#endif

#endif /* !_AARCH64_ASM_H_ */
