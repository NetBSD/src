/*	$NetBSD: prom.S,v 1.3 2008/04/28 20:23:29 martin Exp $	*/

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
#include <mips/cpuregs.h>
#include <machine/prom.h>


LEAF(prom_restart)
	li	v0, MIPS_PROM_RESTART
	j	v0
	move	a0, zero
END(prom_restart)

NESTED(prom_open, CALLFRAME_SIZ, ra)
	.mask	0x80000000, -4
	subu	sp, sp, CALLFRAME_SIZ
	sw	ra, CALLFRAME_RA(sp)
	li	v0, MIPS_PROM_OPEN
	jal	v0
	nop
	/*
	 * On return t0 contains our _iob structure
	 * v0 contains the file descriptor number
	 */
	la	t4, saiob
	sll	t5, v0, 2
	add	t4, t4, t5
	lw	ra, CALLFRAME_RA(sp)
	addu	sp, sp, CALLFRAME_SIZ
	sw	t0, 0(t4)
	j	ra
	nop
END(prom_open)

#ifndef LIBSA_NO_DEV_CLOSE	
LEAF(prom_close)
	li	v0, MIPS_PROM_CLOSE
	j	v0
	nop
END(prom_close)
#endif
	
LEAF(prom_read)
	li	v0, MIPS_PROM_READ
	j	v0
	nop
END(prom_read)

#ifndef LIBSA_NO_FS_WRITE	
LEAF(prom_write)
	li	v0, MIPS_PROM_WRITE
	j	v0
	nop
END(prom_write)
#endif
	
LEAF(printf)
	li	v0, MIPS_PROM_PRINTF
	j	v0
	nop
END(printf)

#ifndef NO_GETCHAR
LEAF(getchar)
	li	v0, MIPS_PROM_GETCHAR
	j	v0
	nop
END(getchar)
	
LEAF(putchar)
	li	v0, MIPS_PROM_PUTCHAR
	j	v0
	nop
END(putchar)
#endif

	.sdata

	.global	saiob
saiob:	.space	8*4
				
