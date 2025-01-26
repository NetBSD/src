/*	$NetBSD: dnsstream.h,v 1.1.1.1 2025/01/26 16:12:31 christos Exp $	*/

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

#include <isc/buffer.h>
#include <isc/mem.h>

typedef struct isc_dnsstream_assembler isc_dnsstream_assembler_t;
/*!<
 * \brief The 'isc_dnsstream_assembler_t' object is built on top of
 * 'isc_buffer_t' and intended to encapsulate the state machine
 * used for handling DNS messages received in the format used for
 * messages transmitted over TCP.
 *
 * The idea is that the object accepts the input data received from a
 * socket (or anywhere else, for that matter), tries to assemble DNS
 * messages from the incoming data and calls the callback passing it
 * the status of the incoming data as well as a pointer to the memory
 * region referencing the data of the assembled message (in the case
 * there is enough data to assemble the message). It is capable of
 * assembling DNS messages no matter how "torn apart" they are when
 * sent over network.
 *
 * The implementation is completely decoupled from the networking code
 * itself makes it trivial to write unit tests for it, leading to
 * better verification of its correctness.  Another important aspect
 * of its functioning is directly related to the fact that it is built
 * on top of 'isc_buffer_t', which tries to manage memory in a
 * smart way. In particular:
 *
 *\li	It tries to use a static buffer for smaller messages, reducing
 *      pressure on the memory manager (hot path);
 *
 *\li	When allocating dynamic memory for larger messages, it tries to
 *      allocate memory conservatively (generic path).
 *
 * That is, when using 'isc_dnsstream_assembler_t', we allocate memory
 * conservatively, avoiding any allocations whatsoever for small DNS
 * messages (whose size is lesser of equal to 512 bytes). The last
 * characteristic is important in the context of DNS, as most of DNS
 * messages are small.
 */

typedef bool (*isc_dnsstream_assembler_cb_t)(isc_dnsstream_assembler_t *dnsasm,
					     const isc_result_t		result,
					     isc_region_t *restrict region,
					     void *cbarg, void *userarg);
/*!<
 * /brief The type of callback called when processing the data passed to a
 * 'isc_dnsstream_assembler_t' type.
 *
 * The callback accepts the following arguments:
 *
 *\li	'isc_dnsstream_assembler_t *dnsasm' - a pointer to the
 *		'isc_dnsstream_assembler_t' object in use;
 *\li	'isc_result_t result' - processing status;
 *\li	'isc_region_t *region' - the region referencing the DNS message if
 *		assembled, empty otherwise;
 *\li	'void *cbarg' - the callback argument, set during the object
 *		initialisation or when setting the callback;
 *\li	'void *userarg' - the callback argument passed to it when processing the
 *      current chunk of data;
 *
 * Return values:
 *
 *\li	'true' - continue processing data, if there is any non-processed data
 *		left;
 *\li	'false' - stop processing data regardless of non-processed data
 *		availability.
 *
 * Processing status values:
 *
 *\li	'ISC_R_SUCCESS' - a message has been successfully assembled;
 *\li	'ISC_R_NOMORE'  - not enough data to assemble a DNS message, need to get
more;
 *\li	'ISC_R_RANGE' - there was an attempt to process a zero-sized DNS
message (i.e. someone attempts to send us junk data).
 */

#define ISC_DNSSTREAM_STATIC_BUFFER_SIZE (512)

struct isc_dnsstream_assembler {
	isc_buffer_t dnsbuf; /*!< Internal buffer for assembling DNS
				   messages. */
	uint8_t	      buf[ISC_DNSSTREAM_STATIC_BUFFER_SIZE];
	isc_buffer_t *current; /*!< Pointer to the currently used data buffer.
				  Most of the time it point to the 'dnsbuf'
				  except when dealing with data in place (when
				  it points to a temporary buffer) */
	isc_dnsstream_assembler_cb_t onmsg_cb; /*!< Data processing callback. */
	void			    *cbarg;    /*!< Callback argument. */
	bool calling_cb; /*<! Callback calling marker. Used to detect recursive
			    object uses (changing the data state from withing
			    the callback). */
	isc_result_t result; /*<! The last passed to the callback processing
				status value. */
	isc_mem_t *mctx;
};

