/*	$NetBSD: cksnprintb.c,v 1.15 2024/05/12 18:49:36 rillig Exp $	*/

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
__RCSID("$NetBSD: cksnprintb.c,v 1.15 2024/05/12 18:49:36 rillig Exp $");
#endif

#include <stdbool.h>
#include <string.h>

#include "lint1.h"

typedef struct {
	bool new_style;
	const buffer *fmt;
	uint64_t possible_value_bits;

	quoted_iterator it;
	uint64_t field_width;
	uint64_t covered;
	const char *covered_start[64];
	int covered_len[64];
} checker;

static int
len(quoted_iterator it)
{
	return (int)(it.end - it.start);
}

static int
range(quoted_iterator start, quoted_iterator end)
{
	return (int)(end.end - start.start);
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
		for (size_t i = it.start + 2; i < it.end; i++) {
			if (ch_isupper(buf->data[i]))
				upper = true;
			if (ch_islower(buf->data[i]))
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

static void
check_bit(checker *ck, uint64_t dir_lsb, uint64_t width,
	  const char *start, int len)
{
	unsigned lsb = (unsigned)(ck->new_style ? dir_lsb : dir_lsb - 1);
	if (lsb >= 64 || width == 0 || width > 64)
		return;

	uint64_t field_mask = value_bits((unsigned)width) << lsb;
	for (unsigned i = lsb; i < 64; i++) {
		if (ck->covered & field_mask & bit(i)) {
			/* '%.*s' overlaps earlier '%.*s' on bit %u */
			warning(376,
			    len, start,
			    ck->covered_len[i], ck->covered_start[i],
			    ck->new_style ? i : i + 1);
			break;
		}
	}

	ck->covered |= field_mask;
	for (unsigned i = lsb; i < 64; i++) {
		if (field_mask & bit(i)) {
			ck->covered_start[i] = start;
			ck->covered_len[i] = len;
		}
	}

	if (!(ck->possible_value_bits & field_mask))
		/* conversion '%.*s' is unreachable by input value */
		warning(378, len, start);
}

static bool
parse_description(checker *ck, const quoted_iterator *dir)
{
	size_t descr_start = 0;
	quoted_iterator it = ck->it;
	uint64_t end_marker = ck->new_style ? 0 : 32;

	while (quoted_next(ck->fmt, &it) && it.value > end_marker) {
		ck->it = it;
		if (descr_start == 0)
			descr_start = it.start;
		if (it.escaped)
			/* escaped character '%.*s' in description ... */
			warning(363,
			    len(it), start(it, ck->fmt),
			    range(*dir, it), start(*dir, ck->fmt));
	}
	return descr_start > 0;
}

static bool
check_conversion(checker *ck)
{
	bool new_style = ck->new_style;
	const buffer *fmt = ck->fmt;
	quoted_iterator *it = &ck->it;

	if (!quoted_next(fmt, it))
		return false;
	quoted_iterator dir = *it;

	bool has_bit = !new_style
	    || dir.value == 'b' || dir.value == 'f' || dir.value == 'F';
	if (has_bit && new_style && !quoted_next(fmt, it)) {
		/* missing bit position after '%.*s' */
		warning(364, range(dir, *it), start(dir, fmt));
		return false;
	}
	/* LINTED 86 "automatic 'bit' hides external declaration" */
	quoted_iterator bit = *it;

	bool has_width = new_style
	    && (dir.value == 'f' || dir.value == 'F');
	if (has_width && !quoted_next(fmt, it)) {
		/* missing field width after '%.*s' */
		warning(365, range(dir, *it), start(dir, fmt));
		return false;
	}
	quoted_iterator width = *it;

	bool has_cmp = new_style
	    && (dir.value == '=' || dir.value == ':');
	if (has_cmp && !quoted_next(fmt, it)) {
		/* missing comparison value after conversion '%.*s' */
		warning(368, range(dir, *it), start(dir, fmt));
		return false;
	}
	quoted_iterator cmp = *it;

	bool has_default = new_style && dir.value == '*';

	if (dir.value == '\0') {
		quoted_iterator end = *it;
		if (!quoted_next(fmt, &end)) {
			/* redundant '\0' at the end of the format */
			warning(377);
			return false;
		}
	}

	if (!has_bit && !has_cmp && !has_default) {
		/* unknown conversion '%.*s', must be one of 'bfF=:*' */
		warning(374, len(dir), start(dir, fmt));
		return false;
	}
	if (new_style && dir.escaped)
		/* conversion '%.*s' should not be escaped */
		warning(362, len(dir), start(dir, fmt));

	bool needs_descr = !(new_style && dir.value == 'F');
	bool seen_descr = parse_description(ck, &dir);
	bool seen_null = new_style
	    && quoted_next(ck->fmt, &ck->it) && ck->it.value == 0;

	if (has_bit)
		check_hex_escape(fmt, bit);
	if (has_width)
		check_hex_escape(fmt, width);
	if (has_bit && bit.octal_digits == 0 && bit.hex_digits == 0)
		/* bit position '%.*s' in '%.*s' should be escaped as ... */
		warning(369, len(bit), start(bit, fmt),
		    range(dir, *it), start(dir, fmt));
	if (has_width && width.octal_digits == 0 && width.hex_digits == 0)
		/* field width '%.*s' in '%.*s' should be escaped as ... */
		warning(370, len(width), start(width, fmt),
		    range(dir, *it), start(dir, fmt));
	if (has_bit && (new_style ? bit.value > 63 : bit.value - 1 > 31))
		/* bit position '%.*s' (%ju) in '%.*s' out of range %u..%u */
		warning(371,
		    len(bit), start(bit, fmt), val(bit),
		    range(dir, *it), start(dir, fmt),
		    new_style ? 0 : 1, new_style ? 63 : 32);
	if (has_width && width.value > 64)
		/* field width '%.*s' (%ju) in '%.*s' out of range 0..64 */
		warning(372,
		    len(width), start(width, fmt), val(width),
		    range(dir, *it), start(dir, fmt));
	if (has_width && bit.value + width.value > 64)
		/* bit field end %ju in '%.*s' out of range 0..64 */
		warning(373, val(bit) + val(width),
		    range(dir, *it), start(dir, fmt));
	if (has_cmp && ck->field_width > 0 && ck->field_width < 64
	    && cmp.value & ~value_bits((unsigned)ck->field_width))
		/* comparison value '%.*s' (%ju) exceeds maximum field ... */
		warning(375, len(cmp), start(cmp, fmt), val(cmp),
		    (uintmax_t)value_bits((unsigned)ck->field_width));
	if (has_bit)
		check_bit(ck, bit.value, has_width ? width.value : 1,
		    ck->fmt->data + dir.start, (int)(it->end - dir.start));
	if (needs_descr && !seen_descr)
		/* empty description in '%.*s' */
		warning(367, range(dir, *it), start(dir, fmt));
	if (new_style && !seen_null)
		/* missing '\0' at the end of '%.*s' */
		warning(366, range(dir, *it), start(dir, fmt));

	if (has_width)
		ck->field_width = width.value;
	return true;
}

void
check_snprintb(const function_call *call)
{
	const char *name;
	const buffer *fmt;
	const tnode_t *value;

	if (!(call->func->tn_op == ADDR
	    && call->func->u.ops.left->tn_op == NAME
	    && (name = call->func->u.ops.left->u.sym->s_name, true)
	    && ((strcmp(name, "snprintb") == 0 && call->args_len == 4)
		|| (strcmp(name, "snprintb_m") == 0 && call->args_len == 5))
	    && call->args[2]->tn_op == CVT
	    && call->args[2]->u.ops.left->tn_op == ADDR
	    && call->args[2]->u.ops.left->u.ops.left->tn_op == STRING
	    && (fmt = call->args[2]->u.ops.left->u.ops.left->u.str_literals,
		fmt->data != NULL)
	    && (value = call->args[3], true)))
		return;

	checker ck = {
		.fmt = fmt,
		.possible_value_bits = possible_bits(value),
		.field_width = 64,
	};

	if (!quoted_next(fmt, &ck.it)) {
		/* missing new-style '\177' or old-style number base */
		warning(359);
		return;
	}
	ck.new_style = ck.it.value == '\177';
	if (ck.new_style && !quoted_next(fmt, &ck.it)) {
		/* missing new-style number base after '\177' */
		warning(360);
		return;
	}
	if (ck.it.value != 8 && ck.it.value != 10 && ck.it.value != 16) {
		/* number base '%.*s' is %ju, must be 8, 10 or 16 */
		warning(361, len(ck.it), start(ck.it, fmt), val(ck.it));
		return;
	}

	while (check_conversion(&ck))
		continue;
}
