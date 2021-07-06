/*	$NetBSD: swapcontext.S,v 1.5 2021/07/06 12:38:40 thorpej Exp $	*/

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

#include "SYS.h"
#include "assym.h"

#define	FRAME_SIZE	32
#define	FRAME_RA	(32-16)
#define	FRAME_OUCP	(32-24)
#define	FRAME_UCP	(32-32)

NESTED(swapcontext, 2, FRAME_SIZE, ra, IM_RA|IM_A0|IM_A1, 0)
	LDGP(pv)
	lda	sp, -FRAME_SIZE(sp)
	stq	ra, FRAME_RA(sp)	/* must save ra, oucp, ucp */
	stq	a0, FRAME_OUCP(sp)
	stq	a1, FRAME_UCP(sp)
	CALL(_getcontext)		/* getcontext(oucp) */
	ldq	t0, FRAME_OUCP(sp)
	bne	v0, Lerr
	ldq	t1, FRAME_RA(sp)
	stq	t1, (UC_GREGS + _REG_RA*8)(t0)	/* Adjust saved RA */
	stq	t1, (UC_GREGS + _REG_PC*8)(t0)	/* Adjust saved PC */
	lda	t1, FRAME_SIZE(sp)
	stq	t1, (UC_GREGS + _REG_SP*8)(t0)	/* Adjust saved SP */

	ldq	a0, FRAME_UCP(sp)
	CALL(setcontext)		/* setcontext(ucp) */
Lerr:
	ldq	ra, FRAME_RA(sp)
	lda	sp, FRAME_SIZE(sp)
	RET
END(swapcontext)
