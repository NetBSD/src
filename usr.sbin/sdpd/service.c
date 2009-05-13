/*	$NetBSD: service.c,v 1.1.2.2 2009/05/13 19:20:39 jym Exp $	*/

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
__RCSID("$NetBSD: service.c,v 1.1.2.2 2009/05/13 19:20:39 jym Exp $");

#include <bluetooth.h>
#include <sdp.h>

#include "sdpd.h"

/*
 * This structure is a collection of pointers describing an output
 * buffer for sdpd_put_byte(), below. bytes are written at next when
 * it falls inside the range [start .. end - 1]
 */
typedef struct {
	uint8_t *start;	/* start of buffer window */
	uint8_t	*next;	/* current write position */
	uint8_t *end;	/* end of buffer window */
} sdpd_data_t;

static bool sdpd_valid_ssp(sdp_data_t *);
static bool sdpd_valid_ail(sdp_data_t *);
static bool sdpd_match_ail(record_t *, sdp_data_t, sdpd_data_t *);
static void sdpd_put_byte(sdpd_data_t *, uint8_t);
static void sdpd_put_attr(sdpd_data_t *, uint16_t, sdp_data_t *);
static void sdpd_open_seq(sdpd_data_t *);
static void sdpd_close_seq(sdpd_data_t *, uint8_t *);

uint16_t
service_search_request(server_t *srv, int fd)
{
	record_t	*r;
	sdp_data_t	d, s;
	int		max, total, count;

	d.next = srv->ibuf;
	d.end = srv->ibuf + srv->pdu.len;

	/*
	 * extract ServiceSearchPattern
	 */
	if (!sdp_get_seq(&d, &s)
	    || !sdpd_valid_ssp(&s))
		return SDP_ERROR_CODE_INVALID_REQUEST_SYNTAX;

	/*
	 * extract MaximumServiceRecordCount
	 */
	if (d.next + sizeof(uint16_t) > d.end)
		return SDP_ERROR_CODE_INVALID_REQUEST_SYNTAX;

	max = be16dec(d.next);
	d.next += sizeof(uint16_t);
	if (max < 0x0001)
		return SDP_ERROR_CODE_INVALID_REQUEST_SYNTAX;

	/*
	 * validate ContinuationState
	 * If none given, this is a new request
	 */
	if (d.next + 1 > d.end
	    || d.next[0] > 16
	    || d.next + 1 + d.next[0] != d.end)
		return SDP_ERROR_CODE_INVALID_REQUEST_SYNTAX;

	if (d.next[0] == 0) {
		srv->fdidx[fd].offset = 0;
		db_unselect(srv, fd);
		db_select_ssp(srv, fd, &s);
	} else if (srv->fdidx[fd].offset == 0
	    || d.next[0] != sizeof(uint16_t)
	    || be16dec(d.next + 1) != srv->fdidx[fd].offset)
		return SDP_ERROR_CODE_INVALID_CONTINUATION_STATE;

	/*
	 * Ready our output buffer. We leave space at the start for
	 * TotalServiceRecordCount and CurrentServiceRecordCount and
	 * at the end for ContinuationState, and we must have space
	 * for at least one ServiceRecordHandle. Then, step through
	 * selected records and write as many handles that will fit
	 * into the data space
	 */
	d.next = srv->obuf + sizeof(uint16_t) + sizeof(uint16_t);
	d.end = srv->obuf + srv->fdidx[fd].omtu - 1 - sizeof(uint16_t);
	count = total = 0;

	if (d.next + sizeof(uint32_t) > d.end)
		return SDP_ERROR_CODE_INSUFFICIENT_RESOURCES;

	r = NULL;
	while (db_next(srv, fd, &r) && total < max) {
		if (total >= srv->fdidx[fd].offset
		    && d.next + sizeof(uint32_t) <= d.end) {
			be32enc(d.next, r->handle);
			d.next += sizeof(uint32_t);
			count++;
		}

		total++;
	}

	/*
	 * encode TotalServiceRecordCount and CurrentServiceRecordCount
	 */
	be16enc(srv->obuf, total);
	be16enc(srv->obuf + sizeof(uint16_t), count);

	/*
	 * encode ContinuationState which in this case will be the
	 * number of ServiceRecordHandles already sent.
	 */
	if (r == NULL || total == max) {
		srv->fdidx[fd].offset = 0;
		db_unselect(srv, fd);
		d.next[0] = 0;
		d.next += 1;
	} else {
		srv->fdidx[fd].offset += count;
		d.next[0] = sizeof(uint16_t);
		be16enc(d.next + 1, srv->fdidx[fd].offset);
		d.next += 1 + sizeof(uint16_t);
	}

	/*
	 * fill in PDU header and we are done
	 */
	srv->pdu.pid = SDP_PDU_SERVICE_SEARCH_RESPONSE;
	srv->pdu.len = d.next - srv->obuf;
	return 0;
}

