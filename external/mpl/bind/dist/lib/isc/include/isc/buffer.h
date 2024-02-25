/*	$NetBSD: buffer.h,v 1.7.2.2 2024/02/25 15:47:20 martin Exp $	*/

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

#pragma once

/*****
***** Module Info
*****/

/*! \file isc/buffer.h
 *
 * \brief A buffer is a region of memory, together with a set of related
 * subregions. Buffers are used for parsing and I/O operations.
 *
 * The 'used region' and the 'available region' are disjoint, and their
 * union is the buffer's region.  The used region extends from the beginning
 * of the buffer region to the last used byte.  The available region
 * extends from one byte greater than the last used byte to the end of the
 * buffer's region.  The size of the used region can be changed using various
 * buffer commands.  Initially, the used region is empty.
 *
 * The used region is further subdivided into two disjoint regions: the
 * 'consumed region' and the 'remaining region'.  The union of these two
 * regions is the used region.  The consumed region extends from the beginning
 * of the used region to the byte before the 'current' offset (if any).  The
 * 'remaining' region extends from the current offset to the end of the used
 * region.  The size of the consumed region can be changed using various
 * buffer commands.  Initially, the consumed region is empty.
 *
 * The 'active region' is an (optional) subregion of the remaining region.
 * It extends from the current offset to an offset in the remaining region
 * that is selected with isc_buffer_setactive().  Initially, the active region
 * is empty.  If the current offset advances beyond the chosen offset, the
 * active region will also be empty.
 *
 * \verbatim
 *  /------------entire length---------------\
 *  /----- used region -----\/-- available --\
 *  +----------------------------------------+
 *  | consumed  | remaining |                |
 *  +----------------------------------------+
 *  a           b     c     d                e
 *
 * a == base of buffer.
 * b == current pointer.  Can be anywhere between a and d.
 * c == active pointer.  Meaningful between b and d.
 * d == used pointer.
 * e == length of buffer.
 *
 * a-e == entire length of buffer.
 * a-d == used region.
 * a-b == consumed region.
 * b-d == remaining region.
 * b-c == optional active region.
 *\endverbatim
 *
 * The following invariants are maintained by all routines:
 *
 *\code
 *	length > 0
 *
 *	base is a valid pointer to length bytes of memory
 *
 *	0 <= used <= length
 *
 *	0 <= current <= used
 *
 *	0 <= active <= used
 *	(although active < current implies empty active region)
 *\endcode
 *
 * \li MP:
 *	Buffers have no synchronization.  Clients must ensure exclusive
 *	access.
 *
 * \li Reliability:
 *	No anticipated impact.
 *
 * \li Resources:
 *	Memory: 1 pointer + 6 unsigned integers per buffer.
 *
 * \li Security:
 *	No anticipated impact.
 *
 * \li Standards:
 *	None.
 */

/***
 *** Imports
 ***/

#include <inttypes.h>
#include <stdbool.h>

#include <isc/assertions.h>
#include <isc/formatcheck.h>
#include <isc/lang.h>
#include <isc/list.h>
#include <isc/magic.h>
#include <isc/region.h>
#include <isc/string.h>
#include <isc/types.h>

ISC_LANG_BEGINDECLS

/*@{*/
/*!
 *** Magic numbers
 ***/
#define ISC_BUFFER_MAGIC    0x42756621U /* Buf!. */
#define ISC_BUFFER_VALID(b) ISC_MAGIC_VALID(b, ISC_BUFFER_MAGIC)
/*@}*/

/*!
 * Size granularity for dynamically resizable buffers; when reserving
 * space in a buffer, we round the allocated buffer length up to the
 * nearest * multiple of this value.
 */
#define ISC_BUFFER_INCR 2048

/*
 * The following macros MUST be used only on valid buffers.  It is the
 * caller's responsibility to ensure this by using the ISC_BUFFER_VALID
 * check above, or by calling another isc_buffer_*() function (rather than
 * another macro.)
 */

/*@{*/
/*!
 * Fundamental buffer elements.  (A through E in the introductory comment.)
 */
#define isc_buffer_base(b) ((void *)(b)->base) /*a*/
#define isc_buffer_current(b) \
	((void *)((unsigned char *)(b)->base + (b)->current)) /*b*/
#define isc_buffer_active(b) \
	((void *)((unsigned char *)(b)->base + (b)->active)) /*c*/
