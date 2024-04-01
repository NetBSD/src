/*	$NetBSD: snprintb.c,v 1.45 2024/04/01 08:53:42 rillig Exp $	*/

/*-
 * Copyright (c) 2002, 2024 The NetBSD Foundation, Inc.
 * All rights reserved.
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

#ifndef _STANDALONE
# ifndef _KERNEL

#  if HAVE_NBTOOL_CONFIG_H
#   include "nbtool_config.h"
#  endif

#  include <sys/cdefs.h>
#  if defined(LIBC_SCCS)
__RCSID("$NetBSD: snprintb.c,v 1.45 2024/04/01 08:53:42 rillig Exp $");
#  endif

#  include <sys/types.h>
#  include <inttypes.h>
#  include <stdio.h>
#  include <string.h>
#  include <util.h>
#  include <errno.h>
# else /* ! _KERNEL */
#  include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: snprintb.c,v 1.45 2024/04/01 08:53:42 rillig Exp $");
#  include <sys/param.h>
#  include <sys/inttypes.h>
#  include <sys/systm.h>
#  include <lib/libkern/libkern.h>
# endif /* ! _KERNEL */

# ifndef HAVE_SNPRINTB_M
typedef struct {
	char *const buf;
	size_t const bufsize;
	const char *bitfmt;
	uint64_t const val;
	size_t const line_max;

	char num_fmt[5];
	size_t total_len;
	size_t line_pos;
	size_t comma_pos;
	int in_angle_brackets;
} state;

static void
store(state *s, char c)
{
	if (s->total_len < s->bufsize)
		s->buf[s->total_len] = c;
	s->total_len++;
}

static int
store_num(state *s, const char *fmt, uintmax_t num)
{
	int num_len = s->total_len < s->bufsize
	    ? snprintf(s->buf + s->total_len, s->bufsize - s->total_len,
		fmt, num)
	    : snprintf(NULL, 0, fmt, num);
	if (num_len > 0)
		s->total_len += num_len;
	return num_len;
}

static void
store_eol(state *s)
{
	if (s->total_len - s->line_pos > s->line_max) {
		s->total_len = s->line_pos + s->line_max - 1;
		store(s, '#');
	}
	store(s, '\0');
	s->line_pos = s->total_len;
	s->comma_pos = 0;
	s->in_angle_brackets = 0;
}

static void
store_delimiter(state *s)
{
	if (s->in_angle_brackets) {
		s->comma_pos = s->total_len;
		store(s, ',');
	} else {
		store(s, '<');
		s->in_angle_brackets = 1;
	}
}

static void
maybe_wrap_line(state *s, const char *bitfmt)
{
	if (s->line_max > 0
	    && s->comma_pos > 0
	    && s->total_len - s->line_pos >= s->line_max) {
		s->total_len = s->comma_pos;
		store(s, '>');
		store_eol(s);
		store_num(s, s->num_fmt, s->val);
		s->bitfmt = bitfmt;
	}
}

static int
old_style(state *s)
{
	while (*s->bitfmt != '\0') {
		const char *cur_bitfmt = s->bitfmt;
		uint8_t bit = *s->bitfmt;
		if (bit > ' ')
			return -1;
		if (s->val & (1U << (bit - 1))) {
			store_delimiter(s);
			while ((uint8_t)*++s->bitfmt > ' ')
				store(s, *s->bitfmt);
			maybe_wrap_line(s, cur_bitfmt);
		} else
			while ((uint8_t)*++s->bitfmt > ' ')
				continue;
	}
	return 0;
}

