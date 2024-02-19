/*	$NetBSD: snprintb.c,v 1.36 2024/02/19 23:30:56 rillig Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
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

/*
 * snprintb: print an interpreted bitmask to a buffer
 *
 * => returns the length of the buffer that would be required to print the
 *    string minus the terminating NUL.
 */
#ifndef _STANDALONE
# ifndef _KERNEL

#  if HAVE_NBTOOL_CONFIG_H
#   include "nbtool_config.h"
#  endif

#  include <sys/cdefs.h>
#  if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: snprintb.c,v 1.36 2024/02/19 23:30:56 rillig Exp $");
#  endif

#  include <sys/types.h>
#  include <inttypes.h>
#  include <stdio.h>
#  include <util.h>
#  include <errno.h>
# else /* ! _KERNEL */
#  include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: snprintb.c,v 1.36 2024/02/19 23:30:56 rillig Exp $");
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

	const char *const num_fmt;
	unsigned const val_len;
	unsigned total_len;
	unsigned line_len;

	const char *cur_bitfmt;
	int restart;

	const char *sep_bitfmt;
	unsigned sep_line_len;
	char sep;
} state;

static void
store(state *s, char c)
{
	if (s->total_len < s->bufsize)
		s->buf[s->total_len] = c;
	s->total_len++;
	s->line_len++;
}

static void
backup(state *s)
{
	if (s->sep_line_len > 0) {
		s->total_len -= s->line_len - s->sep_line_len;
		s->sep_line_len = 0;
		s->restart = 1;
		s->bitfmt = s->sep_bitfmt;
	}
	store(s, '>');
	store(s, '\0');
	if (s->total_len < s->bufsize)
		snprintf(s->buf + s->total_len, s->bufsize - s->total_len,
		    s->num_fmt, (uintmax_t)s->val);
	s->total_len += s->val_len;
	s->line_len = s->val_len;
}

static void
put_sep(state *s)
{
	if (s->line_max > 0 && s->line_len >= s->line_max) {
		backup(s);
		store(s, '<');
	} else {
		if (s->line_max > 0 && s->sep != '<') {
			s->sep_line_len = s->line_len;
			s->sep_bitfmt = s->cur_bitfmt;
		}
		store(s, s->sep);
		s->restart = 0;
	}
}

static void
put_chr(state *s, char c)
{
	if (s->line_max > 0 && s->line_len >= s->line_max - 1) {
		backup(s);
		if (s->restart == 0)
			store(s, c);
		else
			s->sep = '<';
	} else {
		store(s, c);
		s->restart = 0;
	}
}

static void
put_bitfmt(state *s)
{
	while (*s->bitfmt++ != 0) {
		put_chr(s, s->bitfmt[-1]);
		if (s->restart)
			break;
	}
}

static int
put_num(state *s, const char *fmt, uintmax_t v)
{
	char *bp = s->total_len < s->bufsize ? s->buf + s->total_len : NULL;
	size_t n = s->total_len < s->bufsize ? s->bufsize - s->total_len : 0;
	int fmt_len = snprintf(bp, n, fmt, v);
	if (fmt_len >= 0) {
		s->total_len += fmt_len;
		s->line_len += fmt_len;
	}
	return fmt_len;
}

static void
old_style(state *s)
{
	for (uint8_t bit; (bit = *s->bitfmt) != 0;) {
		s->cur_bitfmt = s->bitfmt++;
		if (s->val & (1U << (bit - 1))) {
			put_sep(s);
			if (s->restart)
				continue;
			s->sep = ',';
			for (; *s->bitfmt > ' '; ++s->bitfmt) {
				put_chr(s, *s->bitfmt);
				if (s->restart)
					break;
			}
		} else
			for (; *s->bitfmt > ' '; ++s->bitfmt)
				continue;
	}
}

