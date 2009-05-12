/*-
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Alistair Crooks (agc@NetBSD.org)
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
 * Copyright (c) 2005-2008 Nominet UK (www.nic.uk)
 * All rights reserved.
 * Contributors: Ben Laurie, Rachel Willmer. The Contributors have asserted
 * their moral rights under the UK Copyright Design and Patents Act 1988 to
 * be recorded as the authors of this copyright work.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License.
 *
 * You may obtain a copy of the License at
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "config.h"

#include <sys/types.h>
#include <sys/stat.h>

#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif

#ifdef HAVE_SYS_PARAM_H 
#include <sys/param.h>
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_DIRECT_H
#include <direct.h>
#endif

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#ifdef HAVE_OPENSSL_IDEA_H
#include <openssl/cast.h>
#endif

#ifdef HAVE_OPENSSL_IDEA_H
#include <openssl/idea.h>
#endif

#ifdef HAVE_OPENSSL_AES_H
#include <openssl/aes.h>
#endif

#ifdef HAVE_OPENSSL_DES_H
#include <openssl/des.h>
#endif

#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef HAVE_TERMIOS_H
#include <termios.h>
#endif

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif

#include "errors.h"
#include "crypto.h"
#include "create.h"
#include "signature.h"
#include "packet.h"
#include "packet-parse.h"
#include "packet-show.h"
#include "packet.h"
#include "keyring.h"

#include "readerwriter.h"
#include "netpgpdefs.h"
#include "version.h"
#include "parse_local.h"


/**
 * \ingroup Internal_Readers_Generic
 * \brief Starts reader stack
 * \param pinfo Parse settings
 * \param reader Reader to use
 * \param destroyer Destroyer to use
 * \param vp Reader-specific arg
 */
void 
__ops_reader_set(__ops_parseinfo_t *pinfo,
		__ops_reader_func_t *reader,
		__ops_reader_destroyer_t *destroyer,
		void *vp)
{
	pinfo->readinfo.reader = reader;
	pinfo->readinfo.destroyer = destroyer;
	pinfo->readinfo.arg = vp;
}

/**
 * \ingroup Internal_Readers_Generic
 * \brief Adds to reader stack
 * \param pinfo Parse settings
 * \param reader Reader to use
 * \param destroyer Reader's destroyer
 * \param vp Reader-specific arg
 */
void 
__ops_reader_push(__ops_parseinfo_t *pinfo,
		__ops_reader_func_t *reader,
		__ops_reader_destroyer_t *destroyer,
		void *vp)
{
	__ops_reader_t *readinfo = calloc(1, sizeof(*readinfo));

	*readinfo = pinfo->readinfo;
	(void) memset(&pinfo->readinfo, 0x0, sizeof(pinfo->readinfo));
	pinfo->readinfo.next = readinfo;
	pinfo->readinfo.parent = pinfo;

	/* should copy accumulate flags from other reader? RW */
	pinfo->readinfo.accumulate = readinfo->accumulate;

	__ops_reader_set(pinfo, reader, destroyer, vp);
}

/**
 * \ingroup Internal_Readers_Generic
 * \brief Removes from reader stack
 * \param pinfo Parse settings
 */
void 
__ops_reader_pop(__ops_parseinfo_t * pinfo)
{
	__ops_reader_t *next = pinfo->readinfo.next;

	pinfo->readinfo = *next;
	(void) free(next);
}

/**
 * \ingroup Internal_Readers_Generic
 * \brief Gets arg from reader
 * \param readinfo Reader info
 * \return Pointer to reader info's arg
 */
void           *
__ops_reader_get_arg(__ops_reader_t * readinfo)
{
	return readinfo->arg;
}

/**************************************************************************/

#define CRC24_POLY 0x1864cfbL

/**
 * \struct dearmour_t
 */
typedef struct {
	enum {
		OUTSIDE_BLOCK = 0,
		BASE64,
		AT_TRAILER_NAME
	}               state;

	enum {
		NONE = 0,
		BEGIN_PGP_MESSAGE,
		BEGIN_PGP_PUBLIC_KEY_BLOCK,
		BEGIN_PGP_PRIVATE_KEY_BLOCK,
		BEGIN_PGP_MULTI,
		BEGIN_PGP_SIGNATURE,

		END_PGP_MESSAGE,
		END_PGP_PUBLIC_KEY_BLOCK,
		END_PGP_PRIVATE_KEY_BLOCK,
		END_PGP_MULTI,
		END_PGP_SIGNATURE,

		BEGIN_PGP_SIGNED_MESSAGE
	}               lastseen;

	__ops_parseinfo_t *parse_info;
	unsigned	seen_nl:1;
	unsigned	prev_nl:1;
	unsigned	allow_headers_without_gap:1;
			/* !< allow headers in armoured data that are
			* not separated from the data by a blank line
			* */
	unsigned	allow_no_gap:1;
			/* !< allow no blank line at the start of
			* armoured data */
	unsigned	allow_trailing_whitespace:1;
			/* !< allow armoured stuff to have trailing
			* whitespace where we wouldn't strictly expect
			* it */
	/* it is an error to get a cleartext message without a sig */
	unsigned   	expect_sig:1;
	unsigned   	got_sig:1;
	/* base64 stuff */
	unsigned        buffered;
	unsigned char   buffer[3];
	bool   		eof64;
	unsigned long   checksum;
	unsigned long   read_checksum;
	/* unarmoured text blocks */
	unsigned char   unarmoured[NETPGP_BUFSIZ];
	size_t          unarmoredc;
	/* pushed back data (stored backwards) */
	unsigned char  *pushback;
	unsigned        pushbackc;
	/* armoured block headers */
	__ops_headers_t	headers;
}               dearmour_t;

static void 
push_back(dearmour_t * dearmour, const unsigned char *buf,
	  unsigned length)
{
	unsigned        n;

	if (dearmour->pushback) {
		(void) fprintf(stderr, "push_back: already pushed back\n");
	} else {
		dearmour->pushback = calloc(1, length);
		for (n = 0; n < length; ++n) {
			dearmour->pushback[n] = buf[length - n - 1];
		}
		dearmour->pushbackc = length;
	}
}

/* this struct holds a textual header line */
typedef struct headerline_t {
	const char	*s;		/* the header line */
	size_t		 len;		/* its length */
	int		 type;		/* the defined type */
} headerline_t;

static headerline_t	headerlines[] = {
	{ "BEGIN PGP MESSAGE",		17, BEGIN_PGP_MESSAGE },
	{ "BEGIN PGP PUBLIC KEY BLOCK",	26, BEGIN_PGP_PUBLIC_KEY_BLOCK },
	{ "BEGIN PGP PRIVATE KEY BLOCK",27, BEGIN_PGP_PRIVATE_KEY_BLOCK },
	{ "BEGIN PGP MESSAGE, PART ",	25, BEGIN_PGP_MULTI },
	{ "BEGIN PGP SIGNATURE",	19, BEGIN_PGP_SIGNATURE },

	{ "END PGP MESSAGE",		15, END_PGP_MESSAGE },
	{ "END PGP PUBLIC KEY BLOCK",	24, END_PGP_PUBLIC_KEY_BLOCK },
	{ "END PGP PRIVATE KEY BLOCK",	25, END_PGP_PRIVATE_KEY_BLOCK },
	{ "END PGP MESSAGE, PART ",	22, END_PGP_MULTI },
	{ "END PGP SIGNATURE",		17, END_PGP_SIGNATURE },

	{ "BEGIN PGP SIGNED MESSAGE",	24, BEGIN_PGP_SIGNED_MESSAGE },

	{ NULL,				0, -1	}
};

/* search through the table of header lines */
static int
findheaderline(char *headerline)
{
	headerline_t	*hp;

	for (hp = headerlines ; hp->s ; hp++) {
		if (strncmp(headerline, hp->s, hp->len) == 0) {
			break;
		}
	}
	return hp->type;
}

