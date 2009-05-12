/*	$NetBSD: record.c,v 1.1 2009/05/12 10:05:07 plunky Exp $	*/

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
__RCSID("$NetBSD: record.c,v 1.1 2009/05/12 10:05:07 plunky Exp $");

#include <bluetooth.h>
#include <sdp.h>
#include <string.h>

#include "sdpd.h"

static bool sdpd_valid_record(sdp_data_t *);

/*
 * These record manipulation requests are not part of the SDP
 * specification, they are a private extension and valid only
 * for privileged clients on the control socket.
 */

uint16_t
record_insert_request(server_t *srv, int fd)
{
	sdp_data_t	seq;
	bdaddr_t	bdaddr;

	seq.next = srv->ibuf;
	seq.end = srv->ibuf + srv->pdu.len;

	if (!srv->fdidx[fd].control
	    || !srv->fdidx[fd].priv)
		return SDP_ERROR_CODE_INVALID_REQUEST_SYNTAX;

	srv->fdidx[fd].offset = 0;
	db_unselect(srv, fd);

	/*
	 * extract BluetoothDeviceAddress
	 */
	if (seq.next + sizeof(bdaddr_t) > seq.end)
		return SDP_ERROR_CODE_INVALID_REQUEST_SYNTAX;

	memcpy(&bdaddr, seq.next, sizeof(bdaddr_t));
	seq.next += sizeof(bdaddr_t);

	/*
	 * extract ServiceRecord and add to database
	 */
	if (!sdp_get_seq(&seq, &seq)
	    || !sdpd_valid_record(&seq))
		return SDP_ERROR_CODE_INVALID_REQUEST_SYNTAX;

	/* (ignores any additional data) */

	if (!db_create(srv, fd, &bdaddr, srv->handle, &seq))
		return SDP_ERROR_CODE_INSUFFICIENT_RESOURCES;

	/*
	 * encode 'success' ErrorCode and ServiceRecordHandle and
	 * bump server handle
	 */
	be16enc(srv->obuf, 0x0000);
	be32enc(srv->obuf + sizeof(uint16_t), srv->handle++);

	/*
	 * fill in PDU header and we are done
	 */
	srv->pdu.pid = SDP_PDU_ERROR_RESPONSE;
	srv->pdu.len = sizeof(uint16_t) + sizeof(uint32_t);
	return 0;
}

uint16_t
record_update_request(server_t *srv, int fd)
{
	record_t	*rec;
	sdp_data_t	seq;

	seq.next = srv->ibuf;
	seq.end = srv->ibuf + srv->pdu.len;

	if (!srv->fdidx[fd].control
	    || !srv->fdidx[fd].priv)
		return SDP_ERROR_CODE_INVALID_REQUEST_SYNTAX;

	srv->fdidx[fd].offset = 0;
	db_unselect(srv, fd);

	/*
	 * extract ServiceRecordHandle and select record
	 */
	if (seq.next + sizeof(uint32_t) > seq.end)
		return SDP_ERROR_CODE_INVALID_REQUEST_SYNTAX;

	db_select_handle(srv, fd, be32dec(seq.next));
	seq.next += sizeof(uint32_t);

	rec = NULL;
	db_next(srv, fd, &rec);
	if (rec == NULL || rec->fd != fd)
		return SDP_ERROR_CODE_INVALID_SERVICE_RECORD_HANDLE;

	db_unselect(srv, fd);

	/*
	 * extract ServiceRecord and add to database
	 */
	if (!sdp_get_seq(&seq, &seq)
	    || !sdpd_valid_record(&seq))
		return SDP_ERROR_CODE_INVALID_REQUEST_SYNTAX;

	/* (ignores any additional data) */

	if (!db_create(srv, fd, &rec->bdaddr, rec->handle, &seq))
		return SDP_ERROR_CODE_INSUFFICIENT_RESOURCES;

	/*
	 * encode 'success' ErrorCode
	 */
	be16enc(srv->obuf, 0x0000);

	/*
	 * fill in PDU header and we are done
	 */
	srv->pdu.pid = SDP_PDU_ERROR_RESPONSE;
	srv->pdu.len = sizeof(uint16_t);
	return 0;
}

uint16_t
record_remove_request(server_t *srv, int fd)
{
	record_t	*rec;

	if (!srv->fdidx[fd].control
	    || !srv->fdidx[fd].priv)
		return SDP_ERROR_CODE_INVALID_REQUEST_SYNTAX;

	srv->fdidx[fd].offset = 0;
	db_unselect(srv, fd);

	/*
	 * extract ServiceRecordHandle
	 */
	if (srv->pdu.len != sizeof(uint32_t))
		return SDP_ERROR_CODE_INVALID_REQUEST_SYNTAX;

	db_select_handle(srv, fd, be32dec(srv->ibuf));

	rec = NULL;
	db_next(srv, fd, &rec);
	if (rec == NULL || rec->fd != fd)
		return SDP_ERROR_CODE_INVALID_SERVICE_RECORD_HANDLE;

	/*
	 * expire the record
	 */
	rec->refcnt--;
	rec->valid = false;
	rec->fd = -1;
	db_unselect(srv, fd);

	/*
	 * encode 'success' ErrorCode
	 */
	be16enc(srv->obuf, 0x0000);

	/*
	 * fill in PDU header and we are done
	 */
	srv->pdu.pid = SDP_PDU_ERROR_RESPONSE;
	srv->pdu.len = sizeof(uint16_t);
	return 0;
}

/*
 * validate ServiceRecord data element list
 *
 * The record must contain a list of attribute/value pairs where the
 * attributes are unsigned 16-bit integer values in ascending order.
 */
static bool
sdpd_valid_record(sdp_data_t *data)
{
	sdp_data_t	d, s;
	uintmax_t	a0, a;

	s = *data;
	if (!sdp_data_valid(&s))
		return false;

	/* The first attribute must be ServiceRecordHandle */
	if (!sdp_get_data(&s, &d)
	    || sdp_data_type(&d) != SDP_DATA_UINT16
	    || !sdp_get_uint(&d, &a0)
	    || a0 != SDP_ATTR_SERVICE_RECORD_HANDLE
	    || !sdp_get_data(&s, &d)
	    || sdp_data_type(&d) != SDP_DATA_UINT32)
		return false;

	/* and remaining attribute IDs must be in ascending order */
	while (sdp_get_data(&s, &d)
	    && sdp_data_type(&d) == SDP_DATA_UINT16
	    && sdp_get_uint(&d, &a)
	    && a > a0
	    && sdp_get_data(&s, &d))
		a0 = a;

	if (s.next != s.end)
		return false;

	return true;
}
