/*	$NetBSD: locore_r5900.S,v 1.1 2001/10/16 16:31:39 uch Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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

/* Toshiba R5900 specific functions */

#include <mips/asm.h>
#include <mips/cpuregs.h>
#include <mips/r5900/cpuregs.h>	

	.set	noreorder	
	.set	mips3
	.text
	.align 6	/* align cache line size (64B) */

LEAF(r5900_FlushCache)
	di
	li	t1, R5900_C_SIZE_I
	li	t2, R5900_C_SIZE_D

	/*
	 * Flush the instruction cache.
	 */
	li	t0, MIPS_KSEG0_START
	srl	t1, 1			# Two way set assoc
	addu	t1, t0, t1		# End address
	la	v0, 1f
	or	v0, MIPS_KSEG1_START	# Run uncached.
	j	v0
	 nop
1:	
	sync.l
	sync.p
1:
	#	[12:6]	... line
	#	[0]	... way
	# line +0
	cache	R5900_C_IINV_I,  0(t0);sync.l;sync.p	# way 0
	cache	R5900_C_IINV_I,  1(t0);sync.l;sync.p	# way 1
	# line +1	
	cache	R5900_C_IINV_I, 64(t0);sync.l;sync.p	# way 0
	cache	R5900_C_IINV_I, 65(t0);sync.l;sync.p	# way 1
	# line +2
	cache	R5900_C_IINV_I,128(t0);sync.l;sync.p	# way 0
	cache	R5900_C_IINV_I,129(t0);sync.l;sync.p	# way 1
	# line +3	
	cache	R5900_C_IINV_I,192(t0);sync.l;sync.p	# way 0
	cache	R5900_C_IINV_I,193(t0);sync.l;sync.p	# way 1
	addu	t0, t0, 256	
	bne	t0, t1, 1b
	 nop
	
	la	v0, 1f			# Run cached.
	j	v0
	 nop
1:		

	/*
	 * Flush the data cache.
	 */
	li	t0, MIPS_KSEG0_START
	srl	t2, 1			# Two way set assoc
	addu	t1, t0, t2		# End address
	sync.l
	sync.p
1:
	# line +0
	cache	R5900_C_IWBINV_D,  0(t0);sync.l;sync.p	# way 0
	cache	R5900_C_IWBINV_D,  1(t0);sync.l;sync.p	# way 1
	# line +1
	cache	R5900_C_IWBINV_D, 64(t0);sync.l;sync.p	# way 0
	cache	R5900_C_IWBINV_D, 65(t0);sync.l;sync.p	# way 1
	# line +2
	cache	R5900_C_IWBINV_D,128(t0);sync.l;sync.p	# way 0
	cache	R5900_C_IWBINV_D,129(t0);sync.l;sync.p	# way 1
	# line +3
	cache	R5900_C_IWBINV_D,192(t0);sync.l;sync.p	# way 0
	cache	R5900_C_IWBINV_D,193(t0);sync.l;sync.p	# way 1
	addu	t0, t0, 256	
	bne	t0, t1, 1b
	 nop
	j	ra
	 ei
END(r5900_FlushCache)
	
LEAF(r5900_FlushICache)
	di
	li	t0, R5900_C_SIZE_I
	li	t1, MIPS_KSEG0_START
	addu	a1, 255		# Align (I $ inval of partials is ok)
	srl	t0, 1		# Two way set assoc offset
	addu	t2, t0, -1	# Cache index mask
	and	a0, a0, t2	# Only keep index bits (avoid KSEG2, way 0)
	or	a0, a0, t1	# Make KSEG0
	srl	a1, a1, 8	# Number of unrolled loops
	
	la	v0, 1f			# Run uncached.
	or	v0, MIPS_KSEG1_START
	j	v0
	 nop
1:	
	sync.l
	sync.p
1:
	cache	R5900_C_IINV_I,  0(a0);sync.l;sync.p
	cache	R5900_C_IINV_I,  1(a0);sync.l;sync.p
	cache	R5900_C_IINV_I, 64(a0);sync.l;sync.p
	cache	R5900_C_IINV_I, 65(a0);sync.l;sync.p
	addu	a1, -1
	cache	R5900_C_IINV_I,128(a0);sync.l;sync.p
	cache	R5900_C_IINV_I,129(a0);sync.l;sync.p
	cache	R5900_C_IINV_I,192(a0);sync.l;sync.p
	cache	R5900_C_IINV_I,193(a0);sync.l;sync.p
	bgt	a1, zero, 1b
	 addu	a0, 256
	
	la	v0, 1f			# Run cached.
	j	v0
	 nop
1:		
	j	ra
	ei
END(r5900_FlushICache)

LEAF(r5900_FlushDCache)
	di
	li	a2, R5900_C_SIZE_D
	srl	a3, a2, 1	# two way set associative cache
	addu	a2, a3, -1	# offset mask
	and	a0, a0, a2	# get index into primary cache
	addu	a1, a1, a0	# add offset to length
	and	a0, a0, -256	# block align address
	subu	a1, a1, a0	# subtract aligned offset -> inc len by align
	addu	a1, 255		# tail align length
	li	a2, MIPS_KSEG0_START
	addu	a0, a0, a2	# K0(vindex)
	srl	a1, a1, 8	# Compute number of cache blocks
	sync.l	
	sync.p	
1:
	cache	R5900_C_IWBINV_D,  0(a0);sync.l;sync.p
	cache	R5900_C_IWBINV_D,  1(a0);sync.l;sync.p
	cache	R5900_C_IWBINV_D, 64(a0);sync.l;sync.p
	cache	R5900_C_IWBINV_D, 65(a0);sync.l;sync.p
	addu	a1, -1
	cache	R5900_C_IWBINV_D,128(a0);sync.l;sync.p
	cache	R5900_C_IWBINV_D,129(a0);sync.l;sync.p
	cache	R5900_C_IWBINV_D,192(a0);sync.l;sync.p
	cache	R5900_C_IWBINV_D,193(a0);sync.l;sync.p
	bgtz	a1, 1b
	 addu	a0, 256
	j	ra
	ei
END(r5900_FlushDCache)
	
LEAF(r5900_HitFlushDCache)
	di
	beq	a1, zero, 2f
	addu	a1, 63		# Branch delay slot; align length
	and	a0, a0, -64
	srl	a1, a1, 6	# Compute number of cache lines
	sync.l
	sync.p
1:
	addu	a1, -1
	cache	R5900_C_HWBINV_D, 0(a0)
	sync.l
	sync.p
	bne	a1, zero, 1b
	 addu	a0, 64
2:
	j	ra
	ei
END(r5900_HitFlushDCache)
	
LEAF(r5900_InvalidateDCache)
	di
	addu	a1, a1, a0		# compute ending address
	sync.l
	sync.p
1:
	cache	R5900_C_HINV_D,0(a0)
	sync.l
	sync.p
	nop
	addu	a0, a0, 64
	bltu	a0, a1, 1b
	 nop
	j	ra
	ei
END(r5900_InvalidateDCache)

/*
 * R5900 don't have virtual coherency exceptions.
 */	
LEAF_NOPROFILE(mips3_VCED)
	PANIC("unknown exception")
END(mips3_VCED)

LEAF_NOPROFILE(mips3_VCEI)
	PANIC("unknown exception")
END(mips3_VCEI)
