/*	$NetBSD: mime_codecs.c,v 1.10 2012/11/24 21:40:02 christos Exp $	*/

/*-
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Anon Ymous.
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
 * This module contains all mime related codecs.  Typically there are
 * two versions: one operating on buffers and one operating on files.
 * All exported routines have a "mime_" prefix.  The file oriented
 * routines have a "mime_f" prefix replacing the "mime_" prefix of the
 * equivalent buffer based version.
 *
 * The file based API should be:
 *
 *   mime_f<name>_{encode,decode}(FILE *in, FILE *out, void *cookie)
 *
 * XXX - currently this naming convention has not been adheared to.
 *
 * where the cookie is a generic way to pass arguments to the routine.
 * This way these routines can be run by run_function() in mime.c.
 *
 * The buffer based API is not as rigid.
 */

#ifdef MIME_SUPPORT

#include <sys/cdefs.h>
#ifndef __lint__
__RCSID("$NetBSD: mime_codecs.c,v 1.10 2012/11/24 21:40:02 christos Exp $");
#endif /* not __lint__ */

#include <assert.h>
#include <iconv.h>
#include <stdio.h>
#include <stdlib.h>
#include <util.h>

#include "def.h"
#include "extern.h"
#include "mime_codecs.h"


#ifdef CHARSET_SUPPORT
/************************************************************************
 * Core character set conversion routines.
 *
 */

/*
 * Fault-tolerant iconv() function.
 *
 * This routine was borrowed from nail-11.25/mime.c and modified.  It
 * tries to handle errno == EILSEQ by restarting at the next input
 * byte (is this a good idea?).  All other errors are handled by the
 * caller.
 */
PUBLIC size_t
mime_iconv(iconv_t cd, const char **inb, size_t *inbleft, char **outb, size_t *outbleft)
{
	size_t sz = 0;

	while ((sz = iconv(cd, inb, inbleft, outb, outbleft)) == (size_t)-1
			&& errno == EILSEQ) {
		if (*outbleft > 0) {
			*(*outb)++ = '?';
			(*outbleft)--;
		} else {
			**outb = '\0';
			return E2BIG;
		}
		if (*inbleft > 0) {
			(*inb)++;
			(*inbleft)--;
		} else {
			**outb = '\0';
			break;
		}
	}
	return sz;
}

/*
 * This routine was mostly borrowed from src/usr.bin/iconv/iconv.c.
 * We don't care about the invalid character count, so don't bother
 * with __iconv().  We do care about robustness, so call iconv_ft()
 * above to try to recover from errors.
 */
#define INBUFSIZE 1024
#define OUTBUFSIZE (INBUFSIZE * 2)

PUBLIC void
mime_ficonv(FILE *fi, FILE *fo, void *cookie)
{
	char inbuf[INBUFSIZE], outbuf[OUTBUFSIZE], *out;
	const char *in;
	size_t inbytes, outbytes, ret;
	iconv_t cd;

	/*
	 * NOTE: iconv_t is actually a pointer typedef, so this
	 * conversion is not what it appears to be!
	 */
	cd = (iconv_t)cookie;

	while ((inbytes = fread(inbuf, 1, INBUFSIZE, fi)) > 0) {
		in = inbuf;
		while (inbytes > 0) {
			out = outbuf;
			outbytes = OUTBUFSIZE;
			ret = mime_iconv(cd, &in, &inbytes, &out, &outbytes);
			if (ret == (size_t)-1 && errno != E2BIG) {
				if (errno != EINVAL || in == inbuf) {
					/* XXX - what is proper here?
					 * Just copy out the remains? */
					(void)fprintf(fo,
					    "\n\t[ iconv truncated message: %s ]\n\n",
					    strerror(errno));
					return;
				}
				/*
				 * If here: errno == EINVAL && in != inbuf
				 */
				/* incomplete input character */
				(void)memmove(inbuf, in, inbytes);
				ret = fread(inbuf + inbytes, 1,
				    INBUFSIZE - inbytes, fi);
				if (ret == 0) {
					if (feof(fi)) {
						(void)fprintf(fo,
						    "\n\t[ unexpected end of file; "
						    "the last character is "
						    "incomplete. ]\n\n");
						return;
					}
					(void)fprintf(fo,
					    "\n\t[ fread(): %s ]\n\n",
					    strerror(errno));
					return;
				}
				in = inbuf;
				inbytes += ret;

			}
			if (outbytes < OUTBUFSIZE)
				(void)fwrite(outbuf, 1, OUTBUFSIZE - outbytes, fo);
		}
	}
	/* reset the shift state of the output buffer */
	outbytes = OUTBUFSIZE;
	out = outbuf;
	ret = iconv(cd, NULL, NULL, &out, &outbytes);
	if (ret == (size_t)-1) {
		(void)fprintf(fo, "\n\t[ iconv(): %s ]\n\n",
		    strerror(errno));
		return;
	}
	if (outbytes < OUTBUFSIZE)
		(void)fwrite(outbuf, 1, OUTBUFSIZE - outbytes, fo);
}

