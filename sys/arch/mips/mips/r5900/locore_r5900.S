/*	$NetBSD: locore_r5900.S,v 1.2.4.2 2002/01/10 19:46:15 thorpej Exp $	*/

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

/*
 * _intr_resume, _intr_suspend:	
 *	 Fast interrupt locking functions.
 */	
LEAF_NOPROFILE(_intr_resume)
	beqz	a0, 1f
	nop
	ei
1:	
	j	ra
	 nop
END(_intr_resume)

LEAF_NOPROFILE(_intr_suspend)
	mfc0	v0, MIPS_COP_0_STATUS_REG	
	lui	t0, 0x1		/* EIE bit */
	and	v0, v0, t0
1:	di
	sync.p
	mfc0	t1, MIPS_COP_0_STATUS_REG
	and	t2, t0, t1
	beq	t2, t0, 1b
	 nop
	j	ra
	 nop
END(_intr_suspend)

/*
 * R5900 don't have virtual coherency exceptions.
 */	
LEAF_NOPROFILE(mips3_VCED)
	PANIC("unknown exception")
END(mips3_VCED)

LEAF_NOPROFILE(mips3_VCEI)
	PANIC("unknown exception")
END(mips3_VCEI)