#define isc_buffer_used(b) \
	((void *)((unsigned char *)(b)->base + (b)->used)) /*d*/
#define isc_buffer_length(b) ((b)->length)		   /*e*/
/*@}*/

/*@{*/
/*!
 * Derived lengths.  (Described in the introductory comment.)
 */
#define isc_buffer_usedlength(b)      ((b)->used)		   /* d-a */
#define isc_buffer_consumedlength(b)  ((b)->current)		   /* b-a */
#define isc_buffer_remaininglength(b) ((b)->used - (b)->current)   /* d-b */
#define isc_buffer_activelength(b)    ((b)->active - (b)->current) /* c-b */
#define isc_buffer_availablelength(b) ((b)->length - (b)->used)	   /* e-d */
/*@}*/

/*!
 * Note that the buffer structure is public.  This is principally so buffer
 * operations can be implemented using macros.  Applications are strongly
 * discouraged from directly manipulating the structure.
 */

struct isc_buffer {
	unsigned int magic;
	void	    *base;
	/*@{*/
	/*! The following integers are byte offsets from 'base'. */
	unsigned int length;
	unsigned int used;
	unsigned int current;
	unsigned int active;
	/*@}*/
	/*! linkable */
	ISC_LINK(isc_buffer_t) link;
	/*! private internal elements */
	isc_mem_t *mctx;
	/* automatically realloc buffer at put* */
	bool autore;
};

/***
 *** Functions
 ***/

void
isc_buffer_allocate(isc_mem_t *mctx, isc_buffer_t **dynbuffer,
		    unsigned int length);
/*!<
 * \brief Allocate a dynamic linkable buffer which has "length" bytes in the
 * data region.
 *
 * Requires:
 *\li	"mctx" is valid.
 *
 *\li	"dynbuffer" is non-NULL, and "*dynbuffer" is NULL.
 *
 * Note:
 *\li	Changing the buffer's length field is not permitted.
 */

isc_result_t
isc_buffer_reserve(isc_buffer_t **dynbuffer, unsigned int size);
/*!<
 * \brief Make "size" bytes of space available in the buffer. The buffer
 * pointer may move when you call this function.
 *
 * Requires:
 *\li	"dynbuffer" is not NULL.
 *
 *\li	"*dynbuffer" is a valid dynamic buffer.
 *
 * Returns:
 *\li	ISC_R_SUCCESS		- success
 *\li	ISC_R_NOMEMORY		- no memory available
 *
 * Ensures:
 *\li	"*dynbuffer" will be valid on return and will contain all the
 *	original data. However, the buffer pointer may be moved during
 *	reallocation.
 */

void
isc_buffer_free(isc_buffer_t **dynbuffer);
/*!<
 * \brief Release resources allocated for a dynamic buffer.
 *
 * Requires:
 *\li	"dynbuffer" is not NULL.
 *
 *\li	"*dynbuffer" is a valid dynamic buffer.
 *
 * Ensures:
 *\li	"*dynbuffer" will be NULL on return, and all memory associated with
 *	the dynamic buffer is returned to the memory context used in
 *	isc_buffer_allocate().
 */

void
isc__buffer_initnull(isc_buffer_t *b);

void
isc_buffer_reinit(isc_buffer_t *b, void *base, unsigned int length);
/*!<
 * \brief Make 'b' refer to the 'length'-byte region starting at base.
 * Any existing data will be copied.
 *
 * Requires:
 *
 *\li	'length' > 0 AND length >= previous length
 *
 *\li	'base' is a pointer to a sequence of 'length' bytes.
 *
 */

void
isc_buffer_setautorealloc(isc_buffer_t *b, bool enable);
/*!<
 * \brief Enable or disable autoreallocation on 'b'.
 *
 * Requires:
 *\li	'b' is a valid dynamic buffer (b->mctx != NULL).
 *
 */

void
isc_buffer_compact(isc_buffer_t *b);
/*!<
 * \brief Compact the used region by moving the remaining region so it occurs
 * at the start of the buffer.  The used region is shrunk by the size of
 * the consumed region, and the consumed region is then made empty.
 *
 * Requires:
 *
 *\li	'b' is a valid buffer
 *
 * Ensures:
 *
 *\li	current == 0
 *
 *\li	The size of the used region is now equal to the size of the remaining
 *	region (as it was before the call).  The contents of the used region
 *	are those of the remaining region (as it was before the call).
 */

