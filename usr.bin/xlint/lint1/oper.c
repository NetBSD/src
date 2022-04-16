/*	$NetBSD: oper.c,v 1.11 2022/04/16 22:21:10 rillig Exp $	*/

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

#include <sys/types.h>
#include "op.h"
#include "param.h"

const mod_t modtab[NOPS] =
#define begin_ops() {
#define op(name, repr, \
		is_binary, is_logical, takes_bool, requires_bool, \
		is_integer, is_complex, is_arithmetic, is_scalar, \
		can_fold, is_value, unused, balances_operands, \
		side_effects, left_unsigned, right_unsigned, \
		precedence_confusion, is_comparison, \
		valid_on_enum, bad_on_enum, warn_if_eq) \
	{ \
		is_binary	+ 0 > 0, is_logical		+ 0 > 0, \
		takes_bool	+ 0 > 0, requires_bool		+ 0 > 0, \
		is_integer	+ 0 > 0, is_complex		+ 0 > 0, \
		is_arithmetic	+ 0 > 0, is_scalar		+ 0 > 0, \
		can_fold	+ 0 > 0, is_value		+ 0 > 0, \
		balances_operands + 0 > 0, \
		side_effects	+ 0 > 0, left_unsigned		+ 0 > 0, \
		right_unsigned	+ 0 > 0, precedence_confusion	+ 0 > 0, \
		is_comparison	+ 0 > 0, valid_on_enum		+ 0 > 0, \
		bad_on_enum	+ 0 > 0, warn_if_eq		+ 0 > 0, \
		repr, \
	},
#define end_ops(n) };
#include "ops.def"
