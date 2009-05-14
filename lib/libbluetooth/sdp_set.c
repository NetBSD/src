/*	$NetBSD: sdp_set.c,v 1.2 2009/05/14 19:12:45 plunky Exp $	*/

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
__RCSID("$NetBSD: sdp_set.c,v 1.2 2009/05/14 19:12:45 plunky Exp $");

#include <bluetooth.h>
#include <limits.h>
#include <sdp.h>

/******************************************************************************
 *	sdp_set_xxxx(data, value)
 *
 * update a value, will fail if data element is not of the correct type
 */

bool
sdp_set_bool(const sdp_data_t *data, bool value)
{
	uint8_t *p = data->next;

	if (p + 2 > data->end
	    || *p != SDP_DATA_BOOL)
		return false;

	*p = (value ? 0x01 : 0x00);
	return true;
}

bool
sdp_set_uint(const sdp_data_t *data, uintmax_t value)
{
	uint8_t *p = data->next;

	if (p + 1 > data->end)
		return false;

	switch (*p++) {
	case SDP_DATA_UINT8:
		if (p + 1 > data->end
		    || value > UINT8_MAX)
			return false;

		*p = (uint8_t)value;
		break;

	case SDP_DATA_UINT16:
		if (p + 2 > data->end
		    || value > UINT16_MAX)
			return false;

		be16enc(p, (uint16_t)value);
		break;

	case SDP_DATA_UINT32:
		if (p + 4 > data->end
		    || value > UINT32_MAX)
			return false;

		be32enc(p, (uint32_t)value);
		break;

	case SDP_DATA_UINT64:
		if (p + 8 > data->end
		    || value > UINT64_MAX)
			return false;

		be64enc(p, (uint64_t)value);
		break;

	case SDP_DATA_UINT128:
		if (p + 16 > data->end)
			return false;

		be64enc(p + 0, (uint64_t)0);
		be64enc(p + 8, (uint64_t)value);
		break;

	default:
		return false;
	}

	return true;
}

bool
sdp_set_int(const sdp_data_t *data, intmax_t value)
{
	uint8_t *p = data->next;

	if (p + 1 > data->end)
		return false;

	switch (*p++) {
	case SDP_DATA_INT8:
		if (p + 1 > data->end
		    || value > INT8_MAX
		    || value < INT8_MIN)
			return false;

		*p = (uint8_t)value;
		break;

	case SDP_DATA_INT16:
		if (p + 2 > data->end
		    || value > INT16_MAX
		    || value < INT16_MIN)
			return false;

		be16enc(p, (uint16_t)value);
		break;

	case SDP_DATA_INT32:
		if (p + 4 > data->end
		    || value > INT32_MAX
		    || value < INT32_MIN)
			return false;

		be32enc(p, (uint32_t)value);
		break;

	case SDP_DATA_INT64:
		if (p + 8 > data->end
		    || value > INT64_MAX
		    || value < INT64_MIN)
			return false;

		be64enc(p, (uint64_t)value);
		break;

	case SDP_DATA_INT128:
		if (p + 16 > data->end)
			return false;

		be64enc(p + 0, (uint64_t)0);
		be64enc(p + 8, (uint64_t)value);
		break;

	default:
		return false;
	}

	return true;
}

static bool
_sdp_set_ext(uint8_t type, const sdp_data_t *data, ssize_t len)
{
	uint8_t *p = data->next;

	if (p + 1 > data->end
	    || SDP_DATA_TYPE(*p) != type)
		return false;

	switch (SDP_DATA_SIZE(*p++)) {
	case SDP_DATA_EXT8:
		if (len == -1) {
			if (p + 1 > data->end)
				return false;

			len = data->end - p - 1;
		} else if (p + 1 + len > data->end)
			return false;

		if (len > UINT8_MAX)
			return false;

		*p = (uint8_t)len;
		break;

	case SDP_DATA_EXT16:
		if (len == -1) {
			if (p + 2 > data->end)
				return false;

			len = data->end - p - 2;
		} else if (p + 2 + len > data->end)
			return false;

		if (len > UINT16_MAX)
			return false;

		be16enc(p, (uint16_t)len);
		break;

	case SDP_DATA_EXT32:
		if (len == -1) {
			if (p + 4 > data->end)
				return false;

			len = data->end - p - 4;
		} else if (p + 4 + len > data->end)
			return false;

		if ((size_t)len > UINT32_MAX)
			return false;

		be32enc(p, (uint32_t)len);
		break;

	default:
		return false;
	}

	return true;
}

bool
sdp_set_seq(const sdp_data_t *data, ssize_t len)
{

	return _sdp_set_ext(SDP_DATA_SEQ, data, len);
}

bool
sdp_set_alt(const sdp_data_t *data, ssize_t len)
{

	return _sdp_set_ext(SDP_DATA_ALT, data, len);
}