uint8_t
isc_buffer_getuint8(isc_buffer_t *b);
/*!<
 * \brief Read an unsigned 8-bit integer from 'b' and return it.
 *
 * Requires:
 *
 *\li	'b' is a valid buffer.
 *
 *\li	The length of the remaining region of 'b' is at least 1.
 *
 * Ensures:
 *
 *\li	The current pointer in 'b' is advanced by 1.
 *
 * Returns:
 *
 *\li	A 8-bit unsigned integer.
 */

uint16_t
isc_buffer_getuint16(isc_buffer_t *b);
/*!<
 * \brief Read an unsigned 16-bit integer in network byte order from 'b',
 * convert it to host byte order, and return it.
 *
 * Requires:
 *
 *\li	'b' is a valid buffer.
 *
 *\li	The length of the remaining region of 'b' is at least 2.
 *
 * Ensures:
 *
 *\li	The current pointer in 'b' is advanced by 2.
 *
 * Returns:
 *
 *\li	A 16-bit unsigned integer.
 */

uint32_t
isc_buffer_getuint32(isc_buffer_t *b);
/*!<
 * \brief Read an unsigned 32-bit integer in network byte order from 'b',
 * convert it to host byte order, and return it.
 *
 * Requires:
 *
 *\li	'b' is a valid buffer.
 *
 *\li	The length of the remaining region of 'b' is at least 4.
 *
 * Ensures:
 *
 *\li	The current pointer in 'b' is advanced by 4.
 *
 * Returns:
 *
 *\li	A 32-bit unsigned integer.
 */

uint64_t
isc_buffer_getuint48(isc_buffer_t *b);
/*!<
 * \brief Read an unsigned 48-bit integer in network byte order from 'b',
 * convert it to host byte order, and return it.
 *
 * Requires:
 *
 *\li	'b' is a valid buffer.
 *
 *\li	The length of the remaining region of 'b' is at least 6.
 *
 * Ensures:
 *
 *\li	The current pointer in 'b' is advanced by 6.
 *
 * Returns:
 *
 *\li	A 48-bit unsigned integer (stored in a 64-bit integer).
 */

void
isc_buffer_putdecint(isc_buffer_t *b, int64_t v);
/*!<
 * \brief Put decimal representation of 'v' in b
 *
 * Requires:
 *\li	'b' is a valid buffer.
 *
 *\li	The length of the available region of 'b' is at least strlen(dec('v'))
 *	or the buffer has autoreallocation enabled.
 *
 * Ensures:
 *\li	The used pointer in 'b' is advanced by strlen(dec('v')).
 */

isc_result_t
isc_buffer_copyregion(isc_buffer_t *b, const isc_region_t *r);
/*!<
 * \brief Copy the contents of 'r' into 'b'.
 *
 * Notes:
 *\li	If 'b' has autoreallocation enabled, and the length of 'r' is greater
 *	than the length of the available region of 'b', 'b' is reallocated.
 *
 * Requires:
 *\li	'b' is a valid buffer.
 *
 *\li	'r' is a valid region.
 *
 * Returns:
 *\li	ISC_R_SUCCESS
 *\li	ISC_R_NOSPACE			The available region of 'b' is not
 *					big enough.
 */

isc_result_t
isc_buffer_dup(isc_mem_t *mctx, isc_buffer_t **dstp, const isc_buffer_t *src);
/*!<
 * \brief Allocate 'dst' and copy used contents of 'src' into it.
 *
 * Requires:
 *\li	'dstp' is not NULL and *dst is NULL.
 *\li	'src' is a valid buffer.
 *
 * Returns:
 *\li	ISC_R_SUCCESS
 */

isc_result_t
isc_buffer_printf(isc_buffer_t *b, const char *format, ...)
	ISC_FORMAT_PRINTF(2, 3);
