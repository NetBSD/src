/*	$NetBSD: oper.c,v 1.16 2024/03/31 20:28:45 rillig Exp $	*/

/*-
 * Copyright (c) 2021 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Roland Illig.
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

#include "op.h"
#include "param.h"

#define X true
#define _ false

const mod_t modtab[NOPS] = {

/*-
 * Operator properties:
 *
 *	 b   binary
 *	   l   logical
 *	     b   takes _Bool
 *	       z   compares with zero
 *		 i   requires integer
 *		   c   requires integer or complex
 *		     a   requires arithmetic
 *		       s   requires scalar
 *			 f   fold constant operands
 *			   v   value context
 *			     b   balance operands
 *			       s   has side effects
 *				 l   warn if left operand unsigned
 *				   r   warn if right operand unsigned
 *				     p   possible precedence confusion
 *				       c   comparison
 *					 e   valid on enum
 *					   e   bad on enum
 *					     =   warn if operand '='
 *					       o   has operands
 */

/*	 b l b z i c a s f v b s l r p c e e = o */
	{_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_, "no-op" },
	{X,_,X,_,_,_,_,_,_,X,_,_,_,_,_,_,_,_,_,X, "->" },
	{X,_,X,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X, "." },
	{_,X,X,X,_,_,_,X,X,_,_,_,_,_,_,_,_,X,_,X, "!" },
	{_,_,_,_,_,X,_,_,X,X,_,_,_,_,_,_,_,X,X,X, "~" },
	/*
	 * The '++' and '--' operators are conceptually unary operators, but
	 * lint implements them as binary operators due to the pre-multiplied
	 * pointer arithmetics, see build_prepost_incdec and build_plus_minus.
	 */
	{_,_,_,_,_,_,_,X,_,_,_,X,_,_,_,_,_,X,_,X, "++x" },
	{_,_,_,_,_,_,_,X,_,_,_,X,_,_,_,_,_,X,_,X, "--x" },
	{_,_,_,_,_,_,_,X,_,_,_,X,_,_,_,_,_,X,_,X, "x++" },
	{_,_,_,_,_,_,_,X,_,_,_,X,_,_,_,_,_,X,_,X, "x--" },
	{_,_,_,_,_,_,X,_,X,X,_,_,_,_,_,_,_,X,X,X, "+" },
	{_,_,_,_,_,_,X,_,X,X,_,_,X,_,_,_,_,X,X,X, "-" },
	{_,_,_,_,_,_,_,_,_,X,_,_,_,_,_,_,_,_,_,X, "*" },
	{_,_,X,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X, "&" },
/*	 b l b z i c a s f v b s l r p c e e = o */
	{X,_,_,_,_,_,X,_,X,X,X,_,_,X,_,_,_,X,X,X, "*" },
	{X,_,_,_,_,_,X,_,X,X,X,_,X,X,_,_,_,X,X,X, "/" },
	{X,_,_,_,X,_,_,_,X,X,X,_,X,X,_,_,_,X,X,X, "%" },
	{X,_,_,_,_,_,_,X,X,X,X,_,_,_,_,_,_,X,_,X, "+" },
	{X,_,_,_,_,_,_,X,X,X,X,_,_,_,_,_,_,X,_,X, "-" },
	{X,_,_,_,X,_,_,_,X,X,_,_,_,_,X,_,_,X,X,X, "<<" },
	{X,_,_,_,X,_,_,_,X,X,_,_,X,_,X,_,_,X,X,X, ">>" },
/*	 b l b z i c a s f v b s l r p c e e = o */
	{X,X,_,_,_,_,_,X,X,X,X,_,X,X,_,X,X,_,X,X, "<" },
	{X,X,_,_,_,_,_,X,X,X,X,_,X,X,_,X,X,_,X,X, "<=" },
	{X,X,_,_,_,_,_,X,X,X,X,_,X,X,_,X,X,_,X,X, ">" },
	{X,X,_,_,_,_,_,X,X,X,X,_,X,X,_,X,X,_,X,X, ">=" },
	{X,X,X,_,_,_,_,X,X,X,X,_,_,_,_,X,X,_,X,X, "==" },
	{X,X,X,_,_,_,_,X,X,X,X,_,_,_,_,X,X,_,X,X, "!=" },
/*	 b l b z i c a s f v b s l r p c e e = o */
	{X,_,X,_,X,_,_,_,X,X,X,_,_,_,X,_,_,X,_,X, "&" },
	{X,_,X,_,X,_,_,_,X,X,X,_,_,_,X,_,_,X,_,X, "^" },
	{X,_,X,_,X,_,_,_,X,X,X,_,_,_,X,_,_,X,_,X, "|" },
	{X,X,X,X,_,_,_,X,X,_,_,_,_,_,_,_,_,X,_,X, "&&" },
	{X,X,X,X,_,_,_,X,X,_,_,_,_,_,X,_,_,X,_,X, "||" },
	{X,_,_,X,_,_,_,_,X,_,_,_,_,_,_,_,_,_,_,X, "?" },
	{X,_,X,_,_,_,_,_,_,X,X,_,_,_,_,_,X,_,_,X, ":" },
/*	 b l b z i c a s f v b s l r p c e e = o */
	{X,_,X,_,_,_,_,_,_,_,_,X,_,_,_,_,X,_,_,X, "=" },
	{X,_,_,_,_,_,X,_,_,_,_,X,_,_,_,_,_,X,_,X, "*=" },
	{X,_,_,_,_,_,X,_,_,_,_,X,_,X,_,_,_,X,_,X, "/=" },
	{X,_,_,_,X,_,_,_,_,_,_,X,_,X,_,_,_,X,_,X, "%=" },
	{X,_,_,_,_,_,_,X,_,_,_,X,_,_,_,_,_,X,_,X, "+=" },
	{X,_,_,_,_,_,_,X,_,_,_,X,_,_,_,_,_,X,_,X, "-=" },
	{X,_,_,_,X,_,_,_,_,_,_,X,_,_,_,_,_,X,_,X, "<<=" },
	{X,_,_,_,X,_,_,_,_,_,_,X,_,_,_,_,_,X,_,X, ">>=" },
	{X,_,X,_,X,_,_,_,_,_,_,X,_,_,_,_,_,X,_,X, "&=" },
	{X,_,X,_,X,_,_,_,_,_,_,X,_,_,_,_,_,X,_,X, "^=" },
	{X,_,X,_,X,_,_,_,_,_,_,X,_,_,_,_,_,X,_,X, "|=" },
/*	 b l b z i c a s f v b s l r p c e e = o */
	{_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_, "name" },
	{_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_, "constant" },
	{_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_, "string" },
	{_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X, "fsel" },
	{_,_,_,_,_,_,_,_,_,_,_,X,_,_,_,_,_,_,_,_, "call" },
	{X,_,X,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X,X, "," },
	{_,_,_,_,_,_,_,_,_,X,_,_,_,_,_,_,_,_,_,X, "convert" },
	{_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X, "load" },
	{X,_,X,_,_,_,_,_,_,_,_,X,_,_,_,_,X,_,_,X, "return" },
	{_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X, "real" },
	{_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,X, "imag" },
	{X,_,X,_,_,_,_,_,_,_,_,X,_,_,_,_,X,_,_,X, "init" },
	{_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_, "case" },
	{X,_,X,_,_,_,_,_,_,_,_,_,_,_,_,_,X,_,_,X, "farg" },
};
