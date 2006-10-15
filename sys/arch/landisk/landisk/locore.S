/*	$NetBSD: locore.S,v 1.3 2006/10/15 02:31:00 uwe Exp $	*/

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

#include <sh3/asm.h>
__KERNEL_RCSID(0, "$NetBSD: locore.S,v 1.3 2006/10/15 02:31:00 uwe Exp $")

#include <sh3/psl.h>
#include <sh3/mmu_sh4.h>


/*
 * LINTSTUB: Func: void start(int howto, void *bootinfo)
 *				__attribute__((__noreturn__))
 *	Kernel entry point.
 */
NENTRY(start)
ALTENTRY(kernel_text)
	/* Init SR */
	stc	sr, r0
	mov.l	.L_SR_clear, r1
	mov.l	.L_SR_set, r2
	and	r1, r0
	or	r2, r0
	ldc	r0, sr

	/* Disable MMU */
	mov.l	.L_MMUCR, r0
	mov	#0, r1
	mov.l	r1, @r0

	/* Go! */
	mov.l	.L_landisk_startup, r0
	mov.l	.L_bootstrap_stack, sp
	jmp	@r0
	 nop

	/* NOTREACHED */	
	.align	2

/* Select register bank 0 */
.L_SR_clear:		.long	~PSL_RB

/* Mask all interrupts */
.L_SR_set:		.long	PSL_IMASK

.L_MMUCR:		.long	SH4_MMUCR
.L_landisk_startup:	.long	_C_LABEL(landisk_startup)
.L_bootstrap_stack:	.long	_C_LABEL(start)

	SET_ENTRY_SIZE(start)
