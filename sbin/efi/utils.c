/* $NetBSD: utils.c,v 1.3 2025/03/02 00:03:41 riastradh Exp $ */

/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: utils.c,v 1.3 2025/03/02 00:03:41 riastradh Exp $");
#endif /* not lint */

#include <sys/uuid.h>

#include <assert.h>
#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <util.h>

#include "defs.h"
#include "utils.h"

#define UCS2_REPLACEMENT_CHARACTER	0xfffd

/************************************************************************
 * Character encoding conversion routnes
 *
 * UEFI uses UCS-2 character encoding.  Note that this is not UTF-16
 * as it doesn't interpret surrogate pairs.
 *
 * From <https://www.unicode.org/faq/utf_bom.html#utf16-11>:
 *
 * "UCS-2 is obsolete terminology which refers to a Unicode
 * implementation up to Unicode 1.1, before surrogate code points and
 * UTF-16 were added to Version 2.0 of the standard. This term should
 * now be avoided.
 *
 * UCS-2 does not describe a data format distinct from UTF-16, because
 * both use exactly the same 16-bit code unit
 * representations. However, UCS-2 does not interpret surrogate code
 * points, and thus cannot be used to conformantly represent
 * supplementary characters.
 *
 * Sometimes in the past an implementation has been labeled "UCS-2" to
 * indicate that it does not support supplementary characters and
 * doesn't interpret pairs of surrogate code points as
 * characters. Such an implementation would not handle processing of
 * character properties, code point boundaries, collation, etc. for
 * supplementary characters, nor would it be able to support most
 * emoji, for example. [AF]"
 *
 * Regarding illegal UTF-8 sequences, the same document says:
 *
 * "None of the UTFs can generate every arbitrary byte sequence. For
 * example, in UTF-8 every byte of the form 110xxxxx_2 must be
 * followed with a byte of the form 10xxxxxx_2. A sequence such as
 * <110xxxxx_2 0xxxxxxx_2> is illegal, and must never be
 * generated. When faced with this illegal byte sequence while
 * transforming or interpreting, a UTF-8 conformant process must treat
 * the first byte 110xxxxx_2 as an illegal termination error: for
 * example, either signaling an error, filtering the byte out, or
 * representing the byte with a marker such as U+FFFD REPLACEMENT
 * CHARACTER. In the latter two cases, it will continue processing at
 * the second byte 0xxxxxxx_2.
 *
 * A conformant process must not interpret illegal or ill-formed byte
 * sequences as characters, however, it may take error recovery
 * actions. No conformant process may use irregular byte sequences to
 * encode out-of-band information."
 */

/*
 * ibuf  = input buffer (uint16_t *)
 * isz   = bytes in input buffer
 * obuf  = output buffer (char *)
 * osz   = bytes in output buffer (size_t *)
 *
 * if (obuf == NULL), malloc obuf.
 * if (obuf != NULL), write to existing output buffer.
 *
 * Return resulting utf8 string.
 */
PUBLIC char *
ucs2_to_utf8(const uint16_t *ibuf, size_t isz, char *obuf, size_t *osz)
{
	uint8_t *dst;
	size_t dsz, i, j, j_max, n;
	uint16_t c;

	assert(isz > 0);

	if (obuf != NULL) {
		assert(osz != NULL);
		dsz = *osz;
		dst = (uint8_t *)obuf;
	}
	else {
		dsz = isz * sizeof(*dst);
		dst = malloc(dsz);
	}

	/*
	 * Each UCS2 character will encode as at most 3 UTF8 bytes.
	 * 'isz' is the number of bytes in the source buffer which
	 * may well be larger than the UCS2 string.  'osz' is the
	 * actual number of bytes in the NUL terminated USC2 string.
	 */
	n = isz / sizeof(*ibuf); /* max number of characters in input buffer */
	j = 0;
	j_max = dsz / sizeof(*dst);
	for (i = 0; i < n; i++) {
		c = le16toh(ibuf[i]);
		if (c == 0) {
			break;
		}
		if (c < 0x0080) {
			if (j + 1 >= j_max)
				break;
			dst[j++] = (uint8_t)c;
		}
		else if (c < 0x0800) {
			if (j + 2 >= j_max)
				break;
			dst[j++] = 0xc0 | (uint8_t)(c >> 6);
			dst[j++] = 0x80 | (uint8_t)(c & 0x3f);
		}
		else {
			if (j + 3 >= j_max)
				break;
			dst[j++] = 0xe0 | (uint8_t)(c >> 12);
			dst[j++] = 0x80 | (uint8_t)((c >> 6) & 0x3f);
			dst[j++] = 0x80 | (uint8_t)(c & 0x3f);
		}
	}
	if (dst != NULL)
		dst[j] = '\0';

	if (osz)
		*osz = j;

	return (char *)dst;
}