static int 
set_lastseen_headerline(dearmour_t *dearmour, char *hdr, __ops_error_t **errors)
{
	int	prev = dearmour->lastseen;
	int	lastseen;

	if ((lastseen = findheaderline(hdr)) == -1) {
		OPS_ERROR_1(errors, OPS_E_R_BAD_FORMAT,
			"Unrecognised Header Line %s", hdr);
		return 0;
	}
	dearmour->lastseen = lastseen;
	if (__ops_get_debug_level(__FILE__)) {
		printf("set header: hdr=%s, dearmour->lastseen=%d, prev=%d\n",
			hdr, dearmour->lastseen, prev);
	}
	switch (dearmour->lastseen) {
	case NONE:
		OPS_ERROR_1(errors, OPS_E_R_BAD_FORMAT,
			"Unrecognised last seen Header Line %s", hdr);
		break;

	case END_PGP_MESSAGE:
		if (prev != BEGIN_PGP_MESSAGE) {
			OPS_ERROR(errors, OPS_E_R_BAD_FORMAT,
				"Got END PGP MESSAGE, but not after BEGIN");
		}
		break;

	case END_PGP_PUBLIC_KEY_BLOCK:
		if (prev != BEGIN_PGP_PUBLIC_KEY_BLOCK) {
			OPS_ERROR(errors, OPS_E_R_BAD_FORMAT,
			"Got END PGP PUBLIC KEY BLOCK, but not after BEGIN");
		}
		break;

	case END_PGP_PRIVATE_KEY_BLOCK:
		if (prev != BEGIN_PGP_PRIVATE_KEY_BLOCK) {
			OPS_ERROR(errors, OPS_E_R_BAD_FORMAT,
			"Got END PGP PRIVATE KEY BLOCK, but not after BEGIN");
		}
		break;

	case BEGIN_PGP_MULTI:
	case END_PGP_MULTI:
		OPS_ERROR(errors, OPS_E_R_UNSUPPORTED,
			"Multi-part messages are not yet supported");
		break;

	case END_PGP_SIGNATURE:
		if (prev != BEGIN_PGP_SIGNATURE) {
			OPS_ERROR(errors, OPS_E_R_BAD_FORMAT,
			"Got END PGP SIGNATURE, but not after BEGIN");
		}
		break;

	case BEGIN_PGP_MESSAGE:
	case BEGIN_PGP_PUBLIC_KEY_BLOCK:
	case BEGIN_PGP_PRIVATE_KEY_BLOCK:
	case BEGIN_PGP_SIGNATURE:
	case BEGIN_PGP_SIGNED_MESSAGE:
		break;
	}
	return 1;
}

static int 
read_char(dearmour_t * dearmour,
		__ops_error_t **errors,
		__ops_reader_t *readinfo,
		__ops_callback_data_t *cbinfo,
		bool skip)
{
	unsigned char   c[1];

	do {
		if (dearmour->pushbackc) {
			c[0] = dearmour->pushback[--dearmour->pushbackc];
			if (dearmour->pushbackc == 0) {
				(void) free(dearmour->pushback);
				dearmour->pushback = NULL;
			}
		}
		/*
		 * XXX: should __ops_stacked_read exist? Shouldn't this be a
		 * limited_read?
		 */
		else if (__ops_stacked_read(c, 1, errors, readinfo, cbinfo) != 1)
			return -1;
	}
	while (skip && c[0] == '\r');

	dearmour->prev_nl = dearmour->seen_nl;
	dearmour->seen_nl = c[0] == '\n';

	return c[0];
}

static int 
eat_whitespace(int first,
	       dearmour_t *dearmour,
	       __ops_error_t **errors,
	       __ops_reader_t *readinfo,
	       __ops_callback_data_t *cbinfo,
	       bool skip)
{
	int             c = first;

	while (c == ' ' || c == '\t') {
		c = read_char(dearmour, errors, readinfo, cbinfo, skip);
	}
	return c;
}

static int 
read_and_eat_whitespace(dearmour_t *dearmour,
			__ops_error_t **errors,
			__ops_reader_t *readinfo,
			__ops_callback_data_t *cbinfo,
			bool skip)
{
	int             c;

	do {
		c = read_char(dearmour, errors, readinfo, cbinfo, skip);
	} while (c == ' ' || c == '\t');
	return c;
}

static void 
flush(dearmour_t *dearmour, __ops_callback_data_t *cbinfo)
{
	__ops_packet_t	content;

	if (dearmour->unarmoredc == 0) {
		return;
	}
	content.u.unarmoured_text.data = dearmour->unarmoured;
	content.u.unarmoured_text.length = dearmour->unarmoredc;
	CALLBACK(cbinfo, OPS_PTAG_CT_UNARMOURED_TEXT, &content);
	dearmour->unarmoredc = 0;
}

static int 
unarmoured_read_char(dearmour_t *dearmour,
			__ops_error_t **errors,
			__ops_reader_t *readinfo,
			__ops_callback_data_t *cbinfo,
			bool skip)
{
	int             c;

	do {
		c = read_char(dearmour, errors, readinfo, cbinfo, false);
		if (c < 0) {
			return c;
		}
		dearmour->unarmoured[dearmour->unarmoredc++] = c;
		if (dearmour->unarmoredc == sizeof(dearmour->unarmoured)) {
			flush(dearmour, cbinfo);
		}
	} while (skip && c == '\r');
	return c;
}

/**
 * \param headers
 * \param key
 *
 * \return header value if found, otherwise NULL
 */
static const char *
__ops_find_header(__ops_headers_t *headers, const char *key)
{
	unsigned        n;

	for (n = 0; n < headers->headerc; ++n) {
		if (strcmp(headers->headers[n].key, key) == 0) {
			return headers->headers[n].value;
		}
	}
	return NULL;
}

/**
 * \param dest
 * \param src
 */
static void 
__ops_dup_headers(__ops_headers_t *dest, const __ops_headers_t *src)
{
	unsigned        n;

	dest->headers = calloc(src->headerc, sizeof(*dest->headers));
	dest->headerc = src->headerc;
	for (n = 0; n < src->headerc; ++n) {
		dest->headers[n].key = strdup(src->headers[n].key);
		dest->headers[n].value = strdup(src->headers[n].value);
	}
}

/*
 * Note that this skips CRs so implementations always see just straight LFs
 * as line terminators
 */
static int 
process_dash_escaped(dearmour_t *dearmour,
			__ops_error_t **errors,
			__ops_reader_t *readinfo,
			__ops_callback_data_t *cbinfo)
{
	__ops_packet_t content;
	__ops_packet_t content2;
	__ops_cleartext_body_t *body = &content.u.cleartext_body;
	__ops_cleartext_trailer_t *trailer = &content2.u.cleartext_trailer;
	const char     *hashstr;
	__ops_hash_t     *hash;
	int             total;

	hash = calloc(1, sizeof(*hash));
	hashstr = __ops_find_header(&dearmour->headers, "Hash");
	if (hashstr) {
		__ops_hash_alg_t alg;

		alg = __ops_str_to_hash_alg(hashstr);
		if (!__ops_is_hash_alg_supported(&alg)) {
			(void) free(hash);
			OPS_ERROR_1(errors, OPS_E_R_BAD_FORMAT,
				"Unsupported hash algorithm '%s'", hashstr);
			return -1;
		}
		if (alg == OPS_HASH_UNKNOWN) {
			(void) free(hash);
			OPS_ERROR_1(errors, OPS_E_R_BAD_FORMAT,
				"Unknown hash algorithm '%s'", hashstr);
			return -1;
		}
		__ops_hash_any(hash, alg);
	} else {
		__ops_hash_md5(hash);
	}

	hash->init(hash);

	body->length = 0;
	total = 0;
	for (;;) {
		int             c;
		unsigned        count;

		if ((c = read_char(dearmour, errors, readinfo, cbinfo, true)) < 0) {
			return -1;
		}
		if (dearmour->prev_nl && c == '-') {
			if ((c = read_char(dearmour, errors, readinfo, cbinfo,
						false)) < 0) {
				return -1;
			}
			if (c != ' ') {
				/* then this had better be a trailer! */
				if (c != '-') {
					OPS_ERROR(errors, OPS_E_R_BAD_FORMAT,
						"Bad dash-escaping");
				}
				for (count = 2; count < 5; ++count) {
					if ((c = read_char(dearmour, errors,
						readinfo, cbinfo, false)) < 0) {
						return -1;
					}
					if (c != '-') {
						OPS_ERROR(errors,
						OPS_E_R_BAD_FORMAT,
						"Bad dash-escaping (2)");
					}
				}
				dearmour->state = AT_TRAILER_NAME;
				break;
			}
			/* otherwise we read the next character */
			if ((c = read_char(dearmour, errors, readinfo, cbinfo,
						false)) < 0) {
				return -1;
			}
		}
		if (c == '\n' && body->length) {
			if (memchr(body->data + 1, '\n', body->length - 1)
						!= NULL) {
				(void) fprintf(stderr,
				"process_dash_escaped: newline found\n");
				return -1;
			}
			if (body->data[0] == '\n') {
				hash->add(hash, (const unsigned char *)"\r", 1);
			}
			hash->add(hash, body->data, body->length);
			if (__ops_get_debug_level(__FILE__)) {
				fprintf(stderr, "Got body:\n%s\n", body->data);
			}
			CALLBACK(cbinfo, OPS_PTAG_CT_SIGNED_CLEARTEXT_BODY,
						&content);
			body->length = 0;
		}
		body->data[body->length++] = c;
		++total;
		if (body->length == sizeof(body->data)) {
			if (__ops_get_debug_level(__FILE__)) {
				(void) fprintf(stderr, "Got body (2):\n%s\n",
						body->data);
			}
			CALLBACK(cbinfo, OPS_PTAG_CT_SIGNED_CLEARTEXT_BODY,
					&content);
			body->length = 0;
		}
	}
	if (body->data[0] != '\n') {
		(void) fprintf(stderr,
			"process_dash_escaped: no newline in body data\n");
		return -1;
	}
	if (body->length != 1) {
		(void) fprintf(stderr,
			"process_dash_escaped: bad body length\n");
		return -1;
	}

	/* don't send that one character, because it's part of the trailer */
	trailer->hash = hash;
	CALLBACK(cbinfo, OPS_PTAG_CT_SIGNED_CLEARTEXT_TRAILER, &content2);
	return total;
}