static inline void
isc_dnsstream_assembler_init(isc_dnsstream_assembler_t *restrict dnsasm,
			     isc_mem_t *memctx, isc_dnsstream_assembler_cb_t cb,
			     void *cbarg);
/*!<
 * \brief Initialise the given 'isc_dnsstream_assembler_t' object, attach
 * to the memory context.
 *
 * Requires:
 *\li	'dnsasm' is not NULL;
 *\li	'memctx' is not NULL;
 *\li	'cb' is not NULL.
 */

static inline void
isc_dnsstream_assembler_uninit(isc_dnsstream_assembler_t *restrict dnsasm);
/*!<
 * \brief Un-initialise the given 'isc_dnsstream_assembler_t' object, detach
 * to the attached memory context. Destroys any internal unprocessed data.
 *
 * Requires:
 *\li	'dnsasm' is not NULL.
 */

static inline isc_dnsstream_assembler_t *
isc_dnsstream_assembler_new(isc_mem_t *memctx, isc_dnsstream_assembler_cb_t cb,
			    void *cbarg);
/*!<
 * \brief Allocate and initialise a new 'isc_dnsstream_assembler_t' object,
 * attach to the memory context.
 *
 * Requires:
 *\li	'dnsasm' is not NULL;
 *\li	'memctx' is not NULL;
 *\li	'cb' is not NULL.
 */

static inline void
isc_dnsstream_assembler_free(isc_dnsstream_assembler_t **restrict dnsasm);
/*!<
 * \brief Un-initialise the given 'isc_dnsstream_assembler_t' object, detach
 * to the attached memory context, free the memory consumed by the object.
 *
 * Requires:
 *\li	'dnsasm' is not NULL;
 *\li	'dnsasm' is not pointing to NULL.
 */

static inline void
isc_dnsstream_assembler_setcb(isc_dnsstream_assembler_t *restrict dnsasm,
			      isc_dnsstream_assembler_cb_t cb, void *cbarg);
/*!<
 * \brief Change the data processing callback and its argument within the given
 * 'isc_dnsstream_assembler_t' object.
 *
 * Requires:
 *\li	'dnsasm' is not NULL;
 *\li	'cb' is not NULL.
 */

static inline void
isc_dnsstream_assembler_incoming(isc_dnsstream_assembler_t *restrict dnsasm,
				 void		   *userarg, void *restrict buf,
				 const unsigned int buf_size);
/*!<
 * \brief Process the new incoming data to the given
 * 'isc_dnsstream_assembler_t' or continue processing the currently
 * unprocessed data (when 'buf' equals NULL and 'buf_size' equals
 * 0). Call the callback passing a status of data to it.
 *
 * To avoid erroneously recursive usage of the object, it is forbidden to call
 * this function from within the callback. Doing so will abort the program.
 *
 * Requires:
 *\li	'dnsasm' is not NULL.
 */

static inline isc_result_t
isc_dnsstream_assembler_result(const isc_dnsstream_assembler_t *restrict dnsasm);
/*!<
 * \brief Return the last data processing status passed to the
 * callback.
 *
 * Requires:
 *\li	'dnsasm' is not NULL.
 *
 * Return values:
 *\li	'ISC_R_SUCCESS' - a message has been successfully assembled;
 *\li	'ISC_R_NOMORE'  - not enough data to assemble a DNS message, need to get
more;
 *\li	'ISC_R_RANGE' - there was an attempt to process a zero-sized DNS;
 *\li	'ISC_R_UNSET' - not data has been passed to the object.
 */

static inline size_t
isc_dnsstream_assembler_remaininglength(
	const isc_dnsstream_assembler_t *restrict dnsasm);
/*!<
 * \brief Return the amount of currently unprocessed data within the given
 * 'isc_dnsstream_assembler_t' object
 *
 * Requires:
 *\li	'dnsasm' is not NULL.
 */