/*
 * ibuf  = input buffer (char *)
 * isz   = bytes in input buffer
 * obuf  = output buffer (uint16_t *)
 * osz   = bytes in output buffer (size_t *)
 *
 * if (obuf == NULL), malloc obuf.
 * if (obuf != NULL), write to existing output buffer.
 *
 * Return resulting ucs2 string.
 */
PUBLIC uint16_t *
utf8_to_ucs2(const char *ibuf, size_t isz, uint16_t *obuf, size_t *osz)
{
	const uint8_t *src = (const uint8_t *)ibuf;
	uint16_t *dst;
	uint16_t out;
	size_t dsz, i, j, j_max;

	if (obuf != NULL) {
		assert(osz != NULL);
		dst = obuf;
		dsz = *osz;
	}
	else {
		dsz = isz * sizeof(*dst);
		dst = malloc(dsz);
	}

	j = 0;
	j_max = dsz / sizeof(*dst);
	for (i = 0; i < isz; i++) {
		out = src[i];
		if (out == '\0') {
			break;
		}
		else if (j + 1 >= j_max) {
			break;
		}
		else if ((out & 0x80) == 0) {
			/* we're good to go */
		}
		else if ((out & 0xe0) == 0xc0) {
			if (i + 1 >= isz) { /* insufficient source */
				break;
			}
			if ((src[i + 1] & 0xc0) != 0x80) {
				out = UCS2_REPLACEMENT_CHARACTER;
			}
			else {
				out &= 0x1f;
				out <<= 6;
				out |= src[++i] & 0x3f;
			}
		}
		else if ((out & 0xf0) == 0xe0) {
			if (i + 2 >= isz) { /* insufficient source */
				break;
			}
			if ((src[i + 1] & 0xc0) != 0x80 ||
			    (src[i + 2] & 0xc0) != 0x80) {
				out = UCS2_REPLACEMENT_CHARACTER;
			}
			else {
				out &= 0x0f;
				out <<= 6;
				out |= src[++i] & 0x3f;
				out <<= 6;
				out |= src[++i] & 0x3f;
			}
		}
		else {	/* cannot encode as USC2 */
			out = UCS2_REPLACEMENT_CHARACTER;
		}
		dst[j++] = htole16(out);
	}
	dst[j] = '\0';

	if (src[i] != '\0')
		warnx("bad UTF8 string");

	if (osz)
		*osz = (j + 1) * sizeof(*dst);
	return dst;
}

PUBLIC size_t
utf8_to_ucs2_size(const char *src)
{
#if 0
	uint16_t *dst;
	size_t sz;

	dst = utf8_to_ucs2(src, strlen(src) + 1, NULL, &sz);
	free(dst);
	return sz;
#else
	const uint8_t *buf = (const uint8_t *)src;
	uint out;
	size_t i, j;

	j = 0;
	for (i = 0; (out = buf[i]) != '\0'; i++) {
		if ((out & 0x80) == 0) {
			/* we're good to go */
		}
		else if ((out & 0xe0) == 0xc0) {
			if ((buf[i + 1] & 0xc0) == 0x80) {
				i++;
			}
		}
		else if ((out & 0xf0) == 0xe0) {
			if ((buf[i + 1] & 0xc0) == 0x80 &&
			    (buf[i + 2] & 0xc0) == 0x80) {
				i += 2;
			}
		}
		j++;
	}

	return (j + 1) * sizeof(uint16_t);
#endif
}

/************************************************************************
 * UUID routines
 */
PUBLIC int
uuid_scanf(struct uuid *uuid, const char *str)
{

	return sscanf(str,
	    "%08x-%04hx-%04hx-%02hhx%02hhx-%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx",
	    &uuid->time_low, &uuid->time_mid, &uuid->time_hi_and_version,
	    &uuid->clock_seq_hi_and_reserved, &uuid->clock_seq_low,
	    &uuid->node[0], &uuid->node[1], &uuid->node[2], &uuid->node[3],
	    &uuid->node[4], &uuid->node[5]);
}

