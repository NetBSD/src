/*	$NetBSD: sdp_record.c,v 1.1 2009/05/12 10:05:06 plunky Exp $	*/

/*-
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Iain Hibbert.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: sdp_record.c,v 1.1 2009/05/12 10:05:06 plunky Exp $");

#include <errno.h>
#include <sdp.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "sdp-int.h"

/*
 * This is the interface to sdpd(8); These PDU IDs are NOT part
 * of the Bluetooth specification.
 */

bool
sdp_record_insert(struct sdp_session *ss, bdaddr_t *bdaddr,
    uint32_t *handle, const sdp_data_t *rec)
{
	struct iovec	req[4];
	sdp_data_t	hdr;
	uint8_t		data[5];
	ssize_t		len;
	bdaddr_t	ba;
	uint16_t	ec;

	/*
	 * setup BluetoothDeviceAddress
	 */
	bdaddr_copy(&ba, (bdaddr == NULL) ? BDADDR_ANY : bdaddr);
	req[1].iov_base = &ba;
	req[1].iov_len = sizeof(bdaddr_t);

	/*
	 * setup ServiceRecord
	 */
	len = rec->end - rec->next;
	if (len < 0 || len > UINT16_MAX) {
		errno = EINVAL;
		return false;
	}

	hdr.next = data;
	hdr.end = data + sizeof(data) + len;
	sdp_put_seq(&hdr, len);
	req[2].iov_base = data;
	req[2].iov_len = hdr.next - data;

	req[3].iov_base = rec->next;
	req[3].iov_len = len;

	/*
	 * InsertRecord Transaction
	 */
	if (!_sdp_send_pdu(ss, SDP_PDU_RECORD_INSERT_REQUEST,
	    req, __arraycount(req)))
		return false;

	len = _sdp_recv_pdu(ss, SDP_PDU_ERROR_RESPONSE);
	if (len == -1)
		return false;

	if (len != sizeof(uint16_t) + sizeof(uint32_t)) {
		errno = EIO;
		return false;
	}

	/*
	 * extract and check ErrorCode (success == 0)
	 */
	ec = be16dec(ss->ibuf);
	if (ec != 0) {
		errno = _sdp_errno(ec);
		return false;
	}

	/*
	 * extract ServiceRecordHandle if required
	 */
	if (handle != NULL)
		*handle = be32dec(ss->ibuf + sizeof(uint16_t));

	return true;
}

bool
sdp_record_update(struct sdp_session *ss, uint32_t handle,
    const sdp_data_t *rec)
{
	struct iovec	req[4];
	sdp_data_t	hdr;
	uint8_t		data[5];
	ssize_t		len;
	uint16_t	ec;

	/*
	 * setup ServiceRecordHandle
	 */
	handle = htobe32(handle);
	req[1].iov_base = &handle;
	req[1].iov_len = sizeof(handle);

	/*
	 * setup ServiceRecord
	 */
	len = rec->end - rec->next;
	if (len < 0 || len > UINT16_MAX) {
		errno = EINVAL;
		return false;
	}

	hdr.next = data;
	hdr.end = data + sizeof(data) + len;
	sdp_put_seq(&hdr, len);
	req[2].iov_base = data;
	req[2].iov_len = hdr.next - data;

	req[3].iov_base = rec->next;
	req[3].iov_len = len;

	/*
	 * UpdateRecord Transaction
	 */
	if (!_sdp_send_pdu(ss, SDP_PDU_RECORD_UPDATE_REQUEST,
	    req, __arraycount(req)))
		return false;

	len = _sdp_recv_pdu(ss, SDP_PDU_ERROR_RESPONSE);
	if (len == -1)
		return false;

	if (len != sizeof(uint16_t)) {
		errno = EIO;
		return false;
	}

	/*
	 * extract and check ErrorCode (success == 0)
	 */
	if ((ec = be16dec(ss->ibuf)) != 0) {
		errno = _sdp_errno(ec);
		return false;
	}

	return true;
}

bool
sdp_record_remove(struct sdp_session *ss, uint32_t handle)
{
	struct iovec	req[2];
	ssize_t		len;
	uint16_t	ec;

	/*
	 * setup ServiceRecordHandle
	 */
	handle = htobe32(handle);
	req[1].iov_base = &handle;
	req[1].iov_len = sizeof(handle);

	/*
	 * RemoveRecord Transaction
	 */
	if (!_sdp_send_pdu(ss, SDP_PDU_RECORD_REMOVE_REQUEST,
	    req, __arraycount(req)))
		return false;

	len = _sdp_recv_pdu(ss, SDP_PDU_ERROR_RESPONSE);
	if (len == -1)
		return false;

	if (len != sizeof(uint16_t)) {
		errno = EIO;
		return false;
	}

	/*
	 * extract and check ErrorCode (success == 0)
	 */
	ec = be16dec(ss->ibuf);
	if (ec != 0) {
		errno = _sdp_errno(ec);
		return false;
	}

	return true;
}
