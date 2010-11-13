/*	$NetBSD: sdp_put.c,v 1.3 2010/11/13 19:43:56 plunky Exp $	*/

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
__RCSID("$NetBSD: sdp_put.c,v 1.3 2010/11/13 19:43:56 plunky Exp $");

#include <bluetooth.h>
#include <limits.h>
#include <sdp.h>
#include <string.h>

/******************************************************************************
 *	sdp_put_xxxx(data, value)
 *
 * write a value to data space and advance data pointers,
 * fail if data space is not large enough
 */

bool
sdp_put_data(sdp_data_t *data, sdp_data_t *value)
{
	ssize_t len;

	len = value->end - value->next;

	if (data->next + len > data->end)
		return false;

	memcpy(data->next, value->next, (size_t)len);
	data->next += len;
	return true;
}

bool
sdp_put_attr(sdp_data_t *data, uint16_t attr, sdp_data_t *value)
{
	sdp_data_t d = *data;

	if (!sdp_put_uint16(&d, attr)
	    || sdp_put_data(&d, value))
		return false;

	*data = d;
	return true;
}

bool
sdp_put_uuid(sdp_data_t *data, const uuid_t *uuid)
{
	uuid_t u = *uuid;

	u.time_low = 0;

	if (uuid_equal(&u, &BLUETOOTH_BASE_UUID, NULL) == 0)
		return sdp_put_uuid128(data, uuid);

	if (uuid->time_low > UINT16_MAX)
		return sdp_put_uuid32(data, (uint32_t)uuid->time_low);

	return sdp_put_uuid16(data, (uint16_t)uuid->time_low);
}

bool
sdp_put_uuid16(sdp_data_t *data, uint16_t uuid)
{

	if (data->next + 3 > data->end)
		return false;

	data->next[0] = SDP_DATA_UUID16;
	be16enc(data->next + 1, uuid);
	data->next += 3;
	return true;
}

bool
sdp_put_uuid32(sdp_data_t *data, uint32_t uuid)
{

	if (data->next + 5 > data->end)
		return false;

	data->next[0] = SDP_DATA_UUID32;
	be32enc(data->next + 1, uuid);
	data->next += 5;
	return true;
}

bool
sdp_put_uuid128(sdp_data_t *data, const uuid_t *uuid)
{

	if (data->next + 17 > data->end)
		return false;

	data->next[0] = SDP_DATA_UUID128;
	uuid_enc_be(data->next + 1, uuid);
	data->next += 17;
	return true;
}

bool
sdp_put_bool(sdp_data_t *data, bool value)
{

	if (data->next + 2 > data->end)
		return false;

	data->next[0] = SDP_DATA_BOOL;
	data->next[1] = (value ? 0x01 : 0x00);
	data->next += 2;
	return true;
}

bool
sdp_put_uint(sdp_data_t *data, uintmax_t value)
{

	if (value > UINT64_MAX)
		return false;

	if (value > UINT32_MAX)
		return sdp_put_uint64(data, (uint64_t)value);

	if (value > UINT16_MAX)
		return sdp_put_uint32(data, (uint32_t)value);

	if (value > UINT8_MAX)
		return sdp_put_uint16(data, (uint16_t)value);

	return sdp_put_uint8(data, (uint8_t)value);
}

bool
sdp_put_uint8(sdp_data_t *data, uint8_t value)
{

	if (data->next + 2 > data->end)
		return false;

	data->next[0] = SDP_DATA_UINT8;
	data->next[1] = value;
	data->next += 2;
	return true;
}

bool
sdp_put_uint16(sdp_data_t *data, uint16_t value)
{

	if (data->next + 3 > data->end)
		return false;

	data->next[0] = SDP_DATA_UINT16;
	be16enc(data->next + 1, value);
	data->next += 3;
	return true;
}

bool
sdp_put_uint32(sdp_data_t *data, uint32_t value)
{

	if (data->next + 5 > data->end)
		return false;

	data->next[0] = SDP_DATA_UINT32;
	be32enc(data->next + 1, value);
	data->next += 5;
	return true;
}

