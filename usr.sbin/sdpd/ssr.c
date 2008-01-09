/*	$NetBSD: ssr.c,v 1.1.10.1 2008/01/09 02:02:29 matt Exp $	*/

/*
 * ssr.c
 *
 * Copyright (c) 2004 Maksim Yevmenkin <m_evmenkin@yahoo.com>
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $Id: ssr.c,v 1.1.10.1 2008/01/09 02:02:29 matt Exp $
 * $FreeBSD: src/usr.sbin/bluetooth/sdpd/ssr.c,v 1.3 2005/01/05 18:37:37 emax Exp $
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: ssr.c,v 1.1.10.1 2008/01/09 02:02:29 matt Exp $");

#include <sys/queue.h>
#include <sys/uio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <bluetooth.h>
#include <errno.h>
#include <sdp.h>
#include <string.h>
#include "profile.h"
#include "provider.h"
#include "server.h"
#include "uuid-private.h"

/*
 * Extract ServiceSearchPattern from request to uuid array
 *	return count or 0 if error
 */
int
server_get_service_search_pattern(uint8_t const **buf, uint8_t const *end, uint128_t *uuid)
{
	uint8_t const *req = *buf;
	uint32_t type, ssplen;
	int count;

	if (req + 1 > end)
		return 0;

	SDP_GET8(type, req);

	ssplen = 0;
	switch (type) {
	case SDP_DATA_SEQ8:
		if (req + 1 > end)
			return 0;

		SDP_GET8(ssplen, req);
		break;

	case SDP_DATA_SEQ16:
		if (req + 2 > end)
			return 0;

		SDP_GET16(ssplen, req);
		break;

	case SDP_DATA_SEQ32:
		if (req + 4 > end)
			return 0;

		SDP_GET32(ssplen, req);
		break;

	default:
		return 0;
	}

	if (req + ssplen > end)
		return 0;

	count = 0;
	while (ssplen > 0) {
		if (count == 12)
			return 0;

		SDP_GET8(type, req);
		ssplen--;

		switch (type) {
		case SDP_DATA_UUID16:
			if (ssplen < 2)
				return 0;

			memcpy(uuid, &uuid_base, sizeof(*uuid));
			uuid->b[2] = *req++;
			uuid->b[3] = *req++;
			ssplen -= 2;
			break;

		case SDP_DATA_UUID32:
			if (ssplen < 4)
				return 0;

			memcpy(uuid, &uuid_base, sizeof(*uuid));
			uuid->b[0] = *req++;
			uuid->b[1] = *req++;
			uuid->b[2] = *req++;
			uuid->b[3] = *req++;
			ssplen -= 4;
			break;

		case SDP_DATA_UUID128:
			if (ssplen < 16)
				return 0;

			memcpy(uuid, req, 16);
			req += 16;
			ssplen -= 16;
			break;

		default:
			return 0;
		}

		count++;
		uuid++;
	}

	*buf = req;
	return count;
}

/*
 * Prepare SDP Service Search Response
 */

