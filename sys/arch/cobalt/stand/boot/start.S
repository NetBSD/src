/*	$NetBSD: start.S,v 1.3.24.1 2007/07/26 15:06:34 liamjfoy Exp $	*/

/*
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Wayne Knowles
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <mips/asm.h>
#include <mips/cache_r4k.h>

LEAF(start)
	.set	noreorder
	.set	mips3
#ifdef __GP_SUPPORT__
	la      gp, _C_LABEL (_gp)
#endif
	la	sp, start - CALLFRAME_SIZ
	sw	zero, CALLFRAME_RA(sp)		# clear ra for debugger
	sw	zero, CALLFRAME_SP(sp)		# clear fp for debugger
	move	s0, a0				# save argc
	move	s1, a1				# save argv

	jal	_C_LABEL(flushcache)
	 nop

	la	a0, _C_LABEL (edata)		# clear BSS
	move	a1, zero
	la	a2, _C_LABEL (end)
	jal	_C_LABEL(memset)		# memset(edata, 0, end - edata)
	subu	a2, a2, a0

	move	a0, s0				# restore argc
	jal	_C_LABEL(main)			# main(unsigned int)
	move	a1, s1				# restore argv

XLEAF(_xtt)
	jal	_C_LABEL(cpu_reboot)		# failed, reboot
	nop
END(start)

LEAF(flushcache)
	li	t0, 0x80000000
	addu	t1, t0, 32*1024		/* flush 32KB */
	subu	t1, t1, 0x100		/* per 256 bytes */

1:
	/* flush I-cache */
	cache	CACHE_R4K_I | CACHEOP_R4K_INDEX_INV, 0x000(t0)
	cache	CACHE_R4K_I | CACHEOP_R4K_INDEX_INV, 0x020(t0)
	cache	CACHE_R4K_I | CACHEOP_R4K_INDEX_INV, 0x040(t0)
	cache	CACHE_R4K_I | CACHEOP_R4K_INDEX_INV, 0x060(t0)
	cache	CACHE_R4K_I | CACHEOP_R4K_INDEX_INV, 0x080(t0)
	cache	CACHE_R4K_I | CACHEOP_R4K_INDEX_INV, 0x0a0(t0)
	cache	CACHE_R4K_I | CACHEOP_R4K_INDEX_INV, 0x0c0(t0)
	cache	CACHE_R4K_I | CACHEOP_R4K_INDEX_INV, 0x0e0(t0)

	/* flush D-cache */
	cache	CACHE_R4K_D | CACHEOP_R4K_INDEX_WB_INV, 0x000(t0)
	cache	CACHE_R4K_D | CACHEOP_R4K_INDEX_WB_INV, 0x020(t0)
	cache	CACHE_R4K_D | CACHEOP_R4K_INDEX_WB_INV, 0x040(t0)
	cache	CACHE_R4K_D | CACHEOP_R4K_INDEX_WB_INV, 0x060(t0)
	cache	CACHE_R4K_D | CACHEOP_R4K_INDEX_WB_INV, 0x080(t0)
	cache	CACHE_R4K_D | CACHEOP_R4K_INDEX_WB_INV, 0x0a0(t0)
	cache	CACHE_R4K_D | CACHEOP_R4K_INDEX_WB_INV, 0x0c0(t0)
	cache	CACHE_R4K_D | CACHEOP_R4K_INDEX_WB_INV, 0x0e0(t0)

	bne	t0, t1, 1b
	 addu	t0, t0, 0x100

	j	ra
	 nop
END(flushcache)