uint16_t
service_attribute_request(server_t *srv, int fd)
{
	record_t	*r;
	sdp_data_t	a, d;
	sdpd_data_t	b;
	uint8_t		*tmp;
	uint32_t	handle;
	int		max;

	d.next = srv->ibuf;
	d.end = srv->ibuf + srv->pdu.len;

	/*
	 * extract ServiceRecordHandle
	 */
	if (d.next + sizeof(uint32_t) > d.end)
		return SDP_ERROR_CODE_INVALID_REQUEST_SYNTAX;

	handle = be32dec(d.next);
	d.next += sizeof(uint32_t);

	/*
	 * extract MaximumAttributeByteCount
	 */
	if (d.next + sizeof(uint16_t) > d.end)
		return SDP_ERROR_CODE_INVALID_REQUEST_SYNTAX;

	max = be16dec(d.next);
	d.next += sizeof(uint16_t);
	if (max < 0x0007)
		return SDP_ERROR_CODE_INVALID_REQUEST_SYNTAX;

	/*
	 * extract AttributeIDList
	 */
	if (!sdp_get_seq(&d, &a)
	    || !sdpd_valid_ail(&a))
		return SDP_ERROR_CODE_INVALID_REQUEST_SYNTAX;

	/*
	 * validate ContinuationState
	 * If none given, this is a new request
	 */
	if (d.next + 1 > d.end
	    || d.next[0] > 16
	    || d.next + 1 + d.next[0] != d.end)
		return SDP_ERROR_CODE_INVALID_REQUEST_SYNTAX;

	if (d.next[0] == 0) {
		srv->fdidx[fd].offset = 0;
		db_unselect(srv, fd);
		db_select_handle(srv, fd, handle);
	} else if (srv->fdidx[fd].offset == 0
	    || d.next[0] != sizeof(uint16_t)
	    || be16dec(d.next + 1) != srv->fdidx[fd].offset)
		return SDP_ERROR_CODE_INVALID_CONTINUATION_STATE;

	/*
	 * Set up the buffer window and write pointer, leaving space at
	 * buffer start for AttributeListByteCount and for ContinuationState
	 * at the end
	 */
	b.start = srv->obuf + sizeof(uint16_t);
	b.next = b.start - srv->fdidx[fd].offset;
	b.end = srv->obuf + srv->fdidx[fd].omtu - 1;
	if (b.start + max < b.end)
		b.end = b.start + max;

	/*
	 * Match the selected record against AttributeIDList, writing
	 * the data to the sparce buffer.
	 */
	r = NULL;
	db_next(srv, fd, &r);
	if (r == NULL)
		return SDP_ERROR_CODE_INVALID_SERVICE_RECORD_HANDLE;

	sdpd_match_ail(r, a, &b);

	if (b.next > b.end) {
		/*
		 * b.end is the limit of AttributeList that we are allowed
		 * to send so if we have exceeded that we need to adjust our
		 * response downwards. Recalculate the new cut off to allow
		 * writing the ContinuationState offset and ensure we don't
		 * exceed MaximumAttributeByteCount. Also, make sure that
		 * the continued length is not too short.
		 */
		tmp = b.next;
		b.next = srv->obuf + srv->fdidx[fd].omtu - 1 - sizeof(uint16_t);
		if (b.next > b.end)
			b.next = b.end;

		if (tmp - b.next < 0x0002)
			b.next = tmp - 0x0002;

		/* encode AttributeListByteCount */
		be16enc(srv->obuf, (b.next - b.start));

		/* calculate & append ContinuationState */
		srv->fdidx[fd].offset += (b.next - b.start);
		b.next[0] = sizeof(uint16_t);
		be16enc(b.next + 1, srv->fdidx[fd].offset);
		b.next += 1 + sizeof(uint16_t);
	} else {
		/* encode AttributeListByteCount */
		be16enc(srv->obuf, (b.next - b.start));

		/* reset & append ContinuationState */
		srv->fdidx[fd].offset = 0;
		db_unselect(srv, fd);
		b.next[0] = 0;
		b.next += 1;
	}

	/*
	 * fill in PDU header and we are done
	 */
	srv->pdu.pid = SDP_PDU_SERVICE_ATTRIBUTE_RESPONSE;
	srv->pdu.len = b.next - srv->obuf;
	return 0;
}