#endif	/* CHARSET_SUPPORT */



/************************************************************************
 * Core base64 routines
 *
 * Defined in sec 6.8 of RFC 2045.
 */

/*
 * Decode a base64 buffer.
 *
 *   bin:  buffer to hold the decoded (binary) result (see note 1).
 *   b64:  buffer holding the encoded (base64) source.
 *   cnt:  number of bytes in the b64 buffer to decode (see note 2).
 *
 * Return: the number of bytes written to the 'bin' buffer or -1 on
 *         error.
 * NOTES:
 *   1) It is the callers responsibility to ensure that bin is large
 *      enough to hold the result.
 *   2) The b64 buffer should always contain a multiple of 4 bytes of
 *      data!
 */
PUBLIC ssize_t
mime_b64tobin(char *bin, const char *b64, size_t cnt)
{
	static const signed char b64index[] = {
		-1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
		-1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
		-1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,62, -1,-1,-1,63,
		52,53,54,55, 56,57,58,59, 60,61,-1,-1, -1,-2,-1,-1,
		-1, 0, 1, 2,  3, 4, 5, 6,  7, 8, 9,10, 11,12,13,14,
		15,16,17,18, 19,20,21,22, 23,24,25,-1, -1,-1,-1,-1,
		-1,26,27,28, 29,30,31,32, 33,34,35,36, 37,38,39,40,
		41,42,43,44, 45,46,47,48, 49,50,51,-1, -1,-1,-1,-1
	};
	unsigned char *p;
	const unsigned char *q, *end;

#define EQU	(unsigned)-2
#define BAD	(unsigned)-1
#define uchar64(c)  ((c) >= sizeof(b64index) ? BAD : (unsigned)b64index[(c)])

	p = (unsigned char *)bin;
	q = (const unsigned char *)b64;
	for (end = q + cnt; q < end; q += 4) {
		unsigned a = uchar64(q[0]);
		unsigned b = uchar64(q[1]);
		unsigned c = uchar64(q[2]);
		unsigned d = uchar64(q[3]);

		if (a == BAD || a == EQU || b == BAD || b == EQU ||
		    c == BAD || d == BAD)
			return -1;

		*p++ = ((a << 2) | ((b & 0x30) >> 4));
		if (c == EQU)	{ /* got '=' */
			if (d != EQU)
				return -1;
			break;
		}
		*p++ = (((b & 0x0f) << 4) | ((c & 0x3c) >> 2));
		if (d == EQU) { /* got '=' */
			break;
		}
		*p++ = (((c & 0x03) << 6) | d);
	}

#undef uchar64
#undef EQU
#undef BAD

	return p - (unsigned char*)bin;
}

