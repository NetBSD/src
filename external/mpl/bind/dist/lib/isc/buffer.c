/*	$NetBSD: buffer.c,v 1.4 2019/11/27 05:48:42 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

/*! \file */

#include <config.h>

#include <inttypes.h>
#include <stdbool.h>
#include <stdarg.h>

#include <isc/buffer.h>
#include <isc/mem.h>
#include <isc/print.h>
#include <isc/region.h>
#include <isc/string.h>
#include <isc/util.h>

void
isc__buffer_init(isc_buffer_t *b, void *base, unsigned int length) {
	/*
	 * Make 'b' refer to the 'length'-byte region starting at 'base'.
	 * XXXDCL see the comment in buffer.h about base being const.
	 */

	REQUIRE(b != NULL);

	ISC__BUFFER_INIT(b, base, length);
}

void
isc__buffer_initnull(isc_buffer_t *b) {
	/*
	 * Initialize a new buffer which has no backing store.  This can
	 * later be grown as needed and swapped in place.
	 */

	ISC__BUFFER_INIT(b, NULL, 0);
}

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
isc__buffer_invalidate(isc_buffer_t *b) {
	/*
	 * Make 'b' an invalid buffer.
	 */

	REQUIRE(ISC_BUFFER_VALID(b));
	REQUIRE(!ISC_LINK_LINKED(b, link));
	REQUIRE(b->mctx == NULL);

	ISC__BUFFER_INVALIDATE(b);
}

void
isc_buffer_setautorealloc(isc_buffer_t *b, bool enable) {
	REQUIRE(ISC_BUFFER_VALID(b));
	REQUIRE(b->mctx != NULL);
	b->autore = enable;
}

void
isc__buffer_region(isc_buffer_t *b, isc_region_t *r) {
	/*
	 * Make 'r' refer to the region of 'b'.
	 */

	REQUIRE(ISC_BUFFER_VALID(b));
	REQUIRE(r != NULL);

	ISC__BUFFER_REGION(b, r);
}

void
isc__buffer_usedregion(const isc_buffer_t *b, isc_region_t *r) {
	/*
	 * Make 'r' refer to the used region of 'b'.
	 */

	REQUIRE(ISC_BUFFER_VALID(b));
	REQUIRE(r != NULL);

	ISC__BUFFER_USEDREGION(b, r);
}

void
isc__buffer_availableregion(isc_buffer_t *b, isc_region_t *r) {
	/*
	 * Make 'r' refer to the available region of 'b'.
	 */

	REQUIRE(ISC_BUFFER_VALID(b));
	REQUIRE(r != NULL);

	ISC__BUFFER_AVAILABLEREGION(b, r);
}

void
isc__buffer_add(isc_buffer_t *b, unsigned int n) {
	/*
	 * Increase the 'used' region of 'b' by 'n' bytes.
	 */

	REQUIRE(ISC_BUFFER_VALID(b));
	REQUIRE(b->used + n <= b->length);

	ISC__BUFFER_ADD(b, n);
}

void
isc__buffer_subtract(isc_buffer_t *b, unsigned int n) {
	/*
	 * Decrease the 'used' region of 'b' by 'n' bytes.
	 */

	REQUIRE(ISC_BUFFER_VALID(b));
	REQUIRE(b->used >= n);

	ISC__BUFFER_SUBTRACT(b, n);
}

void
isc__buffer_clear(isc_buffer_t *b) {
	/*
	 * Make the used region empty.
	 */

	REQUIRE(ISC_BUFFER_VALID(b));

	ISC__BUFFER_CLEAR(b);
}

void
isc__buffer_consumedregion(isc_buffer_t *b, isc_region_t *r) {
	/*
	 * Make 'r' refer to the consumed region of 'b'.
	 */

	REQUIRE(ISC_BUFFER_VALID(b));
	REQUIRE(r != NULL);

	ISC__BUFFER_CONSUMEDREGION(b, r);
}

void
isc__buffer_remainingregion(isc_buffer_t *b, isc_region_t *r) {
	/*
	 * Make 'r' refer to the remaining region of 'b'.
	 */

	REQUIRE(ISC_BUFFER_VALID(b));
	REQUIRE(r != NULL);

	ISC__BUFFER_REMAININGREGION(b, r);
}