/*!<
 * \brief Append a formatted string to the used region of 'b'.
 *
 * Notes:
 *
 *\li	The 'format' argument is a printf(3) string, with additional arguments
 *	as necessary.
 *
 *\li	If 'b' has autoreallocation enabled, and the length of the formatted
 *	string is greater than the length of the available region of 'b', 'b'
 *	is reallocated.
 *
 * Requires:
 *
 *\li	'b' is a valid buffer.
 *
 * Ensures:
 *
 *\li	The used pointer in 'b' is advanced by the number of bytes appended
 *	(excluding the terminating NULL byte).
 *
 * Returns:
 *
 *\li	#ISC_R_SUCCESS	Operation succeeded.
 *\li	#ISC_R_NOSPACE	'b' does not allow reallocation and appending the
 *			formatted string to it would cause it to overflow.
 *\li	#ISC_R_NOMEMORY	Reallocation failed.
 *\li	#ISC_R_FAILURE	Other error occurred.
 */

/*
 * Buffer functions implemented as inline.
 */

/*! \note
 * XXXDCL Something more could be done with initializing buffers that
 * point to const data.  For example, isc_buffer_constinit() could
 * set a new boolean flag in the buffer structure indicating whether
 * the buffer was initialized with that function.  Then if the
 * boolean were true, the isc_buffer_put* functions could assert a
 * contractual requirement for a non-const buffer.
 *
 * One drawback is that the isc_buffer_* functions that return
 * pointers would still need to return non-const pointers to avoid compiler
 * warnings, so it would be up to code that uses them to have to deal
 * with the possibility that the buffer was initialized as const --
 * a problem that they *already* have to deal with but have absolutely
 * no ability to.  With a new isc_buffer_isconst() function returning
 * true/false, they could at least assert a contractual requirement for
 * non-const buffers when needed.
 */

/*!
 * \brief Make 'b' refer to the 'length'-byte region starting at 'base'.
 *
 * Requires:
 *
 *\li	'length' > 0
 *
 *\li	'base' is a pointer to a sequence of 'length' bytes.
 */
static inline void
isc_buffer_init(isc_buffer_t *b, void *base, unsigned int length) {
	ISC_REQUIRE(b != NULL);

	*b = (isc_buffer_t){ .base = base,
			     .length = length,
			     .magic = ISC_BUFFER_MAGIC };
	ISC_LINK_INIT(b, link);
}

/*!
 *\brief Initialize a buffer 'b' with a null data field and zero length.
 * This can later be grown as needed and swapped in place.
 */
static inline void
isc_buffer_initnull(isc_buffer_t *b) {
	*b = (isc_buffer_t){ .magic = ISC_BUFFER_MAGIC };
	ISC_LINK_INIT(b, link);
}

/*!
 * \brief Make 'b' refer to the 'length'-byte constant region starting
 * at 'base'.
 *
 * Requires:
 *
 *\li	'length' > 0
 *\li	'base' is a pointer to a sequence of 'length' bytes.
 */
#define isc_buffer_constinit(_b, _d, _l)                    \
	do {                                                \
		union {                                     \
			void	   *_var;                   \
			const void *_const;                 \
		} _deconst;                                 \
		_deconst._const = (_d);                     \
		isc_buffer_init((_b), _deconst._var, (_l)); \
	} while (0)

/*!
 * \brief Make 'b' an invalid buffer.
 *
 * Requires:
 *\li	'b' is a valid buffer.
 *
 * Ensures:
 *\li	Future attempts to use 'b' without calling isc_buffer_init() on
 *	it will cause an assertion failure.
 */
static inline void
isc_buffer_invalidate(isc_buffer_t *b) {
	ISC_REQUIRE(ISC_BUFFER_VALID(b));
	ISC_REQUIRE(!ISC_LINK_LINKED(b, link));
	ISC_REQUIRE(b->mctx == NULL);

	b->magic = 0;
	b->base = NULL;
	b->length = 0;
	b->used = 0;
	b->current = 0;
	b->active = 0;
}

/*!
 * \brief Make 'r' refer to the region of 'b'.
 *
 * Requires:
 *
 *\li	'b' is a valid buffer.
 *
 *\li	'r' points to a region structure.
 */
static inline void
isc_buffer_region(isc_buffer_t *b, isc_region_t *r) {
	ISC_REQUIRE(ISC_BUFFER_VALID(b));
	ISC_REQUIRE(r != NULL);

	r->base = b->base;
	r->length = b->length;
}

/*!
 * \brief Make 'r' refer to the used region of 'b'.
 *
 * Requires:
 *
 *\li	'b' is a valid buffer.
 *
 *\li	'r' points to a region structure.
 */