static inline void
isc_dnsstream_assembler_clear(isc_dnsstream_assembler_t *restrict dnsasm);
/*!<
 * \brief Clear the given 'isc_dnsstream_assembler_t' object from
 * any unprocessed data, clear the last data processing status (set it to
 * 'ISC_R_UNSET').
 *
 * Requires:
 *\li	'dnsasm' is not NULL.
 */

static inline void
isc_dnsstream_assembler_init(isc_dnsstream_assembler_t *restrict dnsasm,
			     isc_mem_t *memctx, isc_dnsstream_assembler_cb_t cb,
			     void *cbarg) {
	REQUIRE(dnsasm != NULL);
	REQUIRE(memctx != NULL);
	REQUIRE(cb != NULL);

	*dnsasm = (isc_dnsstream_assembler_t){ .result = ISC_R_UNSET };
	isc_dnsstream_assembler_setcb(dnsasm, cb, cbarg);
	isc_mem_attach(memctx, &dnsasm->mctx);

	isc_buffer_init(&dnsasm->dnsbuf, dnsasm->buf, sizeof(dnsasm->buf));
	isc_buffer_setmctx(&dnsasm->dnsbuf, dnsasm->mctx);

	dnsasm->current = &dnsasm->dnsbuf;
}

static inline void
isc_dnsstream_assembler_uninit(isc_dnsstream_assembler_t *restrict dnsasm) {
	REQUIRE(dnsasm != NULL);
	/*
	 * Uninitialising the object from withing the callback does not
	 * make any sense.
	 */
	INSIST(dnsasm->calling_cb == false);
	isc_buffer_clearmctx(&dnsasm->dnsbuf);
	isc_buffer_invalidate(&dnsasm->dnsbuf);
	if (dnsasm->mctx != NULL) {
		isc_mem_detach(&dnsasm->mctx);
	}
	dnsasm->current = NULL;
}

static inline isc_dnsstream_assembler_t *
isc_dnsstream_assembler_new(isc_mem_t *memctx, isc_dnsstream_assembler_cb_t cb,
			    void *cbarg) {
	isc_dnsstream_assembler_t *newasm;

	REQUIRE(memctx != NULL);
	REQUIRE(cb != NULL);

	newasm = isc_mem_get(memctx, sizeof(*newasm));
	isc_dnsstream_assembler_init(newasm, memctx, cb, cbarg);

	return newasm;
}

static inline void
isc_dnsstream_assembler_free(isc_dnsstream_assembler_t **restrict dnsasm) {
	isc_dnsstream_assembler_t *restrict oldasm = NULL;
	isc_mem_t *memctx = NULL;
	REQUIRE(dnsasm != NULL && *dnsasm != NULL);

	oldasm = *dnsasm;

	isc_mem_attach(oldasm->mctx, &memctx);
	isc_dnsstream_assembler_uninit(oldasm);
	isc_mem_putanddetach(&memctx, oldasm, sizeof(*oldasm));

	*dnsasm = NULL;
}

static inline void
isc_dnsstream_assembler_setcb(isc_dnsstream_assembler_t *restrict dnsasm,
			      isc_dnsstream_assembler_cb_t cb, void *cbarg) {
	REQUIRE(dnsasm != NULL);
	REQUIRE(cb != NULL);
	dnsasm->onmsg_cb = cb;
	dnsasm->cbarg = cbarg;
}

static inline bool
isc__dnsstream_assembler_callcb(isc_dnsstream_assembler_t *restrict dnsasm,
				const isc_result_t result,
				isc_region_t *restrict region, void *userarg) {
	bool ret;

	dnsasm->result = result;
	dnsasm->calling_cb = true;
	ret = dnsasm->onmsg_cb(dnsasm, result, region, dnsasm->cbarg, userarg);
	dnsasm->calling_cb = false;

	return ret;
}