static int
new_style(state *s)
{
	uint64_t field = s->val;
	int matched = 1;
	const char *prev_bitfmt = s->bitfmt;
	while (*s->bitfmt != '\0') {
		const char *cur_bitfmt = s->bitfmt;
		uint8_t kind = cur_bitfmt[0];
		switch (kind) {
		case 'b':
			prev_bitfmt = cur_bitfmt;
			uint8_t b_bit = cur_bitfmt[1];
			if (b_bit >= 64)
				return -1;
			s->bitfmt += 2;
			if (((s->val >> b_bit) & 1) == 0)
				goto skip_description;
			store_delimiter(s);
			while (*s->bitfmt++ != '\0')
				store(s, s->bitfmt[-1]);
			maybe_wrap_line(s, cur_bitfmt);
			break;
		case 'f':
		case 'F':
			prev_bitfmt = cur_bitfmt;
			matched = 0;
			uint8_t f_lsb = cur_bitfmt[1];
			if (f_lsb >= 64)
				return -1;
			uint8_t f_width = cur_bitfmt[2];
			if (f_width > 64)
				return -1;
			field = s->val >> f_lsb;
			if (f_width < 64)
				field &= ((uint64_t) 1 << f_width) - 1;
			s->bitfmt += 3;
			store_delimiter(s);
			if (kind == 'F')
				goto skip_description;
			while (*s->bitfmt++ != '\0')
				store(s, s->bitfmt[-1]);
			store(s, '=');
			store_num(s, s->num_fmt, field);
			maybe_wrap_line(s, cur_bitfmt);
			break;
		case '=':
		case ':':
			s->bitfmt += 2;
			uint8_t cmp = cur_bitfmt[1];
			if (field != cmp)
				goto skip_description;
			matched = 1;
			if (kind == '=')
				store(s, '=');
			while (*s->bitfmt++ != '\0')
				store(s, s->bitfmt[-1]);
			maybe_wrap_line(s, prev_bitfmt);
			break;
		case '*':
			s->bitfmt++;
			if (matched)
				goto skip_description;
			matched = 1;
			if (store_num(s, s->bitfmt, field) < 0)
				return -1;
			maybe_wrap_line(s, prev_bitfmt);
			goto skip_description;
		default:
			return -1;
		skip_description:
			while (*s->bitfmt++ != '\0')
				continue;
			break;
		}
	}
	return 0;
}

static void
finish_buffer(state *s)
{
	if (s->line_max > 0) {
		store_eol(s);
		store(s, '\0');
		if (s->bufsize >= 3 && s->total_len > s->bufsize)
			s->buf[s->bufsize - 3] = '#';
		if (s->bufsize >= 2 && s->total_len > s->bufsize)
			s->buf[s->bufsize - 2] = '\0';
		if (s->bufsize >= 1 && s->total_len > s->bufsize)
			s->buf[s->bufsize - 1] = '\0';
	} else {
		store(s, '\0');
		if (s->bufsize >= 2 && s->total_len > s->bufsize)
			s->buf[s->bufsize - 2] = '#';
		if (s->bufsize >= 1 && s->total_len > s->bufsize)
			s->buf[s->bufsize - 1] = '\0';
	}
}

int
snprintb_m(char *buf, size_t bufsize, const char *bitfmt, uint64_t val,
	   size_t line_max)
{
	int old = *bitfmt != '\177';
	if (!old)
		bitfmt++;

	state s = {
		.buf = buf,
		.bufsize = bufsize,
		.bitfmt = bitfmt,
		.val = val,
		.line_max = line_max,
	};
	int had_error = 0;

	switch (*s.bitfmt++) {
	case 8:
		memcpy(s.num_fmt, "%#jo", 4);
		break;
	case 10:
		memcpy(s.num_fmt, "%ju", 4);
		break;
	case 16:
		memcpy(s.num_fmt, "%#jx", 4);
		break;
	default:
		goto had_error;
	}

	store_num(&s, s.num_fmt, val);

	if ((old ? old_style(&s) : new_style(&s)) < 0) {
had_error:
#ifndef _KERNEL
		errno = EINVAL;
#endif
		had_error = 1;
		store(&s, '#');
	} else if (s.in_angle_brackets)
		store(&s, '>');
	finish_buffer(&s);
	return had_error ? -1 : (int)(s.total_len - 1);
}

int
snprintb(char *buf, size_t bufsize, const char *bitfmt, uint64_t val)
{
	return snprintb_m(buf, bufsize, bitfmt, val, 0);
}
# endif /* ! HAVE_SNPRINTB_M */
#endif /* ! _STANDALONE */
