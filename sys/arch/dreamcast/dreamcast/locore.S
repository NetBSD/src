/*	$NetBSD: locore.S,v 1.2.8.3 2002/06/23 17:35:35 jdolecek Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
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

#include "opt_memsize.h"
#include "assym.h"

#include <sh3/asm.h>
#include <sh3/locore.h>
#include <sh3/exception.h>
#include <sh3/mmu_sh4.h>
#include <sh3/cache_sh4.h>

/*
 * void start(void) __attribute__((__noreturn__)):
 *	Kernel entry point.
 */	
NENTRY(start)
	/* Set bootstrap stack pointer */
	mov.l	_L.bootstrap_stack, r15

	/* Mask all interrupt */
	__INTR_MASK(r0, r1)

	/* 
	 * Cache initialize. 
	 *	I/D-cache enable, P0, U0, P3 write-back P1 write-through.
	 */
	mov.l	_L.sh4_icache_sync_all, r0
	jsr	@r0
	 nop
	mov.l	_L.CCR,	r0
	mov.l	_L.CCRconfig, r1
	mov.l	r1, @r0

	/* MMU off */
	mov.l	_L.MMUCR, r0
	xor	r1,	r1
	mov.l	r1,	@r0
	
	/* Make sure to change register bank 0 */
	mov	#0x20,	r1
	stc	sr,	r0
	swap.b	r1,	r1
	swap.w	r1,	r1
	not	r1,	r1
	and	r1,	r0
	ldc	r0,	sr	/* SR.RB = 0 */

	/* Go! */
	mov.l	_L.dreamcast_startup, r0
	jmp	@r0
	 nop
	/* NOTREACHED */	
	.align	2
_L.dreamcast_startup:	.long	_C_LABEL(dreamcast_startup)	
_L.bootstrap_stack:	.long	_C_LABEL(start)
_L.sh4_icache_sync_all:	.long	_C_LABEL(sh4_icache_sync_all)
_L.CCR:			.long	SH4_CCR	
_L.CCRconfig:		.long	0x00000909	
_L.MMUCR:		.long	SH4_MMUCR
	.data
	.align	2
	.globl	_C_LABEL(intrcnt), _C_LABEL(eintrcnt)
	.globl	_C_LABEL(intrnames), _C_LABEL(eintrnames)
_C_LABEL(intrcnt):
_C_LABEL(eintrcnt):
_C_LABEL(intrnames):
_C_LABEL(eintrnames):