/*
 * Encode a buffer as a base64 result.
 *
 *   b64:  buffer to hold the encoded (base64) result (see note).
 *   bin:  buffer holding the binary source.
 *   cnt:  number of bytes in the bin buffer to encode.
 *
 * NOTE: it is the callers responsibility to ensure that 'b64' is
 *       large enough to hold the result.
 */
PUBLIC void
mime_bintob64(char *b64, const char *bin, size_t cnt)
{
	static const char b64table[] =
	    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	const unsigned char *p = (const unsigned char*)bin;
	ssize_t i;

	for (i = cnt; i > 0; i -= 3) {
		unsigned a = p[0];
		unsigned b = p[1];
		unsigned c = p[2];

		b64[0] = b64table[a >> 2];
		switch(i) {
		case 1:
			b64[1] = b64table[((a & 0x3) << 4)];
			b64[2] = '=';
			b64[3] = '=';
			break;
		case 2:
			b64[1] = b64table[((a & 0x3) << 4) | ((b & 0xf0) >> 4)];
			b64[2] = b64table[((b & 0xf) << 2)];
			b64[3] = '=';
			break;
		default:
			b64[1] = b64table[((a & 0x3) << 4) | ((b & 0xf0) >> 4)];
			b64[2] = b64table[((b & 0xf) << 2) | ((c & 0xc0) >> 6)];
			b64[3] = b64table[c & 0x3f];
			break;
		}
		p   += 3;
		b64 += 4;
	}
}


#define MIME_BASE64_LINE_MAX	(4 * 19)  /* max line length is 76: see RFC2045 sec 6.8 */

static void
mime_fB64_encode(FILE *fi, FILE *fo, void *cookie __unused)
{
	static char b64[MIME_BASE64_LINE_MAX];
	static char mem[3 * (MIME_BASE64_LINE_MAX / 4)];
	size_t cnt;
	char *cp;
	size_t limit;
#ifdef __lint__
	cookie = cookie;
#endif
	limit = 0;
	if ((cp = value(ENAME_MIME_B64_LINE_MAX)) != NULL)
		limit = (size_t)atoi(cp);
	if (limit == 0 || limit > sizeof(b64))
		limit = sizeof(b64);

	limit = 3 * roundup(limit, 4) / 4;
	if (limit < 3)
		limit = 3;

	while ((cnt = fread(mem, sizeof(*mem), limit, fi)) > 0) {
		mime_bintob64(b64, mem, (size_t)cnt);
		(void)fwrite(b64, sizeof(*b64), (size_t)4 * roundup(cnt, 3) / 3, fo);
		(void)putc('\n', fo);
	}
}

static void
mime_fB64_decode(FILE *fi, FILE *fo, void *add_lf)
{
	char *line;
	size_t len;
	char *buf;
	size_t buflen;

	buflen = 3 * (MIME_BASE64_LINE_MAX / 4);
	buf = emalloc(buflen);

	while ((line = fgetln(fi, &len)) != NULL) {
		ssize_t binlen;
		if (line[len-1] == '\n') /* forget the trailing newline */
			len--;

		/* trash trailing white space */
		for (/*EMPTY*/; len > 0 && is_WSP(line[len-1]); len--)
			continue;

		/* skip leading white space */
		for (/*EMPTY*/; len > 0 && is_WSP(line[0]); len--, line++)
			continue;

		if (len == 0)
			break;

		if (3 * len > 4 * buflen) {
			buflen *= 2;
			buf = erealloc(buf, buflen);
		}

		binlen = mime_b64tobin(buf, line, len);

		if (binlen <= 0) {
			(void)fprintf(fo, "WARN: invalid base64 encoding\n");
			break;
		}
		(void)fwrite(buf, 1, (size_t)binlen, fo);
	}

	free(buf);

	if (add_lf)
		(void)fputc('\n', fo);
}


/************************************************************************
 * Core quoted-printable routines.
 *
 * Note: the header QP routines are slightly different and burried
 * inside mime_header.c
 */

