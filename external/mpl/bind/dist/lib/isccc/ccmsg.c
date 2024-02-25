/*	$NetBSD: ccmsg.c,v 1.6.2.1 2024/02/25 15:47:30 martin Exp $	*/

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
#include <isccc/events.h>

#define CCMSG_MAGIC	 ISC_MAGIC('C', 'C', 'm', 's')
#define VALID_CCMSG(foo) ISC_MAGIC_VALID(foo, CCMSG_MAGIC)

static void
recv_data(isc_nmhandle_t *handle, isc_result_t eresult, isc_region_t *region,
	  void *arg) {
	isccc_ccmsg_t *ccmsg = arg;
	size_t size;

	INSIST(VALID_CCMSG(ccmsg));

	switch (eresult) {
	case ISC_R_SHUTTINGDOWN:
	case ISC_R_CANCELED:
	case ISC_R_EOF:
		ccmsg->result = eresult;
		goto done;
	case ISC_R_SUCCESS:
		if (region == NULL) {
			ccmsg->result = ISC_R_EOF;
			goto done;
		}
		ccmsg->result = ISC_R_SUCCESS;
		break;
	default:
		ccmsg->result = eresult;
		goto done;
	}

	if (!ccmsg->length_received) {
		if (region->length < sizeof(uint32_t)) {
			ccmsg->result = ISC_R_UNEXPECTEDEND;
			goto done;
		}

		ccmsg->size = ntohl(*(uint32_t *)region->base);

		if (ccmsg->size == 0) {
			ccmsg->result = ISC_R_UNEXPECTEDEND;
			goto done;
		}
		if (ccmsg->size > ccmsg->maxsize) {
			ccmsg->result = ISC_R_RANGE;
			goto done;
		}

		isc_region_consume(region, sizeof(uint32_t));
		isc_buffer_allocate(ccmsg->mctx, &ccmsg->buffer, ccmsg->size);

		ccmsg->length_received = true;
	}

	/*
	 * If there's no more data, wait for more
	 */
	if (region->length == 0) {
		return;
	}

	/* We have some data in the buffer, read it */

	size = ISC_MIN(isc_buffer_availablelength(ccmsg->buffer),
		       region->length);
	isc_buffer_putmem(ccmsg->buffer, region->base, size);
	isc_region_consume(region, size);

	if (isc_buffer_usedlength(ccmsg->buffer) == ccmsg->size) {
		ccmsg->result = ISC_R_SUCCESS;
		goto done;
	}

	/* Wait for more data to come */
	return;

done:
	isc_nm_pauseread(handle);
	ccmsg->cb(handle, ccmsg->result, ccmsg->cbarg);
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
		.handle = handle,
		.result = ISC_R_UNEXPECTED /* None yet. */
	};
}

void
isccc_ccmsg_setmaxsize(isccc_ccmsg_t *ccmsg, unsigned int maxsize) {
	REQUIRE(VALID_CCMSG(ccmsg));

	ccmsg->maxsize = maxsize;
}

void
isccc_ccmsg_readmessage(isccc_ccmsg_t *ccmsg, isc_nm_cb_t cb, void *cbarg) {
	REQUIRE(VALID_CCMSG(ccmsg));

	if (ccmsg->buffer != NULL) {
		isc_buffer_free(&ccmsg->buffer);
	}

	ccmsg->cb = cb;
	ccmsg->cbarg = cbarg;
	ccmsg->result = ISC_R_UNEXPECTED; /* unknown right now */
	ccmsg->length_received = false;

	if (ccmsg->reading) {
		isc_nm_resumeread(ccmsg->handle);
	} else {
		isc_nm_read(ccmsg->handle, recv_data, ccmsg);
		ccmsg->reading = true;
	}
}

void
isccc_ccmsg_cancelread(isccc_ccmsg_t *ccmsg) {
	REQUIRE(VALID_CCMSG(ccmsg));

	if (ccmsg->reading) {
		isc_nm_cancelread(ccmsg->handle);
		ccmsg->reading = false;
	}
}

void
isccc_ccmsg_invalidate(isccc_ccmsg_t *ccmsg) {
	REQUIRE(VALID_CCMSG(ccmsg));

	ccmsg->magic = 0;

	if (ccmsg->buffer != NULL) {
		isc_buffer_free(&ccmsg->buffer);
	}
}
