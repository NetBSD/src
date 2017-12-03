/*	$NetBSD: cpu_startup.S,v 1.1.18.2 2017/12/03 11:36:09 jdolecek Exp $ */

/*-
 * Copyright (c) 2015 Michael Lorenz
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

#include "opt_cputype.h"
#include "opt_multiprocessor.h"

#include <sys/cdefs.h>
#include <sys/endian.h>

#include <mips/asm.h>
RCSID("$NetBSD: cpu_startup.S,v 1.1.18.2 2017/12/03 11:36:09 jdolecek Exp $");

#ifdef MULTIPROCESSOR

#include <mips/cpuregs.h>
#include <mips/cache_r4k.h>

#include "assym.h"

#define CACHE_SIZE (32 * 1024)
#define CACHE_LINESIZE 32

NESTED_NOPROFILE(ingenic_trampoline, 0, ra)
	/*
	 * We act as the idle lwp so make it CURLWP.  When know
	 * that the cpu_info is a KSEG0 address.
	 */
	move	a0, a1
	// Loop until idlelwp is filled in.
1:	PTR_L	MIPS_CURLWP, CPU_INFO_IDLELWP(a0)
	nop
	beqz	MIPS_CURLWP, 1b
	 nop
	PTR_S	MIPS_CURLWP, CPU_INFO_CURLWP(a0)

	li	v0, 0
	mtc0	v0, MIPS_COP_0_STATUS		# reset to known state
	COP0_SYNC

	PTR_L	sp, L_MD_UTF(MIPS_CURLWP)	# fetch KSP

	/*
	 * Indicate that no one has called us.
	 */
	move	ra, zero
	REG_S	ra, CALLFRAME_RA(sp)

	/*
	 * New execution constant needs GP to be loaded.
	 */
	PTR_LA	gp, _C_LABEL(_gp)

	/*
	 * and off we go.
	 */
	j	_C_LABEL(cpu_hatch)		# does everything
	 nop
END(ingenic_trampoline)


/*
 * exception vector secondary CPUs take when started
 */
.p2align 16
VECTOR(ingenic_wakeup, unknown)
	.set	noat

	mtc0	zero, MIPS_COP_0_CAUSE
	COP0_SYNC

	/* init caches */
	li	t0, MIPS_KSEG0_START
	ori	t1, t0, CACHE_SIZE
	mtc0	zero, MIPS_COP_0_TAG_LO, 0
	COP0_SYNC
1:	cache	CACHEOP_R4K_INDEX_STORE_TAG | CACHE_R4K_I, 0(t0)
	cache	CACHEOP_R4K_INDEX_STORE_TAG | CACHE_R4K_D, 0(t0)
	addiu	t0, t0, CACHE_LINESIZE
	bne	t0, t1, 1b
	 nop

	/* kseg0 cache attribute */
	mfc0	t0, MIPS_COP_0_CONFIG, 0
	ori	t0, t0, MIPS3_TLB_ATTR_WB_NONCOHERENT
	mtc0	t0, MIPS_COP_0_CONFIG, 0
	COP0_SYNC

	/* pagemask */
	mtc0	zero, MIPS_COP_0_TLB_PG_MASK, 0
	COP0_SYNC

	/*
	 * - set a1 to corresponding cpu_info
	 * - set sp to ci->ci_data.cpu_idlelwp->l_md.md_utf
	 * - jump to cpu_trampoline
	 */
	PTR_L	a1, _C_LABEL(startup_cpu_info)
	nop

	j	ingenic_trampoline
	  nop
	.set	at
VECTOR_END(ingenic_wakeup)

#endif /* MULTIPROCESSOR */