static int
mustquote(unsigned char *p, unsigned char *end, size_t l)
{
#define N	0	/* do not quote */
#define Q	1	/* must quote */
#define SP	2	/* white space */
#define XF	3	/* special character 'F' - maybe quoted */
#define XD	4	/* special character '.' - maybe quoted */
#define EQ	Q	/* '=' must be quoted */
#define TB	SP	/* treat '\t' as a space */
#define NL	N	/* don't quote '\n' (NL) - XXX - quoting here breaks the line length algorithm */
#define CR	Q	/* always quote a '\r' (CR) - it occurs only in a CRLF combo */

	static const signed char quotetab[] = {
  		 Q, Q, Q, Q,  Q, Q, Q, Q,  Q,TB,NL, Q,  Q,CR, Q, Q,
		 Q, Q, Q, Q,  Q, Q, Q, Q,  Q, Q, Q, Q,  Q, Q, Q, Q,
		SP, N, N, N,  N, N, N, N,  N, N, N, N,  N, N,XD, N,
		 N, N, N, N,  N, N, N, N,  N, N, N, N,  N,EQ, N, N,

		 N, N, N, N,  N, N,XF, N,  N, N, N, N,  N, N, N, N,
		 N, N, N, N,  N, N, N, N,  N, N, N, N,  N, N, N, N,
		 N, N, N, N,  N, N, N, N,  N, N, N, N,  N, N, N, N,
		 N, N, N, N,  N, N, N, N,  N, N, N, N,  N, N, N, Q,
	};
	int flag = *p > 0x7f ? Q : quotetab[*p];

	if (flag == N)
		return 0;
	if (flag == Q)
		return 1;
	if (flag == SP)
		return p + 1 < end && p[1] == '\n'; /* trailing white space */

	/* The remainder are special start-of-line cases. */
	if (l != 0)
		return 0;

	if (flag == XF)	/* line may start with "From" */
		return p + 4 < end && p[1] == 'r' && p[2] == 'o' && p[3] == 'm';

	if (flag == XD)	/* line may consist of a single dot */
		return p + 1 < end && p[1] == '\n';

	errx(EXIT_FAILURE,
	    "mustquote: invalid logic: *p=0x%x (%d) flag=%d, l=%zu\n",
	    *p, *p, flag, l);
	/* NOT REACHED */
	return 0;	/* appease GCC */

#undef N
#undef Q
#undef SP
#undef XX
#undef EQ
#undef TB
#undef NL
#undef CR
}


#define MIME_QUOTED_LINE_MAX	76  /* QP max length: see RFC2045 sec 6.7 */

static void
fput_quoted_line(FILE *fo, char *line, size_t len, size_t limit)
{
	size_t l;	/* length of current output line */
	unsigned char *beg;
	unsigned char *end;
	unsigned char *p;

	assert(limit <= MIME_QUOTED_LINE_MAX);

	beg = (unsigned char*)line;
	end = beg + len;
	l = 0;
	for (p = (unsigned char*)line; p < end; p++) {
		if (mustquote(p, end, l)) {
			if (l + 4 > limit) {
				(void)fputs("=\n", fo);
				l = 0;
			}
			(void)fprintf(fo, "=%02X", *p);
			l += 3;
		}
		else {
			if (*p == '\n') {
				if (p > beg && p[-1] == '\r')
					(void)fputs("=0A=", fo);
				l = (size_t)-1;
			}
			else if (l + 2 > limit) {
				(void)fputs("=\n", fo);
				l = 0;
			}
			(void)putc(*p, fo);
			l++;
		}
	}
	/*
	 * Lines ending in a blank must escape the newline.
	 */
	if (len && is_WSP(p[-1]))
		(void)fputs("=\n", fo);
}