bool
sdp_put_uint64(sdp_data_t *data, uint64_t value)
{

	if (data->next + 9 > data->end)
		return false;

	data->next[0] = SDP_DATA_UINT64;
	be64enc(data->next + 1, value);
	data->next += 9;
	return true;
}

bool
sdp_put_int(sdp_data_t *data, intmax_t value)
{

	if (value > INT64_MAX || value < INT64_MIN)
		return false;

	if (value > INT32_MAX || value < INT32_MIN)
		return sdp_put_int64(data, (int64_t)value);

	if (value > INT16_MAX || value < INT16_MIN)
		return sdp_put_int32(data, (int32_t)value);

	if (value > INT8_MAX || value < INT8_MIN)
		return sdp_put_int16(data, (int16_t)value);

	return sdp_put_int8(data, (int8_t)value);
}

bool
sdp_put_int8(sdp_data_t *data, int8_t value)
{

	if (data->next + 2 > data->end)
		return false;

	data->next[0] = SDP_DATA_INT8;
	data->next[1] = (uint8_t)value;
	data->next += 2;
	return true;
}

bool
sdp_put_int16(sdp_data_t *data, int16_t value)
{

	if (data->next + 3 > data->end)
		return false;

	data->next[0] = SDP_DATA_INT16;
	be16enc(data->next + 1, (uint16_t)value);
	data->next += 3;
	return true;
}

bool
sdp_put_int32(sdp_data_t *data, int32_t value)
{

	if (data->next + 5 > data->end)
		return false;

	data->next[0] = SDP_DATA_INT32;
	be32enc(data->next + 1, (uint32_t)value);
	data->next += 5;
	return true;
}

bool
sdp_put_int64(sdp_data_t *data, int64_t value)
{

	if (data->next + 9 > data->end)
		return false;

	data->next[0] = SDP_DATA_INT64;
	be64enc(data->next + 1, (uint64_t)value);
	data->next += 9;
	return true;
}

static bool
_sdp_put_ext(uint8_t type, sdp_data_t *data, ssize_t len)
{
	uint8_t *p = data->next;

	if (len == -1) {
		if (p + 2 > data->end)
			return false;

		len = data->end - p - 2;

		if (len > UINT8_MAX)
			len -= 1;

		if (len > UINT16_MAX)
			len -= 2;
	}

	if ((size_t)len > UINT32_MAX)
		return false;

	if ((size_t)len > UINT16_MAX) {
		if (p + 5 + len > data->end)
			return false;

		p[0] = type | SDP_DATA_EXT32;
		be32enc(p + 1, (uint32_t)len);
		p += 5;
	} else if ((size_t)len > UINT8_MAX) {
		if (p + 3 + len > data->end)
			return false;

		p[0] = type | SDP_DATA_EXT16;
		be16enc(p + 1, (uint16_t)len);
		p += 3;
	} else {
		if (p + 2 + len > data->end)
			return false;

		p[0] = type | SDP_DATA_EXT8;
		p[1] = (uint8_t)len;
		p += 2;
	}

	data->next = p;
	return true;
}

bool
sdp_put_seq(sdp_data_t *data, ssize_t len)
{

	return _sdp_put_ext(SDP_DATA_SEQ, data, len);
}

bool
sdp_put_alt(sdp_data_t *data, ssize_t len)
{

	return _sdp_put_ext(SDP_DATA_ALT, data, len);
}

bool
sdp_put_str(sdp_data_t *data, const char *str, ssize_t len)
{

	if (len == -1)
		len = strlen(str);

	if (!_sdp_put_ext(SDP_DATA_STR, data, len))
		return false;

	memcpy(data->next, str, (size_t)len);
	data->next += len;
	return true;
}

bool
sdp_put_url(sdp_data_t *data, const char *url, ssize_t len)
{

	if (len == -1)
		len = strlen(url);

	if (!_sdp_put_ext(SDP_DATA_URL, data, len))
		return false;

	memcpy(data->next, url, (size_t)len);
	data->next += len;
	return true;
}