static inline bool
isc__dnsstream_assembler_handle_message(
	isc_dnsstream_assembler_t *restrict dnsasm, void *userarg) {
	bool	     cont = false;
	isc_region_t region = { 0 };
	uint16_t     dnslen = 0;
	isc_result_t result;

	INSIST(dnsasm->calling_cb == false);

	result = isc_buffer_peekuint16(dnsasm->current, &dnslen);

	switch (result) {
	case ISC_R_SUCCESS:
		if (dnslen == 0) {
			/*
			 * Someone seems to send us binary junk or output from
			 * /dev/zero
			 */
			result = ISC_R_RANGE;
			isc_dnsstream_assembler_clear(dnsasm);
			break;
		}

		if (dnslen > (isc_buffer_remaininglength(dnsasm->current) -
			      sizeof(uint16_t)))
		{
			result = ISC_R_NOMORE;
			break;
		}
		break;
	case ISC_R_NOMORE:
		break;
	default:
		UNREACHABLE();
	}

	if (result == ISC_R_SUCCESS) {
		(void)isc_buffer_getuint16(dnsasm->current);
		isc_buffer_remainingregion(dnsasm->current, &region);
		region.length = dnslen;
		cont = isc__dnsstream_assembler_callcb(dnsasm, result, &region,
						       userarg);
		if (isc_buffer_remaininglength(dnsasm->current) >= dnslen) {
			isc_buffer_forward(dnsasm->current, dnslen);
		}
	} else {
		cont = false;
		(void)isc__dnsstream_assembler_callcb(dnsasm, result, NULL,
						      userarg);
	}

	return cont;
}

static inline void
isc__dnsstream_assembler_processing(isc_dnsstream_assembler_t *restrict dnsasm,
				    void *userarg) {
	while (isc__dnsstream_assembler_handle_message(dnsasm, userarg)) {
		if (isc_buffer_remaininglength(dnsasm->current) == 0) {
			break;
		}
	}
}

static inline void
isc__dnsstream_assembler_incoming_direct(
	isc_dnsstream_assembler_t *restrict dnsasm, void *userarg,
	void *restrict buf, const unsigned int		  buf_size) {
	isc_buffer_t data = { 0 };
	isc_region_t remaining = { 0 };
	INSIST(dnsasm->current == &dnsasm->dnsbuf);

	isc_buffer_init(&data, buf, buf_size);
	isc_buffer_add(&data, buf_size);

	/*
	 * Replace the internal buffer within the assembler
	 * object with a temporary buffer referring to the
	 * passed data directly.
	 */
	dnsasm->current = &data;

	/* process the data internally */
	isc__dnsstream_assembler_processing(dnsasm, userarg);

	/* set the internal buffer back */
	dnsasm->current = &dnsasm->dnsbuf;

	isc_buffer_remainingregion(&data, &remaining);
	if (remaining.length != 0) {
		/*
		 * Some unprocessed data left - let's put it
		 * into the internal buffer for processing
		 * later.
		 */
		isc_buffer_putmem(dnsasm->current, remaining.base,
				  remaining.length);
	}
}

static inline bool
isc__dnsstream_assembler_incoming_direct_non_empty(
	isc_dnsstream_assembler_t *restrict dnsasm, void *userarg,
	void *restrict buf, unsigned int		  buf_size) {
	size_t	 remaining;
	uint16_t dnslen = 0;
	size_t	 until_complete = 0;
	size_t	 remaining_no_len;

	if (isc_buffer_peekuint16(dnsasm->current, &dnslen) != ISC_R_SUCCESS) {
		return false;
	}

	remaining = isc_buffer_remaininglength(dnsasm->current);
	remaining_no_len = remaining - sizeof(uint16_t);

	/*
	 * We have data for more than one DNS message - that means that on
	 * previous iteration we stopped prematurely intentionally.
	 */
	if (remaining_no_len >= dnslen) {
		return false;
	}

	/*
	 * At this point we know that we have incomplete message in the
	 * internal buffer, let's find how much data do we need to
	 * complete the message and then check if we have enough data to
	 * handle it.
	 */
	until_complete = dnslen - remaining_no_len;

	if (buf_size >= until_complete) {
		bool	 cont;
		uint8_t *unprocessed_buf = NULL;
		size_t	 unprocessed_size;

		isc_buffer_putmem(dnsasm->current, buf, until_complete);
		unprocessed_buf = ((uint8_t *)buf + until_complete);
		unprocessed_size = buf_size - until_complete;

		/* handle the message */
		cont = isc__dnsstream_assembler_handle_message(dnsasm, userarg);
		isc_buffer_trycompact(dnsasm->current);

		INSIST(isc_buffer_remaininglength(dnsasm->current) == 0);
		if (unprocessed_size == 0) {
			return true;
		}

		if (cont) {
			/*
			 * The callback logic told us to continue processing
			 * messages, let's try to process the rest directly.
			 */
			isc__dnsstream_assembler_incoming_direct(
				dnsasm, userarg, unprocessed_buf,
				unprocessed_size);
		} else {
			/*
			 * The callback logic told us to stop, let's copy the
			 * remaining data into the internal buffer to process it
			 * later.
			 */
			isc_buffer_putmem(dnsasm->current, unprocessed_buf,
					  unprocessed_size);
		}

		return true;
	}

	return false;
}