static inline void
isc_buffer_usedregion(const isc_buffer_t *b, isc_region_t *r) {
	ISC_REQUIRE(ISC_BUFFER_VALID(b));
	ISC_REQUIRE(r != NULL);

	r->base = b->base;
	r->length = b->used;
}

/*!
 * \brief Make 'r' refer to the available region of 'b'.
 *
 * Requires:
 *
 *\li	'b' is a valid buffer.
 *
 *\li	'r' points to a region structure.
 */
static inline void
isc_buffer_availableregion(isc_buffer_t *b, isc_region_t *r) {
	ISC_REQUIRE(ISC_BUFFER_VALID(b));
	ISC_REQUIRE(r != NULL);

	r->base = isc_buffer_used(b);
	r->length = isc_buffer_availablelength(b);
}

/*!
 * \brief Increase the 'used' region of 'b' by 'n' bytes.
 *
 * Requires:
 *
 *\li	'b' is a valid buffer
 *
 *\li	used + n <= length
 */
static inline void
isc_buffer_add(isc_buffer_t *b, unsigned int n) {
	ISC_REQUIRE(ISC_BUFFER_VALID(b));
	ISC_REQUIRE(b->used + n <= b->length);

	b->used += n;
}

/*!
 * \brief Decrease the 'used' region of 'b' by 'n' bytes.
 *
 * Requires:
 *
 *\li	'b' is a valid buffer
 *
 *\li	used >= n
 */
static inline void
isc_buffer_subtract(isc_buffer_t *b, unsigned int n) {
	ISC_REQUIRE(ISC_BUFFER_VALID(b));
	ISC_REQUIRE(b->used >= n);

	b->used -= n;
	if (b->current > b->used) {
		b->current = b->used;
	}
	if (b->active > b->used) {
		b->active = b->used;
	}
}

/*!<
 * \brief Make the used region empty.
 *
 * Requires:
 *
 *\li	'b' is a valid buffer
 *
 * Ensures:
 *
 *\li	used = 0
 */
static inline void
isc_buffer_clear(isc_buffer_t *b) {
	ISC_REQUIRE(ISC_BUFFER_VALID(b));

	b->used = 0;
	b->current = 0;
	b->active = 0;
}

/*!
 * \brief Make 'r' refer to the consumed region of 'b'.
 *
 * Requires:
 *
 *\li	'b' is a valid buffer.
 *
 *\li	'r' points to a region structure.
 */
static inline void
isc_buffer_consumedregion(isc_buffer_t *b, isc_region_t *r) {
	ISC_REQUIRE(ISC_BUFFER_VALID(b));
	ISC_REQUIRE(r != NULL);

	r->base = b->base;
	r->length = b->current;
}

/*!
 * \brief Make 'r' refer to the remaining region of 'b'.
 *
 * Requires:
 *
 *\li	'b' is a valid buffer.
 *
 *\li	'r' points to a region structure.
 */
static inline void
isc_buffer_remainingregion(isc_buffer_t *b, isc_region_t *r) {
	ISC_REQUIRE(ISC_BUFFER_VALID(b));
	ISC_REQUIRE(r != NULL);

	r->base = isc_buffer_current(b);
	r->length = isc_buffer_remaininglength(b);
}

/*!
 * \brief Make 'r' refer to the active region of 'b'.
 *
 * Requires:
 *
 *\li	'b' is a valid buffer.
 *
 *\li	'r' points to a region structure.
 */
static inline void
isc_buffer_activeregion(isc_buffer_t *b, isc_region_t *r) {
	ISC_REQUIRE(ISC_BUFFER_VALID(b));
	ISC_REQUIRE(r != NULL);

	if (b->current < b->active) {
		r->base = isc_buffer_current(b);
		r->length = isc_buffer_activelength(b);
	} else {
		r->base = NULL;
		r->length = 0;
	}
}

/*!
 * \brief Sets the end of the active region 'n' bytes after current.
 *
 * Requires:
 *
 *\li	'b' is a valid buffer.
 *
 *\li	current + n <= used
 */
static inline void
isc_buffer_setactive(isc_buffer_t *b, unsigned int n) {
	ISC_REQUIRE(ISC_BUFFER_VALID(b));
	ISC_REQUIRE(b->current + n <= b->used);

	b->active = b->current + n;
}

/*!<
 * \brief Make the consumed region empty.
 *
 * Requires:
 *
 *\li	'b' is a valid buffer
 *
 * Ensures:
 *
 *\li	current == 0
 */
