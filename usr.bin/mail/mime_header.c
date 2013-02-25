/*	$NetBSD: mime_header.c,v 1.8.12.1 2013/02/25 00:30:36 tls Exp $	*/

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
 * This module contains the core MIME header decoding routines.
 * Please refer to RFC 2047 and RFC 2822.
 */

#ifdef MIME_SUPPORT

#include <sys/cdefs.h>
#ifndef __lint__
__RCSID("$NetBSD: mime_header.c,v 1.8.12.1 2013/02/25 00:30:36 tls Exp $");
#endif /* not __lint__ */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "def.h"
#include "extern.h"
#include "mime.h"
#include "mime_header.h"
#include "mime_codecs.h"

static const char *
grab_charset(char *from_cs, size_t from_cs_len, const char *p)
{
	char *q;
	q = from_cs;
	for (/*EMPTY*/; *p != '?'; p++) {
		if (*p == '\0' || q >= from_cs + from_cs_len - 1)
			return NULL;
		*q++ = *p;
	}
	*q = '\0';
	return ++p;	/* if here, then we got the '?' */
}

/*
 * An encoded word is a string of at most 75 non-white space
 * characters of the following form:
 *
 *  =?charset?X?encoding?=
 *
 * where:
 *   'charset'	is the original character set of the unencoded string.
 *
 *   'X'	is the encoding type 'B' or 'Q' for "base64" or
 *              "quoted-printable", respectively,
 *   'encoding'	is the encoded string.
 *
 * Both 'charset' and 'X' are case independent and 'encoding' cannot
 * contain any whitespace or '?' characters.  The 'encoding' must also
 * be fully contained within the encoded words, i.e., it cannot be
 * split between encoded words.
 *
 * Note: the 'B' encoding is a slightly modified "quoted-printable"
 * encoding.  In particular, spaces (' ') may be encoded as '_' to
 * improve undecoded readability.
 */
static int
decode_word(const char **ibuf, char **obuf, char *oend, const char *to_cs)
{
	ssize_t declen;
	size_t enclen, dstlen;
	char decword[LINESIZE];
	char from_cs[LINESIZE];
	const char *encword, *iend, *p;
	char *dstend;
	char enctype;

	p = *ibuf;
	if (p[0] != '=' && p[1] != '?')
		return -1;
	if (strlen(p) <  2 + 1 + 3 + 1 + 2)
		return -1;
	p = grab_charset(from_cs, sizeof(from_cs), p + 2);
	if (p == NULL)
		return -1;
	enctype = *p++;
	if (*p++ != '?')
		return -1;
	encword = p;
	p = strchr(p, '?');
	if (p == NULL || p[1] != '=')
		return -1;
	enclen = p - encword;	/* length of encoded substring */
	iend = p + 2;
	/* encoded words are at most 75 characters (RFC 2047, sec 2) */
	if (iend > *ibuf + 75)
		return -1;

	if (oend < *obuf + 1) {
		assert(/*CONSTCOND*/ 0);	/* We have a coding error! */
		return -1;
	}
	dstend = to_cs ? decword : *obuf;
	dstlen = (to_cs ? sizeof(decword) : (size_t)(oend - *obuf)) - 1;

	declen = mime_rfc2047_decode(enctype, dstend, dstlen, encword, enclen);
	if (declen == -1)
		return -1;

	dstend += declen;
#ifdef CHARSET_SUPPORT
	if (to_cs != NULL) {
		iconv_t cd;
		const char *src;
		size_t srclen;
		size_t cnt;

		cd = iconv_open(to_cs, from_cs);
		if (cd == (iconv_t)-1)
			return -1;

		src = decword;
		srclen = declen;
		dstend = *obuf;
		dstlen = oend - *obuf - 1;
		cnt = mime_iconv(cd, &src, &srclen, &dstend, &dstlen);

		(void)iconv_close(cd);
		if (cnt == (size_t)-1)
			return -1;
	}
#endif /* CHARSET_SUPPORT */
	*dstend = '\0';
	*ibuf = iend;
	*obuf = dstend;
	return 0;
}


/*
 * Folding White Space.  See RFC 2822.
 *
 * Note: RFC 2822 specifies that '\n' and '\r' only occur as CRLF
 * pairs (i.e., "\r\n") and never separately.  However, by the time
 * mail(1) sees the messages, all CRLF pairs have been converted to
 * '\n' characters.
 *
 * XXX - pull is_FWS() and skip_FWS() up to def.h?
 */
