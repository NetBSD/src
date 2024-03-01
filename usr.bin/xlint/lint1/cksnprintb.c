/*	$NetBSD: cksnprintb.c,v 1.1 2024/03/01 19:40:45 rillig Exp $	*/

/*-
 * Copyright (c) 2024 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Roland Illig <rillig@NetBSD.org>.
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

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <sys/cdefs.h>
#if defined(__RCSID)
__RCSID("$NetBSD: cksnprintb.c,v 1.1 2024/03/01 19:40:45 rillig Exp $");
#endif

#include <stdbool.h>
#include <string.h>

#include "lint1.h"

static bool
match_string_literal(const tnode_t *tn, const buffer **str)
{
	while (tn->tn_op == CVT)
		tn = tn_ck_left(tn);
	return tn->tn_op == ADDR
	    && tn->tn_left->tn_op == STRING
	    && (*str = tn->tn_left->tn_string, (*str)->data != NULL);
}

static bool
match_snprintb_call(const function_call *call,
    const buffer **out_fmt, const tnode_t **out_val)
{
	const char *func;
	const tnode_t *val;
	const buffer *str;

	if (call->func->tn_op == ADDR
	    && call->func->tn_left->tn_op == NAME
	    && (func = call->func->tn_left->tn_sym->s_name, true)
	    && ((strcmp(func, "snprintb") == 0 && call->args_len == 4)
		|| (strcmp(func, "snprintb_m") == 0 && call->args_len == 5))
	    && match_string_literal(call->args[2], &str)
	    && (val = call->args[3], true)) {
		*out_fmt = str;
		*out_val = val;
		return true;
	}
	return false;
}

static int
len(quoted_iterator it)
{
	return (int)(it.i - it.start);
}

static int
range(quoted_iterator start, quoted_iterator end)
{
	return (int)(end.i - start.start);
}

static const char *
start(quoted_iterator it, const buffer *buf)
{
	return buf->data + it.start;
}

static uintmax_t
val(quoted_iterator it)
{
	return it.value;
}

static void
check_hex_escape(const buffer *buf, quoted_iterator it)
{
	if (it.hex_digits > 1) {
		bool upper = false;
		bool lower = false;
		for (size_t i = it.start + 2; i < it.i; i++) {
			if (isupper((unsigned char)buf->data[i]))
				upper = true;
			if (islower((unsigned char)buf->data[i]))
				lower = true;
		}
		if (upper && lower)
			/* hex escape '%.*s' mixes uppercase and lower... */
			warning(357, len(it), start(it, buf));
	}
	if (it.hex_digits > 2)
		/* hex escape '%.*s' has more than 2 digits */
		warning(358, len(it), start(it, buf));
}