static int 
add_header(dearmour_t * dearmour, const char *key, const char *value)
{
	int	n;

	/*
         * Check that the header is valid
         */
	if (strcmp(key, "Version") == 0 ||
	    strcmp(key, "Comment") == 0 ||
	    strcmp(key, "MessageID") == 0 ||
	    strcmp(key, "Hash") == 0 ||
	    strcmp(key, "Charset") == 0) {
		n = dearmour->headers.headerc;
		dearmour->headers.headers = realloc(dearmour->headers.headers,
				(n + 1) * sizeof(*dearmour->headers.headers));
		dearmour->headers.headers[n].key = strdup(key);
		dearmour->headers.headers[n].value = strdup(value);
		dearmour->headers.headerc = n + 1;
		return 1;
	}
	return 0;
}

/* \todo what does a return value of 0 indicate? 1 is good, -1 is bad */
static int 
parse_headers(dearmour_t * dearmour, __ops_error_t ** errors,
	      __ops_reader_t * readinfo, __ops_callback_data_t * cbinfo)
{
	unsigned        nbuf;
	unsigned        size;
	char           *buf;
	bool   		first = true;
	int             rtn = 1;

	buf = NULL;
	nbuf = size = 0;

	for (;;) {
		int             c;

		if ((c = read_char(dearmour, errors, readinfo, cbinfo, true)) < 0) {
			OPS_ERROR(errors, OPS_E_R_BAD_FORMAT, "Unexpected EOF");
			rtn = -1;
			break;
		}
		if (c == '\n') {
			char           *s;

			if (nbuf == 0) {
				break;
			}

			if (nbuf >= size) {
				(void) fprintf(stderr,
					"parse_headers: bad size\n");
				return -1;
			}
			buf[nbuf] = '\0';

			s = strchr(buf, ':');
			if (!s) {
				if (!first && !dearmour->allow_headers_without_gap) {
					/*
					 * then we have seriously malformed
					 * armour
					 */
					OPS_ERROR(errors, OPS_E_R_BAD_FORMAT, "No colon in armour header");
					rtn = -1;
					break;
				} else {
					if (first &&
					    !(dearmour->allow_headers_without_gap || dearmour->allow_no_gap)) {
						OPS_ERROR(errors, OPS_E_R_BAD_FORMAT, "No colon in armour header (2)");
						/*
						 * then we have a nasty
						 * armoured block with no
						 * headers, not even a blank
						 * line.
						 */
						buf[nbuf] = '\n';
						push_back(dearmour, (unsigned char *) buf, nbuf + 1);
						rtn = -1;
						break;
					}
				}
			} else {
				*s = '\0';
				if (s[1] != ' ') {
					OPS_ERROR(errors, OPS_E_R_BAD_FORMAT, "No space in armour header");
					rtn = -1;
					goto end;
				}
				if (!add_header(dearmour, buf, s + 2)) {
					OPS_ERROR_1(errors, OPS_E_R_BAD_FORMAT, "Invalid header %s", buf);
					rtn = -1;
					goto end;
				}
				nbuf = 0;
			}
			first = false;
		} else {
			if (size <= nbuf + 1) {
				size += size + 80;
				buf = realloc(buf, size);
			}
			buf[nbuf++] = c;
		}
	}

end:
	(void) free(buf);

	return rtn;
}

static int 
read4(dearmour_t *dearmour, __ops_error_t **errors,
      __ops_reader_t *readinfo, __ops_callback_data_t *cbinfo,
      int *pc, unsigned *pn, unsigned long *pl)
{
	int             n, c;
	unsigned long   l = 0;

	for (n = 0; n < 4; ++n) {
		c = read_char(dearmour, errors, readinfo, cbinfo, true);
		if (c < 0) {
			dearmour->eof64 = true;
			return -1;
		}
		if (c == '-' || c == '=') {
			break;
		}
		l <<= 6;
		if (c >= 'A' && c <= 'Z') {
			l += c - 'A';
		} else if (c >= 'a' && c <= 'z') {
			l += c - 'a' + 26;
		} else if (c >= '0' && c <= '9') {
			l += c - '0' + 52;
		} else if (c == '+') {
			l += 62;
		} else if (c == '/') {
			l += 63;
		} else {
			--n;
			l >>= 6;
		}
	}

	*pc = c;
	*pn = n;
	*pl = l;

	return 4;
}

unsigned 
__ops_crc24(unsigned checksum, unsigned char c)
{
	unsigned        i;

	checksum ^= c << 16;
	for (i = 0; i < 8; i++) {
		checksum <<= 1;
		if (checksum & 0x1000000)
			checksum ^= CRC24_POLY;
	}
	return checksum & 0xffffffL;
}

static int 
decode64(dearmour_t *dearmour, __ops_error_t **errors,
	 __ops_reader_t *readinfo, __ops_callback_data_t *cbinfo)
{
	unsigned        n;
	int             n2;
	unsigned long   l;
	int             c;
	int             ret;

	if (dearmour->buffered != 0) {
		(void) fprintf(stderr, "decode64: bad dearmour->buffered\n");
		return 0;
	}

	ret = read4(dearmour, errors, readinfo, cbinfo, &c, &n, &l);
	if (ret < 0) {
		OPS_ERROR(errors, OPS_E_R_BAD_FORMAT, "Badly formed base64");
		return 0;
	}
	if (n == 3) {
		if (c != '=') {
			OPS_ERROR(errors, OPS_E_R_BAD_FORMAT,
					"Badly terminated base64 (2)");
			return 0;
		}
		dearmour->buffered = 2;
		dearmour->eof64 = true;
		l >>= 2;
	} else if (n == 2) {
		if (c != '=') {
			OPS_ERROR(errors, OPS_E_R_BAD_FORMAT,
					"Badly terminated base64 (3)");
			return 0;
		}
		dearmour->buffered = 1;
		dearmour->eof64 = true;
		l >>= 4;
		c = read_char(dearmour, errors, readinfo, cbinfo, false);
		if (c != '=') {
			OPS_ERROR(errors, OPS_E_R_BAD_FORMAT,
					"Badly terminated base64");
			return 0;
		}
	} else if (n == 0) {
		if (!dearmour->prev_nl || c != '=') {
			OPS_ERROR(errors, OPS_E_R_BAD_FORMAT,
					"Badly terminated base64 (4)");
			return 0;
		}
		dearmour->buffered = 0;
	} else {
		if (n != 4) {
			(void) fprintf(stderr,
				"decode64: bad n (!= 4)\n");
			return 0;
		}
		dearmour->buffered = 3;
		if (c == '-' || c == '=') {
			(void) fprintf(stderr, "decode64: bad c\n");
			return 0;
		}
	}

	if (dearmour->buffered < 3 && dearmour->buffered > 0) {
		/* then we saw padding */
		if (c != '=') {
			(void) fprintf(stderr, "decode64: bad c (=)\n");
			return 0;
		}
		c = read_and_eat_whitespace(dearmour, errors, readinfo, cbinfo,
				true);
		if (c != '\n') {
			OPS_ERROR(errors, OPS_E_R_BAD_FORMAT,
				"No newline at base64 end");
			return 0;
		}
		c = read_char(dearmour, errors, readinfo, cbinfo, false);
		if (c != '=') {
			OPS_ERROR(errors, OPS_E_R_BAD_FORMAT,
				"No checksum at base64 end");
			return 0;
		}
	}
	if (c == '=') {
		/* now we are at the checksum */
		ret = read4(dearmour, errors, readinfo, cbinfo, &c, &n,
				&dearmour->read_checksum);
		if (ret < 0 || n != 4) {
			OPS_ERROR(errors, OPS_E_R_BAD_FORMAT,
					"Error in checksum");
			return 0;
		}
		c = read_char(dearmour, errors, readinfo, cbinfo, true);
		if (dearmour->allow_trailing_whitespace)
			c = eat_whitespace(c, dearmour, errors, readinfo, cbinfo,
					true);
		if (c != '\n') {
			OPS_ERROR(errors, OPS_E_R_BAD_FORMAT,
					"Badly terminated checksum");
			return 0;
		}
		c = read_char(dearmour, errors, readinfo, cbinfo, false);
		if (c != '-') {
			OPS_ERROR(errors, OPS_E_R_BAD_FORMAT,
					"Bad base64 trailer (2)");
			return 0;
		}
	}
	if (c == '-') {
		for (n = 0; n < 4; ++n)
			if (read_char(dearmour, errors, readinfo, cbinfo,
						false) != '-') {
				OPS_ERROR(errors, OPS_E_R_BAD_FORMAT,
						"Bad base64 trailer");
				return 0;
			}
		dearmour->eof64 = true;
	} else {
		if (!dearmour->buffered) {
			(void) fprintf(stderr, "decode64: not buffered\n");
			return 0;
		}
	}

	for (n = 0; n < dearmour->buffered; ++n) {
		dearmour->buffer[n] = (unsigned char)l;
		l >>= 8;
	}

	for (n2 = dearmour->buffered - 1; n2 >= 0; --n2)
		dearmour->checksum = __ops_crc24((unsigned)dearmour->checksum,
					dearmour->buffer[n2]);

	if (dearmour->eof64 && dearmour->read_checksum != dearmour->checksum) {
		OPS_ERROR(errors, OPS_E_R_BAD_FORMAT, "Checksum mismatch");
		return 0;
	}
	return 1;
}

