/*	$NetBSD: buffer.c,v 1.7.2.2 2024/02/25 15:47:15 martin Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

/*! \file */

#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>

#include <isc/buffer.h>
#include <isc/mem.h>
#include <isc/print.h>
#include <isc/region.h>
#include <isc/string.h>
#include <isc/util.h>

void
isc_buffer_reinit(isc_buffer_t *b, void *base, unsigned int length) {
	/*
	 * Re-initialize the buffer enough to reconfigure the base of the
	 * buffer.  We will swap in the new buffer, after copying any
	 * data we contain into the new buffer and adjusting all of our
	 * internal pointers.
	 *
	 * The buffer must not be smaller than the length of the original
	 * buffer.
	 */
	REQUIRE(b->length <= length);
	REQUIRE(base != NULL);
	REQUIRE(!b->autore);

	if (b->length > 0U) {
		(void)memmove(base, b->base, b->length);
	}

	b->base = base;
	b->length = length;
}

void
isc_buffer_setautorealloc(isc_buffer_t *b, bool enable) {
	REQUIRE(ISC_BUFFER_VALID(b));
	REQUIRE(b->mctx != NULL);
	b->autore = enable;
}

void
isc_buffer_compact(isc_buffer_t *b) {
	unsigned int length;
	void *src;

	/*
	 * Compact the used region by moving the remaining region so it occurs
	 * at the start of the buffer.  The used region is shrunk by the size
	 * of the consumed region, and the consumed region is then made empty.
	 */

	REQUIRE(ISC_BUFFER_VALID(b));

	src = isc_buffer_current(b);
	length = isc_buffer_remaininglength(b);
	if (length > 0U) {
		(void)memmove(b->base, src, (size_t)length);
	}

	if (b->active > b->current) {
		b->active -= b->current;
	} else {
		b->active = 0;
	}
	b->current = 0;
	b->used = length;
}

uint8_t
isc_buffer_getuint8(isc_buffer_t *b) {
	unsigned char *cp;
	uint8_t result;

	/*
	 * Read an unsigned 8-bit integer from 'b' and return it.
	 */

	REQUIRE(ISC_BUFFER_VALID(b));
	REQUIRE(b->used - b->current >= 1);

	cp = isc_buffer_current(b);
	b->current += 1;
	result = ((uint8_t)(cp[0]));

	return (result);
}

uint16_t
isc_buffer_getuint16(isc_buffer_t *b) {
	unsigned char *cp;
	uint16_t result;

	/*
	 * Read an unsigned 16-bit integer in network byte order from 'b',
	 * convert it to host byte order, and return it.
	 */

	REQUIRE(ISC_BUFFER_VALID(b));
	REQUIRE(b->used - b->current >= 2);

	cp = isc_buffer_current(b);
	b->current += 2;
	result = ((unsigned int)(cp[0])) << 8;
	result |= ((unsigned int)(cp[1]));

	return (result);
}

uint32_t
isc_buffer_getuint32(isc_buffer_t *b) {
	unsigned char *cp;
	uint32_t result;

	/*
	 * Read an unsigned 32-bit integer in network byte order from 'b',
	 * convert it to host byte order, and return it.
	 */

	REQUIRE(ISC_BUFFER_VALID(b));
	REQUIRE(b->used - b->current >= 4);

	cp = isc_buffer_current(b);
	b->current += 4;
	result = ((unsigned int)(cp[0])) << 24;
	result |= ((unsigned int)(cp[1])) << 16;
	result |= ((unsigned int)(cp[2])) << 8;
	result |= ((unsigned int)(cp[3]));

	return (result);
}

uint64_t
isc_buffer_getuint48(isc_buffer_t *b) {
	unsigned char *cp;
	uint64_t result;

	/*
	 * Read an unsigned 48-bit integer in network byte order from 'b',
	 * convert it to host byte order, and return it.
	 */

	REQUIRE(ISC_BUFFER_VALID(b));
	REQUIRE(b->used - b->current >= 6);

	cp = isc_buffer_current(b);
	b->current += 6;
	result = ((int64_t)(cp[0])) << 40;
	result |= ((int64_t)(cp[1])) << 32;
	result |= ((int64_t)(cp[2])) << 24;
	result |= ((int64_t)(cp[3])) << 16;
	result |= ((int64_t)(cp[4])) << 8;
	result |= ((int64_t)(cp[5]));

	return (result);
}

void
isc_buffer_putdecint(isc_buffer_t *b, int64_t v) {
	unsigned int l = 0;
	unsigned char *cp;
	char buf[21];
	isc_result_t result;

	REQUIRE(ISC_BUFFER_VALID(b));

	/* xxxwpk do it more low-level way ? */
	l = snprintf(buf, 21, "%" PRId64, v);
	RUNTIME_CHECK(l <= 21);
	if (b->autore) {
		result = isc_buffer_reserve(&b, l);
		REQUIRE(result == ISC_R_SUCCESS);
	}
	REQUIRE(isc_buffer_availablelength(b) >= l);

	cp = isc_buffer_used(b);
	memmove(cp, buf, l);
	b->used += l;
}

