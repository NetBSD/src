/*	$NetBSD: __sigsetjmp14.S,v 1.3 2003/03/24 14:29:34 scw Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <machine/asm.h>
#include <machine/setjmp.h>

/*
 * C library -- sigsetjmp, siglongjmp
 *
 *	siglongjmp(a,v)
 * will generate a "return(v)" from
 * the last call to
 *	sigsetjmp(a, mask)
 * by restoring registers from the stack.
 * If `mask' is non-zero, the previous signal
 * state will be restored.
 */

ENTRY_NOPROFILE(__sigsetjmp14)
	LINK_FRAME(8)
	PIC_PROLOGUE
	st.q	r14, 0, r2
#ifndef _LP64
	st.l	r2, _JBLEN*4, r3	/* Save mask */
#else
	st.q	r2, _JBLEN*8, r3	/* Save mask */
#endif
	pta/l	1f, tr0
	pta/l	2f, tr1
	beq/l	r3, r63, tr0		/* If mask == 0, don't save signals */

	PIC_PTAL(_C_LABEL(__setjmp14), r0, tr0)
	blink	tr1, r63

1:	PIC_PTAL(_C_LABEL(_setjmp), r0, tr0)

2:	blink	tr0, r18		/* Call appropriate setjmp() */

	/*
	 * We have to compensate for the stack frame and PIC_PROLOGUE
	 */
	ld.q	r14, 0, r3
	PIC_EPILOGUE
	UNLINK_FRAME(8)
	ptabs/l	r18, tr0
#ifdef PIC
	st.q	r3, _JB_R12, r12	/* PIC 'GOT' pointer */
#endif
	st.q	r3, _JB_R14, r14
	st.q	r3, _JB_R15, r15
	st.q	r3, _JB_R18, r18
	st.q	r3, _JB_PC, r18
	blink	tr0, r63


ENTRY_NOPROFILE(__siglongjmp14)
	PIC_PROLOGUE
#ifndef _LP64
	ld.l	r2, _JBLEN*4, r4	/* Fetch mask */
#else
	ld.q	r2, _JBLEN*8, r4	/* Fetch mask */
#endif
	PIC_PTAL(_C_LABEL(__longjmp14), r0, tr0)
	PIC_PTAU(_C_LABEL(_longjmp), r0, tr1)
	beq/u	r4, r63, tr1		/* If mask == 0, don't restore sigs */
	blink	tr0, r63		/* Else, restore them */
