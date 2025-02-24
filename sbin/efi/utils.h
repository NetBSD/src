/* $NetBSD: utils.h,v 1.1 2025/02/24 13:47:57 christos Exp $ */

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

#ifndef _UTILS_H_
#define _UTILS_H_

#ifndef lint
__RCSID("$NetBSD: utils.h,v 1.1 2025/02/24 13:47:57 christos Exp $");
#endif /* not lint */

#if 0
/* See sys/kern/kern_uuid.c */
int	uuid_printf(const struct uuid *);
void	uuid_dec_be(const void *, struct uuid *);
void	uuid_dec_le(const void *, struct uuid *);
void	uuid_enc_be(void *, const struct uuid *);
void	uuid_enc_le(void *, const struct uuid *);
int	uuidgen(struct uuid *, int);
#endif

#include <string.h>
#include <util.h>

struct uuid;

static inline void *
memdup(const void *src, size_t sz)
{
	void *dst;

	dst = emalloc(sz);
	memcpy(dst, src, sz);
	return dst;
}

static inline char *
encode_data(const uint8_t *data, size_t len)
{
	char *bp;

	bp = emalloc(len * 2 + 1);
	for (size_t i = 0; i < len; i++)
		snprintf(&bp[i * 2], 3, "%02x", data[i]);
	bp[len * 2] = '\0';

	return bp;
}

static inline size_t
ucs2nlen(const uint16_t *src, size_t maxlen)
{
	size_t i;

	for (i = 0; i < maxlen; i++) {
		if (src[i] == '\0')
			break;
	}
	return i;
}

char *ucs2_to_utf8(const uint16_t *, size_t, char *, size_t *);
uint16_t *utf8_to_ucs2(const char *, size_t, uint16_t *, size_t *);
size_t utf8_to_ucs2_size(const char *);

int uuid_scanf(struct uuid *, const char *);
int uuid_snprintf(char *, size_t, const struct uuid *);
int uuid_printf(const struct uuid *);

void show_data(const uint8_t *, size_t, const char *);
uint16_t strtous(const char *, char **, int);

char *read_file(const char *, size_t *);

#endif /* _UTILS_H_ */
