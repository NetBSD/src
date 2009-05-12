/*	$NetBSD: sdp_get.c,v 1.1 2009/05/12 10:05:06 plunky Exp $	*/

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
__RCSID("$NetBSD: sdp_get.c,v 1.1 2009/05/12 10:05:06 plunky Exp $");

#include <sdp.h>
#include <limits.h>

/******************************************************************************
 *	sdp_get_xxxx(data, value)
 *
 * examine first SDP data element in list for xxx type, extracting to given
 * storage and advancing pointer if found.
 * - these functions will not modify data pointer unless the value was
 *   extracted successfully
 * - these functions always update the data pointer before the value pointer,
 *   so where the value is a sdp_data_t the data struct can be discarded.
 */

bool
sdp_get_data(sdp_data_t *data, sdp_data_t *value)
{
	uint8_t *p = data->next;
	ssize_t l = sdp_data_size(data);

	if (l == -1
	    || p + l > data->end)
		return false;

	data->next = p + l;
	value->next = p;
	value->end = p + l;
	return true;
}

bool
sdp_get_attr(sdp_data_t *data, uint16_t *attr, sdp_data_t *value)
{
	sdp_data_t v, d = *data;
	uintmax_t a;

	if (sdp_data_type(&d) != SDP_DATA_UINT16
	    || !sdp_get_uint(&d, &a)
	    || !sdp_get_data(&d, &v))
		return false;

	*attr = (uint16_t)a;
	*data = d;
	*value = v;
	return true;
}

bool
sdp_get_uuid(sdp_data_t *data, uuid_t *uuid)
{
	uint8_t *p = data->next;

	if (p + 1 > data->end)
		return false;

	switch (*p++) {
	case SDP_DATA_UUID16:
		if (p + 2 > data->end)
			return false;

		*uuid = BLUETOOTH_BASE_UUID;
		uuid->time_low = be16dec(p);
		p += 2;
		break;

	case SDP_DATA_UUID32:
		if (p + 4 > data->end)
			return false;

		*uuid = BLUETOOTH_BASE_UUID;
		uuid->time_low = be32dec(p);
		p += 4;
		break;

	case SDP_DATA_UUID128:
		if (p + 16 > data->end)
			return false;

		uuid_dec_be(p, uuid);
		p += 16;
		break;

	default:
		return false;
	}

	data->next = p;
	return true;
}

bool
sdp_get_bool(sdp_data_t *data, bool *value)
{
	uint8_t *p = data->next;
	uint8_t v;

	if (p + 1 > data->end)
		return false;

	switch (*p++) {
	case SDP_DATA_BOOL:
		if (p + 1 > data->end)
			return false;

		v = *p;
		p += 1;
		break;

	default:
		return false;
	}

	data->next = p;
	*value = ((v != 0) ? true : false);
	return true;
}

bool
sdp_get_uint(sdp_data_t *data, uintmax_t *value)
{
	uint8_t *p = data->next;
	uint64_t v, x;

	if (p + 1 > data->end)
		return false;

	switch (*p++) {
	case SDP_DATA_UINT8:
		if (p + 1 > data->end)
			return false;

		v = *p;
		p += 1;
		break;

	case SDP_DATA_UINT16:
		if (p + 2 > data->end)
			return false;

		v = be16dec(p);
		p += 2;
		break;

	case SDP_DATA_UINT32:
		if (p + 4 > data->end)
			return false;

		v = be32dec(p);
		p += 4;
		break;

	case SDP_DATA_UINT64:
		if (p + 8 > data->end)
			return false;

		v = be64dec(p);
		if (v > UINTMAX_MAX)
			return false;

		p += 8;
		break;

	case SDP_DATA_UINT128:
		if (p + 16 > data->end)
			return false;

		x = be64dec(p);
		v = be64dec(p + 8);
		if (x != 0 || v > UINTMAX_MAX)
			return false;

		p += 16;
		break;

	default:
		return false;
	}

	data->next = p;
	*value = (uintmax_t)v;
	return true;
}

bool
sdp_get_int(sdp_data_t *data, intmax_t *value)
{
	uint8_t *p = data->next;
	int64_t v, x;

	if (p + 1 > data->end)
		return false;

	switch (*p++) {
	case SDP_DATA_INT8:
		if (p + 1 > data->end)
			return false;

		v = *(int8_t *)p;
		p += 1;
		break;

	case SDP_DATA_INT16:
		if (p + 2 > data->end)
			return false;

		v = (int16_t)be16dec(p);
		p += 2;
		break;

	case SDP_DATA_INT32:
		if (p + 4 > data->end)
			return false;

		v = (int32_t)be32dec(p);
		p += 4;
		break;

	case SDP_DATA_INT64:
		if (p + 8 > data->end)
			return false;

		v = (int64_t)be64dec(p);
		if (v > INTMAX_MAX || v < INTMAX_MIN)
			return false;

		p += 8;
		break;

	case SDP_DATA_INT128:
		if (p + 16 > data->end)
			return false;

		x = (int64_t)be64dec(p);
		v = (int64_t)be64dec(p + 8);
		if (x == 0) {
			if (v > INTMAX_MAX)
				return false;
		} else if (x == -1) {
			if (v < INTMAX_MIN)
				return false;
		} else {
			return false;
		}

		p += 16;
		break;

	default:
		return false;
	}

	data->next = p;
	*value = (intmax_t)v;
	return true;
}

static bool
_sdp_get_ext(uint8_t type, sdp_data_t *data, sdp_data_t *ext)
{
	uint8_t *p = data->next;
	uint32_t l;

	if (p + 1 > data->end
	    || SDP_DATA_TYPE(*p) != type)
		return false;

	switch (SDP_DATA_SIZE(*p++)) {
	case SDP_DATA_EXT8:
		if (p + 1 > data->end)
			return false;

		l = *p;
		p += 1;
		break;

	case SDP_DATA_EXT16:
		if (p + 2 > data->end)
			return false;

		l = be16dec(p);
		p += 2;
		break;

	case SDP_DATA_EXT32:
		if (p + 4 > data->end)
			return false;

		l = be32dec(p);
		p += 4;
		break;

	default:
		return false;
	}

	if (p + l > data->end)
		return false;

	data->next = p + l;
	ext->next = p;
	ext->end = p + l;
	return true;
}

bool
sdp_get_seq(sdp_data_t *data, sdp_data_t *seq)
{

	return _sdp_get_ext(SDP_DATA_SEQ, data, seq);
}

bool
sdp_get_alt(sdp_data_t *data, sdp_data_t *alt)
{

	return _sdp_get_ext(SDP_DATA_ALT, data, alt);
}

bool
sdp_get_str(sdp_data_t *data, char **str, size_t *len)
{
	sdp_data_t s;

	if (!_sdp_get_ext(SDP_DATA_STR, data, &s))
		return false;

	*str = (char *)s.next;
	*len = s.end - s.next;
	return true;
}

bool
sdp_get_url(sdp_data_t *data, char **url, size_t *len)
{
	sdp_data_t u;

	if (!_sdp_get_ext(SDP_DATA_URL, data, &u))
		return false;

	*url = (char *)u.next;
	*len = u.end - u.next;
	return true;
}