static inline int
is_FWS(int c)
{
	return c == ' ' || c == '\t' || c == '\n';
}

static inline const char *
skip_FWS(const char *p)
{
	while (is_FWS(*p))
		p++;
	return p;
}

static inline void
copy_skipped_FWS(char **dst, char *dstend, const char **src, const char *srcend)
{
	const char *p, *pend;
	char *q, *qend;

	p = *src;
	q = *dst;
	pend = srcend;
	qend = dstend;

	if (p) {  /* copy any skipped linear-white-space */
		while (p < pend && q < qend)
			*q++ = *p++;
		*dst = q;
		*src = NULL;
	}
}

/*
 * Decode an unstructured field.
 *
 * See RFC 2822 Sec 2.2.1 and 3.6.5.
 * Encoded words may occur anywhere in unstructured fields provided
 * they are separated from any other text or encoded words by at least
 * one linear-white-space character. (See RFC 2047 sec 5.1.)  If two
 * encoded words occur sequentially (separated by only FWS) then the
 * separating FWS is removed.
 *
 * NOTE: unstructured fields cannot contain 'quoted-pairs' (see
 * RFC2822 sec 3.2.6 and RFC 2047), but that is no problem as a '\\'
 * (or any non-whitespace character) immediately before an
 * encoded-word will prevent it from being decoded.
 *
 * hstring should be a NULL terminated string.
 * outbuf should be sufficiently large to hold the result.
 */
static void
mime_decode_usfield(char *outbuf, size_t outsize, const char *hstring)
{
	const char *p, *p0;
	char *q, *qend;
	int lastc;
	const char *charset;

	charset = value(ENAME_MIME_CHARSET);
	qend = outbuf + outsize - 1; /* Make sure there is room for the trailing NULL! */
	q = outbuf;
	p = hstring;
	p0 = NULL;
	lastc = (unsigned char)' ';
	while (*p && q < qend) {
		const char *p1;
		char *q1;
		if (is_FWS(lastc) && p[0] == '=' && p[1] == '?' &&
		    decode_word((p1 = p, &p1), (q1 = q, &q1), qend, charset) == 0 &&
		    (*p1 == '\0' || is_FWS(*p1))) {
			p0 = p1;  /* pointer to first character after encoded word */
			q = q1;
			p = skip_FWS(p1);
			lastc = (unsigned char)*p0;
		}
		else {
			copy_skipped_FWS(&q, qend, &p0, p);
			lastc = (unsigned char)*p;
			if (q < qend)
				*q++ = *p++;
		}
	}
	copy_skipped_FWS(&q, qend, &p0, p);
	*q = '\0';
}

/*
 * Decode a field comment.
 *
 * Comments only occur in structured fields, can be nested (rfc 2822,
 * sec 3.2.3), and can contain 'encoded-words' and 'quoted-pairs'.
 * Otherwise, they can be regarded as unstructured fields that are
 * bounded by '(' and ')' characters.
 */
static int
decode_comment(char **obuf, char *oend, const char **ibuf, const char *iend, const char *charset)
{
	const char *p, *pend, *p0;
	char *q, *qend;
	int lastc;

	p = *ibuf;
	q = *obuf;
	pend = iend;
	qend = oend;
	lastc = ' ';
	p0 = NULL;
	while (p < pend && q < qend) {
		const char *p1;
		char *q1;

		if (is_FWS(lastc) && p[0] == '=' && p[1] == '?' &&
		    decode_word((p1 = p, &p1), (q1 = q, &q1), qend, charset) == 0 &&
		    (*p1 == ')' || is_FWS(*p1))) {
			lastc = (unsigned char)*p1;
			p0 = p1;
			q = q1;
			p = skip_FWS(p1);
			/*
			 * XXX - this check should be unnecessary as *pend should
			 * be '\0' which will stop skip_FWS()
			 */
			if (p > pend)
				p = pend;
		}
		else {
			copy_skipped_FWS(&q, qend, &p0, p);
			if (q >= qend)	/* XXX - q > qend cannot happen */
				break;

			if (*p == ')') {
				*q++ = *p++;	/* copy the closing ')' */
				break;		/* and get out of here! */
			}

			if (*p == '(') {
				*q++ = *p++;	/* copy the opening '(' */
				if (decode_comment(&q, qend, &p, pend, charset) == -1)
					return -1;	/* is this right or should we update? */
				lastc = ')';
			}
			else if (*p == '\\' && p + 1 < pend) {	/* quoted-pair */
				if (p[1] == '(' || p[1] == ')' || p[1] == '\\') /* need quoted-pair*/
					*q++ = *p;
				p++;
				lastc = (unsigned char)*p;
				if (q < qend)
					*q++ = *p++;
			}
			else {
				lastc = (unsigned char)*p;
				*q++ = *p++;
			}
		}
	}
	*ibuf = p;
	*obuf = q;
	return 0;
}

