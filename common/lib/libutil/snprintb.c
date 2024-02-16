/*	$NetBSD: snprintb.c,v 1.27 2024/02/16 17:42:49 rillig Exp $	*/

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
__RCSID("$NetBSD: snprintb.c,v 1.27 2024/02/16 17:42:49 rillig Exp $");
#  endif

#  include <sys/types.h>
#  include <inttypes.h>
#  include <stdio.h>
#  include <util.h>
#  include <errno.h>
# else /* ! _KERNEL */
#  include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: snprintb.c,v 1.27 2024/02/16 17:42:49 rillig Exp $");
#  include <sys/param.h>
#  include <sys/inttypes.h>
#  include <sys/systm.h>
#  include <lib/libkern/libkern.h>
# endif /* ! _KERNEL */

# ifndef HAVE_SNPRINTB_M
int
snprintb_m(char *buf, size_t bufsize, const char *bitfmt, uint64_t val,
	   size_t line_max)
{
	char *bp = buf, *sep_bp = NULL;
	const char *c_fmt, *sep_fmt = NULL, *cur_fmt;
	const char *num_fmt;
	int ch, total_len, sep_line_len = 0, line_len, val_len, sep;
	int restart = 0;

#ifdef _KERNEL
	/*
	 * For safety; no other *s*printf() do this, but in the kernel
	 * we don't usually check the return value
	 */
	(void)memset(buf, 0, bufsize);
#endif /* _KERNEL */

	ch = *bitfmt++;
	switch (ch != '\177' ? ch : *bitfmt++) {
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

	/* Reserve space for trailing blank line if needed */
	if (line_max > 0)
		bufsize--;

	total_len = snprintf(bp, bufsize, num_fmt, (uintmax_t)val);
	if (total_len < 0)
		goto internal;

	val_len = line_len = total_len;

	if ((size_t)total_len < bufsize)
		bp += total_len;
	else
		bp += bufsize - 1;

	if (val == 0 && ch != '\177')
		goto done;

#define	STORE(c) do {							\
		line_len++;						\
		if ((size_t)(++total_len) < bufsize)			\
			*bp++ = (c);					\
	} while (0)

#define	BACKUP() do {							\
		if (sep_bp != NULL) {					\
			bp = sep_bp;					\
			sep_bp = NULL;					\
			total_len -= line_len - sep_line_len;		\
			restart = 1;					\
			bitfmt = sep_fmt;				\
		}							\
		STORE('>');						\
		STORE('\0');						\
		if ((size_t)total_len < bufsize)			\
			snprintf(bp, bufsize - total_len, num_fmt,	\
			    (uintmax_t)val);				\
		total_len += val_len;					\
		line_len = val_len;					\
		bp += val_len;						\
	} while (0)

#define	PUTSEP() do {							\
		if (line_max > 0 && (size_t)line_len >= line_max) {	\
			BACKUP();					\
			STORE('<');					\
		} else {						\
			/* Remember separator location */		\
			if (line_max > 0 && sep != '<') {		\
				sep_line_len = line_len;		\
				sep_bp = bp;				\
				sep_fmt = cur_fmt;			\
			}						\
			STORE(sep);					\
			restart = 0;					\
		}							\
	} while (0)

#define	PUTCHR(c) do {							\
		if (line_max > 0 && (size_t)line_len >= line_max - 1) {	\
			BACKUP();					\
			if (restart == 0)				\
				STORE(c);				\
			else						\
				sep = '<';				\
		} else {						\
			STORE(c);					\
			restart = 0;					\
		}							\
	} while (0)

#define	PUTS(s) do {							\
		while ((ch = *(s)++) != 0) {				\
			PUTCHR(ch);					\
			if (restart)					\
				break;					\
		}							\
	} while (0)

#define	FMTSTR(sb, f) do {						\
		int fmt_len = snprintf(bp, bufsize - total_len, sb,	\
		    (uintmax_t)f);					\
		if (fmt_len < 0)					\
			goto internal;					\
		total_len += fmt_len;					\
		line_len += fmt_len;					\
		if ((size_t)total_len < bufsize)			\
			bp += fmt_len;					\
	} while (0)

	sep = '<';
	if (ch != '\177') {
		/* old-style format, 32-bit, 1-origin. */
		for (int bit; (bit = *bitfmt) != 0;) {
			cur_fmt = bitfmt++;
			if (val & (1U << (bit - 1))) {
				PUTSEP();
				if (restart)
					continue;
				sep = ',';
				for (; (ch = *bitfmt) > ' '; ++bitfmt) {
					PUTCHR(ch);
					if (restart)
						break;
				}
			} else
				for (; *bitfmt > ' '; ++bitfmt)
					continue;
		}
	} else {
		/* new-style format, 64-bit, 0-origin; also does fields. */
		uint64_t field = val;
		int matched = 1;
		while (c_fmt = bitfmt, (ch = *bitfmt++) != '\0') {
			int bit = *bitfmt++;
			switch (ch) {
			case 'b':
				if (((val >> bit) & 1) == 0)
					goto skip;
				cur_fmt = c_fmt;
				PUTSEP();
				if (restart)
					break;
				PUTS(bitfmt);
				if (restart == 0)
					sep = ',';
				break;
			case 'f':
			case 'F':
				matched = 0;
				cur_fmt = c_fmt;
				int field_width = *bitfmt++;
				field = (val >> bit) &
				    (((uint64_t)1 << field_width) - 1);
				PUTSEP();
				if (restart == 0)
					sep = ',';
				if (ch == 'F') {	/* just extract */
					/* duplicate PUTS() effect on bitfmt */
					while (*bitfmt++ != '\0')
						continue;
					break;
				}
				if (restart == 0)
					PUTS(bitfmt);
				if (restart == 0)
					PUTCHR('=');
				if (restart == 0) {
					FMTSTR(num_fmt, field);
					if (line_max > 0
					    && (size_t)line_len > line_max)
						PUTCHR('#');
				}
				break;
			case '=':
			case ':':
				/*
				 * Here "bit" is actually a value instead,
				 * to be compared against the last field.
				 * This only works for values in [0..255],
				 * of course.
				 */
				if ((int)field != bit)
					goto skip;
				matched = 1;
				if (ch == '=')
					PUTCHR('=');
				PUTS(bitfmt);
				break;
			case '*':
				bitfmt--;
				if (!matched) {
					matched = 1;
					FMTSTR(bitfmt, field);
				}
				/*FALLTHROUGH*/
			default:
			skip:
				while (*bitfmt++ != '\0')
					continue;
				break;
			}
		}
	}
	if (sep != '<')
		STORE('>');
done:
	if (line_max > 0) {
		bufsize++;
		STORE('\0');
		if ((size_t)total_len >= bufsize && bufsize > 1)
			buf[bufsize - 2] = '\0';
	}
	STORE('\0');
	if ((size_t)total_len >= bufsize && bufsize > 0)
		buf[bufsize - 1] = '\0';
	return total_len - 1;
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