static inline void
isc_dnsstream_assembler_incoming(isc_dnsstream_assembler_t *restrict dnsasm,
				 void		   *userarg, void *restrict buf,
				 const unsigned int buf_size) {
	REQUIRE(dnsasm != NULL);
	INSIST(!dnsasm->calling_cb);

	if (buf != NULL && buf_size > 0) {
		size_t remaining;

		remaining = isc_buffer_remaininglength(&dnsasm->dnsbuf);

		if (remaining == 0) {
			/*
			 * We can try to handle messages in-place (without
			 * memory copying/re-allocation) in the case we have no
			 * other data in the internal buffer and have received
			 * one or more complete messages at once. This way we
			 * can avoid copying memory into the assembler's
			 * internal buffer.
			 */
			isc__dnsstream_assembler_incoming_direct(
				dnsasm, userarg, buf, buf_size);
			return;
		} else if (isc__dnsstream_assembler_incoming_direct_non_empty(
				   dnsasm, userarg, buf, buf_size))
		{
			/*
			 * We had incomplete message in the buffer, but received
			 * enough data to handle it. After that we handle the
.			 * rest (if any) of the messages directly without
			 * copying into the internal buffer. Any data, belonging
			 * to incomplete messages at the end of the buffer, was
			 * copied into the internal buffer to be processed later
			 * when receiving the next batch of data.
			 */
			return;
		} else if (remaining == 1) {
			/* Mostly the same case as above, but we have incomplete
			 * message length in the buffer and received at least
			 * one byte to complete it.
			 */
			void  *unprocessed_buf = NULL;
			size_t unprocessed_size;

			isc_buffer_putmem(dnsasm->current, buf, 1);
			unprocessed_buf = (uint8_t *)buf + 1;
			unprocessed_size = buf_size - 1;

			if (isc__dnsstream_assembler_incoming_direct_non_empty(
				    dnsasm, userarg, unprocessed_buf,
				    unprocessed_size))
			{
				return;
			}

			if (unprocessed_size > 0) {
				isc_buffer_putmem(dnsasm->current,
						  unprocessed_buf,
						  unprocessed_size);
			}
			/* let's continue processing via the generic path */
		} else {
			/*
			 * Put the data into the internal buffer for
			 * processing.
			 */
			isc_buffer_putmem(dnsasm->current, buf, buf_size);
		}
	}

	isc__dnsstream_assembler_processing(dnsasm, userarg);

	isc_buffer_trycompact(dnsasm->current);
}

static inline isc_result_t
isc_dnsstream_assembler_result(
	const isc_dnsstream_assembler_t *restrict dnsasm) {
	REQUIRE(dnsasm != NULL);

	return dnsasm->result;
}

static inline size_t
isc_dnsstream_assembler_remaininglength(
	const isc_dnsstream_assembler_t *restrict dnsasm) {
	REQUIRE(dnsasm != NULL);

	return isc_buffer_remaininglength(dnsasm->current);
}

static inline void
isc_dnsstream_assembler_clear(isc_dnsstream_assembler_t *restrict dnsasm) {
	REQUIRE(dnsasm != NULL);

	isc_buffer_clear(dnsasm->current);
	if (dnsasm->current != &dnsasm->dnsbuf) {
		isc_buffer_clear(&dnsasm->dnsbuf);
	}
	dnsasm->result = ISC_R_UNSET;
}