static bool
check_directive(const buffer *fmt, quoted_iterator *it, bool new_style,
		uint64_t *prev_field_width)
{

	if (!quoted_next(fmt, it))
		return false;
	quoted_iterator dir = *it;

	bool has_bit = !new_style
	    || dir.value == 'b' || dir.value == 'f' || dir.value == 'F';
	if (has_bit && new_style && !quoted_next(fmt, it)) {
		/* missing bit position after '%.*s' */
		warning(364, len(dir), start(dir, fmt));
		return false;
	}
	/* LINTED 86 "automatic 'bit' hides external declaration" */
	quoted_iterator bit = *it;

	bool has_width = new_style
	    && (dir.value == 'f' || dir.value == 'F');
	if (has_width && !quoted_next(fmt, it)) {
		/* missing field width after '%.*s' */
		warning(365, range(dir, bit), start(dir, fmt));
		return false;
	}
	quoted_iterator width = *it;

	bool has_cmp = new_style
	    && (dir.value == '=' || dir.value == ':');
	if (has_cmp && !quoted_next(fmt, it)) {
		/* missing comparison value after directive '%.*s' */
		warning(368, range(dir, bit), start(dir, fmt));
		return false;
	}
	quoted_iterator cmp = *it;

	bool has_default = new_style && dir.value == '*';
	if (has_default && !quoted_next(fmt, it)) {
		/* missing '\0' at the end of '%.*s' */
		warning(366, range(dir, *it), start(dir, fmt));
		return false;
	}

	if (!has_bit && !has_cmp && !has_default) {
		/* unknown directive '%.*s' */
		warning(374, len(dir), start(dir, fmt));
		return false;
	}

	if (!quoted_next(fmt, it)) {
		if (new_style && dir.value != '*')
			/* missing '\0' at the end of '%.*s' */
			warning(366, range(dir, *it), start(dir, fmt));
		else
			/* empty description in '%.*s' */
			warning(367, range(dir, *it), start(dir, fmt));
		return false;
	}
	quoted_iterator descr = *it;

	quoted_iterator prev = *it;
	for (;;) {
		if (new_style && it->value == 0)
			break;
		if (!new_style && it->value == 0)
			/* old-style format contains '\0' */
			warning(362);
		if (!new_style && it->value <= 32) {
			*it = prev;
			break;
		}
		if (it->escaped && !isprint((unsigned char)it->value)) {
			/* non-printing character '%.*s' in description ... */
			warning(363,
			    len(*it), start(*it, fmt),
			    range(descr, *it), start(descr, fmt));
		}
		prev = *it;
		if (!quoted_next(fmt, it)) {
			if (new_style) {
				/* missing '\0' at the end of '%.*s' */
				warning(366, range(dir, prev),
				    start(dir, fmt));
			}
			break;
		}
	}

	if (has_bit)
		check_hex_escape(fmt, bit);
	if (has_width)
		check_hex_escape(fmt, width);
	if (has_bit && bit.octal_digits == 0 && bit.hex_digits == 0) {
		/* bit position '%.*s' in '%.*s' should be escaped as ... */
		warning(369, len(bit), start(bit, fmt),
		    range(dir, *it), start(dir, fmt));
	}
	if (has_width && width.octal_digits == 0 && width.hex_digits == 0) {
		/* field width '%.*s' in '%.*s' should be escaped as ... */
		warning(370, len(width), start(width, fmt),
		    range(dir, *it), start(dir, fmt));
	}
	if (has_bit && (new_style ? bit.value > 63 : bit.value - 1 > 31)) {
		/* bit position '%.*s' (%ju) in '%.*s' out of range %u..%u */
		warning(371,
		    len(bit), start(bit, fmt), val(bit),
		    range(dir, *it), start(dir, fmt),
		    new_style ? 0 : 1, new_style ? 63 : 32);
	}
	if (has_width && width.value > (new_style ? 64 : 32)) {
		/* field width '%.*s' (%ju) in '%.*s' out of range 0..%u */
		warning(372,
		    len(width), start(width, fmt), val(width),
		    range(dir, *it), start(dir, fmt),
		    new_style ? 64 : 32);
	}
	if (has_width && bit.value + width.value > 64) {
		/* bit field end %ju in '%.*s' out of range 0..64 */
		warning(373, val(bit) + val(width),
		    range(dir, *it), start(dir, fmt));
	}
	if (has_cmp && *prev_field_width < 64
	    && cmp.value & ~(uint64_t)0 << *prev_field_width) {
		/* comparison value '%.*s' (%ju) exceeds field width %ju */
		warning(375, len(cmp), start(cmp, fmt), val(cmp),
		    *prev_field_width);
	}
	if (descr.i == prev.i && dir.value != 'F') {
		/* empty description in '%.*s' */
		warning(367, range(dir, *it), start(dir, fmt));
	}

	if (has_width)
		*prev_field_width = width.value;
	return true;
}

void
check_snprintb(const tnode_t *expr)
{
	const buffer *fmt;
	const tnode_t *value;
	if (!match_snprintb_call(expr->tn_call, &fmt, &value))
		return;

	quoted_iterator it = { .start = 0 };
	if (!quoted_next(fmt, &it)) {
		/* missing new-style '\177' or old-style number base */
		warning(359);
		return;
	}
	bool new_style = it.value == '\177';
	if (new_style && !quoted_next(fmt, &it)) {
		/* missing new-style number base after '\177' */
		warning(360);
		return;
	}
	if (it.value != 8 && it.value != 10 && it.value != 16) {
		/* number base '%.*s' is %ju, should be 8, 10 or 16 */
		warning(361, len(it), start(it, fmt), val(it));
		return;
	}

	uint64_t prev_field_width = 64;
	while (check_directive(fmt, &it, new_style, &prev_field_width))
		continue;
}
