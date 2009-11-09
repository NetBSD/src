/*	$NetBSD: rmixls_subr.S,v 1.1.2.1 2009/11/09 10:14:54 cliff Exp $	*/

#include "opt_cputype.h"

#include <sys/cdefs.h>

#include <mips/asm.h>
#include <mips/cpuregs.h>

#include "assym.h"

        .set    noreorder
        .set    mips3
        .text

/*
 * some useful labels for debugging
 */
.global	mips_kseg0
.equiv	mips_kseg0,		MIPS_KSEG0_START
.global	mips_kseg1
.equiv	mips_kseg1,		MIPS_KSEG1_START
.global	mips_kseg2
.equiv	mips_kseg2,		MIPS_KSEG2_START
.global	mips_xkphys
.global	mips_xkphys_u
.global	mips_xkphys_cca3
.global	mips_xkphys_cca4
.global	mips_xkseg

.equiv	mips_xkphys,	MIPS_XKPHYS_START
.equiv	mips_xkphys_u,	MIPS_XKPHYS_UNCACHED
.equiv	mips_xkphys_cca3,	MIPS_XKPHYS_CCA3
.equiv	mips_xkphys_cca4,	MIPS_XKPHYS_CCA4
.equiv	mips_xkseg,		MIPS_XKSEG_START

/*
 * read XLS Processor Control register
 *
 * uint64_t rmixls_mfcr(u_int cr);
 */
LEAF(rmixls_mfcr)
	j	ra
	.word	0x70820018	/* mfcr v0, a0 */
END(rmixls_mfcr)

/*
 * write XLS Processor Control register
 *
 * void rmixls_mtcr(u_int cr, uint64_t val);
 */
LEAF(rmixls_mtcr)
	j	ra
	.word	0x70850019	/* mtcr	a1, a0 */
END(rmixls_mtcr)