static inline void
isc_buffer_first(isc_buffer_t *b) {
	ISC_REQUIRE(ISC_BUFFER_VALID(b));

	b->current = 0;
}

/*!
 * \brief Increase the 'consumed' region of 'b' by 'n' bytes.
 *
 * Requires:
 *
 *\li	'b' is a valid buffer
 *
 *\li	current + n <= used
 */
static inline void
isc_buffer_forward(isc_buffer_t *b, unsigned int n) {
	ISC_REQUIRE(ISC_BUFFER_VALID(b));
	ISC_REQUIRE(b->current + n <= b->used);

	b->current += n;
}

/*!
 * \brief Decrease the 'consumed' region of 'b' by 'n' bytes.
 *
 * Requires:
 *
 *\li	'b' is a valid buffer
 *
 *\li	n <= current
 */
static inline void
isc_buffer_back(isc_buffer_t *b, unsigned int n) {
	ISC_REQUIRE(ISC_BUFFER_VALID(b));
	ISC_REQUIRE(n <= b->current);

	b->current -= n;
}

/*!
 * \brief Store an unsigned 8-bit integer from 'val' into 'b'.
 *
 * Requires:
 *\li	'b' is a valid buffer.
 *
 *\li	The length of the available region of 'b' is at least 1
 *	or the buffer has autoreallocation enabled.
 *
 * Ensures:
 *\li	The used pointer in 'b' is advanced by 1.
 */
static inline void
isc_buffer_putuint8(isc_buffer_t *b, uint8_t val) {
	unsigned char *cp;

	ISC_REQUIRE(ISC_BUFFER_VALID(b));

	if (b->autore) {
		isc_buffer_t *tmp = b;
		ISC_REQUIRE(isc_buffer_reserve(&tmp, 1) == ISC_R_SUCCESS);
	}

	ISC_REQUIRE(isc_buffer_availablelength(b) >= 1U);

	cp = isc_buffer_used(b);
	b->used++;
	cp[0] = val;
}

/*!
 * \brief Store an unsigned 16-bit integer in host byte order from 'val'
 * into 'b' in network byte order.
 *
 * Requires:
 *\li	'b' is a valid buffer.
 *
 *\li	The length of the available region of 'b' is at least 2
 *	or the buffer has autoreallocation enabled.
 *
 * Ensures:
 *\li	The used pointer in 'b' is advanced by 2.
 */
static inline void
isc_buffer_putuint16(isc_buffer_t *b, uint16_t val) {
	unsigned char *cp;

	ISC_REQUIRE(ISC_BUFFER_VALID(b));

	if (b->autore) {
		isc_buffer_t *tmp = b;
		ISC_REQUIRE(isc_buffer_reserve(&tmp, 2) == ISC_R_SUCCESS);
	}

	ISC_REQUIRE(isc_buffer_availablelength(b) >= 2U);

	cp = isc_buffer_used(b);
	b->used += 2;
	cp[0] = (unsigned char)(val >> 8);
	cp[1] = (unsigned char)val;
}

/*!
 * Store an unsigned 24-bit integer in host byte order from 'val'
 * into 'b' in network byte order.
 *
 * Requires:
 *\li	'b' is a valid buffer.
 *
 *\li	The length of the available region of 'b' is at least 3
 *	or the buffer has autoreallocation enabled.
 *
 * Ensures:
 *\li	The used pointer in 'b' is advanced by 3.
 */
static inline void
isc_buffer_putuint24(isc_buffer_t *b, uint32_t val) {
	unsigned char *cp;

	ISC_REQUIRE(ISC_BUFFER_VALID(b));

	if (b->autore) {
		isc_buffer_t *tmp = b;
		ISC_REQUIRE(isc_buffer_reserve(&tmp, 3) == ISC_R_SUCCESS);
	}

	ISC_REQUIRE(isc_buffer_availablelength(b) >= 3U);

	cp = isc_buffer_used(b);
	b->used += 3;
	cp[0] = (unsigned char)(val >> 16);
	cp[1] = (unsigned char)(val >> 8);
	cp[2] = (unsigned char)val;
}

/*!
 * \brief Store an unsigned 32-bit integer in host byte order from 'val'
 * into 'b' in network byte order.
 *
 * Requires:
 *\li	'b' is a valid buffer.
 *
 *\li	The length of the available region of 'b' is at least 4
 *	or the buffer has autoreallocation enabled.
 *
 * Ensures:
 *\li	The used pointer in 'b' is advanced by 4.
 */