/*
 * Decode a quoted-string or no-fold-quote.
 *
 * These cannot contain encoded words.  They can contain quoted-pairs,
 * making '\\' special.  They have no other structure.  See RFC 2822
 * sec 3.2.5 and 3.6.4.
 */
static void
decode_quoted_string(char **obuf, char *oend, const char **ibuf, const char *iend)
{
	const char *p, *pend;
	char *q, *qend;

	qend = oend;
	pend = iend;
	p = *ibuf;
	q = *obuf;
	while (p < pend && q < qend) {
		if (*p == '"') {
			*q++ = *p++;	/* copy the closing '"' */
			break;
		}
		if (*p == '\\' && p + 1 < pend) { /* quoted-pair */
			if (p[1] == '"' || p[1] == '\\') {
				*q++ = *p;
				if (q >= qend)
					break;
			}
			p++;
		}
		*q++ = *p++;
	}
	*ibuf = p;
	*obuf = q;
}

/*
 * Decode a domain-literal or no-fold-literal.
 *
 * These cannot contain encoded words.  They can have quoted pairs and
 * are delimited by '[' and ']' making '\\', '[', and ']' special.
 * They have no other structure.  See RFC 2822 sec 3.4.1 and 3.6.4.
 */
static void
decode_domain_literal(char **obuf, char *oend, const char **ibuf, const char *iend)
{
	const char *p, *pend;
	char *q, *qend;

	qend = oend;
	pend = iend;
	p = *ibuf;
	q = *obuf;
	while (p < pend && q < qend) {
		if (*p == ']') {
			*q++ = *p++;	/* copy the closing ']' */
			break;
		}
		if (*p == '\\' && p + 1 < pend) { /* quoted-pair */
			if (p[1] == '[' || p[1] == ']' || p[1] == '\\') {
				*q++ = *p;
				if (q >= qend)
					break;
			}
			p++;
		}
		*q++ = *p++;
	}
	*ibuf = p;
	*obuf = q;
}

/*
 * Specials: see RFC 2822 sec 3.2.1.
 */
static inline int
is_specials(int c)
{
	static const char specialtab[] = {
		0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
		0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
		0, 0, 1, 0,  0, 0, 0, 0,  1, 1, 0, 0,  1, 0, 1, 0,
		0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 1, 1,  1, 0, 1, 0,

		1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
		0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 1,  1, 1, 0, 0,
		0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
		0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,
	};
	return !(c & ~0x7f) ? specialtab[c] : 0;
}

/*
 * Decode a structured field.
 *
 * At the top level, structured fields can only contain encoded-words
 * via 'phrases' and 'comments'.  See RFC 2047 sec 5.
 */
