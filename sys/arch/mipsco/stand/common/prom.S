/*	$NetBSD: prom.S,v 1.1 2000/09/18 11:40:48 wdk Exp $	*/

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
#include <mips/cpuregs.h>
#include <machine/prom.h>


/* XXX:	address has only been tested on Version 5.4 firmware */
	.global	saiob
	.equ	saiob, 0x80004cd8

	.set noreorder
LEAF(prom_restart)
	li	v0, MIPS_PROM_RESTART
	j	v0
	move	a0, zero
END(prom_restart)

LEAF(prom_open)
	li	v0, MIPS_PROM_OPEN
	j	v0
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