static inline void
isc_buffer_putuint32(isc_buffer_t *b, uint32_t val) {
	unsigned char *cp;

	ISC_REQUIRE(ISC_BUFFER_VALID(b));

	if (b->autore) {
		isc_buffer_t *tmp = b;
		ISC_REQUIRE(isc_buffer_reserve(&tmp, 4) == ISC_R_SUCCESS);
	}

	ISC_REQUIRE(isc_buffer_availablelength(b) >= 4U);

	cp = isc_buffer_used(b);
	b->used += 4;
	cp[0] = (unsigned char)(val >> 24);
	cp[1] = (unsigned char)(val >> 16);
	cp[2] = (unsigned char)(val >> 8);
	cp[3] = (unsigned char)val;
}

/*!
 * \brief Store an unsigned 48-bit integer in host byte order from 'val'
 * into 'b' in network byte order.
 *
 * Requires:
 *\li	'b' is a valid buffer.
 *
 *\li	The length of the available region of 'b' is at least 6
 *	or the buffer has autoreallocation enabled.
 *
 * Ensures:
 *\li	The used pointer in 'b' is advanced by 6.
 */
static inline void
isc_buffer_putuint48(isc_buffer_t *b, uint64_t val) {
	unsigned char *cp = NULL;

	ISC_REQUIRE(ISC_BUFFER_VALID(b));

	if (b->autore) {
		isc_buffer_t *tmp = b;
		ISC_REQUIRE(isc_buffer_reserve(&tmp, 6) == ISC_R_SUCCESS);
	}

	ISC_REQUIRE(isc_buffer_availablelength(b) >= 6U);

	cp = isc_buffer_used(b);
	b->used += 6;
	cp[0] = (unsigned char)(val >> 40);
	cp[1] = (unsigned char)(val >> 32);
	cp[2] = (unsigned char)(val >> 24);
	cp[3] = (unsigned char)(val >> 16);
	cp[4] = (unsigned char)(val >> 8);
	cp[5] = (unsigned char)val;
}

/*!
 * \brief Copy 'length' bytes of memory at 'base' into 'b'.
 *
 * Requires:
 *\li	'b' is a valid buffer.
 *
 *\li	'base' points to 'length' bytes of valid memory.
 *
 *\li	The length of the available region of 'b' is at least 'length'
 *	or the buffer has autoreallocation enabled.
 *
 * Ensures:
 *\li	The used pointer in 'b' is advanced by 'length'.
 */
static inline void
isc_buffer_putmem(isc_buffer_t *b, const unsigned char *base,
		  unsigned int length) {
	ISC_REQUIRE(ISC_BUFFER_VALID(b));

	if (b->autore) {
		isc_buffer_t *tmp = b;
		ISC_REQUIRE(isc_buffer_reserve(&tmp, length) == ISC_R_SUCCESS);
	}

	ISC_REQUIRE(isc_buffer_availablelength(b) >= (unsigned int)length);

	if (length > 0U) {
		memmove(isc_buffer_used(b), base, length);
		b->used += length;
	}
}

/*!
 * \brief Copy 'source' into 'b', not including terminating NUL.
 *
 * Requires:
 *\li	'b' is a valid buffer.
 *
 *\li	'source' is a valid NULL terminated string.
 *
 *\li	The length of the available region of 'b' is at least strlen('source')
 *	or the buffer has autoreallocation enabled.
 *
 * Ensures:
 *\li	The used pointer in 'b' is advanced by strlen('source').
 */
static inline void
isc_buffer_putstr(isc_buffer_t *b, const char *source) {
	unsigned int   length;
	unsigned char *cp;

	ISC_REQUIRE(ISC_BUFFER_VALID(b));
	ISC_REQUIRE(source != NULL);

	length = (unsigned int)strlen(source);
	if (b->autore) {
		isc_buffer_t *tmp = b;
		ISC_REQUIRE(isc_buffer_reserve(&tmp, length) == ISC_R_SUCCESS);
	}

	ISC_REQUIRE(isc_buffer_availablelength(b) >= length);

	cp = isc_buffer_used(b);
	memmove(cp, source, length);
	b->used += length;
}
ISC_LANG_ENDDECLS