static void
mime_decode_sfield(char *linebuf, size_t bufsize, const char *hstring)
{
	const char *p, *pend, *p0;
	char *q, *qend;
	const char *charset;
	int lastc;

	charset = value(ENAME_MIME_CHARSET);

	p = hstring;
	q = linebuf;
	pend = hstring + strlen(hstring);
	qend = linebuf + bufsize - 1;	/* save room for the NULL terminator */
	lastc = (unsigned char)' ';
	p0 = NULL;
	while (p < pend && q < qend) {
		const char *p1;
		char *q1;

		if (*p != '=') {
			copy_skipped_FWS(&q, qend, &p0, p);
			if (q >= qend)
				break;
		}

		switch (*p) {
		case '(':	/* start of comment */
			*q++ = *p++;	/* copy the opening '(' */
			(void)decode_comment(&q, qend, &p, pend, charset);
			lastc = (unsigned char)p[-1];
			break;

		case '"':	/* start of quoted-string or no-fold-quote */
			*q++ = *p++;	/* copy the opening '"' */
			decode_quoted_string(&q, qend, &p, pend);
			lastc = (unsigned char)p[-1];
			break;

		case '[':	/* start of domain-literal or no-fold-literal */
			*q++ = *p++;	/* copy the opening '[' */
			decode_domain_literal(&q, qend, &p, pend);
			lastc = (unsigned char)p[-1];
			break;

		case '\\':	/* start of quoted-pair */
			if (p + 1 < pend) {		/* quoted pair */
				if (is_specials(p[1])) {
					*q++ = *p;
					if (q >= qend)
						break;
				}
				p++;	/* skip the '\\' */
			}
			goto copy_char;

		case '=':
			/*
			 * At this level encoded words can appear via
			 * 'phrases' (possibly delimited by ',' as in
			 * 'keywords').  Thus we handle them as such.
			 * Hopefully this is sufficient.
			 */
			if ((lastc == ',' || is_FWS(lastc)) && p[1] == '?' &&
			    decode_word((p1 = p, &p1), (q1 = q, &q1), qend, charset) == 0 &&
			    (*p1 == '\0' || *p1 == ',' || is_FWS(*p1))) {
				lastc = (unsigned char)*p1;
				p0 = p1;
				q = q1;
				p = skip_FWS(p1);
				/*
				 * XXX - this check should be
				 * unnecessary as *pend should be '\0'
				 * which will stop skip_FWS()
				 */
				if (p > pend)
					p = pend;
				break;
			}
			else {
				copy_skipped_FWS(&q, qend, &p0, p);
				if (q >= qend)
					break;
				goto copy_char;
			}

		case '<':	/* start of angle-addr, msg-id, or path. */
			/*
			 * A msg-id cannot contain encoded-pairs or
			 * encoded-words, but angle-addr and path can.
			 * Distinguishing between them seems to be
			 * unnecessary, so let's be loose and just
			 * decode them as if they were all the same.
			 */
		default:
	copy_char:
			lastc = (unsigned char)*p;
			*q++ = *p++;
			break;
		}
	}
	copy_skipped_FWS(&q, qend, &p0, p);
	*q = '\0';	/* null terminate the result! */
}

/*
 * Returns the correct hfield decoder, or NULL if none.
 * Info extracted from RFC 2822.
 *
 * name - pointer to field name of header line (with colon).
 */
PUBLIC hfield_decoder_t
mime_hfield_decoder(const char *name)
{
	static const struct field_decoder_tbl_s {
		const char *field_name;
		size_t field_len;
		hfield_decoder_t decoder;
	} field_decoder_tbl[] = {
#define X(s)	s, sizeof(s) - 1
		{ X("Received:"),			NULL },

		{ X("Content-Type:"),			NULL },
		{ X("Content-Disposition:"),		NULL },
		{ X("Content-Transfer-Encoding:"),	NULL },
		{ X("Content-Description:"),		mime_decode_sfield },
		{ X("Content-ID:"),			mime_decode_sfield },
		{ X("MIME-Version:"),			mime_decode_sfield },

		{ X("Bcc:"),				mime_decode_sfield },
		{ X("Cc:"),				mime_decode_sfield },
		{ X("Date:"),				mime_decode_sfield },
		{ X("From:"),				mime_decode_sfield },
		{ X("In-Reply-To:"),			mime_decode_sfield },
		{ X("Keywords:"),			mime_decode_sfield },
		{ X("Message-ID:"),			mime_decode_sfield },
		{ X("References:"),			mime_decode_sfield },
		{ X("Reply-To:"),			mime_decode_sfield },
		{ X("Return-Path:"),			mime_decode_sfield },
		{ X("Sender:"),				mime_decode_sfield },
		{ X("To:"),				mime_decode_sfield },
		{ X("Subject:"),			mime_decode_usfield },
		{ X("Comments:"),			mime_decode_usfield },
		{ X("X-"),				mime_decode_usfield },
		{ NULL, 0,				mime_decode_usfield },	/* optional-fields */
#undef X
	};
	const struct field_decoder_tbl_s *fp;

	/* XXX - this begs for a hash table! */
	for (fp = field_decoder_tbl; fp->field_name; fp++)
		if (strncasecmp(name, fp->field_name, fp->field_len) == 0)
			break;
	return fp->decoder;
}

#endif /* MIME_SUPPORT */
