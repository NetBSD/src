/*	$NetBSD: cache_r5k_subr.S,v 1.1.126.1 2009/09/07 23:47:27 matt Exp $ 	*/

/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permited provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <mips/asm.h>
#include <mips/cpuregs.h>
#include <mips/cache_r4k.h>
#include <mips/cache_r5k.h>

#include "assym.h"

	.set mips3
	.set noreorder

/*
 * r5k_enable_sdcache:
 *
 *	Enable and clear out the R5k secondary (unified) cache.
 */
LEAF_NOPROFILE(r5k_enable_sdcache)
	lw	t2, _C_LABEL(mips_sdcache_size)
	la	t1, MIPS_KSEG0_START

	beq	t2, zero, 3f		# if no sdcache, we can bail now
	nop

	add	t2, t1, t2

	la	v0, 1f
	or	v0, MIPS_KSEG1_START

	j	v0			# run the rest from uncached space
	nop

1:
        mfc0    v0, MIPS_COP_0_CONFIG	
        or      v1, v0, MIPS3_CONFIG_SE
        mtc0    v1, MIPS_COP_0_CONFIG	# enable the secondary cache 
	nop
	nop
	nop

2:
	cache	0x17, 0(t1)		# 0x17 == Page_Invalidate_SD
	addiu	t1, t1, 4096

	sltu	v0, t1, t2
	bne	v0, zero, 2b
	nop

3:
	j	ra
	nop	
END(r5k_enable_sdcache)