static void 
base64(dearmour_t * dearmour)
{
	dearmour->state = BASE64;
	dearmour->checksum = CRC24_INIT;
	dearmour->eof64 = false;
	dearmour->buffered = 0;
}

/* This reader is rather strange in that it can generate callbacks for */
/* content - this is because plaintext is not encapsulated in PGP */
/* packets... it also calls back for the text between the blocks. */

static int 
armoured_data_reader(void *dest_, size_t length, __ops_error_t ** errors,
		     __ops_reader_t * readinfo,
		     __ops_callback_data_t * cbinfo)
{
	dearmour_t *dearmour = __ops_reader_get_arg(readinfo);
	__ops_packet_t content;
	int             ret;
	bool   first;
	unsigned char  *dest = dest_;
	int             saved = length;

	if (dearmour->eof64 && !dearmour->buffered) {
		if (dearmour->state != OUTSIDE_BLOCK &&
		    dearmour->state != AT_TRAILER_NAME) {
			(void) fprintf(stderr,
				"armoured_data_reader: bad dearmour state\n");
			return 0;
		}
	}

	while (length > 0) {
		unsigned        count;
		unsigned        n;
		char            buf[1024];
		int             c;

		flush(dearmour, cbinfo);
		switch (dearmour->state) {
		case OUTSIDE_BLOCK:
			/*
			 * This code returns EOF rather than EARLY_EOF
			 * because if we don't see a header line at all, then
			 * it is just an EOF (and not a BLOCK_END)
			 */
			while (!dearmour->seen_nl) {
				if ((c = unarmoured_read_char(dearmour, errors,
						readinfo, cbinfo, true)) < 0) {
					return 0;
				}
			}

			/*
			 * flush at this point so we definitely have room for
			 * the header, and so we can easily erase it from the
			 * buffer
			 */
			flush(dearmour, cbinfo);
			/* Find and consume the 5 leading '-' */
			for (count = 0; count < 5; ++count) {
				if ((c = unarmoured_read_char(dearmour, errors,
						readinfo, cbinfo, false)) < 0) {
					return 0;
				}
				if (c != '-') {
					goto reloop;
				}
			}

			/* Now find the block type */
			for (n = 0; n < sizeof(buf) - 1;) {
				if ((c = unarmoured_read_char(dearmour, errors,
						readinfo, cbinfo, false)) < 0) {
					return 0;
				}
				if (c == '-') {
					goto got_minus;
				}
				buf[n++] = c;
			}
			/* then I guess this wasn't a proper header */
			break;

got_minus:
			buf[n] = '\0';

			/* Consume trailing '-' */
			for (count = 1; count < 5; ++count) {
				if ((c = unarmoured_read_char(dearmour, errors,
						readinfo, cbinfo, false)) < 0) {
					return 0;
				}
				if (c != '-') {
					/* wasn't a header after all */
					goto reloop;
				}
			}

			/* Consume final NL */
			if ((c = unarmoured_read_char(dearmour, errors, readinfo,
						cbinfo, true)) < 0) {
				return 0;
			}
			if (dearmour->allow_trailing_whitespace) {
				if ((c = eat_whitespace(c, dearmour, errors,
						readinfo, cbinfo, true)) < 0) {
					return 0;
				}
			}
			if (c != '\n') {
				/* wasn't a header line after all */
				break;
			}

			/*
			 * Now we've seen the header, scrub it from the
			 * buffer
			 */
			dearmour->unarmoredc = 0;

			/*
			 * But now we've seen a header line, then errors are
			 * EARLY_EOF
			 */
			if ((ret = parse_headers(dearmour, errors, readinfo,
					cbinfo)) <= 0) {
				return -1;
			}

			if (!set_lastseen_headerline(dearmour, buf, errors)) {
				return -1;
			}

			if (strcmp(buf, "BEGIN PGP SIGNED MESSAGE") == 0) {
				__ops_dup_headers(
				&content.u.cleartext_head.headers,
				&dearmour->headers);
				CALLBACK(cbinfo,
					OPS_PTAG_CT_SIGNED_CLEARTEXT_HEADER,
					&content);
				ret = process_dash_escaped(dearmour, errors,
						readinfo, cbinfo);
				if (ret <= 0) {
					return ret;
				}
			} else {
				content.u.armour_header.type = buf;
				content.u.armour_header.headers =
						dearmour->headers;
				(void) memset(&dearmour->headers, 0x0,
						sizeof(dearmour->headers));
				CALLBACK(cbinfo, OPS_PTAG_CT_ARMOUR_HEADER,
						&content);
				base64(dearmour);
			}
			break;

		case BASE64:
			first = true;
			while (length > 0) {
				if (!dearmour->buffered) {
					if (!dearmour->eof64) {
						ret = decode64(dearmour,
							errors, readinfo, cbinfo);
						if (ret <= 0) {
							return ret;
						}
					}
					if (!dearmour->buffered) {
						if (!dearmour->eof64) {
							(void) fprintf(stderr,
"armoured_data_reader: bad dearmour eof64\n");
							return 0;
						}
						if (first) {
							dearmour->state =
								AT_TRAILER_NAME;
							goto reloop;
						}
						return -1;
					}
				}
				if (!dearmour->buffered) {
					(void) fprintf(stderr,
			"armoured_data_reader: bad dearmour buffered\n");
					return 0;
				}
				*dest = dearmour->buffer[--dearmour->buffered];
				++dest;
				--length;
				first = false;
			}
			if (dearmour->eof64 && !dearmour->buffered) {
				dearmour->state = AT_TRAILER_NAME;
			}
			break;

		case AT_TRAILER_NAME:
			for (n = 0; n < sizeof(buf) - 1;) {
				if ((c = read_char(dearmour, errors, readinfo,
						cbinfo, false)) < 0) {
					return -1;
				}
				if (c == '-') {
					goto got_minus2;
				}
				buf[n++] = c;
			}
			/* then I guess this wasn't a proper trailer */
			OPS_ERROR(errors, OPS_E_R_BAD_FORMAT,
					"Bad ASCII armour trailer");
			break;

got_minus2:
			buf[n] = '\0';

			if (!set_lastseen_headerline(dearmour, buf, errors)) {
				return -1;
			}

			/* Consume trailing '-' */
			for (count = 1; count < 5; ++count) {
				if ((c = read_char(dearmour, errors, readinfo,
						cbinfo, false)) < 0) {
					return -1;
				}
				if (c != '-') {
					/* wasn't a trailer after all */
					OPS_ERROR(errors, OPS_E_R_BAD_FORMAT,
						"Bad ASCII armour trailer (2)");
				}
			}

			/* Consume final NL */
			if ((c = read_char(dearmour, errors, readinfo, cbinfo,
						true)) < 0) {
				return -1;
			}
			if (dearmour->allow_trailing_whitespace) {
				if ((c = eat_whitespace(c, dearmour, errors,
						readinfo, cbinfo, true)) < 0) {
					return 0;
				}
			}
			if (c != '\n') {
				/* wasn't a trailer line after all */
				OPS_ERROR(errors, OPS_E_R_BAD_FORMAT,
					"Bad ASCII armour trailer (3)");
			}

			if (strncmp(buf, "BEGIN ", 6) == 0) {
				if (!set_lastseen_headerline(dearmour, buf,
						errors)) {
					return -1;
				}
				if ((ret = parse_headers(dearmour, errors,
						readinfo, cbinfo)) <= 0) {
					return ret;
				}
				content.u.armour_header.type = buf;
				content.u.armour_header.headers =
						dearmour->headers;
				(void) memset(&dearmour->headers, 0x0,
						sizeof(dearmour->headers));
				CALLBACK(cbinfo, OPS_PTAG_CT_ARMOUR_HEADER,
						&content);
				base64(dearmour);
			} else {
				content.u.armour_trailer.type = buf;
				CALLBACK(cbinfo, OPS_PTAG_CT_ARMOUR_TRAILER,
						&content);
				dearmour->state = OUTSIDE_BLOCK;
			}
			break;
		}
reloop:
		continue;
	}

	return saved;
}

