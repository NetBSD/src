/*	$NetBSD: snprintb.c,v 1.6 2006/10/15 19:34:51 christos Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: snprintb.c,v 1.6 2006/10/15 19:34:51 christos Exp $");
#endif

/*
 * snprintb: print an interpreted bitmask to a buffer
 *
 * => returns the length of the buffer that would require to print the string.
 */

#include <stdio.h>
#include <util.h>

int
snprintb(buf, buflen, bitfmt, val)
	char *buf;
	size_t buflen;
	const char *bitfmt;
	uint64_t val;
{
	char *bp = buf;
	const char *sbase;
	int bit, ch, len, sep, flen;
	uint64_t field;

	ch = *bitfmt++;
	switch (ch != '\177' ? ch : *bitfmt++) {
	case 8:
		sbase = "0%llo";
		break;
	case 16:
		sbase = "0x%llx";
		break;
	case 10:
	default:
		sbase = "%lld";
		break;
	}

	len = snprintf(bp, buflen, sbase, val);
	if (len < buflen)
		bp += len;
	else
		bp += buflen - 1;

	/*
	 * If the value we printed was 0 and we're using the old-style format,
	 * we're done.
	 */
	if ((val == 0) && (ch != '\177')) {
		*bp = '\0';
		return len;
	}

#define PUTC(c) if (++len < buflen) *bp++ = (c)
#define PUTS(s) while ((ch = *(s)++) != 0) PUTC(ch)

	/*
	 * Chris Torek's new bitmask format is identified by a leading \177
	 */
	sep = '<';
	if (ch != '\177') {
		/* old (standard) format. */
		for (;(bit = *bitfmt++) != 0;) {
			if (val & (1 << (bit - 1))) {
				PUTC(sep);
				for (; (ch = *bitfmt) > ' '; ++bitfmt)
					PUTC(ch);
				sep = ',';
			} else
				for (; *bitfmt > ' '; ++bitfmt)
					continue;
		}
	} else {
		/* new quad-capable format; also does fields. */
		field = val;
		while ((ch = *bitfmt++) != '\0') {
			bit = *bitfmt++;	/* now 0-origin */
			switch (ch) {
			case 'b':
				if (((u_int)(val >> bit) & 1) == 0)
					goto skip;
				PUTC(sep);
				PUTS(bitfmt);
				sep = ',';
				break;
			case 'f':
			case 'F':
				flen = *bitfmt++;	/* field length */
				field = (val >> bit) &
					    (((uint64_t)1 << flen) - 1);
				if (ch == 'F')	/* just extract */
					break;
				PUTC(sep);
				sep = ',';
				PUTS(bitfmt);
				PUTC('=');
				flen = snprintf(bp, buflen - len, sbase, field);
				len += flen;
				if (len < buflen)
					bp += flen;
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
				if (ch == '=') {
					PUTC('=');
				}
				PUTS(bitfmt);
				break;
			default:
			skip:
				while (*bitfmt++ != '\0')
					continue;
				break;
			}
		}
	}
	if (sep != '<') {
		PUTC('>');
	}
	*bp = '\0';
	return len;
}
