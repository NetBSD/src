/*	$NetBSD: op.h,v 1.10 2021/01/05 23:50:29 rillig Exp $	*/

/*
 * Copyright (c) 1994, 1995 Jochen Pohl
 * All Rights Reserved.
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
 *      This product includes software developed by Jochen Pohl for
 *	The NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdbool.h>

/*
 * Various information about operators
 */
typedef	struct {
	bool	m_binary : 1;	/* binary operator */
	bool	m_logical : 1;	/* logical operator, result is int */
	bool	m_requires_integer : 1;
	bool	m_requires_scalar : 1;
	bool	m_requires_arith : 1;
	bool	m_fold : 1;	/* operands should be folded */
	bool	m_vctx : 1;	/* value context for left operand */
	bool	m_tctx : 1;	/* test context for left operand */
	bool	m_balance : 1;	/* operator requires balancing */
	bool	m_sideeff : 1;	/* operator has side effect */
	bool	m_tlansiu : 1;	/* warn if left op. is unsign. in ANSI C */
	bool	m_transiu : 1;	/* warn if right op. is unsign. in ANSI C */
	bool	m_possible_precedence_confusion : 1;
	bool	m_comp : 1;	/* operator performs comparison */
	bool	m_valid_on_enum : 1;	/* valid operation on enums */
	bool	m_bad_on_enum : 1;	/* dubious operation on enums */
	bool	m_eqwarn : 1;	/* warning if on operand stems from == */
	bool	m_requires_integer_or_complex : 1;
	const char *m_name;	/* name of op. */
} mod_t;

extern mod_t   modtab[];

#define begin_ops() typedef enum {
#define op(name, repr, \
		bi, lo, in, sc, ar, fo, va, ts, ba, se, \
		lu, ru, pc, cm, ve, de, ew, ic, active) \
	name,
#define end_ops() } op_t;
#include "ops.def"

const char *getopname(op_t);
void initmtab(void);