static void 
armoured_data_destroyer(__ops_reader_t * readinfo)
{
	(void) free(__ops_reader_get_arg(readinfo));
}

/**
 * \ingroup Core_Readers_Armour
 * \brief Pushes dearmouring reader onto stack
 * \param parse_info Usual structure containing information about to how to do the parse
 * \sa __ops_reader_pop_dearmour()
 */
void 
__ops_reader_push_dearmour(__ops_parseinfo_t * parse_info)
/*
 * This function originally had these parameters to cater for packets which
 * didn't strictly match the RFC. The initial 0.5 release is only going to
 * support strict checking. If it becomes desirable to support loose checking
 * of armoured packets and these params are reinstated, parse_headers() must
 * be fixed so that these flags work correctly.
 * 
 * // Allow headers in armoured data that are not separated from the data by a
 * blank line bool without_gap,
 * 
 * // Allow no blank line at the start of armoured data bool no_gap,
 * 
 * //Allow armoured data to have trailing whitespace where we strictly would not
 * expect it			      bool trailing_whitespace
 */
{
	dearmour_t *dearmour;

	dearmour = calloc(1, sizeof(*dearmour));
	dearmour->seen_nl = true;
	/*
	    dearmour->allow_headers_without_gap=without_gap;
	    dearmour->allow_no_gap=no_gap;
	    dearmour->allow_trailing_whitespace=trailing_whitespace;
	*/
	dearmour->expect_sig = false;
	dearmour->got_sig = false;

	__ops_reader_push(parse_info, armoured_data_reader,
			armoured_data_destroyer, dearmour);
}

/**
 * \ingroup Core_Readers_Armour
 * \brief Pops dearmour reader from stock
 * \param pinfo
 * \sa __ops_reader_push_dearmour()
 */
void 
__ops_reader_pop_dearmour(__ops_parseinfo_t * pinfo)
{
	dearmour_t *dearmour;

	dearmour = __ops_reader_get_arg(__ops_parse_get_rinfo(pinfo));
	(void) free(dearmour);
	__ops_reader_pop(pinfo);
}

/**************************************************************************/

/* this is used for *decrypting* */
typedef struct {
	unsigned char	 decrypted[1024];
	size_t		 decryptc;
	size_t		 decryptoff;
	__ops_crypt_t	*decrypt;
	__ops_region_t	*region;
	unsigned	 prevplain:1;
} encrypted_t;

static int 
encrypted_data_reader(void *dest, size_t length, __ops_error_t ** errors,
		      __ops_reader_t * readinfo,
		      __ops_callback_data_t * cbinfo)
{
	encrypted_t *encrypted = __ops_reader_get_arg(readinfo);
	int             saved = length;
	char           *cdest;

	/*
	 * V3 MPIs have the count plain and the cipher is reset after each
	 * count
	 */
	if (encrypted->prevplain && !readinfo->parent->reading_mpi_len) {
		if (!readinfo->parent->reading_v3_secret) {
			(void) fprintf(stderr,
				"encrypted_data_reader: bad v3 secret\n");
			return -1;
		}
		encrypted->decrypt->decrypt_resync(encrypted->decrypt);
		encrypted->prevplain = false;
	} else if (readinfo->parent->reading_v3_secret &&
		   readinfo->parent->reading_mpi_len) {
		encrypted->prevplain = true;
	}
	while (length > 0) {
		if (encrypted->decryptc) {
			unsigned        n;

			/*
			 * if we are reading v3 we should never read
			 * more than we're asked for */
			if (length < encrypted->decryptc &&
			     (readinfo->parent->reading_v3_secret ||
			      readinfo->parent->exact_read)) {
				(void) fprintf(stderr,
					"encrypted_data_reader: bad v3 read\n");
				return 0;
			}

			n = MIN(length, encrypted->decryptc);

			(void) memcpy(dest,
			encrypted->decrypted + encrypted->decryptoff, n);
			encrypted->decryptc -= n;
			encrypted->decryptoff += n;
			length -= n;
			cdest = dest;
			cdest += n;
			dest = cdest;
		} else {
			unsigned        n = encrypted->region->length;
			unsigned char   buffer[1024];

			if (!n) {
				return -1;
			}
			if (!encrypted->region->indeterminate) {
				n -= encrypted->region->length_read;
				if (n == 0)
					return saved - length;
				if (n > sizeof(buffer))
					n = sizeof(buffer);
			} else {
				n = sizeof(buffer);
			}

			/*
			 * we can only read as much as we're asked for
			 * in v3 keys because they're partially
			 * unencrypted!  */
			if ((readinfo->parent->reading_v3_secret ||
			     readinfo->parent->exact_read) && n > length) {
				n = length;
			}

			if (!__ops_stacked_limited_read(buffer, n,
				encrypted->region, errors, readinfo, cbinfo)) {
				return -1;
			}
			if (!readinfo->parent->reading_v3_secret ||
			    !readinfo->parent->reading_mpi_len) {
				encrypted->decryptc =
					__ops_decrypt_se_ip(encrypted->decrypt,
					encrypted->decrypted, buffer, n);

				if (__ops_get_debug_level(__FILE__)) {
					int             i;

					(void) fprintf(stderr,
						"READING:\nencrypted: ");
					for (i = 0; i < 16; i++) {
						(void) fprintf(stderr,
							"%2x ", buffer[i]);
					}
					(void) fprintf(stderr, "\ndecrypted: ");
					for (i = 0; i < 16; i++) {
						(void) fprintf(stderr, "%2x ",
						encrypted->decrypted[i]);
					}
					(void) fprintf(stderr, "\n");
				}
			} else {
				(void) memcpy(
	&encrypted->decrypted[encrypted->decryptoff], buffer, n);
				encrypted->decryptc = n;
			}

			if (encrypted->decryptc == 0) {
				(void) fprintf(stderr,
				"encrypted_data_reader: 0 decrypted count\n");
				return 0;
			}

			encrypted->decryptoff = 0;
		}
	}

	return saved;
}

static void 
encrypted_data_destroyer(__ops_reader_t * readinfo)
{
	(void) free(__ops_reader_get_arg(readinfo));
}

/**
 * \ingroup Core_Readers_SE
 * \brief Pushes decryption reader onto stack
 * \sa __ops_reader_pop_decrypt()
 */
void 
__ops_reader_push_decrypt(__ops_parseinfo_t * pinfo, __ops_crypt_t * decrypt,
			__ops_region_t * region)
{
	encrypted_t	*encrypted;
	
	encrypted = calloc(1, sizeof(*encrypted));
	encrypted->decrypt = decrypt;
	encrypted->region = region;
	__ops_decrypt_init(encrypted->decrypt);
	__ops_reader_push(pinfo, encrypted_data_reader,
			encrypted_data_destroyer, encrypted);
}

/**
 * \ingroup Core_Readers_Encrypted
 * \brief Pops decryption reader from stack
 * \sa __ops_reader_push_decrypt()
 */
void 
__ops_reader_pop_decrypt(__ops_parseinfo_t * pinfo)
{
	encrypted_t	*encrypted;

	encrypted = __ops_reader_get_arg(__ops_parse_get_rinfo(pinfo));
	encrypted->decrypt->decrypt_finish(encrypted->decrypt);
	(void) free(encrypted);
	__ops_reader_pop(pinfo);
}

/**************************************************************************/