int32_t
server_prepare_service_search_response(server_p srv, int32_t fd)
{
	uint8_t const	*req = srv->req + sizeof(sdp_pdu_t);
	uint8_t const	*req_end = req + ((sdp_pdu_p)(srv->req))->len;
	uint8_t		*rsp = srv->fdidx[fd].rsp;
	uint8_t const	*rsp_end = rsp + L2CAP_MTU_MAXIMUM;

	provider_t	*provider = NULL;
	int32_t		 ucount, rsp_limit, cslen, cs;
	uint128_t	 ulist[12];

	/*
	 * Minimal SDP Service Search Request
	 *
	 * seq8 len8		- 2 bytes
	 *	uuid16 value16	- 3 bytes ServiceSearchPattern
	 * value16		- 2 bytes MaximumServiceRecordCount
	 * value8		- 1 byte  ContinuationState
	 */

	/* Get ServiceSearchPattern into uuid array */
	ucount = server_get_service_search_pattern(&req, req_end, ulist);
	if (ucount < 1 || ucount > 12)
		return (SDP_ERROR_CODE_INVALID_REQUEST_SYNTAX);

	/* Get MaximumServiceRecordCount */
	if (req + 2 > req_end)
		return (SDP_ERROR_CODE_INVALID_REQUEST_SYNTAX);

	SDP_GET16(rsp_limit, req);
	if (rsp_limit <= 0)
		return (SDP_ERROR_CODE_INVALID_REQUEST_SYNTAX);

	/* Get ContinuationState */
	if (req + 1 > req_end)
		return (SDP_ERROR_CODE_INVALID_REQUEST_SYNTAX);

	SDP_GET8(cslen, req);
	if (cslen == 2 && req + 2 == req_end)
		SDP_GET16(cs, req);
	else if (cslen == 0 && req == req_end)
		cs = 0;
	else
		return (SDP_ERROR_CODE_INVALID_CONTINUATION_STATE);

	/* Process the request. First, check continuation state */
	if (srv->fdidx[fd].rsp_cs != cs)
		return (SDP_ERROR_CODE_INVALID_CONTINUATION_STATE);
	if (srv->fdidx[fd].rsp_size > 0)
		return (0);

	/*
	 * Service Search Response format
	 *
	 * value16	- 2 bytes TotalServiceRecordCount (not incl.)
	 * value16	- 2 bytes CurrentServiceRecordCount (not incl.)
	 * value32	- 4 bytes handle
	 * [ value32 ]
	 */

	/* Look for the record handles and add to the rsp buffer */
	for (provider = provider_get_first();
	     provider != NULL;
	     provider = provider_get_next(provider)) {
		if (!provider_match_bdaddr(provider, &srv->req_sa.bt_bdaddr))
			continue;

		if (!provider_match_uuid(provider, ulist, ucount))
			continue;

		if (rsp + 4 > rsp_end)
			break;

		SDP_PUT32(provider->handle, rsp);
	}

	/* Set reply size (not counting PDU header and continuation state) */
	srv->fdidx[fd].rsp_limit = srv->fdidx[fd].omtu - sizeof(sdp_pdu_t) - 4;
	srv->fdidx[fd].rsp_size = rsp - srv->fdidx[fd].rsp;
	srv->fdidx[fd].rsp_cs = 0;

	return (0);
}

/*
 * Send SDP Service Search Response
 */

int32_t
server_send_service_search_response(server_p srv, int32_t fd)
{
	uint8_t		*rsp = srv->fdidx[fd].rsp + srv->fdidx[fd].rsp_cs;
	uint8_t		*rsp_end = srv->fdidx[fd].rsp + srv->fdidx[fd].rsp_size;

	struct iovec	iov[4];
	sdp_pdu_t	pdu;
	uint16_t	rcounts[2];
	uint8_t		cs[3];
	int32_t		size;

	/* First update continuation state (assume we will send all data) */
	size = rsp_end - rsp;
	srv->fdidx[fd].rsp_cs += size;

	if (size + 1 > srv->fdidx[fd].rsp_limit) {
		/*
		 * We need to split out response. Add 3 more bytes for the
		 * continuation state and move rsp_end and rsp_cs backwards.
		 */

		while ((rsp_end - rsp) + 3 > srv->fdidx[fd].rsp_limit) {
			rsp_end -= 4;
			srv->fdidx[fd].rsp_cs -= 4;
		}

		cs[0] = 2;
		cs[1] = srv->fdidx[fd].rsp_cs >> 8;
		cs[2] = srv->fdidx[fd].rsp_cs & 0xff;
	} else
		cs[0] = 0;

	assert(rsp_end >= rsp);

	rcounts[0] = srv->fdidx[fd].rsp_size / 4; /* TotalServiceRecordCount */
	rcounts[1] = (rsp_end - rsp) / 4; /* CurrentServiceRecordCount */

	pdu.pid = SDP_PDU_SERVICE_SEARCH_RESPONSE;
	pdu.tid = ((sdp_pdu_p)(srv->req))->tid;
	pdu.len = htons(sizeof(rcounts) + rcounts[1] * 4 + 1 + cs[0]);

	rcounts[0] = htons(rcounts[0]);
	rcounts[1] = htons(rcounts[1]);

	iov[0].iov_base = &pdu;
	iov[0].iov_len = sizeof(pdu);

	iov[1].iov_base = rcounts;
	iov[1].iov_len = sizeof(rcounts);

	iov[2].iov_base = rsp;
	iov[2].iov_len = rsp_end - rsp;

	iov[3].iov_base = cs;
	iov[3].iov_len = 1 + cs[0];

	do {
		size = writev(fd, (struct iovec const *) &iov, sizeof(iov)/sizeof(iov[0]));
	} while (size < 0 && errno == EINTR);

	/* Check if we have sent (or failed to sent) last response chunk */
	if (srv->fdidx[fd].rsp_cs == srv->fdidx[fd].rsp_size) {
		srv->fdidx[fd].rsp_cs = 0;
		srv->fdidx[fd].rsp_size = 0;
		srv->fdidx[fd].rsp_limit = 0;
	}

	return ((size < 0)? errno : 0);
}