uint16_t
service_search_attribute_request(server_t *srv, int fd)
{
	record_t	*r;
	sdpd_data_t	b;
	sdp_data_t	a, d, s;
	uint8_t		*tmp;
	int		max;

	d.next = srv->ibuf;
	d.end = srv->ibuf + srv->pdu.len;

	/*
	 * extract ServiceSearchPattern
	 */
	if (!sdp_get_seq(&d, &s)
	    || !sdpd_valid_ssp(&s))
		return SDP_ERROR_CODE_INVALID_REQUEST_SYNTAX;

	/*
	 * extract MaximumAttributeByteCount
	 */
	if (d.next + sizeof(uint16_t) > d.end)
		return SDP_ERROR_CODE_INVALID_REQUEST_SYNTAX;

	max = be16dec(d.next);
	d.next += sizeof(uint16_t);
	if (max < 0x0007)
		return SDP_ERROR_CODE_INVALID_REQUEST_SYNTAX;

	/*
	 * extract AttributeIDList
	 */
	if (!sdp_get_seq(&d, &a)
	    || !sdpd_valid_ail(&a))
		return SDP_ERROR_CODE_INVALID_REQUEST_SYNTAX;

	/*
	 * validate ContinuationState
	 * If none given, this is a new request
	 */
	if (d.next + 1 > d.end
	    || d.next[0] > 16
	    || d.next + 1 + d.next[0] != d.end)
		return SDP_ERROR_CODE_INVALID_REQUEST_SYNTAX;

	if (d.next[0] == 0) {
		srv->fdidx[fd].offset = 0;
		db_unselect(srv, fd);
		db_select_ssp(srv, fd, &s);
	} else if (srv->fdidx[fd].offset == 0
	    || d.next[0] != sizeof(uint16_t)
	    || be16dec(d.next + 1) != srv->fdidx[fd].offset)
		return SDP_ERROR_CODE_INVALID_CONTINUATION_STATE;

	/*
	 * Set up the buffer window and write pointer, leaving space at
	 * buffer start for AttributeListByteCount and for ContinuationState
	 * at the end.
	 */
	b.start = srv->obuf + sizeof(uint16_t);
	b.end = srv->obuf + srv->fdidx[fd].omtu - 1;
	b.next = b.start - srv->fdidx[fd].offset;
	if (b.start + max < b.end)
		b.end = b.start + max;

	/*
	 * match all selected records against the AttributeIDList,
	 * wrapping the whole in a sequence. Where a record does
	 * not match any attributes, delete the empty sequence.
	 */
	sdpd_open_seq(&b);

	r = NULL;
	while (db_next(srv, fd, &r)) {
		tmp = b.next;
		if (!sdpd_match_ail(r, a, &b))
			b.next = tmp;
	}

	sdpd_close_seq(&b, b.start - srv->fdidx[fd].offset);

	if (b.next > b.end) {
		/*
		 * b.end is the limit of AttributeLists that we are allowed
		 * to send so if we have exceeded that we need to adjust our
		 * response downwards. Recalculate the new cut off to allow
		 * writing the ContinuationState offset and ensure we don't
		 * exceed MaximumAttributeByteCount. Also, make sure that
		 * the continued length is not too short.
		 */
		tmp = b.next;
		b.next = srv->obuf + srv->fdidx[fd].omtu - 1 - sizeof(uint16_t);
		if (b.next > b.end)
			b.next = b.end;

		if (tmp - b.next < 0x0002)
			b.next = tmp - 0x0002;

		/* encode AttributeListsByteCount */
		be16enc(srv->obuf, (b.next - b.start));

		/* calculate & append ContinuationState */
		srv->fdidx[fd].offset += (b.next - b.start);
		b.next[0] = sizeof(uint16_t);
		be16enc(b.next + 1, srv->fdidx[fd].offset);
		b.next += 1 + sizeof(uint16_t);
	} else {
		/* encode AttributeListsByteCount */
		be16enc(srv->obuf, (b.next - b.start));

		/* reset & append ContinuationState */
		srv->fdidx[fd].offset = 0;
		db_unselect(srv, fd);
		b.next[0] = 0;
		b.next += 1;
	}

	/*
	 * fill in PDU header and we are done
	 */
	srv->pdu.pid = SDP_PDU_SERVICE_SEARCH_ATTRIBUTE_RESPONSE;
	srv->pdu.len = b.next - srv->obuf;
	return 0;
}