static void
mime_fQP_encode(FILE *fi, FILE *fo, void *cookie __unused)
{
	char *line;
	size_t len;
	char *cp;
	size_t limit;

#ifdef __lint__
	cookie = cookie;
#endif
	limit = 0;
	if ((cp = value(ENAME_MIME_QP_LINE_MAX)) != NULL)
		limit = (size_t)atoi(cp);
	if (limit == 0 || limit > MIME_QUOTED_LINE_MAX)
		limit = MIME_QUOTED_LINE_MAX;
	if (limit < 4)
		limit = 4;

	while ((line = fgetln(fi, &len)) != NULL)
		fput_quoted_line(fo, line, len, limit);
}

static void
mime_fQP_decode(FILE *fi, FILE *fo, void *cookie __unused)
{
	char *line;
	size_t len;

#ifdef __lint__
	cookie = cookie;
#endif
	while ((line = fgetln(fi, &len)) != NULL) {
		char *p;
		char *end;

		end = line + len;
		for (p = line; p < end; p++) {
			if (*p == '=') {
				p++;
				while (p < end && is_WSP(*p))
					p++;
				if (*p != '\n' && p + 1 < end) {
					int c;
					char buf[3];

					buf[0] = *p++;
					buf[1] = *p;
					buf[2] = '\0';
					c = (int)strtol(buf, NULL, 16);
					(void)fputc(c, fo);
				}
			}
			else
				(void)fputc(*p, fo);
		}
	}
}


/************************************************************************
 * Routines to select the codec by name.
 */

PUBLIC void
mime_fio_copy(FILE *fi, FILE *fo, void *cookie __unused)
{
	int c;

#ifdef __lint__
	cookie = cookie;
#endif
	while ((c = getc(fi)) != EOF)
		(void)putc(c, fo);

	(void)fflush(fo);
	if (ferror(fi)) {
		warn("read");
		rewind(fi);
		return;
	}
	if (ferror(fo)) {
		warn("write");
		(void)Fclose(fo);
		rewind(fi);
		return;
	}
}


static const struct transfer_encoding_s {
	const char 	*name;
	mime_codec_t	enc;
	mime_codec_t	dec;
} transfer_encoding_tbl[] = {
	{ MIME_TRANSFER_7BIT,	mime_fio_copy,	    mime_fio_copy },
	{ MIME_TRANSFER_8BIT, 	mime_fio_copy,	    mime_fio_copy },
	{ MIME_TRANSFER_BINARY,	mime_fio_copy,	    mime_fio_copy },
	{ MIME_TRANSFER_QUOTED, mime_fQP_encode,    mime_fQP_decode },
	{ MIME_TRANSFER_BASE64, mime_fB64_encode,   mime_fB64_decode },
	{ NULL,			NULL,		    NULL },
};


PUBLIC mime_codec_t
mime_fio_encoder(const char *ename)
{
	const struct transfer_encoding_s *tep = NULL;

	if (ename == NULL)
		return NULL;

	for (tep = transfer_encoding_tbl; tep->name; tep++)
		if (strcasecmp(tep->name, ename) == 0)
			break;
	return tep->enc;
}

PUBLIC mime_codec_t
mime_fio_decoder(const char *ename)
{
	const struct transfer_encoding_s *tep = NULL;

	if (ename == NULL)
		return NULL;

	for (tep = transfer_encoding_tbl; tep->name; tep++)
		if (strcasecmp(tep->name, ename) == 0)
			break;
	return tep->dec;
}

/*
 * This is for use in complete.c and mime.c to get the list of
 * encoding names without exposing the transfer_encoding_tbl[].  The
 * first name is returned if called with a pointer to a NULL pointer.
 * Subsequent calls with the same cookie give successive names.  A
 * NULL return indicates the end of the list.
 */
PUBLIC const char *
mime_next_encoding_name(const void **cookie)
{
	const struct transfer_encoding_s *tep;

	tep = *cookie;
	if (tep == NULL)
		tep = transfer_encoding_tbl;

	*cookie = tep->name ? &tep[1] : NULL;

	return tep->name;
}

#endif /* MIME_SUPPORT */