isc_result_t
isc_buffer_dup(isc_mem_t *mctx, isc_buffer_t **dstp, const isc_buffer_t *src) {
	isc_buffer_t *dst = NULL;
	isc_region_t region;
	isc_result_t result;

	REQUIRE(dstp != NULL && *dstp == NULL);
	REQUIRE(ISC_BUFFER_VALID(src));

	isc_buffer_usedregion(src, &region);

	isc_buffer_allocate(mctx, &dst, region.length);

	result = isc_buffer_copyregion(dst, &region);
	RUNTIME_CHECK(result == ISC_R_SUCCESS); /* NOSPACE is impossible */
	*dstp = dst;
	return (ISC_R_SUCCESS);
}

isc_result_t
isc_buffer_copyregion(isc_buffer_t *b, const isc_region_t *r) {
	isc_result_t result;

	REQUIRE(ISC_BUFFER_VALID(b));
	REQUIRE(r != NULL);

	if (b->autore) {
		result = isc_buffer_reserve(&b, r->length);
		if (result != ISC_R_SUCCESS) {
			return (result);
		}
	}

	if (r->length > isc_buffer_availablelength(b)) {
		return (ISC_R_NOSPACE);
	}

	if (r->length > 0U) {
		memmove(isc_buffer_used(b), r->base, r->length);
		b->used += r->length;
	}

	return (ISC_R_SUCCESS);
}

void
isc_buffer_allocate(isc_mem_t *mctx, isc_buffer_t **dynbuffer,
		    unsigned int length) {
	REQUIRE(dynbuffer != NULL && *dynbuffer == NULL);

	isc_buffer_t *dbuf = isc_mem_get(mctx, sizeof(isc_buffer_t));
	unsigned char *bdata = isc_mem_get(mctx, length);

	isc_buffer_init(dbuf, bdata, length);

	ENSURE(ISC_BUFFER_VALID(dbuf));

	dbuf->mctx = mctx;

	*dynbuffer = dbuf;
}

isc_result_t
isc_buffer_reserve(isc_buffer_t **dynbuffer, unsigned int size) {
	size_t len;

	REQUIRE(dynbuffer != NULL);
	REQUIRE(ISC_BUFFER_VALID(*dynbuffer));

	len = (*dynbuffer)->length;
	if ((len - (*dynbuffer)->used) >= size) {
		return (ISC_R_SUCCESS);
	}

	if ((*dynbuffer)->mctx == NULL) {
		return (ISC_R_NOSPACE);
	}

	/* Round to nearest buffer size increment */
	len = size + (*dynbuffer)->used;
	len = (len + ISC_BUFFER_INCR - 1 - ((len - 1) % ISC_BUFFER_INCR));

	/* Cap at UINT_MAX */
	if (len > UINT_MAX) {
		len = UINT_MAX;
	}

	if ((len - (*dynbuffer)->used) < size) {
		return (ISC_R_NOMEMORY);
	}

	(*dynbuffer)->base = isc_mem_reget((*dynbuffer)->mctx,
					   (*dynbuffer)->base,
					   (*dynbuffer)->length, len);
	(*dynbuffer)->length = (unsigned int)len;

	return (ISC_R_SUCCESS);
}

void
isc_buffer_free(isc_buffer_t **dynbuffer) {
	isc_buffer_t *dbuf;
	isc_mem_t *mctx;

	REQUIRE(dynbuffer != NULL);
	REQUIRE(ISC_BUFFER_VALID(*dynbuffer));
	REQUIRE((*dynbuffer)->mctx != NULL);

	dbuf = *dynbuffer;
	*dynbuffer = NULL; /* destroy external reference */
	mctx = dbuf->mctx;
	dbuf->mctx = NULL;

	isc_mem_put(mctx, dbuf->base, dbuf->length);
	isc_buffer_invalidate(dbuf);
	isc_mem_put(mctx, dbuf, sizeof(isc_buffer_t));
}

isc_result_t
isc_buffer_printf(isc_buffer_t *b, const char *format, ...) {
	va_list ap;
	int n;
	isc_result_t result;

	REQUIRE(ISC_BUFFER_VALID(b));

	va_start(ap, format);
	n = vsnprintf(NULL, 0, format, ap);
	va_end(ap);

	if (n < 0) {
		return (ISC_R_FAILURE);
	}

	if (b->autore) {
		result = isc_buffer_reserve(&b, n + 1);
		if (result != ISC_R_SUCCESS) {
			return (result);
		}
	}

	if (isc_buffer_availablelength(b) < (unsigned int)n + 1) {
		return (ISC_R_NOSPACE);
	}

	va_start(ap, format);
	n = vsnprintf(isc_buffer_used(b), n + 1, format, ap);
	va_end(ap);

	if (n < 0) {
		return (ISC_R_FAILURE);
	}

	b->used += n;

	return (ISC_R_SUCCESS);
}
