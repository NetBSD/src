/*	$NetBSD: ip22_cache.S,v 1.3 2002/03/13 13:12:29 simonb Exp $	*/

/*
 * Copyright 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Code to manipulate the L2 SysAD cache on some (R4600/R5000-based)
 * SGI IP-22 (Indy) systems.
 */

#include <mips/asm.h>
#include <mips/cpuregs.h>

	.set	noreorder
	.set	mips3

#define	PROLOGUE							\
	mfc0	t0, MIPS_COP_0_STATUS				;	\
	nop							;	\
	nop							;	\
	nop							;	\
	li	v0, ~MIPS_SR_INT_IE	/* ints off */		;	\
	and	t1, v0, t0					;	\
	or	t1, MIPS3_SR_KX		/* enable 64-bit */	;	\
	mtc0	t1, MIPS_COP_0_STATUS				;	\
	nop							;	\
	nop							;	\
	nop							;	\
	nop

#define	EPILOGUE							\
	mtc0	t0, MIPS_COP_0_STATUS				;	\
	nop							;	\
	nop							;	\
	nop							;	\
	nop

/*
 * ip22_sdcache_do_wbinv:
 *
 *	Write-back and invalidate the cache lines [a0..a1].
 */
LEAF_NOPROFILE(ip22_sdcache_do_wbinv)
	PROLOGUE

	/*
	 * Translate the cache indices into the magic cache
	 * flush space.
	 */
	dli	v0, 0x9000000080000000	/* base of cache flush space */
	or	a0, v0			/* first */
	or	a1, v0			/* last */

	/*
	 * Flush the cache by performing a store into the
	 * magic cache flush space.
	 */
1:	sw	zero, 0(a0)
	bne	a0, a1, 1b
	daddu	a0, 32

	EPILOGUE

	j	ra
	nop
END(ip22_sdcache_do_wbinv)

LEAF_NOPROFILE(ip22_sdcache_enable)
	PROLOGUE

	li	a0, 0x1
	dsll	a0, 31
	lui	a1, 0x9000
	dsll32	a1, 0
	or	a0, a1, a0
	sb	zero, 0(a0)

	EPILOGUE

	j	ra
	nop
END(ip22_sdcache_enable)

LEAF_NOPROFILE(ip22_sdcache_disable)
	PROLOGUE

	li	a0, 0x1
	dsll	a0, 31
	lui	a1, 0x9000
	dsll32	a1, 0
	or	a0, a1, a0
	sh	zero, 0(a0)

	EPILOGUE

	j	ra
	nop
END(ip22_sdcache_disable)
