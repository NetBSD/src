/*	$NetBSD: util.c,v 1.3 2023/02/02 08:21:32 mlelstv Exp $	*/

/*-
 * Copyright (c) 2017 Netflix, Inc
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: util.c,v 1.3 2023/02/02 08:21:32 mlelstv Exp $");
#if 0
__FBSDID("$FreeBSD: head/sbin/nvmecontrol/util.c 320423 2017-06-27 20:24:25Z imp $");
#endif
#endif

#include <sys/endian.h>

#include <stdlib.h>
#include <string.h>

#include "nvmectl.h"
#include "bn.h"

void
nvme_strvis(u_char *dst, int dlen, const u_char *src, int slen)
{
#define STRVIS_ISWHITE(x) ((x) == ' ' || (x) == '\0' || (x) == (u_char)'\377')
	/* Trim leading and trailing blanks and NULs. */
	while (slen > 0 && STRVIS_ISWHITE(src[0]))
		++src, --slen;
	while (slen > 0 && STRVIS_ISWHITE(src[slen - 1]))
		--slen;

	while (slen > 0) {
		if (*src < 0x20 || *src >= 0x80) {
			/* non-printable characters */
			dlen -= 4;
			if (dlen < 1)
				break;
			*dst++ = '\\';
			*dst++ = ((*src & 0300) >> 6) + '0';
			*dst++ = ((*src & 0070) >> 3) + '0';
			*dst++ = ((*src & 0007) >> 0) + '0';
		} else if (*src == '\\') {
			/* quote characters */
			dlen -= 2;
			if (dlen < 1)
				break;
			*dst++ = '\\';
			*dst++ = '\\';
		} else {
			/* normal characters */
			if (--dlen < 1)
				break;
			*dst++ = *src;
		}
		++src, --slen;
	}

	*dst++ = 0;
}

#define	METRIX_PREFIX_BUFSIZ	17
#define	NO_METRIX_PREFIX_BUFSIZ	42

static void
unit_string(BIGNUM *bn, const char *unit, long scale, char *out, size_t len)
{
	size_t ulen = strlen(unit);
	uint8_t tmp[4];
	BN_CTX *ctx;
	BIGNUM *sn;

	if (6 + ulen + 3 >= len)
		return;

	if (scale > 1) {
		ctx = BN_CTX_new();
		if (ctx == NULL)
			return;

		tmp[0] = (scale >> 24) & 0xff;
		tmp[1] = (scale >> 16) & 0xff;
		tmp[2] = (scale >>  8) & 0xff;
		tmp[3] = scale & 0xff;

		sn = BN_bin2bn(tmp, sizeof(tmp), NULL);
		if (sn != NULL) {
			BN_mul(bn, bn, sn, ctx);
			BN_free(sn);
		}	

		BN_CTX_free(ctx);
	}

	strncpy(out, " (", len);
	humanize_bignum(out+2, 6 + ulen, bn, unit, HN_AUTOSCALE, HN_DECIMAL);
	strncat(out, ")", len);
}

void
print_bignum1(const char *title, uint64_t v[2], const char *suffix,
    const char *unit, long scale)
{
	char buf[64];
	char buf2[64];
	uint8_t tmp[16];
	uint64_t h, l;

#if _BYTE_ORDER != _LITTLE_ENDIAN
	/* Already Converted to host endian */
	h = v[0];
	l = v[1];
	memcpy(tmp, v, sizeof(tmp));
#else
	h = v[1];
	l = v[0];

	tmp[ 0] = (h >> 56) & 0xff;
	tmp[ 1] = (h >> 48) & 0xff;
	tmp[ 2] = (h >> 40) & 0xff;
	tmp[ 3] = (h >> 32) & 0xff;
	tmp[ 4] = (h >> 24) & 0xff;
	tmp[ 5] = (h >> 16) & 0xff;
	tmp[ 6] = (h >> 8) & 0xff;
	tmp[ 7] = h & 0xff;
	tmp[ 8] = (l >> 56) & 0xff;
	tmp[ 9] = (l >> 48) & 0xff;
	tmp[10] = (l >> 40) & 0xff;
	tmp[11] = (l >> 32) & 0xff;
	tmp[12] = (l >> 24) & 0xff;
	tmp[13] = (l >> 16) & 0xff;
	tmp[14] = (l >> 8) & 0xff;
	tmp[15] = l & 0xff;
#endif

	buf[0] = '\0';
	buf2[0] = '\0';

	BIGNUM *bn = BN_bin2bn(tmp, sizeof(tmp), NULL);
	if (bn != NULL) {
		humanize_bignum(buf, METRIX_PREFIX_BUFSIZ + strlen(suffix),
		    bn, suffix, HN_AUTOSCALE, HN_DECIMAL | HN_NOSPACE);
		if (unit)
			unit_string(bn, unit, scale, buf2, sizeof(buf2));
		BN_free(bn);
	}
	if (buf[0] == '\0')
		snprintf(buf, sizeof(buf), "0x%016" PRIx64 "%016" PRIx64, h, l);
	printf("%-31s %s%s\n", title, buf, buf2);
}

void
print_bignum(const char *title, uint64_t v[2], const char *suffix)
{
	print_bignum1(title, v, suffix, NULL, 1);
}

/* "Missing" from endian.h */
uint64_t
le48dec(const void *pp)
{
	uint8_t const *p = (uint8_t const *)pp;

	return (((uint64_t)le16dec(p + 4) << 32) | le32dec(p));
}
