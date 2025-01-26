/*	$NetBSD: buffer.h,v 1.10 2025/01/26 16:25:40 christos Exp $	*/

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
#include <isc/endian.h>
#include <isc/formatcheck.h>
#include <isc/lang.h>
#include <isc/list.h>
#include <isc/magic.h>
#include <isc/mem.h>
#include <isc/region.h>
#include <isc/string.h>
#include <isc/types.h>
#include <isc/util.h>

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
#define ISC_BUFFER_INCR 512

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
	/*! The extra bytes allocated for static buffer */
	unsigned int extra;
	bool	     dynamic;
	/*! linkable */
	ISC_LINK(isc_buffer_t) link;
	/*! private internal elements */
	isc_mem_t *mctx;
};

/***
 *** Functions
 ***/

static inline void
isc_buffer_allocate(isc_mem_t	      *mctx, isc_buffer_t **restrict dynbuffer,
		    const unsigned int length);
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

static inline void
isc_buffer_setmctx(isc_buffer_t *restrict b, isc_mem_t *mctx);
static inline void
isc_buffer_clearmctx(isc_buffer_t *restrict b);
/*!<
 * \brief Sets/Clears the internal memory context, so isc_buffer_reserve() can
 * be used on previously 'static' buffer.
 */

static inline isc_result_t
isc_buffer_reserve(isc_buffer_t *restrict dynbuffer, const unsigned int size);
/*!<
 * \brief Make "size" bytes of space available in the buffer. The buffer
 * pointer may move when you call this function.
 *
 * Requires:
 *\li	"dynbuffer" is a valid dynamic buffer.
 *
 * Returns:
 *\li	ISC_R_SUCCESS		- success
 *\li	ISC_R_NOMEMORY		- no memory available
 */

static inline void
isc_buffer_free(isc_buffer_t **restrict dynbuffer);
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

static inline void
isc_buffer_initnull(isc_buffer_t *restrict b);

static inline void
isc_buffer_reinit(isc_buffer_t *restrict b, void *base,
		  const unsigned int length);
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

static inline void
isc_buffer_trycompact(isc_buffer_t *restrict b);
static inline void
isc_buffer_compact(isc_buffer_t *restrict b);
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

static inline isc_result_t
isc_buffer_peekuint8(const isc_buffer_t *restrict b, uint8_t *valp);
static inline uint8_t
isc_buffer_getuint8(isc_buffer_t *restrict b);
static inline void
isc_buffer_putuint8(isc_buffer_t *restrict b, const uint8_t val);
/*!<
 * \brief Peek/Read/Write an unsigned 8-bit integer from/to 'b'.
 *
 * Requires:
 *
 *\li	'b' is a valid buffer.
 *
 *\li	The length of the available region of 'b' is at least 1
 *	or the buffer has autoreallocation enabled.
 *
 * Ensures (for Read):
 *
 *\li	The current pointer in 'b' is advanced by 1.
 *
 * Ensures (for Write):
 *
 *\li	The used pointer in 'b' is advanced by 1.
 *
 * Returns (for Peek and Read):
 *
 *\li	A 8-bit unsigned integer. (peek and get)
 */

static inline isc_result_t
isc_buffer_peekuint16(const isc_buffer_t *restrict b, uint16_t *valp);
static inline uint16_t
isc_buffer_getuint16(isc_buffer_t *restrict b);
static inline void
isc_buffer_putuint16(isc_buffer_t *restrict b, const uint16_t val);
/*!<
 * \brief Peek/Read/Write an unsigned 16-bit integer in network byte order
 * from/to 'b', convert it to/from host byte order..
 *
 * Requires:
 *
 *\li	'b' is a valid buffer.
 *
 *\li	The length of the available region of 'b' is at least 2
 *	or the buffer has autoreallocation enabled.
 *
 * Ensures (for Read):
 *
 *\li	The current pointer in 'b' is advanced by 2.
 *
 * Ensures (for Write):
 *
 *\li	The used pointer in 'b' is advanced by 2.
 *
 * Returns (for Peek and Read):
 *
 *\li	A 16-bit unsigned integer.
 */

