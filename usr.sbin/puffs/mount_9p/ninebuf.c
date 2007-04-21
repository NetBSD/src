/*      $NetBSD: ninebuf.c,v 1.1 2007/04/21 14:21:43 pooka Exp $	*/

/*
 * Copyright (c) 2006, 2007  Antti Kantee.  All Rights Reserved.
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
__RCSID("$NetBSD: ninebuf.c,v 1.1 2007/04/21 14:21:43 pooka Exp $");
#endif /* !lint */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/vnode.h>

#include <assert.h>
#include <err.h>
#include <errno.h>
#include <stdlib.h>
#include <util.h>
#include <unistd.h>

#include "ninepuffs.h"

/*
 * Originally from my psshfs implementation.  Maybe need to look into
 * unifying these at some level, although there are minor variations.
 *
 * Such as the fact that 9P is a little endian protocol ....
 */

int
p9pbuf_read(struct puffs9p *p9p, struct p9pbuf *pb)
{
	ssize_t n;

	assert(pb->state != P9PBUF_PUT);

 again:
	n = read(p9p->servsock, pb->buf+pb->offset, pb->remain);
	switch (n) {
	case 0:
		errno = EIO;
		return -1;
	case -1:
		if (errno == EAGAIN)
			return 0;
		return -1;
	default:
		pb->offset += n;
		pb->remain -= n;
	}

	if (pb->remain != 0)
		return 0;

	/* ok, at least there's something to do */
	assert(pb->state == P9PBUF_GETLEN || pb->state == P9PBUF_GETDATA);

	if (pb->state == P9PBUF_GETLEN) {
		uint32_t len;

		memcpy(&len, pb->buf, 4);
		pb->remain = le32toh(len) - 4;
		assert(pb->remain <= pb->len); /* XXX */
		pb->offset = 0;

		pb->state = P9PBUF_GETDATA;
		goto again;

	} else if (pb->state == P9PBUF_GETDATA) {
		pb->remain = pb->offset;
		pb->offset = 0;

		pb->state = P9PBUF_GETREADY;

		/* sloppy */
		if (!p9pbuf_get_1(pb, &pb->type))
			errx(1, "invalid server response, no type");
		if (!p9pbuf_get_2(pb, &pb->tagid))
			errx(1, "invalid server response, no tag");

		return 1;
	}

	return -1; /* XXX: impossible */
}

int
p9pbuf_write(struct puffs9p *p9p, struct p9pbuf *pb)
{
	ssize_t n;

	if (pb->state == P9PBUF_PUT) {
		uint32_t len;

		len = htole32(pb->offset);
		memcpy(pb->buf, &len, sizeof(len));

		pb->remain = pb->offset;
		pb->offset = 0;

		pb->state = P9PBUF_PUTDONE;
	}

	assert(pb->state == P9PBUF_PUTDONE);

	n = write(p9p->servsock, pb->buf + pb->offset, pb->remain);
	if (n == 0) {
		errno = EIO;
		return -1;
	}

	if (n == -1) {
		if (errno == EAGAIN)
			return 0;
		else
			return -1;
	}

	pb->offset += n;
	pb->remain -= n;

	if (pb->remain == 0)
		return 1;
	else
		return 0;
}

struct p9pbuf *
p9pbuf_make(size_t reqsize, int incoming)
{
	struct p9pbuf *pb;

	pb = emalloc(sizeof(struct p9pbuf));
	memset(pb, 0, sizeof(struct p9pbuf));
	pb->buf = emalloc(reqsize);
	pb->len = reqsize;

	p9pbuf_recycle(pb, incoming);

	return pb;
}

void
p9pbuf_destroy(struct p9pbuf *pb)
{

	free(pb->buf);
	free(pb);
}

void
p9pbuf_recycle(struct p9pbuf *pb, int incoming)
{

	if (incoming) {
		pb->offset = 0;
		pb->remain = 4;
		pb->state = P9PBUF_GETLEN;
	} else {
		/* save space for len */
		pb->remain = pb->len - 4;
		pb->offset = 4;

		pb->state = P9PBUF_PUT;
	}
}

/*
 * allow put 1,2,4,8 in the middle and do *not* adjust remain
 * in that case.  However, do check the extending is possible
 * only from the end
 */

int
p9pbuf_put_1(struct p9pbuf *pb, uint8_t val)
{

	assert(pb->state == P9PBUF_PUT);

	P9PB_CHECK(pb, 1);

	memcpy(pb->buf + pb->offset, &val, 1);
	if (pb->offset + pb->remain == pb->len)
		pb->remain -= 1;
	pb->offset += 1;

	return 0;
}

int
p9pbuf_put_2(struct p9pbuf *pb, uint16_t val)
{

	assert(pb->state == P9PBUF_PUT);

	P9PB_CHECK(pb, 2);

	HTOLE16(val);
	memcpy(pb->buf + pb->offset, &val, 2);
	if (pb->offset + pb->remain == pb->len)
		pb->remain -= 2;
	else
		assert(pb->offset + pb->remain + 2 <= pb->len);
	pb->offset += 2;

	return 0;
}