typedef struct {
	/* boolean: false once we've done the preamble/MDC checks */
	/* and are reading from the plaintext */
	int             passed_checks;
	unsigned char  *plaintext;
	size_t          plaintext_available;
	size_t          plaintext_offset;
	__ops_region_t   *region;
	__ops_crypt_t    *decrypt;
}               decrypt_se_ip_t;

/*
  Gets entire SE_IP data packet.
  Verifies leading preamble
  Verifies trailing MDC packet
  Then passes up plaintext as requested
*/
static int 
se_ip_data_reader(void *dest_, size_t len, __ops_error_t ** errors,
		  __ops_reader_t * readinfo,
		  __ops_callback_data_t * cbinfo)
{
	unsigned int    n = 0;
	__ops_region_t    decrypted_region;
	decrypt_se_ip_t *se_ip = __ops_reader_get_arg(readinfo);

	if (!se_ip->passed_checks) {
		unsigned char  *buf = NULL;
		__ops_hash_t      hash;
		unsigned char   hashed[OPS_SHA1_HASH_SIZE];
		size_t          b;
		size_t          sz_preamble;
		size_t          sz_mdc_hash;
		size_t          sz_mdc;
		size_t          sz_plaintext;
		unsigned char  *preamble;
		unsigned char  *plaintext;
		unsigned char  *mdc;
		unsigned char  *mdc_hash;

		__ops_hash_any(&hash, OPS_HASH_SHA1);
		hash.init(&hash);

		__ops_init_subregion(&decrypted_region, NULL);
		decrypted_region.length =
			se_ip->region->length - se_ip->region->length_read;
		buf = calloc(1, decrypted_region.length);

		/* read entire SE IP packet */
		if (!__ops_stacked_limited_read(buf, decrypted_region.length,
				&decrypted_region, errors, readinfo, cbinfo)) {
			(void) free(buf);
			return -1;
		}
		if (__ops_get_debug_level(__FILE__)) {
			unsigned int    i = 0;

			fprintf(stderr, "\n\nentire SE IP packet (len=%d):\n",
					decrypted_region.length);
			for (i = 0; i < decrypted_region.length; i++) {
				fprintf(stderr, "0x%02x ", buf[i]);
				if (!((i + 1) % 8))
					fprintf(stderr, "\n");
			}
			fprintf(stderr, "\n");
			fprintf(stderr, "\n");
		}
		/* verify leading preamble */

		if (__ops_get_debug_level(__FILE__)) {
			unsigned int    i = 0;
			fprintf(stderr, "\npreamble: ");
			for (i = 0; i < se_ip->decrypt->blocksize + 2; i++)
				fprintf(stderr, " 0x%02x", buf[i]);
			fprintf(stderr, "\n");
		}
		b = se_ip->decrypt->blocksize;
		if (buf[b - 2] != buf[b] || buf[b - 1] != buf[b + 1]) {
			fprintf(stderr,
			"Bad symmetric decrypt (%02x%02x vs %02x%02x)\n",
				buf[b - 2], buf[b - 1], buf[b], buf[b + 1]);
			OPS_ERROR(errors, OPS_E_PROTO_BAD_SYMMETRIC_DECRYPT,
			"Bad symmetric decrypt when parsing SE IP packet");
			(void) free(buf);
			return -1;
		}
		/* Verify trailing MDC hash */

		sz_preamble = se_ip->decrypt->blocksize + 2;
		sz_mdc_hash = OPS_SHA1_HASH_SIZE;
		sz_mdc = 1 + 1 + sz_mdc_hash;
		sz_plaintext = decrypted_region.length - sz_preamble - sz_mdc;

		preamble = buf;
		plaintext = buf + sz_preamble;
		mdc = plaintext + sz_plaintext;
		mdc_hash = mdc + 2;

		if (__ops_get_debug_level(__FILE__)) {
			unsigned int    i = 0;

			fprintf(stderr, "\nplaintext (len=%" PRIsize "u): ",
				sz_plaintext);
			for (i = 0; i < sz_plaintext; i++)
				fprintf(stderr, " 0x%02x", plaintext[i]);
			fprintf(stderr, "\n");

			fprintf(stderr, "\nmdc (len=%" PRIsize "u): ", sz_mdc);
			for (i = 0; i < sz_mdc; i++)
				fprintf(stderr, " 0x%02x", mdc[i]);
			fprintf(stderr, "\n");
		}
		__ops_calc_mdc_hash(preamble, sz_preamble, plaintext,
				sz_plaintext, &hashed[0]);

		if (memcmp(mdc_hash, hashed, OPS_SHA1_HASH_SIZE)) {
			OPS_ERROR(errors, OPS_E_V_BAD_HASH,
					"Bad hash in MDC packet");
			(void) free(buf);
			return 0;
		}
		/* all done with the checks */
		/* now can start reading from the plaintext */
		if (se_ip->plaintext) {
			(void) fprintf(stderr,
				"se_ip_data_reader: bad plaintext\n");
			return 0;
		}
		se_ip->plaintext = calloc(1, sz_plaintext);
		memcpy(se_ip->plaintext, plaintext, sz_plaintext);
		se_ip->plaintext_available = sz_plaintext;

		se_ip->passed_checks = 1;

		(void) free(buf);
	}
	n = len;
	if (n > se_ip->plaintext_available) {
		n = se_ip->plaintext_available;
	}

	memcpy(dest_, se_ip->plaintext + se_ip->plaintext_offset, n);
	se_ip->plaintext_available -= n;
	se_ip->plaintext_offset += n;
	len -= n;

	return n;
}

static void 
se_ip_data_destroyer(__ops_reader_t * readinfo)
{
	decrypt_se_ip_t *se_ip = __ops_reader_get_arg(readinfo);
	(void) free(se_ip->plaintext);
	(void) free(se_ip);
	/* (void) free(__ops_reader_get_arg(readinfo)); */
}

/**
   \ingroup Internal_Readers_SEIP
*/
void 
__ops_reader_push_se_ip_data(__ops_parseinfo_t *pinfo, __ops_crypt_t *decrypt,
			   __ops_region_t * region)
{
	decrypt_se_ip_t *se_ip = calloc(1, sizeof(*se_ip));

	se_ip->region = region;
	se_ip->decrypt = decrypt;
	__ops_reader_push(pinfo, se_ip_data_reader, se_ip_data_destroyer,
				se_ip);
}

/**
   \ingroup Internal_Readers_SEIP
 */
void 
__ops_reader_pop_se_ip_data(__ops_parseinfo_t * pinfo)
{
	/*
	 * decrypt_se_ip_t
	 * *se_ip=__ops_reader_get_arg(__ops_parse_get_rinfo(pinfo));
	 */
	/* (void) free(se_ip); */
	__ops_reader_pop(pinfo);
}

/**************************************************************************/

/** Arguments for reader_fd
 */
typedef struct mmap_reader_t {
	void		*mem;		/* memory mapped file */
	uint64_t	 size;		/* size of file */
	uint64_t	 offset;	/* current offset in file */
	int		 fd;		/* file descriptor */
} mmap_reader_t;


/**
 * \ingroup Core_Readers
 *
 * __ops_reader_fd() attempts to read up to "plength" bytes from the file
 * descriptor in "parse_info" into the buffer starting at "dest" using the
 * rules contained in "flags"
 *
 * \param	dest	Pointer to previously allocated buffer
 * \param	plength Number of bytes to try to read
 * \param	flags	Rules about reading to use
 * \param	readinfo	Reader info
 * \param	cbinfo	Callback info
 *
 * \return	n	Number of bytes read
 *
 * OPS_R_EARLY_EOF and OPS_R_ERROR push errors on the stack
 */
static int 
fd_reader(void *dest, size_t length, __ops_error_t **errors,
	  __ops_reader_t *readinfo, __ops_callback_data_t *cbinfo)
{
	mmap_reader_t *reader = __ops_reader_get_arg(readinfo);
	int             n = read(reader->fd, dest, length);

	__OPS_USED(cbinfo);
	if (n == 0)
		return 0;
	if (n < 0) {
		OPS_SYSTEM_ERROR_1(errors, OPS_E_R_READ_FAILED, "read",
				   "file descriptor %d", reader->fd);
		return -1;
	}
	return n;
}

static void 
reader_fd_destroyer(__ops_reader_t * readinfo)
{
	(void) free(__ops_reader_get_arg(readinfo));
}

/**
   \ingroup Core_Readers_First
   \brief Starts stack with file reader
*/

void 
__ops_reader_set_fd(__ops_parseinfo_t * pinfo, int fd)
{
	mmap_reader_t *reader = calloc(1, sizeof(*reader));

	reader->fd = fd;
	__ops_reader_set(pinfo, fd_reader, reader_fd_destroyer, reader);
}