static int
new_style(state *s)
{
	uint64_t field = s->val;
	int matched = 1;
	while (*s->bitfmt != '\0') {
		uint8_t kind = *s->bitfmt++;
		uint8_t bit = *s->bitfmt++;
		switch (kind) {
		case 'b':
			if (((s->val >> bit) & 1) == 0)
				goto skip;
			s->cur_bitfmt = s->bitfmt - 2;
			put_sep(s);
			if (s->restart)
				break;
			put_bitfmt(s);
			if (s->restart == 0)
				s->sep = ',';
			break;
		case 'f':
		case 'F':
			matched = 0;
			s->cur_bitfmt = s->bitfmt - 2;
			uint8_t field_width = *s->bitfmt++;
			field = (s->val >> bit) &
			    (((uint64_t)1 << field_width) - 1);
			put_sep(s);
			if (s->restart == 0)
				s->sep = ',';
			if (kind == 'F')
				goto skip;
			if (s->restart == 0)
				put_bitfmt(s);
			if (s->restart == 0)
				put_chr(s, '=');
			if (s->restart == 0) {
				if (put_num(s, s->num_fmt, field) < 0)
					return -1;
				if (s->line_max > 0
				    && s->line_len > s->line_max)
					put_chr(s, '#');
			}
			break;
		case '=':
		case ':':
			/* Here "bit" is actually a value instead. */
			if (field != bit)
				goto skip;
			matched = 1;
			if (kind == '=')
				put_chr(s, '=');
			if (s->restart == 0)
				put_bitfmt(s);
			break;
		case '*':
			s->bitfmt--;
			if (!matched) {
				matched = 1;
				if (put_num(s, s->bitfmt, field) < 0)
					return -1;
			}
			/*FALLTHROUGH*/
		default:
		skip:
			while (*s->bitfmt++ != '\0')
				continue;
			break;
		}
	}
	return 0;
}

int
snprintb_m(char *buf, size_t bufsize, const char *bitfmt, uint64_t val,
	   size_t line_max)
{
#ifdef _KERNEL
	/*
	 * For safety; no other *s*printf() do this, but in the kernel
	 * we don't usually check the return value
	 */
	(void)memset(buf, 0, bufsize);
#endif /* _KERNEL */


	int old = *bitfmt != '\177';
	if (!old)
		bitfmt++;

	const char *num_fmt;
	switch (*bitfmt++) {
	case 8:
		num_fmt = "%#jo";
		break;
	case 10:
		num_fmt = "%ju";
		break;
	case 16:
		num_fmt = "%#jx";
		break;
	default:
		goto internal;
	}

	int val_len = snprintf(buf, bufsize, num_fmt, (uintmax_t)val);
	if (val_len < 0)
		goto internal;

	state s = {
		.buf = buf,
		.bufsize = bufsize,
		.bitfmt = bitfmt,
		.val = val,
		.line_max = line_max,

		.num_fmt = num_fmt,
		.val_len = val_len,
		.total_len = val_len,
		.line_len = val_len,

		.sep = '<',
	};

	if (old)
		old_style(&s);
	else if (new_style(&s) < 0)
		goto internal;

	if (s.sep != '<')
		store(&s, '>');
	if (s.line_max > 0) {
		store(&s, '\0');
		if (s.bufsize >= 2 && s.total_len > s.bufsize - 2)
			s.buf[s.bufsize - 2] = '\0';
	}
	store(&s, '\0');
	if (s.bufsize >= 1 && s.total_len > s.bufsize - 1)
		s.buf[s.bufsize - 1] = '\0';
	return (int)(s.total_len - 1);
internal:
#ifndef _KERNEL
	errno = EINVAL;
#endif
	return -1;
}

int
snprintb(char *buf, size_t bufsize, const char *bitfmt, uint64_t val)
{
	return snprintb_m(buf, bufsize, bitfmt, val, 0);
}
# endif /* ! HAVE_SNPRINTB_M */
#endif /* ! _STANDALONE */
