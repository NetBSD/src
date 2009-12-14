/*	$NetBSD: rmixl_subr.S,v 1.2 2009/12/14 00:46:08 matt Exp $	*/

#include "opt_cputype.h"

#include <sys/cdefs.h>

#include <mips/asm.h>
#include <mips/cpuregs.h>

	.set	push
        .set    noreorder
        .set    mips64
        .text

/*
 * read XLS Processor Control register
 *
 * uint64_t rmixl_mfcr(u_int cr);
 */
LEAF(rmixl_mfcr)
	j	ra
	mfcr	v0, a0
END(rmixl_mfcr)

/*
 * write XLS Processor Control register
 *
 * void rmixl_mtcr(u_int cr, uint64_t val);
 */
LEAF(rmixl_mtcr)
	j	ra
	mtcr	a1, a0
END(rmixl_mtcr)

	.set	pop