/**************************************************************************/

typedef struct {
	const unsigned char *buffer;
	size_t          length;
	size_t          offset;
}               reader_mem_t;

static int 
mem_reader(void *dest, size_t length, __ops_error_t ** errors,
	   __ops_reader_t * readinfo, __ops_callback_data_t * cbinfo)
{
	reader_mem_t *reader = __ops_reader_get_arg(readinfo);
	unsigned        n;

	__OPS_USED(cbinfo);
	__OPS_USED(errors);

	if (reader->offset + length > reader->length)
		n = reader->length - reader->offset;
	else
		n = length;

	if (n == 0)
		return 0;

	memcpy(dest, reader->buffer + reader->offset, n);
	reader->offset += n;

	return n;
}

static void 
mem_destroyer(__ops_reader_t * readinfo)
{
	(void) free(__ops_reader_get_arg(readinfo));
}

/**
   \ingroup Core_Readers_First
   \brief Starts stack with memory reader
*/

void 
__ops_reader_set_memory(__ops_parseinfo_t * pinfo, const void *buffer,
		      size_t length)
{
	reader_mem_t *mem = calloc(1, sizeof(*mem));

	mem->buffer = buffer;
	mem->length = length;
	mem->offset = 0;
	__ops_reader_set(pinfo, mem_reader, mem_destroyer, mem);
}

/**************************************************************************/

/**
 \ingroup Core_Writers
 \brief Create and initialise cinfo and mem; Set for writing to mem
 \param cinfo Address where new cinfo pointer will be set
 \param mem Address when new mem pointer will be set
 \param bufsz Initial buffer size (will automatically be increased when necessary)
 \note It is the caller's responsiblity to free cinfo and mem.
 \sa __ops_teardown_memory_write()
*/
void 
__ops_setup_memory_write(__ops_createinfo_t ** cinfo, __ops_memory_t ** mem, size_t bufsz)
{
	/*
         * initialise needed structures for writing to memory
         */

	*cinfo = __ops_createinfo_new();
	*mem = __ops_memory_new();

	__ops_memory_init(*mem, bufsz);

	__ops_writer_set_memory(*cinfo, *mem);
}

/**
   \ingroup Core_Writers
   \brief Closes writer and frees cinfo and mem
   \param cinfo
   \param mem
   \sa __ops_setup_memory_write()
*/
void 
__ops_teardown_memory_write(__ops_createinfo_t * cinfo, __ops_memory_t * mem)
{
	__ops_writer_close(cinfo);/* new */
	__ops_createinfo_delete(cinfo);
	__ops_memory_free(mem);
}

/**
   \ingroup Core_Readers
   \brief Create parse_info and sets to read from memory
   \param pinfo Address where new parse_info will be set
   \param mem Memory to read from
   \param arg Reader-specific arg
   \param callback Callback to use with reader
   \param accumulate Set if we need to accumulate as we read. (Usually false unless doing signature verification)
   \note It is the caller's responsiblity to free parse_info
   \sa __ops_teardown_memory_read()
*/
void 
__ops_setup_memory_read(__ops_parseinfo_t ** pinfo, __ops_memory_t * mem,
		      void *vp,
		      __ops_parse_cb_return_t callback(const __ops_packet_t *, __ops_callback_data_t *),
		      bool accumulate)
{
	/*
         * initialise needed structures for reading
         */

	*pinfo = __ops_parseinfo_new();
	__ops_parse_cb_set(*pinfo, callback, vp);
	__ops_reader_set_memory(*pinfo,
			      __ops_memory_get_data(mem),
			      __ops_memory_get_length(mem));

	if (accumulate)
		(*pinfo)->readinfo.accumulate = true;
}

/**
   \ingroup Core_Readers
   \brief Frees pinfo and mem
   \param pinfo
   \param mem
   \sa __ops_setup_memory_read()
*/
void 
__ops_teardown_memory_read(__ops_parseinfo_t * pinfo, __ops_memory_t * mem)
{
	__ops_parseinfo_delete(pinfo);
	__ops_memory_free(mem);
}

/**
 \ingroup Core_Writers
 \brief Create and initialise cinfo and mem; Set for writing to file
 \param cinfo Address where new cinfo pointer will be set
 \param filename File to write to
 \param allow_overwrite Allows file to be overwritten, if set.
 \return Newly-opened file descriptor
 \note It is the caller's responsiblity to free cinfo and to close fd.
 \sa __ops_teardown_file_write()
*/
int 
__ops_setup_file_write(__ops_createinfo_t **cinfo, const char *filename,
			bool allow_overwrite)
{
	int             fd = 0;
	int             flags = 0;

	/*
         * initialise needed structures for writing to file
         */
	if (filename == NULL) {
		/* write to stdout */
		fd = STDOUT_FILENO;
	} else {
		flags = O_WRONLY | O_CREAT;
		if (allow_overwrite == true)
			flags |= O_TRUNC;
		else
			flags |= O_EXCL;
#ifdef O_BINARY
		flags |= O_BINARY;
#endif
		fd = open(filename, flags, 0600);
		if (fd < 0) {
			perror(filename);
			return fd;
		}
	}
	*cinfo = __ops_createinfo_new();
	__ops_writer_set_fd(*cinfo, fd);
	return fd;
}

/**
   \ingroup Core_Writers
   \brief Closes writer, frees info, closes fd
   \param cinfo
   \param fd
*/
void 
__ops_teardown_file_write(__ops_createinfo_t * cinfo, int fd)
{
	__ops_writer_close(cinfo);
	close(fd);
	__ops_createinfo_delete(cinfo);
}

/**
   \ingroup Core_Writers
   \brief As __ops_setup_file_write, but appends to file
*/
int 
__ops_setup_file_append(__ops_createinfo_t ** cinfo, const char *filename)
{
	int             fd;
	/*
         * initialise needed structures for writing to file
         */

#ifdef O_BINARY
	fd = open(filename, O_WRONLY | O_APPEND | O_BINARY, 0600);
#else
	fd = open(filename, O_WRONLY | O_APPEND, 0600);
#endif
	if (fd < 0) {
		perror(filename);
		return fd;
	}
	*cinfo = __ops_createinfo_new();

	__ops_writer_set_fd(*cinfo, fd);

	return fd;
}

/**
   \ingroup Core_Writers
   \brief As __ops_teardown_file_write()
*/
void 
__ops_teardown_file_append(__ops_createinfo_t * cinfo, int fd)
{
	__ops_teardown_file_write(cinfo, fd);
}

/**
   \ingroup Core_Readers
   \brief Creates parse_info, opens file, and sets to read from file
   \param pinfo Address where new parse_info will be set
   \param filename Name of file to read
   \param vp Reader-specific arg
   \param callback Callback to use when reading
   \param accumulate Set if we need to accumulate as we read. (Usually false unless doing signature verification)
   \note It is the caller's responsiblity to free parse_info and to close fd
   \sa __ops_teardown_file_read()
*/

int 
__ops_setup_file_read(__ops_parseinfo_t ** pinfo, const char *filename,
		    void *vp,
		    __ops_parse_cb_return_t callback(const __ops_packet_t *, __ops_callback_data_t *),
		    bool accumulate)
{
	int             fd = 0;
	/*
         * initialise needed structures for reading
         */

#ifdef O_BINARY
	fd = open(filename, O_RDONLY | O_BINARY);
#else
	fd = open(filename, O_RDONLY);
#endif
	if (fd < 0) {
		perror(filename);
		return fd;
	}
	*pinfo = __ops_parseinfo_new();
	__ops_parse_cb_set(*pinfo, callback, vp);
#ifdef USE_MMAP_FOR_FILES
	__ops_reader_set_mmap(*pinfo, fd);
#else
	__ops_reader_set_fd(*pinfo, fd);
#endif

	if (accumulate) {
		(*pinfo)->readinfo.accumulate = true;
	}

	if (__ops_get_debug_level(__FILE__)) {
		printf("__ops_setup_file_read: accumulate %d\n", accumulate);
	}
	return fd;
}

/**
   \ingroup Core_Readers
   \brief Frees pinfo and closes fd
   \param pinfo
   \param fd
   \sa __ops_setup_file_read()
*/
void 
__ops_teardown_file_read(__ops_parseinfo_t * pinfo, int fd)
{
	close(fd);
	__ops_parseinfo_delete(pinfo);
}