/*
 * from sys/kern/kern_uuid.c
 */
PUBLIC int
uuid_snprintf(char *buf, size_t sz, const struct uuid *uuid)
{

	return snprintf(buf, sz,
	    "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
	    uuid->time_low, uuid->time_mid, uuid->time_hi_and_version,
	    uuid->clock_seq_hi_and_reserved, uuid->clock_seq_low,
	    uuid->node[0], uuid->node[1], uuid->node[2], uuid->node[3],
	    uuid->node[4], uuid->node[5]);
}

PUBLIC int
uuid_printf(const struct uuid *uuid)
{
	char buf[UUID_STR_LEN];

	(void) uuid_snprintf(buf, sizeof(buf), uuid);
	printf("%s", buf);
	return (0);
}

/************************************************************************
 * Misc routines
 */

PUBLIC void
show_data(const uint8_t *buf, size_t len, const char *prefix)
{
	uint8_t line_buf[17];
	size_t i;

	line_buf[16] = '\0';
	for (i = 0; i < len; i++) {
		if ((i & 0xf) == 0)
			printf("%s%08zx: ", prefix, i);
		line_buf[i & 0xf] = isprint((int)buf[i]) ? buf[i] : '.';
		printf("%02x ", buf[i]);
		if ((i & 0xf) == 0xf)
			printf("  %s\n", line_buf);
		else if ((i & 0x7) == 0x7)
			printf(" ");
	}
	i &= 0xf;
	if (i != 0) {
		line_buf[i] = '\0';
		if (i < 8)
			printf(" ");
		while (i++ < 16)
			printf("   ");

		printf("  %s\n", line_buf);
	}
}

PUBLIC uint16_t
strtous(const char *str, char **endptr, int base)
{
	uintmax_t val;
	int rstatus;

	val = strtou(str, endptr, base, 0, USHRT_MAX, &rstatus);

	switch (rstatus) {
	case EINVAL:
		assert(0);
		break;
	case ENOTSUP:
		if (endptr != NULL)
			break;
		/*FALLTHROUGH*/
	case ECANCELED:
		err(EXIT_FAILURE, "invalid numeric string: %s\n", str);
	case ERANGE:
		err(EXIT_FAILURE, "value out of range [0,%#x]: %s\n",
		    USHRT_MAX, str);
	default:
		break;
	}

	return (uint16_t)val;
}

char *
read_file(const char *fname, size_t *size)
{
	char *buf, *cp, *ep;
	size_t bufsz, cnt, sz;
	ssize_t ssz;
	int fd, fd_flags;

	assert(fname != NULL);
	if (fname == NULL)
		return 0;

	if (strcmp(fname, "-") == 0) {
		fd = STDIN_FILENO;
		if ((fd_flags = fcntl(fd, F_GETFL)) == -1)
			err(EXIT_FAILURE, "fcntl F_GETFL");

		if (fcntl(fd, F_SETFL, O_NONBLOCK | fd_flags) == -1)
			err(EXIT_FAILURE, "fcntl F_SETFL");
	}
	else {
		fd_flags = -1;
		fd = open(fname, O_RDONLY);
		if (fd == -1)
			err(EXIT_FAILURE, "open");
	}

	bufsz = 0x800;
	buf = emalloc(bufsz);
	cp = buf;
	ep = buf + bufsz;
	cnt = 0;
	for (;;) {
		ssz = read(fd, cp, (size_t)(ep - cp));
		if (ssz == -1) {
			if (errno == EAGAIN)
				continue;
			err(EXIT_FAILURE, "read");
		}
		assert(ssz >= 0);
#if 0
		printf("ssz: %zd\n", ssz);
		show_data((uint8_t *)cp, (size_t)ssz, "");
#endif
		if (ssz == 0)
			break;

		cp += ssz;
		sz = (size_t)ssz;
		cnt += sz;

		if (cp < ep) {
			/* XXX: what about UCS-2? */
			*cp = '\0';
			cnt++;
			break;
		}

		if (cp == ep) {
			bufsz *= 2;
			buf = erealloc(buf, bufsz);
			cp = buf + cnt;
			ep = buf + bufsz;
		}
	}
	if (fd_flags != -1)
		fcntl(fd, F_SETFL, fd_flags);
	else
		close(fd);

	*size = cnt;
	return buf;
}
