/*	$NetBSD: oper.c,v 1.2 2021/01/09 22:19:11 rillig Exp $	*/

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

mod_t modtab[NOPS];

static const struct {
	mod_t	m;
	unsigned char	ok;
} imods[] =
#define begin_ops() {
#define op(name, repr, \
		bi, lo, in, sc, ar, fo, va, ts, ba, se, \
		lu, ru, pc, cm, ve, de, ew, ic, active) \
	{ { bi + 0, lo + 0, in + 0, sc + 0, ar + 0, \
	    fo + 0, va + 0, ts + 0, ba + 0, se + 0, \
	    lu + 0, ru + 0, pc + 0, cm + 0, ve + 0, \
	    de + 0, ew + 0, ic + 0, repr }, active },
#define end_ops(n) };
#include "ops.def"

const char *
getopname(op_t op) {
	return imods[op].m.m_name;
}

void
initmtab(void)
{
	size_t i;

	for (i = 0; i < sizeof imods / sizeof imods[0]; i++)
		if (imods[i].ok)
			modtab[i] = imods[i].m;
}