static inline isc_result_t
isc_buffer_peekuint32(const isc_buffer_t *restrict b, uint32_t *restrict valp);
static inline uint32_t
isc_buffer_getuint32(isc_buffer_t *restrict b);
static inline void
isc_buffer_putuint32(isc_buffer_t *restrict b, uint32_t const val);
/*!<
 * \brief Peek/Read/Write an unsigned 32-bit integer in network byte order
 * from/to 'b', convert it to/from host byte order.
 *
 * Requires:
 *
 *\li	'b' is a valid buffer.
 *
 *\li	The length of the available region of 'b' is at least 4
 *	or the buffer has autoreallocation enabled.
 *
 * Ensures (for Read):
 *
 *\li	The current pointer in 'b' is advanced by 4.
 *
 * Ensures (for Write):
 *
 *\li	The used pointer in 'b' is advanced by 4.
 *
 * Returns (for Peek and Read):
 *
 *\li	A 32-bit unsigned integer.
 */

static inline isc_result_t
isc_buffer_peekuint48(const isc_buffer_t *restrict b, uint64_t *valp);
static inline uint64_t
isc_buffer_getuint48(isc_buffer_t *restrict b);
static inline void
isc_buffer_putuint48(isc_buffer_t *restrict b, const uint64_t val);
/*!<
 * \brief Peek/Read/Write an unsigned 48-bit integer in network byte order
 * from/to 'b', convert it to/from host byte order.
 *
 * Requires:
 *
 *\li	'b' is a valid buffer.
 *
 *\li	The length of the available region of 'b' is at least 6
 *	or the buffer has autoreallocation enabled.
 *
 * Ensures (for Read):
 *
 *\li	The current pointer in 'b' is advanced by 6.
 *
 * Ensures (for Write):
 *
 *\li	The used pointer in 'b' is advanced by 6.
 *
 * Returns (for Peek and Read):
 *
 *\li	A 48-bit unsigned integer (stored in a 64-bit integer).
 */

static inline void
isc_buffer_putmem(isc_buffer_t *restrict b, const unsigned char *restrict base,
		  const unsigned int length);
/*!<
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

static inline isc_result_t
isc_buffer_copyregion(isc_buffer_t *restrict b, const isc_region_t *restrict r);
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

static inline isc_result_t
isc_buffer_dup(isc_mem_t *mctx, isc_buffer_t **restrict dstp,
	       const isc_buffer_t *restrict src);
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

static inline isc_result_t
isc_buffer_printf(isc_buffer_t *restrict b, const char *restrict format, ...)
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
isc_buffer_init(isc_buffer_t *restrict b, void *base,
		const unsigned int length) {
	REQUIRE(b != NULL);

	*b = (isc_buffer_t){
		.base = base,
		.length = length,
		.link = ISC_LINK_INITIALIZER,
		.magic = ISC_BUFFER_MAGIC,
	};
}

/*!
 *\brief Initialize a buffer 'b' with a null data field and zero length.
 * This can later be grown as needed and swapped in place.
 */
