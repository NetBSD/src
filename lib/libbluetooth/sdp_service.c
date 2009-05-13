/*	$NetBSD: sdp_service.c,v 1.1.2.2 2009/05/13 19:18:20 jym Exp $	*/

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
__RCSID("$NetBSD: sdp_service.c,v 1.1.2.2 2009/05/13 19:18:20 jym Exp $");

#include <errno.h>
#include <limits.h>
#include <sdp.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "sdp-int.h"

/*
 * If AttributeIDList is given as NULL, request all attributes.
 */
static uint8_t ail_default[] = { 0x0a, 0x00, 0x00, 0xff, 0xff };

/*
 * This provides the maximum size that the response buffer will be
 * allowed to grow to.
 *
 * Default is UINT16_MAX but it can be overridden at runtime.
 */
static size_t
sdp_response_max(void)
{
	static size_t max = UINT16_MAX;
	static bool check = true;
	char *env, *ep;
	unsigned long v;

	while (check) {
		check = false;	/* only check env once */

		env = getenv("SDP_RESPONSE_MAX");
		if (env == NULL)
			break;

		errno = 0;
		v = strtoul(env, &ep, 0);
		if (env[0] == '\0' || *ep != '\0')
			break;

		if (errno == ERANGE && v == ULONG_MAX)
			break;

		/* lower limit is arbitrary */
		if (v < UINT8_MAX || v > UINT32_MAX)
			break;

		max = v;
	}

	return max;
}

bool
sdp_service_search(struct sdp_session *ss, const sdp_data_t *ssp,
    uint32_t *id, int *num)
{
	struct iovec	req[5];
	sdp_data_t	hdr;
	uint8_t		sdata[5], max[2];
	uint8_t		*ptr, *end;
	ssize_t		len;
	uint16_t	total, count, got;

	/*
	 * setup ServiceSearchPattern
	 */
	len = ssp->end - ssp->next;
	if (len < 0 || len > UINT16_MAX) {
		errno = EINVAL;
		return false;
	}

	hdr.next = sdata;
	hdr.end = sdata + sizeof(sdata) + len;
	sdp_put_seq(&hdr, len);
	req[1].iov_base = sdata;
	req[1].iov_len = hdr.next - sdata;

	req[2].iov_base = ssp->next;
	req[2].iov_len = len;

	/*
	 * setup MaximumServiceRecordCount
	 */
	if (*num < 0 || *num > UINT16_MAX) {
		errno = EINVAL;
		return false;
	}
	be16enc(max, *num);
	req[3].iov_base = max;
	req[3].iov_len = sizeof(uint16_t);

	/*
	 * clear ContinuationState
	 */
	ss->cs[0] = 0;

	/*
	 * ServiceSearch Transaction
	 */
	got = 0;
	for (;;) {
		/*
		 * setup ContinuationState
		 */
		req[4].iov_base = ss->cs;
		req[4].iov_len = ss->cs[0] + 1;

		if (!_sdp_send_pdu(ss, SDP_PDU_SERVICE_SEARCH_REQUEST,
		    req, __arraycount(req)))
			return false;

		len = _sdp_recv_pdu(ss, SDP_PDU_SERVICE_SEARCH_RESPONSE);
		if (len == -1)
			return false;

		ptr = ss->ibuf;
		end = ss->ibuf + len;

		/*
		 * extract TotalServiceRecordCount
		 */
		if (ptr + sizeof(uint16_t) > end)
			break;

		total = be16dec(ptr);
		ptr += sizeof(uint16_t);
		if (total > *num)
			break;

		/*
		 * extract CurrentServiceRecordCount
		 */
		if (ptr + sizeof(uint16_t) > end)
			break;

		count = be16dec(ptr);
		ptr += sizeof(uint16_t);
		if (got + count > total)
			break;

		/*
		 * extract ServiceRecordHandleList
		 */
		if (ptr + count * sizeof(uint32_t) > end)
			break;

		while (count-- > 0) {
			id[got++] = be32dec(ptr);
			ptr += sizeof(uint32_t);
		}

		/*
		 * extract ContinuationState
		 */
		if (ptr + 1 > end
		    || ptr[0] > 16
		    || ptr + ptr[0] + 1 != end)
			break;

		memcpy(ss->cs, ptr, ptr[0] + 1);

		/*
		 * Complete?
		 */
		if (ss->cs[0] == 0) {
			*num = got;
			return true;
		}
	}

	errno = EIO;
	return false;
}

