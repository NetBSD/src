/*	$NetBSD: swapcontext.S,v 1.2 2003/01/20 00:26:39 thorpej Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Klaus Klein.
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

#include <machine/asm.h>
#if defined(SYSLIBC_SCCS) && !defined(lint)
	RCSID("$NetBSD: swapcontext.S,v 1.2 2003/01/20 00:26:39 thorpej Exp $")
#endif /* SYSLIBC_SCCS and not lint */

ENTRY(swapcontext)
	mov.l	r5, @-r15
	sts.l	pr, @-r15
	mov.l	_L._getcontext, r0
	jsr	@r0
	 mov.l	r4, @-r15
	mov.l	@r15+, r1
	tst	r0, r0
	bf	1f
	mov.l	@r15, r0
	mov.l	r0, @(36 + 1 * 4, r1)	/* saved pc <- pr */
	mov.l	_L.setcontext, r2
	jsr	@r2
	 mov.l	@(4, r15), r4
1:
	lds.l	@r15+, pr
	rts
	 add	#4, r15

	.align	2
_L._getcontext:		.long _C_LABEL(_getcontext)
_L.setcontext:		.long _C_LABEL(setcontext)