static inline void
isc_buffer_initnull(isc_buffer_t *restrict b) {
	*b = (isc_buffer_t){
		.link = ISC_LINK_INITIALIZER,
		.magic = ISC_BUFFER_MAGIC,
	};
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
isc_buffer_invalidate(isc_buffer_t *restrict b) {
	REQUIRE(ISC_BUFFER_VALID(b));
	REQUIRE(!ISC_LINK_LINKED(b, link));
	REQUIRE(b->mctx == NULL);

	*b = (isc_buffer_t){
		.magic = 0,
	};
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
isc_buffer_region(isc_buffer_t *restrict b, isc_region_t *restrict r) {
	REQUIRE(ISC_BUFFER_VALID(b));
	REQUIRE(r != NULL);

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
isc_buffer_usedregion(const isc_buffer_t *restrict b,
		      isc_region_t *restrict r) {
	REQUIRE(ISC_BUFFER_VALID(b));
	REQUIRE(r != NULL);

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
isc_buffer_availableregion(isc_buffer_t *restrict b, isc_region_t *restrict r) {
	REQUIRE(ISC_BUFFER_VALID(b));
	REQUIRE(r != NULL);

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
isc_buffer_add(isc_buffer_t *restrict b, const unsigned int n) {
	REQUIRE(ISC_BUFFER_VALID(b));
	REQUIRE(b->used + n <= b->length);

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
isc_buffer_subtract(isc_buffer_t *restrict b, const unsigned int n) {
	REQUIRE(ISC_BUFFER_VALID(b));
	REQUIRE(b->used >= n);

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
isc_buffer_clear(isc_buffer_t *restrict b) {
	REQUIRE(ISC_BUFFER_VALID(b));

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
isc_buffer_consumedregion(isc_buffer_t *restrict b, isc_region_t *restrict r) {
	REQUIRE(ISC_BUFFER_VALID(b));
	REQUIRE(r != NULL);

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
isc_buffer_remainingregion(isc_buffer_t *restrict b, isc_region_t *restrict r) {
	REQUIRE(ISC_BUFFER_VALID(b));
	REQUIRE(r != NULL);

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
isc_buffer_activeregion(isc_buffer_t *restrict b, isc_region_t *restrict r) {
	REQUIRE(ISC_BUFFER_VALID(b));
	REQUIRE(r != NULL);

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
isc_buffer_setactive(isc_buffer_t *restrict b, const unsigned int n) {
	REQUIRE(ISC_BUFFER_VALID(b));
	REQUIRE(b->current + n <= b->used);

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
isc_buffer_first(isc_buffer_t *restrict b) {
	REQUIRE(ISC_BUFFER_VALID(b));

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
isc_buffer_forward(isc_buffer_t *restrict b, const unsigned int n) {
	REQUIRE(ISC_BUFFER_VALID(b));
	REQUIRE(b->current + n <= b->used);

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
isc_buffer_back(isc_buffer_t *restrict b, const unsigned int n) {
	REQUIRE(ISC_BUFFER_VALID(b));
	REQUIRE(n <= b->current);

	b->current -= n;
}

#define ISC_BUFFER_PEEK_CHECK(b, s)                 \
	{                                           \
		REQUIRE(ISC_BUFFER_VALID(b));       \
		if ((b)->used - (b)->current < s) { \
			return (ISC_R_NOMORE);      \
		}                                   \
	}

static inline isc_result_t
isc_buffer_peekuint8(const isc_buffer_t *restrict b, uint8_t *valp) {
	ISC_BUFFER_PEEK_CHECK(b, sizeof(*valp));

	uint8_t *cp = isc_buffer_current(b);
	SET_IF_NOT_NULL(valp, (uint8_t)(cp[0]));
	return ISC_R_SUCCESS;
}

static inline uint8_t
isc_buffer_getuint8(isc_buffer_t *restrict b) {
	uint8_t	     val = 0;
	isc_result_t result = isc_buffer_peekuint8(b, &val);
	ENSURE(result == ISC_R_SUCCESS);
	b->current += sizeof(val);
	return val;
}

#define ISC_BUFFER_PUT_RESERVE(b, v, s)                                 \
	{                                                               \
		REQUIRE(ISC_BUFFER_VALID(b));                           \
                                                                        \
		if (b->mctx) {                                          \
			isc_result_t result = isc_buffer_reserve(b, s); \
			ENSURE(result == ISC_R_SUCCESS);                \
		}                                                       \
                                                                        \
		REQUIRE(isc_buffer_availablelength(b) >= s);            \
	}

static inline void
isc_buffer_putuint8(isc_buffer_t *restrict b, const uint8_t val) {
	ISC_BUFFER_PUT_RESERVE(b, val, sizeof(val));

	uint8_t *cp = isc_buffer_used(b);
	b->used += sizeof(val);
	cp[0] = val;
}

static inline isc_result_t
isc_buffer_peekuint16(const isc_buffer_t *restrict b, uint16_t *valp) {
	ISC_BUFFER_PEEK_CHECK(b, sizeof(*valp));

	uint8_t *cp = isc_buffer_current(b);

	SET_IF_NOT_NULL(valp, ISC_U8TO16_BE(cp));
	return ISC_R_SUCCESS;
}

static inline uint16_t
isc_buffer_getuint16(isc_buffer_t *restrict b) {
	uint16_t     val = 0;
	isc_result_t result = isc_buffer_peekuint16(b, &val);
	ENSURE(result == ISC_R_SUCCESS);
	b->current += sizeof(val);
	return val;
}

static inline void
isc_buffer_putuint16(isc_buffer_t *restrict b, const uint16_t val) {
	ISC_BUFFER_PUT_RESERVE(b, val, sizeof(val));

	uint8_t *cp = isc_buffer_used(b);
	b->used += sizeof(val);
	ISC_U16TO8_BE(cp, val);
}

static inline isc_result_t
isc_buffer_peekuint32(const isc_buffer_t *restrict b, uint32_t *valp) {
	ISC_BUFFER_PEEK_CHECK(b, sizeof(*valp));

	uint8_t *cp = isc_buffer_current(b);

	SET_IF_NOT_NULL(valp, ISC_U8TO32_BE(cp));
	return ISC_R_SUCCESS;
}

uint32_t
isc_buffer_getuint32(isc_buffer_t *restrict b) {
	uint32_t     val = 0;
	isc_result_t result = isc_buffer_peekuint32(b, &val);
	ENSURE(result == ISC_R_SUCCESS);
	b->current += sizeof(val);
	return val;
}

static inline void
isc_buffer_putuint32(isc_buffer_t *restrict b, const uint32_t val) {
	ISC_BUFFER_PUT_RESERVE(b, val, sizeof(val));

	uint8_t *cp = isc_buffer_used(b);
	b->used += sizeof(val);

	ISC_U32TO8_BE(cp, val);
}

static inline isc_result_t
isc_buffer_peekuint48(const isc_buffer_t *restrict b, uint64_t *valp) {
	ISC_BUFFER_PEEK_CHECK(b, 6); /* 48-bits */

	uint8_t *cp = isc_buffer_current(b);

	SET_IF_NOT_NULL(valp, ISC_U8TO48_BE(cp));
	return ISC_R_SUCCESS;
}

static inline uint64_t
isc_buffer_getuint48(isc_buffer_t *restrict b) {
	uint64_t     val = 0;
	isc_result_t result = isc_buffer_peekuint48(b, &val);
	ENSURE(result == ISC_R_SUCCESS);
	b->current += 6; /* 48-bits */
	return val;
}

static inline void
isc_buffer_putuint48(isc_buffer_t *restrict b, const uint64_t val) {
	ISC_BUFFER_PUT_RESERVE(b, val, 6); /* 48-bits */

	uint8_t *cp = isc_buffer_used(b);
	b->used += 6;

	ISC_U48TO8_BE(cp, val);
}

static inline void
isc_buffer_putmem(isc_buffer_t *restrict b, const unsigned char *restrict base,
		  const unsigned int length) {
	REQUIRE(ISC_BUFFER_VALID(b));

	if (b->mctx) {
		isc_result_t result = isc_buffer_reserve(b, length);
		REQUIRE(result == ISC_R_SUCCESS);
	}

	REQUIRE(isc_buffer_availablelength(b) >= (unsigned int)length);

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
isc_buffer_putstr(isc_buffer_t *restrict b, const char *restrict source) {
	unsigned int   length;
	unsigned char *cp;

	REQUIRE(ISC_BUFFER_VALID(b));
	REQUIRE(source != NULL);

	length = (unsigned int)strlen(source);
	if (b->mctx) {
		isc_result_t result = isc_buffer_reserve(b, length);
		ENSURE(result == ISC_R_SUCCESS);
	}

	REQUIRE(isc_buffer_availablelength(b) >= length);

	cp = isc_buffer_used(b);
	memmove(cp, source, length);
	b->used += length;
}

static inline void
isc_buffer_reinit(isc_buffer_t *restrict b, void *base,
		  const unsigned int length) {
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
	REQUIRE(b->mctx == NULL);

	if (b->length > 0U) {
		(void)memmove(base, b->base, b->length);
	}

	b->base = base;
	b->length = length;
}

static inline void
isc_buffer_trycompact(isc_buffer_t *restrict b) {
	if (isc_buffer_consumedlength(b) >= isc_buffer_remaininglength(b)) {
		isc_buffer_compact(b);
	}
}

static inline void
isc_buffer_compact(isc_buffer_t *restrict b) {
	unsigned int length;
	void	    *src;

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

static inline void
isc_buffer_allocate(isc_mem_t	      *mctx, isc_buffer_t **restrict dbufp,
		    const unsigned int length) {
	REQUIRE(dbufp != NULL && *dbufp == NULL);

	isc_buffer_t *dbuf = isc_mem_get(mctx, sizeof(*dbuf) + length);
	uint8_t	     *bdata = (uint8_t *)dbuf + sizeof(*dbuf);

	isc_buffer_init(dbuf, bdata, length);
	dbuf->extra = length;
	isc_buffer_setmctx(dbuf, mctx);

	*dbufp = dbuf;
}

static inline void
isc_buffer_setmctx(isc_buffer_t *restrict b, isc_mem_t *mctx) {
	REQUIRE(ISC_BUFFER_VALID(b));

	b->mctx = mctx;
}

static inline void
isc_buffer_clearmctx(isc_buffer_t *restrict b) {
	REQUIRE(ISC_BUFFER_VALID(b));

	if (b->dynamic) {
		isc_mem_put(b->mctx, b->base, b->length);
		b->dynamic = false;
	}

	b->mctx = NULL;
}

static inline isc_result_t
isc_buffer_reserve(isc_buffer_t *restrict dbuf, const unsigned int size) {
	REQUIRE(ISC_BUFFER_VALID(dbuf));

	size_t len;

	len = dbuf->length;
	if ((len - dbuf->used) >= size) {
		return ISC_R_SUCCESS;
	}

	if (dbuf->mctx == NULL) {
		return ISC_R_NOSPACE;
	}

	/* Round to nearest buffer size increment */
	len = size + dbuf->used;
	len = ISC_ALIGN(len, ISC_BUFFER_INCR);

	/* Cap at UINT_MAX */
	if (len > UINT_MAX) {
		len = UINT_MAX;
	}

	if ((len - dbuf->used) < size) {
		return ISC_R_NOMEMORY;
	}

	if (!dbuf->dynamic) {
		void *old_base = dbuf->base;
		dbuf->base = isc_mem_get(dbuf->mctx, len);
		if (old_base != NULL) {
			memmove(dbuf->base, old_base, dbuf->used);
		}
		dbuf->dynamic = true;
	} else {
		dbuf->base = isc_mem_creget(dbuf->mctx, dbuf->base,
					    dbuf->length, len, sizeof(char));
	}
	dbuf->length = (unsigned int)len;

	return ISC_R_SUCCESS;
}

static inline void
isc_buffer_free(isc_buffer_t **restrict dbufp) {
	REQUIRE(dbufp != NULL && ISC_BUFFER_VALID(*dbufp));
	REQUIRE((*dbufp)->mctx != NULL);

	isc_buffer_t *dbuf = *dbufp;
	isc_mem_t    *mctx = dbuf->mctx;
	unsigned int  extra = dbuf->extra;

	*dbufp = NULL; /* destroy external reference */

	isc_buffer_clearmctx(dbuf);

	isc_buffer_invalidate(dbuf);
	isc_mem_put(mctx, dbuf, sizeof(*dbuf) + extra);
}

static inline isc_result_t
isc_buffer_dup(isc_mem_t *mctx, isc_buffer_t **restrict dstp,
	       const isc_buffer_t *restrict src) {
	isc_buffer_t *dst = NULL;
	isc_region_t  region;
	isc_result_t  result;

	REQUIRE(dstp != NULL && *dstp == NULL);
	REQUIRE(ISC_BUFFER_VALID(src));

	isc_buffer_usedregion(src, &region);

	isc_buffer_allocate(mctx, &dst, region.length);

	result = isc_buffer_copyregion(dst, &region);
	RUNTIME_CHECK(result == ISC_R_SUCCESS); /* NOSPACE is impossible */
	*dstp = dst;
	return ISC_R_SUCCESS;
}

static inline isc_result_t
isc_buffer_copyregion(isc_buffer_t *restrict b,
		      const isc_region_t *restrict r) {
	isc_result_t result;

	REQUIRE(ISC_BUFFER_VALID(b));
	REQUIRE(r != NULL);

	if (b->mctx) {
		result = isc_buffer_reserve(b, r->length);
		if (result != ISC_R_SUCCESS) {
			return result;
		}
	}

	if (r->length > isc_buffer_availablelength(b)) {
		return ISC_R_NOSPACE;
	}

	if (r->length > 0U) {
		memmove(isc_buffer_used(b), r->base, r->length);
		b->used += r->length;
	}

	return ISC_R_SUCCESS;
}

static inline isc_result_t
isc_buffer_printf(isc_buffer_t *restrict b, const char *restrict format, ...) {
	va_list	     ap;
	int	     n;
	isc_result_t result;

	REQUIRE(ISC_BUFFER_VALID(b));

	va_start(ap, format);
	n = vsnprintf(NULL, 0, format, ap);
	va_end(ap);

	if (n < 0) {
		return ISC_R_FAILURE;
	}

	if (b->mctx) {
		result = isc_buffer_reserve(b, n + 1);
		if (result != ISC_R_SUCCESS) {
			return result;
		}
	}

	if (isc_buffer_availablelength(b) < (unsigned int)n + 1) {
		return ISC_R_NOSPACE;
	}

	va_start(ap, format);
	n = vsnprintf(isc_buffer_used(b), n + 1, format, ap);
	va_end(ap);

	if (n < 0) {
		return ISC_R_FAILURE;
	}

	b->used += n;

	return ISC_R_SUCCESS;
}

ISC_LANG_ENDDECLS
