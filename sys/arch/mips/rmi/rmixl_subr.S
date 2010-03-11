/*	$NetBSD: rmixl_subr.S,v 1.2.4.2 2010/03/11 15:02:41 yamt Exp $	*/

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
