/*	$NetBSD: swapcontext.S,v 1.1.2.1 2001/11/20 07:59:05 wdk Exp $	*/

/*
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
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

#include "SYS.h"
#include <mips/asm.h>
#include <mips/cpuregs.h>

#if defined(LIBC_SCCS) && !defined(lint)
	ASMSTR("$NetBSD: swapcontext.S,v 1.1.2.1 2001/11/20 07:59:05 wdk Exp $")
#endif /* LIBC_SCCS && !lint */

NESTED(swapcontext, CALLFRAME_SIZ+24, ra)
	PIC_PROLOGUE(swapcontext, t9)
	
	subu	sp, sp, CALLFRAME_SIZ+24
	sw	a0, 16(sp)
	sw	a1, 20(sp)
	sw	ra, 24(sp)

	PIC_CALL(getcontext, t9)
	bne	v0, zero, 1f
	
	/* XXX:	 Offsets are totally wrong! */
	lw	a0, 16(sp)		/* a0 = old_uc */
	lw	ra, 24(sp)
	sw	ra, (40 + 31 * 4)(a0)	/* fixup saved RA */
	la	t0, CALLFRAME_SIZ+24(sp)
	sw	t0, (40 + 29 * 4)(a0)	/* fixup saved SP */

	lw	a0, 20(sp)		/* a0 = new_uc */
	PIC_CALL(setcontext, t9)

	/* don't check for an error; if we got here, something broke. */
1:	lw	ra, 24(sp)
	addu	sp, sp, CALLFRAME_SIZ+24
	j	ra
	nop
END(swapcontext)
