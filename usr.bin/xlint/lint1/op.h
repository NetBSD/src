/*	$NetBSD: op.h,v 1.28 2024/03/31 20:28:45 rillig Exp $	*/

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
 *	This product includes software developed by Jochen Pohl for
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
 * Various information about operators.
 */
typedef struct {
	bool	m_binary: 1;
	bool	m_returns_bool: 1;
	bool	m_takes_bool: 1;
	bool	m_compares_with_zero: 1;
	bool	m_requires_integer: 1;
	bool	m_requires_integer_or_complex: 1;
	bool	m_requires_arith: 1;
	bool	m_requires_scalar: 1;
	bool	m_fold_constant_operands: 1;
	bool	m_value_context: 1;
	bool	m_balance_operands: 1;
	bool	m_has_side_effect: 1;
	bool	m_warn_if_left_unsigned_in_c90: 1;
	bool	m_warn_if_right_unsigned_in_c90: 1;
	bool	m_possible_precedence_confusion: 1;
	bool	m_comparison: 1;
	bool	m_valid_on_enum: 1;
	bool	m_bad_on_enum: 1;
	bool	m_warn_if_operand_eq: 1;
	bool	m_has_operands: 1;
	const char m_name[8];
} mod_t;

typedef enum {
	NOOP,
	ARROW,
	POINT,
	NOT,
	COMPL,
	INCBEF,
	DECBEF,
	INCAFT,
	DECAFT,
	UPLUS,
	UMINUS,
	INDIR,
	ADDR,

	MULT,
	DIV,
	MOD,
	PLUS,
	MINUS,
	SHL,
	SHR,

	LT,
	LE,
	GT,
	GE,
	EQ,
	NE,

	BITAND,
	BITXOR,
	BITOR,
	LOGAND,
	LOGOR,
	QUEST,
	COLON,

	ASSIGN,
	MULASS,
	DIVASS,
	MODASS,
	ADDASS,
	SUBASS,
	SHLASS,
	SHRASS,
	ANDASS,
	XORASS,
	ORASS,

	NAME,
	CON,
	STRING,
	FSEL,
	CALL,
	COMMA,
	CVT,
	LOAD,
	RETURN,
	REAL,
	IMAG,

	INIT,			/* does not appear in the tree */
	CASE,			/* does not appear in the tree */
	/*
	 * FARG is only used temporarily in check_prototype_argument to check
	 * type compatibility and conversion for function arguments.
	 */
	FARG,			/* does not appear in the tree */
} op_t;

#define NOPS ((int)FARG + 1)

extern const mod_t modtab[NOPS];
