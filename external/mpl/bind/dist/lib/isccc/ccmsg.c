/*	$NetBSD: ccmsg.c,v 1.8 2025/01/26 16:25:44 christos Exp $	*/

/*
 * Copyright (C) Internet Systems Consortium, Inc. ("ISC")
 *
 * SPDX-License-Identifier: MPL-2.0 AND ISC
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at https://mozilla.org/MPL/2.0/.
 *
 * See the COPYRIGHT file distributed with this work for additional
 * information regarding copyright ownership.
 */

/*
 * Copyright (C) 2001 Nominum, Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC AND NOMINUM DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*! \file */

#include <inttypes.h>

#include <isc/mem.h>
#include <isc/netmgr.h>
#include <isc/result.h>
#include <isc/string.h>
#include <isc/util.h>

#include <isccc/ccmsg.h>

#define CCMSG_MAGIC	 ISC_MAGIC('C', 'C', 'm', 's')
#define VALID_CCMSG(foo) ISC_MAGIC_VALID(foo, CCMSG_MAGIC)

/*
 * Try parsing a message from the internal read_buffer and set state
 * accordingly. Returns true if a message was successfully parsed, false if not.
 * If no message could be parsed the ccmsg struct remains untouched.
 */
static isc_result_t
try_parse_message(isccc_ccmsg_t *ccmsg) {
	REQUIRE(ccmsg != NULL);

	uint32_t len = 0;
	if (isc_buffer_peekuint32(ccmsg->buffer, &len) != ISC_R_SUCCESS) {
		return ISC_R_NOMORE;
	}
	if (len == 0) {
		return ISC_R_UNEXPECTEDEND;
	}
	if (len > ccmsg->maxsize) {
		return ISC_R_RANGE;
	}
	if (isc_buffer_remaininglength(ccmsg->buffer) < sizeof(uint32_t) + len)
	{
		return ISC_R_NOMORE;
	}
	/* Skip the size we just peeked */
	isc_buffer_forward(ccmsg->buffer, sizeof(uint32_t));
	ccmsg->size = len;
	return ISC_R_SUCCESS;
}

static void
recv_data(isc_nmhandle_t *handle, isc_result_t eresult, isc_region_t *region,
	  void *arg) {
	isccc_ccmsg_t *ccmsg = arg;

	REQUIRE(VALID_CCMSG(ccmsg));

	REQUIRE(handle == ccmsg->handle);
	if (eresult != ISC_R_SUCCESS) {
		goto done;
	}

	REQUIRE(region != NULL);

	/* Copy the received data to our reassembly buffer */
	eresult = isc_buffer_copyregion(ccmsg->buffer, region);
	if (eresult != ISC_R_SUCCESS) {
		goto done;
	}
	isc_region_consume(region, region->length);

	/* Try to parse a single message of the buffer */
	eresult = try_parse_message(ccmsg);
	/* No results from parsing, we need more data */
	if (eresult == ISC_R_NOMORE) {
		return;
	}

done:
	isc_nm_read_stop(handle);
	ccmsg->recv_cb(handle, eresult, ccmsg->recv_cbarg);

	return;
}

void
isccc_ccmsg_init(isc_mem_t *mctx, isc_nmhandle_t *handle,
		 isccc_ccmsg_t *ccmsg) {
	REQUIRE(mctx != NULL);
	REQUIRE(handle != NULL);
	REQUIRE(ccmsg != NULL);

	*ccmsg = (isccc_ccmsg_t){
		.magic = CCMSG_MAGIC,
		.maxsize = 0xffffffffU, /* Largest message possible. */
		.mctx = mctx,
	};

	/* Preallocate the buffer to maximum single TCP read */
	isc_buffer_allocate(ccmsg->mctx, &ccmsg->buffer,
			    UINT16_MAX + sizeof(uint16_t));

	isc_nmhandle_attach(handle, &ccmsg->handle);
}

void
isccc_ccmsg_setmaxsize(isccc_ccmsg_t *ccmsg, unsigned int maxsize) {
	REQUIRE(VALID_CCMSG(ccmsg));

	ccmsg->maxsize = maxsize;
}

void
isccc_ccmsg_readmessage(isccc_ccmsg_t *ccmsg, isc_nm_cb_t cb, void *cbarg) {
	REQUIRE(VALID_CCMSG(ccmsg));

	if (ccmsg->size != 0) {
		/* Remove the previously read message from the buffer */
		isc_buffer_forward(ccmsg->buffer, ccmsg->size);
		ccmsg->size = 0;
		isc_buffer_trycompact(ccmsg->buffer);
	}

	ccmsg->recv_cb = cb;
	ccmsg->recv_cbarg = cbarg;

	/* If we have previous data still in the buffer, try to parse it */
	isc_result_t result = try_parse_message(ccmsg);
	if (result == ISC_R_NOMORE) {
		/* We need to read more data */
		isc_nm_read(ccmsg->handle, recv_data, ccmsg);
		return;
	}

	ccmsg->recv_cb(ccmsg->handle, result, ccmsg->recv_cbarg);
}

static void
ccmsg_senddone(isc_nmhandle_t *handle, isc_result_t eresult, void *arg) {
	isccc_ccmsg_t *ccmsg = arg;

	REQUIRE(VALID_CCMSG(ccmsg));
	REQUIRE(ccmsg->send_cb != NULL);

	isc_nm_cb_t send_cb = ccmsg->send_cb;
	ccmsg->send_cb = NULL;

	send_cb(handle, eresult, ccmsg->send_cbarg);

	isc_nmhandle_detach(&handle);
}

void
isccc_ccmsg_sendmessage(isccc_ccmsg_t *ccmsg, isc_region_t *region,
			isc_nm_cb_t cb, void *cbarg) {
	REQUIRE(VALID_CCMSG(ccmsg));
	REQUIRE(ccmsg->send_cb == NULL);

	ccmsg->send_cb = cb;
	ccmsg->send_cbarg = cbarg;

	isc_nmhandle_ref(ccmsg->handle);
	isc_nm_send(ccmsg->handle, region, ccmsg_senddone, ccmsg);
}

void
isccc_ccmsg_disconnect(isccc_ccmsg_t *ccmsg) {
	REQUIRE(VALID_CCMSG(ccmsg));

	if (ccmsg->handle != NULL) {
		isc_nm_read_stop(ccmsg->handle);
		isc_nmhandle_close(ccmsg->handle);
		isc_nmhandle_detach(&ccmsg->handle);
	}
}

void
isccc_ccmsg_invalidate(isccc_ccmsg_t *ccmsg) {
	REQUIRE(VALID_CCMSG(ccmsg));
	REQUIRE(ccmsg->handle == NULL);

	ccmsg->magic = 0;

	isc_buffer_free(&ccmsg->buffer);
}

void
isccc_ccmsg_toregion(isccc_ccmsg_t *ccmsg, isccc_region_t *ccregion) {
	REQUIRE(VALID_CCMSG(ccmsg));
	REQUIRE(ccmsg->buffer);
	REQUIRE(isc_buffer_remaininglength(ccmsg->buffer) >= ccmsg->size);

	ccregion->rstart = isc_buffer_current(ccmsg->buffer);
	ccregion->rend = ccregion->rstart + ccmsg->size;
}
