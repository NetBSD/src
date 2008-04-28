/*	$NetBSD: swapcontext.S,v 1.3 2008/04/28 20:22:56 martin Exp $	*/

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
#include <machine/mcontext.h>

#if defined(SYSLIBC_SCCS) && !defined(lint)
	ASMSTR("$NetBSD: swapcontext.S,v 1.3 2008/04/28 20:22:56 martin Exp $")
#endif /* SYSLIBC_SCCS && !lint */

#ifdef __ABICALLS__
	.abicalls
#endif
	.set	reorder
	
NESTED(swapcontext, CALLFRAME_SIZ, ra)
	.mask	0x80000030, (CALLFRAME_RA - CALLFRAME_SIZ)
	subu	sp, sp, CALLFRAME_SIZ
	sw	ra, CALLFRAME_RA(sp)	# save ra
	sw	a0, 0(sp)		# stash away oucp
	sw	a1, 4(sp)		# stash away ucp
	SYSTRAP(getcontext)
	lw	ra, CALLFRAME_RA(sp)
	bne	a3, zero, 1f

	lw	v1, 0(sp)		# load oucp again for adjustment
	lw	a0, 4(sp)		# load ucp
	addiu	v0, sp, CALLFRAME_SIZ
	REG_PROLOGUE
	REG_S	zero, (_OFFSETOF_UC_GREGS + _REG_V0 * SZREG)(v1)
	REG_S	ra, (_OFFSETOF_UC_GREGS + _REG_EPC * SZREG)(v1)
	REG_S	v0, (_OFFSETOF_UC_GREGS + _REG_SP * SZREG)(v1)
	REG_EPILOGUE

	SYSTRAP(setcontext)
	/* NOTREACHED */
1:
	addiu	sp, sp, CALLFRAME_SIZ
	la	t9, _C_LABEL(__cerror)
	jr	t9
END(swapcontext)