bool
sdp_service_attribute(struct sdp_session *ss, uint32_t id,
    const sdp_data_t *ail, sdp_data_t *rsp)
{
	struct iovec	req[6];
	sdp_data_t	hdr;
	uint8_t		adata[5], handle[4], max[2];
	uint8_t		*ptr, *end, *rbuf;
	ssize_t		len;
	size_t		rlen, count;

	/*
	 * setup ServiceRecordHandle
	 */
	be32enc(handle, id);
	req[1].iov_base = handle;
	req[1].iov_len = sizeof(uint32_t);

	/*
	 * setup MaximumAttributeByteCount
	 */
	be16enc(max, ss->imtu - sizeof(uint16_t) - sizeof(ss->cs));
	req[2].iov_base = max;
	req[2].iov_len = sizeof(uint16_t);

	/*
	 * setup AttributeIDList
	 */
	len = (ail == NULL ? sizeof(ail_default) : (ail->end - ail->next));
	if (len < 0 || len > UINT16_MAX) {
		errno = EINVAL;
		return false;
	}

	hdr.next = adata;
	hdr.end = adata + sizeof(adata) + len;
	sdp_put_seq(&hdr, len);
	req[3].iov_base = adata;
	req[3].iov_len = hdr.next - adata;

	req[4].iov_base = (ail == NULL ? ail_default : ail->next);
	req[4].iov_len = len;

	/*
	 * clear ContinuationState
	 */
	ss->cs[0] = 0;

	/*
	 * ServiceAttribute Transaction
	 */
	rlen = 0;
	for (;;) {
		/*
		 * setup ContinuationState
		 */
		req[5].iov_base = ss->cs;
		req[5].iov_len = ss->cs[0] + 1;

		if (!_sdp_send_pdu(ss, SDP_PDU_SERVICE_ATTRIBUTE_REQUEST,
		    req, __arraycount(req)))
			return false;

		len = _sdp_recv_pdu(ss, SDP_PDU_SERVICE_ATTRIBUTE_RESPONSE);
		if (len == -1)
			return false;

		ptr = ss->ibuf;
		end = ss->ibuf + len;

		/*
		 * extract AttributeListByteCount
		 */
		if (ptr + sizeof(uint16_t) > end)
			break;

		count = be16dec(ptr);
		ptr += sizeof(uint16_t);
		if (count == 0 || ptr + count > end)
			break;

		/*
		 * extract AttributeList
		 */
		if (rlen + count > sdp_response_max())
			break;

		rbuf = realloc(ss->rbuf, rlen + count);
		if (rbuf == NULL)
			return false;

		ss->rbuf = rbuf;
		memcpy(rbuf + rlen, ptr, count);
		rlen += count;
		ptr += count;

		/*
		 * extract ContinuationState
		 */
		if (ptr + 1 > end
		    || ptr[0] > 16
		    || ptr + ptr[0] + 1 != end)
			break;

		memcpy(ss->cs, ptr, ptr[0] + 1);

		/*
		 * Complete?
		 */
		if (ss->cs[0] == 0) {
			rsp->next = rbuf;
			rsp->end = rbuf + rlen;
			if (sdp_data_size(rsp) != rlen
			    || !sdp_data_valid(rsp)
			    || !sdp_get_seq(rsp, rsp))
				break;

			return true;
		}
	}

	errno = EIO;
	return false;
}

bool
sdp_service_search_attribute(struct sdp_session *ss, const sdp_data_t *ssp,
    const sdp_data_t *ail, sdp_data_t *rsp)
{
	struct iovec	req[7];
	sdp_data_t	hdr;
	uint8_t		sdata[5], adata[5], max[2];
	uint8_t		*ptr, *end, *rbuf;
	ssize_t		len;
	size_t		rlen, count;

	/*
	 * setup ServiceSearchPattern
	 */
	len = ssp->end - ssp->next;
	if (len < 0 || len > UINT16_MAX) {
		errno = EINVAL;
		return false;
	}

	hdr.next = sdata;
	hdr.end = sdata + sizeof(sdata) + len;
	sdp_put_seq(&hdr, len);
	req[1].iov_base = sdata;
	req[1].iov_len = hdr.next - sdata;

	req[2].iov_base = ssp->next;
	req[2].iov_len = len;

	/*
	 * setup MaximumAttributeByteCount
	 */
	be16enc(max, ss->imtu - sizeof(uint16_t) - sizeof(ss->cs));
	req[3].iov_base = max;
	req[3].iov_len = sizeof(uint16_t);

	/*
	 * setup AttributeIDList
	 */
	len = (ail == NULL ? sizeof(ail_default) : (ail->end - ail->next));
	if (len < 0 || len > UINT16_MAX) {
		errno = EINVAL;
		return false;
	}

	hdr.next = adata;
	hdr.end = adata + sizeof(adata) + len;
	sdp_put_seq(&hdr, len);
	req[4].iov_base = adata;
	req[4].iov_len = hdr.next - adata;

	req[5].iov_base = (ail == NULL ? ail_default : ail->next);
	req[5].iov_len = len;

	/*
	 * clear ContinuationState
	 */
	ss->cs[0] = 0;

	/*
	 * ServiceSearchAttribute Transaction
	 */
	rlen = 0;
	for (;;) {
		/*
		 * setup ContinuationState
		 */
		req[6].iov_base = ss->cs;
		req[6].iov_len = ss->cs[0] + 1;

		if (!_sdp_send_pdu(ss, SDP_PDU_SERVICE_SEARCH_ATTRIBUTE_REQUEST,
		    req, __arraycount(req)))
			return false;

		len = _sdp_recv_pdu(ss, SDP_PDU_SERVICE_SEARCH_ATTRIBUTE_RESPONSE);
		if (len == -1)
			return false;

		ptr = ss->ibuf;
		end = ss->ibuf + len;

		/*
		 * extract AttributeListsByteCount
		 */
		if (ptr + sizeof(uint16_t) > end)
			break;

		count = be16dec(ptr);
		ptr += sizeof(uint16_t);
		if (count == 0 || ptr + count > end)
			break;

		/*
		 * extract AttributeLists
		 */
		if (rlen + count > sdp_response_max())
			break;

		rbuf = realloc(ss->rbuf, rlen + count);
		if (rbuf == NULL)
			return false;

		ss->rbuf = rbuf;
		memcpy(rbuf + rlen, ptr, count);
		rlen += count;
		ptr += count;

		/*
		 * extract ContinuationState
		 */
		if (ptr + 1 > end
		    || ptr[0] > 16
		    || ptr + ptr[0] + 1 != end)
			break;

		memcpy(ss->cs, ptr, ptr[0] + 1);

		/*
		 * Complete?
		 */
		if (ss->cs[0] == 0) {
			rsp->next = rbuf;
			rsp->end = rbuf + rlen;
			if (sdp_data_size(rsp) != rlen
			    || !sdp_data_valid(rsp)
			    || !sdp_get_seq(rsp, rsp))
				break;

			return true;
		}
	}

	errno = EIO;
	return false;
}