int
p9pbuf_put_4(struct p9pbuf *pb, uint32_t val)
{

	assert(pb->state == P9PBUF_PUT);

	P9PB_CHECK(pb, 4);

	HTOLE32(val);
	memcpy(pb->buf + pb->offset, &val, 4);
	if (pb->offset + pb->remain == pb->len)
		pb->remain -= 4;
	else
		assert(pb->offset + pb->remain + 4 <= pb->len);
	pb->offset += 4;

	return 0;
}

int
p9pbuf_put_8(struct p9pbuf *pb, uint64_t val)
{

	assert(pb->state == P9PBUF_PUT);

	P9PB_CHECK(pb, 8);

	HTOLE64(val);
	memcpy(pb->buf + pb->offset, &val, 8);
	if (pb->offset + pb->remain == pb->len)
		pb->remain -= 8;
	else
		assert(pb->offset + pb->remain + 8 <= pb->len);
	pb->offset += 8;

	return 0;
}

int
p9pbuf_put_data(struct p9pbuf *pb, const void *data, uint16_t dlen)
{

	assert(pb->state == P9PBUF_PUT);

	P9PB_CHECK(pb, dlen + 2);

	p9pbuf_put_2(pb, dlen);
	memcpy(pb->buf + pb->offset, data, dlen);
	pb->offset += dlen;
	pb->remain -= dlen;

	return 0;
}

int
p9pbuf_put_str(struct p9pbuf *pb, const char *str)
{

	return p9pbuf_put_data(pb, str, strlen(str));
}

int
p9pbuf_write_data(struct p9pbuf *pb, uint8_t *data, uint32_t dlen)
{

	assert(pb->state == P9PBUF_PUT);

	P9PB_CHECK(pb, dlen);
	memcpy(pb->buf + pb->offset, data, dlen);
	pb->offset += dlen;
	pb->remain -= dlen;

	return 0;
}

int
p9pbuf_get_1(struct p9pbuf *pb, uint8_t *val)
{

	assert(pb->state == P9PBUF_GETREADY);

	if (pb->remain < 1)
		return 0;

	memcpy(val, pb->buf + pb->offset, 1);
	pb->offset += 1;
	pb->remain -= 1;

	return 1;
}

int
p9pbuf_get_2(struct p9pbuf *pb, uint16_t *val)
{
	uint16_t v;

	assert(pb->state == P9PBUF_GETREADY);

	if (pb->remain < 2)
		return 0;

	memcpy(&v, pb->buf + pb->offset, 2);
	pb->offset += 2;
	pb->remain -= 2;

	*val = le16toh(v);

	return 1;
}

int
p9pbuf_get_4(struct p9pbuf *pb, uint32_t *val)
{
	uint32_t v;

	assert(pb->state == P9PBUF_GETREADY);

	if (pb->remain < 4)
		return 0;

	memcpy(&v, pb->buf + pb->offset, 4);
	pb->offset += 4;
	pb->remain -= 4;

	*val = le32toh(v);

	return 1;
}

int
p9pbuf_get_8(struct p9pbuf *pb, uint64_t *val)
{
	uint64_t v;

	assert(pb->state == P9PBUF_GETREADY);

	if (pb->remain < 8)
		return 0;

	memcpy(&v, pb->buf + pb->offset, 8);
	pb->offset += 8;
	pb->remain -= 8;

	*val = le64toh(v);

	return 1;
}

int
p9pbuf_get_data(struct p9pbuf *pb, uint8_t **dp, uint16_t *dlenp)
{
	uint8_t *d;
	uint16_t len;

	assert(pb->state == P9PBUF_GETREADY);

	if (!(p9pbuf_get_2(pb, &len)))
		return 0;

	if (pb->remain < len)
		return 0;

	if (dp) {
		d = emalloc(len+1);
		memcpy(d, pb->buf + pb->offset, len);
		d[len] = '\0';
		*dp = d;
	}

	pb->offset += len;
	pb->remain -= len;

	if (dlenp)
		*dlenp = len;

	return 1;
}

int
p9pbuf_read_data(struct p9pbuf *pb, uint8_t *buf, uint32_t dlen)
{

	assert(pb->state == P9PBUF_GETREADY);

	if (pb->remain < dlen)
		return 0;

	memcpy(buf, pb->buf + pb->offset, dlen);
	pb->offset += dlen;
	pb->remain -= dlen;

	return 1;
}

int
p9pbuf_get_str(struct p9pbuf *pb, char **dp, uint16_t *dlenp)
{

	return p9pbuf_get_data(pb, (uint8_t **)dp, dlenp);
}

int
p9pbuf_tell(struct p9pbuf *pb)
{

	return pb->offset;
}

int
p9pbuf_remaining(struct p9pbuf *pb)
{
	
	return pb->remain;
}

void
p9pbuf_seekset(struct p9pbuf *pb, int newoff)
{

	if (newoff > pb->offset)
		pb->remain -= newoff - pb->offset;
	pb->offset = newoff;
}