__ops_parse_cb_return_t
litdata_cb(const __ops_packet_t *pkt, __ops_callback_data_t *cbinfo)
{
	const __ops_parser_content_union_t *content = &pkt->u;

	__OPS_USED(cbinfo);

	if (__ops_get_debug_level(__FILE__)) {
		printf("litdata_cb: ");
		__ops_print_packet(pkt);
	}
	/* Read data from packet into static buffer */
	switch (pkt->tag) {
	case OPS_PTAG_CT_LITERAL_DATA_BODY:
		/* if writer enabled, use it */
		if (cbinfo->cinfo) {
			if (__ops_get_debug_level(__FILE__)) {
				printf("litdata_cb: length is %d\n",
				  content->litdata_body.length);
			}
			__ops_write(content->litdata_body.data,
				  content->litdata_body.length,
				  cbinfo->cinfo);
		}
		break;

	case OPS_PTAG_CT_LITERAL_DATA_HEADER:
		/* ignore */
		break;

	default:
		break;
	}

	return OPS_RELEASE_MEMORY;
}

__ops_parse_cb_return_t
pk_sesskey_cb(const __ops_packet_t * pkt, __ops_callback_data_t * cbinfo)
{
	const __ops_parser_content_union_t *content = &pkt->u;

	__OPS_USED(cbinfo);

	if (__ops_get_debug_level(__FILE__)) {
		__ops_print_packet(pkt);
	}
	/* Read data from packet into static buffer */
	switch (pkt->tag) {
	case OPS_PTAG_CT_PK_SESSION_KEY:
		if (__ops_get_debug_level(__FILE__)) {
			printf("OPS_PTAG_CT_PK_SESSION_KEY\n");
		}
		if (!cbinfo->cryptinfo.keyring) {
			(void) fprintf(stderr,
				"pk_sesskey_cb: bad keyring\n");
			return 0;
		}
		cbinfo->cryptinfo.keydata =
			__ops_keyring_find_key_by_id(cbinfo->cryptinfo.keyring,
				content->pk_sesskey.key_id);
		if (!cbinfo->cryptinfo.keydata)
			break;
		break;

	default:
		/* return callback_general(pkt,cbinfo); */
		break;
	}

	return OPS_RELEASE_MEMORY;
}

/**
 \ingroup Core_Callbacks

\brief Callback to get secret key, decrypting if necessary.

@verbatim
 This callback does the following:
 * finds the session key in the keyring
 * gets a passphrase if required
 * decrypts the secret key, if necessary
 * sets the seckey in the content struct
@endverbatim
*/

__ops_parse_cb_return_t
get_seckey_cb(const __ops_packet_t * pkt, __ops_callback_data_t * cbinfo)
{
	const __ops_parser_content_union_t *content = &pkt->u;
	const __ops_seckey_t *secret;
	__ops_packet_t seckey;

	__OPS_USED(cbinfo);

	if (__ops_get_debug_level(__FILE__)) {
		__ops_print_packet(pkt);
	}
	switch (pkt->tag) {
	case OPS_PARSER_CMD_GET_SECRET_KEY:
		cbinfo->cryptinfo.keydata =
			__ops_keyring_find_key_by_id(cbinfo->cryptinfo.keyring,
				content->get_seckey.pk_sesskey->key_id);
		if (!cbinfo->cryptinfo.keydata ||
		    !__ops_is_key_secret(cbinfo->cryptinfo.keydata)) {
			return 0;
		}

		/* now get the key from the data */
		secret = __ops_get_seckey(cbinfo->cryptinfo.keydata);
		while (!secret) {
			if (!cbinfo->cryptinfo.passphrase) {
				(void) memset(&seckey, 0x0, sizeof(seckey));
				seckey.u.skey_passphrase.passphrase =
					&cbinfo->cryptinfo.passphrase;
				CALLBACK(cbinfo,
				OPS_PARSER_CMD_GET_SK_PASSPHRASE, &seckey);
				if (!cbinfo->cryptinfo.passphrase) {
					fprintf(stderr,
						"can't get passphrase\n");
					return 0;
				}
			}
			/* then it must be encrypted */
			secret = __ops_decrypt_seckey(cbinfo->cryptinfo.keydata,
					cbinfo->cryptinfo.passphrase);
		}

		*content->get_seckey.seckey = secret;
		break;

	default:
		/* return callback_general(pkt,cbinfo); */
		break;
	}

	return OPS_RELEASE_MEMORY;
}

char           *
__ops_malloc_passphrase(char *pp)
{
	size_t          n;
	char           *passphrase;

	n = strlen(pp);
	if ((passphrase = calloc(1, n + 1)) != NULL) {
		(void) memcpy(passphrase, pp, n);
		passphrase[n] = 0x0;
	}
	return passphrase;
}

/**
 \ingroup HighLevel_Callbacks
 \brief Callback to use when you need to prompt user for passphrase
 \param contents
 \param cbinfo
*/
__ops_parse_cb_return_t
get_passphrase_cb(const __ops_packet_t *pkt, __ops_callback_data_t *cbinfo)
{
	const __ops_parser_content_union_t	*content = &pkt->u;

	if (__ops_get_debug_level(__FILE__)) {
		__ops_print_packet(pkt);
	}
	if (cbinfo->cryptinfo.keydata == NULL) {
		(void) fprintf(stderr, "get_passphrase_cb: NULL keydata\n");
	} else {
		__ops_print_pubkeydata(cbinfo->cryptinfo.keydata);
	}
	switch (pkt->tag) {
	case OPS_PARSER_CMD_GET_SK_PASSPHRASE:
		*(content->skey_passphrase.passphrase) =
			__ops_malloc_passphrase(getpass("netpgp passphrase: "));
		return OPS_KEEP_MEMORY;
	default:
		break;
	}
	return OPS_RELEASE_MEMORY;
}

bool 
__ops_reader_set_accumulate(__ops_parseinfo_t *pinfo, bool state)
{
	pinfo->readinfo.accumulate = state;
	return state;
}

/**************************************************************************/

static int 
hash_reader(void *dest, size_t length, __ops_error_t **errors,
	    __ops_reader_t *readinfo, __ops_callback_data_t *cbinfo)
{
	__ops_hash_t	*hash = __ops_reader_get_arg(readinfo);
	int		 r;
	
	r = __ops_stacked_read(dest, length, errors, readinfo, cbinfo);
	if (r <= 0) {
		return r;
	}
	hash->add(hash, dest, (unsigned)r);
	return r;
}

/**
   \ingroup Internal_Readers_Hash
   \brief Push hashed data reader on stack
*/
void 
__ops_reader_push_hash(__ops_parseinfo_t *pinfo, __ops_hash_t *hash)
{
	hash->init(hash);
	__ops_reader_push(pinfo, hash_reader, NULL, hash);
}

/**
   \ingroup Internal_Readers_Hash
   \brief Pop hashed data reader from stack
*/
void 
__ops_reader_pop_hash(__ops_parseinfo_t * pinfo)
{
	__ops_reader_pop(pinfo);
}

/* read memory from the previously mmap-ed file */
static int 
mmap_reader(void *dest, size_t length, __ops_error_t **errors,
	  __ops_reader_t *readinfo, __ops_callback_data_t *cbinfo)
{
	mmap_reader_t	*mem = __ops_reader_get_arg(readinfo);
	unsigned	 n;
	char		*cmem = mem->mem;

	__OPS_USED(errors);
	__OPS_USED(cbinfo);
	n = MIN(length, (unsigned)(mem->size - mem->offset));
	if (n > 0) {
		(void) memcpy(dest, &cmem[(int)mem->offset], (unsigned)n);
		mem->offset += n;
	}
	return n;
}

/* tear down the mmap, close the fd */
static void 
mmap_destroyer(__ops_reader_t * readinfo)
{
	mmap_reader_t *mem = __ops_reader_get_arg(readinfo);

	(void) munmap(mem->mem, (unsigned)mem->size);
	(void) close(mem->fd);
	(void) free(__ops_reader_get_arg(readinfo));
}

/* set up the file to use mmap-ed memory if available, file IO otherwise */
void 
__ops_reader_set_mmap(__ops_parseinfo_t * pinfo, int fd)
{
	mmap_reader_t	*mem = calloc(1, sizeof(*mem));
	struct stat	 st;

	if (fstat(fd, &st) == 0) {
		mem->size = st.st_size;
		mem->offset = 0;
		mem->fd = fd;
		mem->mem = mmap(NULL, (size_t)st.st_size, PROT_READ,
				MAP_FILE | MAP_PRIVATE, fd, 0);
		if (mem->mem == MAP_FAILED) {
			__ops_reader_set(pinfo, fd_reader, reader_fd_destroyer,
					mem);
		} else {
			__ops_reader_set(pinfo, mmap_reader, mmap_destroyer,
					mem);
		}
	}
}