void
isc__buffer_activeregion(isc_buffer_t *b, isc_region_t *r) {
	/*
	 * Make 'r' refer to the active region of 'b'.
	 */

	REQUIRE(ISC_BUFFER_VALID(b));
	REQUIRE(r != NULL);

	ISC__BUFFER_ACTIVEREGION(b, r);
}

void
isc__buffer_setactive(isc_buffer_t *b, unsigned int n) {
	/*
	 * Sets the end of the active region 'n' bytes after current.
	 */

	REQUIRE(ISC_BUFFER_VALID(b));
	REQUIRE(b->current + n <= b->used);

	ISC__BUFFER_SETACTIVE(b, n);
}

void
isc__buffer_first(isc_buffer_t *b) {
	/*
	 * Make the consumed region empty.
	 */

	REQUIRE(ISC_BUFFER_VALID(b));

	ISC__BUFFER_FIRST(b);
}

void
isc__buffer_forward(isc_buffer_t *b, unsigned int n) {
	/*
	 * Increase the 'consumed' region of 'b' by 'n' bytes.
	 */

	REQUIRE(ISC_BUFFER_VALID(b));
	REQUIRE(b->current + n <= b->used);

	ISC__BUFFER_FORWARD(b, n);
}

void
isc__buffer_back(isc_buffer_t *b, unsigned int n) {
	/*
	 * Decrease the 'consumed' region of 'b' by 'n' bytes.
	 */

	REQUIRE(ISC_BUFFER_VALID(b));
	REQUIRE(n <= b->current);

	ISC__BUFFER_BACK(b, n);
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

	if (b->active > b->current)
		b->active -= b->current;
	else
		b->active = 0;
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

void
isc__buffer_putuint8(isc_buffer_t *b, uint8_t val) {
	isc_result_t result;
	REQUIRE(ISC_BUFFER_VALID(b));
	if (ISC_UNLIKELY(b->autore)) {
		result = isc_buffer_reserve(&b, 1);
		REQUIRE(result == ISC_R_SUCCESS);
	}
	REQUIRE(isc_buffer_availablelength(b) >= 1);

	ISC__BUFFER_PUTUINT8(b, val);
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

void
isc__buffer_putuint16(isc_buffer_t *b, uint16_t val) {
	isc_result_t result;
	REQUIRE(ISC_BUFFER_VALID(b));
	if (ISC_UNLIKELY(b->autore)) {
		result = isc_buffer_reserve(&b, 2);
		REQUIRE(result == ISC_R_SUCCESS);
	}
	REQUIRE(isc_buffer_availablelength(b) >= 2);

	ISC__BUFFER_PUTUINT16(b, val);
}

void
isc__buffer_putuint24(isc_buffer_t *b, uint32_t val) {
	isc_result_t result;
	REQUIRE(ISC_BUFFER_VALID(b));
	if (ISC_UNLIKELY(b->autore)) {
		result = isc_buffer_reserve(&b, 3);
		REQUIRE(result == ISC_R_SUCCESS);
	}
	REQUIRE(isc_buffer_availablelength(b) >= 3);

	ISC__BUFFER_PUTUINT24(b, val);
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

void
isc__buffer_putuint32(isc_buffer_t *b, uint32_t val) {
	isc_result_t result;
	REQUIRE(ISC_BUFFER_VALID(b));
	if (ISC_UNLIKELY(b->autore)) {
		result = isc_buffer_reserve(&b, 4);
		REQUIRE(result == ISC_R_SUCCESS);
	}
	REQUIRE(isc_buffer_availablelength(b) >= 4);

	ISC__BUFFER_PUTUINT32(b, val);
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
isc__buffer_putuint48(isc_buffer_t *b, uint64_t val) {
	isc_result_t result;
	uint16_t valhi;
	uint32_t vallo;

	REQUIRE(ISC_BUFFER_VALID(b));
	if (ISC_UNLIKELY(b->autore)) {
		result = isc_buffer_reserve(&b, 6);
		REQUIRE(result == ISC_R_SUCCESS);
	}
	REQUIRE(isc_buffer_availablelength(b) >= 6);

	valhi = (uint16_t)(val >> 32);
	vallo = (uint32_t)(val & 0xFFFFFFFF);
	ISC__BUFFER_PUTUINT16(b, valhi);
	ISC__BUFFER_PUTUINT32(b, vallo);
}

void
isc__buffer_putmem(isc_buffer_t *b, const unsigned char *base,
		   unsigned int length)
{
	isc_result_t result;
	REQUIRE(ISC_BUFFER_VALID(b));
	if (ISC_UNLIKELY(b->autore)) {
		result = isc_buffer_reserve(&b, length);
		REQUIRE(result == ISC_R_SUCCESS);
	}
	REQUIRE(isc_buffer_availablelength(b) >= length);

	ISC__BUFFER_PUTMEM(b, base, length);
}

void
isc__buffer_putstr(isc_buffer_t *b, const char *source) {
	unsigned int l;
	unsigned char *cp;
	isc_result_t result;

	REQUIRE(ISC_BUFFER_VALID(b));
	REQUIRE(source != NULL);

	/*
	 * Do not use ISC__BUFFER_PUTSTR(), so strlen is only done once.
	 */
	l = strlen(source);
	if (ISC_UNLIKELY(b->autore)) {
		result = isc_buffer_reserve(&b, l);
		REQUIRE(result == ISC_R_SUCCESS);
	}
	REQUIRE(isc_buffer_availablelength(b) >= l);

	cp = isc_buffer_used(b);
	memmove(cp, source, l);
	b->used += l;
}

void
isc_buffer_putdecint(isc_buffer_t *b, int64_t v) {
	unsigned int l=0;
	unsigned char *cp;
	char buf[21];
	isc_result_t result;

	REQUIRE(ISC_BUFFER_VALID(b));

	/* xxxwpk do it more low-level way ? */
	l = snprintf(buf, 21, "%" PRId64, v);
	RUNTIME_CHECK(l <= 21);
	if (ISC_UNLIKELY(b->autore)) {
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

	result = isc_buffer_allocate(mctx, &dst, region.length);
	if (result != ISC_R_SUCCESS)
		return (result);

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

	if (ISC_UNLIKELY(b->autore)) {
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

isc_result_t
isc_buffer_allocate(isc_mem_t *mctx, isc_buffer_t **dynbuffer,
		    unsigned int length)
{
	isc_buffer_t *dbuf;
	unsigned char * bdata;
	REQUIRE(dynbuffer != NULL);
	REQUIRE(*dynbuffer == NULL);

	dbuf = isc_mem_get(mctx, sizeof(isc_buffer_t));
	if (dbuf == NULL)
		return (ISC_R_NOMEMORY);

	bdata = isc_mem_get(mctx, length);
	if (bdata == NULL) {
		isc_mem_put(mctx, dbuf, sizeof(isc_buffer_t));
		return (ISC_R_NOMEMORY);
	}

	isc_buffer_init(dbuf, bdata, length);

	ENSURE(ISC_BUFFER_VALID(dbuf));

	dbuf->mctx = mctx;

	*dynbuffer = dbuf;

	return (ISC_R_SUCCESS);
}

isc_result_t
isc_buffer_reserve(isc_buffer_t **dynbuffer, unsigned int size) {
	unsigned char *bdata;
	uint64_t len;

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

	/*
	 * XXXMUKS: This is far more expensive than plain realloc() as
	 * it doesn't remap pages, but does ordinary copy. So is
	 * isc_mem_reallocate(), which has additional issues.
	 */
	bdata = isc_mem_get((*dynbuffer)->mctx, (unsigned int) len);
	if (bdata == NULL) {
		return (ISC_R_NOMEMORY);
	}

	memmove(bdata, (*dynbuffer)->base, (*dynbuffer)->length);
	isc_mem_put((*dynbuffer)->mctx, (*dynbuffer)->base,
		    (*dynbuffer)->length);

	(*dynbuffer)->base = bdata;
	(*dynbuffer)->length = (unsigned int) len;

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
	*dynbuffer = NULL;	/* destroy external reference */
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

	if (ISC_UNLIKELY(b->autore)) {
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
