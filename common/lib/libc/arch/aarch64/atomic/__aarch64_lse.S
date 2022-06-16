/* $NetBSD: __aarch64_lse.S,v 1.3 2022/06/16 08:21:57 skrll Exp $ */

/*-
 * Copyright (c) 2021 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nick Hudson.
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

#include <sys/cdefs.h>

#include "atomic_op_asm.h"

#if SZ == 1
#define OPSFX	b
#define R0	w0
#define R1	w1
#define R4	w4
#endif

#if SZ == 2
#define OPSFX	h
#define R0	w0
#define R1	w1
#define R4	w4
#endif

#if SZ == 4
#define OPSFX
#define R0	w0
#define R1	w1
#define R4	w4
#endif

#if SZ == 8
#define OPSFX
#define R0	x0
#define R1	x1
#define R4	x4
#endif

#if defined(AR_relax)
#define ACQ
#define REL
#endif

#if defined(AR_acq)
#define ACQ	a
#define REL
#endif

#if defined(AR_rel)
#define ACQ
#define REL	l
#endif

#if defined(AR_acq_rel)
#define ACQ	a
#define REL	l
#endif

#if defined(OP_clr)
#define INSNOP	bic
#endif

#if defined(OP_set)
#define INSNOP	orr
#endif

#if defined(OP_add)
#define INSNOP	add
#endif

#if defined(OP_eor)
#define INSNOP	eor
#endif

#define _CONCAT3(A, B, C)	__CONCAT3(A,B,C)
#define _CONCAT4(A, B, C, D)	__CONCAT4(A,B,C,D)
#define _CONCAT5(A, B, C, D, E)	__CONCAT5(A,B,C,D,E)

#define FUNC2			_CONCAT3(__aarch64_,OP,AR)
#define FUNC3			_CONCAT4(__aarch64_,OP,SZ,AR)

#define CASP_FUNC		FUNC2
#define CAS_FUNC		FUNC3
#define SWP_FUNC		FUNC3
#define INSN_FUNC		FUNC3

#define LDXR			_CONCAT4(ld, ACQ, xr, OPSFX)
#define STXR			_CONCAT4(st, REL, xr, OPSFX)
#define LDXP			_CONCAT3(ld, ACQ, xp)
#define STXP			_CONCAT3(st, REL, xp)

#ifdef _HAVE_LSE
#define SWP			_CONCAT4(swp, ACQ, REL, OPSFX)
#define CAS			_CONCAT4(cas, ACQ, REL, OPSFX)
#define CASP			_CONCAT3(casp, ACQ, REL)
#define INSN			_CONCAT5(ld, OP, ACQ, REL, OPSFX)

	.hidden	__aarch64_have_lse_atomics
	.arch armv8-a+lse

#define DO_LSE_INSN_IF_SUPPORTED(label)				\
	adrp	x4, __aarch64_have_lse_atomics			;\
	ldrb	w4, [x4, #:lo12:__aarch64_have_lse_atomics]	;\
	cbnz	w4, label

#endif

#if defined(OP_swp)
ENTRY_NP(SWP_FUNC)
#ifdef _HAVE_LSE
	DO_LSE_INSN_IF_SUPPORTED(99f)
	SWP	R0, R0, [x1]
	ret
99:
#endif
	mov	x4, x0			/* need x0 for return value	 */
1:	LDXR	R0, [x1]		/* load old value		 */
	STXR	w3, R4, [x1]		/* store new value		 */
	cbnz	w3, 2f			/*   succeed?? no, try again	 */
	ret				/* return old value		 */
2:	b	1b
END(SWP_FUNC)
#endif

#if defined(OP_cas)
ENTRY_NP(CAS_FUNC)
#ifdef _HAVE_LSE
	DO_LSE_INSN_IF_SUPPORTED(99f)
	CAS	R0, R1, [x2]
	ret
99:
#endif
	mov	x4, x0			/* need x0 for return value	*/
1:	LDXR	R0, [x2]		/* load old value		*/
	cmp	R0, R4			/* compare			*/
	b.ne	2f			/*   not equal? return		*/
	STXR	w3, R1, [x2]		/* store new value		*/
	cbnz	w3, 3f			/*   succeed? nope, try again.	*/
2:	ret				/* return.			*/
3:	b	1b
END(CAS_FUNC)
#endif

#if defined(OP_casp)
ENTRY_NP(CASP_FUNC)
#ifdef _HAVE_LSE
	DO_LSE_INSN_IF_SUPPORTED(99f)
	CASP	x0, x1, x2, x3, [x4]
	ret
99:
#endif
	mov	x4, x0			/* need x0 for return value	*/
1:	LDXP	x5, x6, [x4]		/* load old value		*/
	cmp	x5, x0			/* compare			*/
	b.ne	2f			/*   not equal? return		*/
	cmp	x6, x1
	b.ne	2f			/*   not equal? return		*/
	STXP	w7, x2, x3, [x4]	/* store new value		*/
	cbnz	w7, 3f			/*   succeed? nope, try again.	*/
2:	ret				/* return.			*/
3:	b	1b
END(CASP_FUNC)
#endif

#if defined(OP_set) || defined(OP_clr) || defined(OP_add) || defined(OP_eor)
ENTRY_NP(INSN_FUNC)
#ifdef _HAVE_LSE
	DO_LSE_INSN_IF_SUPPORTED(99f)
	INSN	R0, R0, [x1]
	ret
99:
#endif
	mov	x4, x0			/* need x0 for return value	 */
1:	LDXR	R0, [x1]		/* load old value		 */
	INSNOP	R4, R0, R4
	STXR	w3, R4, [x1]		/* store new value		 */
	cbnz	w3, 2f			/*   succeed?? no, try again	 */
	ret				/* return old value		 */
2:	b	1b
END(INSN_FUNC)
#endif