/*
 * validate ServiceSearchPattern
 *
 * The SerivceSearchPattern is a list of data elements, where each element
 * is a UUID. The list must contain at least one UUID and the maximum number
 * of UUIDs is 12
 */
static bool
sdpd_valid_ssp(sdp_data_t *ssp)
{
	sdp_data_t	s = *ssp;
	uuid_t		u;
	int		n;

	if (!sdp_data_valid(&s))
		return false;

	n = 0;
	while (sdp_get_uuid(&s, &u))
		n++;

	if (n < 1 || n > 12 || s.next != s.end)
		return false;

	return true;
}

/*
 * validate AttributeIDList
 *
 * The AttributeIDList is a list of data elements, where each element is
 * either an attribute ID encoded as an unsigned 16-bit integer or a range
 * of attribute IDs encoded as an unsigned 32-bit integer where the high
 * order 16-bits are the beginning of the range and the low order 16-bits
 * are the ending
 *
 * The attrbute IDs should be listed in ascending order without duplication
 * of any attribute ID values but we don't worry about that, since if the
 * remote party messes up, their results will be messed up
 */
static bool
sdpd_valid_ail(sdp_data_t *ail)
{
	sdp_data_t	a = *ail;
	sdp_data_t	d;

	if (!sdp_data_valid(&a))
		return false;

	while (sdp_get_data(&a, &d)) {
		if (sdp_data_type(&d) != SDP_DATA_UINT16
		    && sdp_data_type(&d) != SDP_DATA_UINT32)
			return false;
	}

	return true;
}

/*
 * compare attributes in the ServiceRecord with the AttributeIDList
 * and copy any matches to a sequence in the output buffer.
 */
static bool
sdpd_match_ail(record_t *rec, sdp_data_t ail, sdpd_data_t *buf)
{
	sdp_data_t	r, v;
	uint16_t	a;
	uintmax_t	ui;
	uint8_t		*f;
	int		lo, hi;
	bool		rv;

	r = rec->data;
	f = buf->next;
	lo = hi = -1;
	rv = false;

	sdpd_open_seq(buf);

	while (sdp_get_attr(&r, &a, &v)) {
		while (a > hi) {
			if (ail.next == ail.end)
				goto done;

			if (sdp_data_type(&ail) == SDP_DATA_UINT16) {
				sdp_get_uint(&ail, &ui);
				lo = hi = ui;
			} else {
				sdp_get_uint(&ail, &ui);
				lo = (uint16_t)(ui >> 16);
				hi = (uint16_t)(ui);
			}
		}

		if (a < lo)
			continue;

		sdpd_put_attr(buf, a, &v);
		rv = true;
	}

done:
	sdpd_close_seq(buf, f);
	return rv;
}

/*
 * output data. We only actually store the bytes when the
 * pointer is within the valid window.
 */
static void
sdpd_put_byte(sdpd_data_t *buf, uint8_t byte)
{

	if (buf->next >= buf->start && buf->next < buf->end)
		buf->next[0] = byte;

	buf->next++;
}

static void
sdpd_put_attr(sdpd_data_t *buf, uint16_t attr, sdp_data_t *data)
{
	uint8_t	*p;

	sdpd_put_byte(buf, SDP_DATA_UINT16);
	sdpd_put_byte(buf, (uint8_t)(attr >> 8));
	sdpd_put_byte(buf, (uint8_t)(attr));

	for (p = data->next; p < data->end; p++)
		sdpd_put_byte(buf, *p);
}

/*
 * Since we always use a seq16 and never check the length, we will send
 * an invalid header if it grows too large. We could always use a seq32
 * but the chance of overflow is small so ignore it for now.
 */
static void
sdpd_open_seq(sdpd_data_t *buf)
{

	buf->next += 3;
}

static void
sdpd_close_seq(sdpd_data_t *buf, uint8_t *first)
{
	uint8_t	*next;
	size_t	len;

	next = buf->next;
	buf->next = first;
	len = next - first - 3;

	sdpd_put_byte(buf, SDP_DATA_SEQ16);
	sdpd_put_byte(buf, 0xff & (len >> 8));
	sdpd_put_byte(buf, 0xff & (len >> 0));
	buf->next = next;
}
