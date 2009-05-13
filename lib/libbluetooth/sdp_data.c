/*	$NetBSD: sdp_data.c,v 1.1.2.2 2009/05/13 19:18:20 jym Exp $	*/

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
__RCSID("$NetBSD: sdp_data.c,v 1.1.2.2 2009/05/13 19:18:20 jym Exp $");

#include <sdp.h>
#include <stdarg.h>
#include <stdio.h>
#include <vis.h>

#include "sdp-int.h"


/******************************************************************************
 *	sdp_data_type(data)
 *
 * return SDP data element type
 */
int
sdp_data_type(const sdp_data_t *data)
{

	if (data->next + 1 > data->end)
		return -1;

	return data->next[0];
}


/******************************************************************************
 *	sdp_data_size(data)
 *
 * return the size of SDP data element. This will fail (return -1) if
 * the data element does not fit into the data space.
 */
ssize_t
sdp_data_size(const sdp_data_t *data)
{
	uint8_t *p = data->next;

	if (p + 1 > data->end)
		return -1;

	switch (*p++) {
	case SDP_DATA_NIL:
		break;

	case SDP_DATA_BOOL:
	case SDP_DATA_INT8:
	case SDP_DATA_UINT8:
		p += 1;
		break;

	case SDP_DATA_INT16:
	case SDP_DATA_UINT16:
	case SDP_DATA_UUID16:
		p += 2;
		break;

	case SDP_DATA_INT32:
	case SDP_DATA_UINT32:
	case SDP_DATA_UUID32:
		p += 4;
		break;

	case SDP_DATA_INT64:
	case SDP_DATA_UINT64:
		p += 8;
		break;

	case SDP_DATA_INT128:
	case SDP_DATA_UINT128:
	case SDP_DATA_UUID128:
		p += 16;
		break;

	case SDP_DATA_ALT8:
	case SDP_DATA_SEQ8:
	case SDP_DATA_STR8:
	case SDP_DATA_URL8:
		if (p + 1 > data->end)
			return -1;

		p += 1 + *p;
		break;

	case SDP_DATA_ALT16:
	case SDP_DATA_SEQ16:
	case SDP_DATA_STR16:
	case SDP_DATA_URL16:
		if (p + 2 > data->end)
			return -1;

		p += 2 + be16dec(p);
		break;

	case SDP_DATA_ALT32:
	case SDP_DATA_SEQ32:
	case SDP_DATA_STR32:
	case SDP_DATA_URL32:
		if (p + 4 > data->end)
			return -1;

		p += 4 + be32dec(p);
		break;

	default:
		return -1;
	}

	if (p > data->end)
		return -1;

	return (p - data->next);
}

/******************************************************************************
 *	sdp_data_valid(data)
 *
 * validate an SDP data element list recursively, ensuring elements do not
 * expand past the claimed length and that there is no invalid data.
 */
static bool
_sdp_data_valid(uint8_t *ptr, uint8_t *end)
{
	size_t len;

	while (ptr < end) {
		if (ptr + 1 > end)
			return false;

		switch (*ptr++) {
		case SDP_DATA_NIL:
			break;

		case SDP_DATA_BOOL:
		case SDP_DATA_INT8:
		case SDP_DATA_UINT8:
			if (ptr + 1 > end)
				return false;

			ptr += 1;
			break;

		case SDP_DATA_INT16:
		case SDP_DATA_UINT16:
		case SDP_DATA_UUID16:
			if (ptr + 2 > end)
				return false;

			ptr += 2;
			break;

		case SDP_DATA_INT32:
		case SDP_DATA_UINT32:
		case SDP_DATA_UUID32:
			if (ptr + 4 > end)
				return false;

			ptr += 4;
			break;

		case SDP_DATA_INT64:
		case SDP_DATA_UINT64:
			if (ptr + 8 > end)
				return false;

			ptr += 8;
			break;

		case SDP_DATA_INT128:
		case SDP_DATA_UINT128:
		case SDP_DATA_UUID128:
			if (ptr + 16 > end)
				return false;

			ptr += 16;
			break;

		case SDP_DATA_STR8:
		case SDP_DATA_URL8:
			if (ptr + 1 > end)
				return false;

			len = *ptr;
			ptr += 1;

			if (ptr + len > end)
				return false;

			ptr += len;
			break;

		case SDP_DATA_STR16:
		case SDP_DATA_URL16:
			if (ptr + 2 > end)
				return false;

			len = be16dec(ptr);
			ptr += 2;

			if (ptr + len > end)
				return false;

			ptr += len;
			break;

		case SDP_DATA_STR32:
		case SDP_DATA_URL32:
			if (ptr + 4 > end)
				return false;

			len = be32dec(ptr);
			ptr += 4;

			if (ptr + len > end)
				return false;

			ptr += len;
			break;

		case SDP_DATA_SEQ8:
		case SDP_DATA_ALT8:
			if (ptr + 1 > end)
				return false;

			len = *ptr;
			ptr += 1;

			if (ptr + len > end)
				return false;

			if (!_sdp_data_valid(ptr, ptr + len))
				return false;

			ptr += len;
			break;

		case SDP_DATA_SEQ16:
		case SDP_DATA_ALT16:
			if (ptr + 2 > end)
				return false;

			len = be16dec(ptr);
			ptr += 2;

			if (ptr + len > end)
				return false;

			if (!_sdp_data_valid(ptr, ptr + len))
				return false;

			ptr += len;
			break;

		case SDP_DATA_SEQ32:
		case SDP_DATA_ALT32:
			if (ptr + 4 > end)
				return false;

			len = be32dec(ptr);
			ptr += 4;

			if (ptr + len > end)
				return false;

			if (!_sdp_data_valid(ptr, ptr + len))
				return false;

			ptr += len;
			break;

		default:
			return false;
		}
	}

	return true;
}

bool
sdp_data_valid(const sdp_data_t *data)
{

	if (data->next == NULL || data->end == NULL)
		return false;

	if (data->next >= data->end)
		return false;

	return _sdp_data_valid(data->next, data->end);
}

/******************************************************************************
 *	sdp_data_print(data, indent)
 *
 * print out a SDP data element list in human readable format
 */
static void
_sdp_put(int indent, const char *type, const char *fmt, ...)
{
	va_list ap;

	indent = printf("%*s%s", indent, "", type);
	indent = 18 - indent;
	if (indent < 2)
		indent = 2;

	printf("%*s", indent, "");

	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);

	printf("\n");
}

static void
_sdp_putstr(int indent, int style, const char *type,
    const uint8_t *str, size_t len)
{
	char buf[50], *dst = buf;

	indent = printf("%*s%s(%zu)", indent, "", type, len);
	indent = 18 - indent;
	if (indent < 2)
		indent = 2;

	printf("%*s", indent, "");

	style |= VIS_NL;

	while (len > 0 && (dst + 5) < (buf + sizeof(buf))) {
		dst = vis(dst, str[0], style, (len > 0 ? str[1] : 0));
		str++;
		len--;
	}

	printf("\"%s%s\n", buf, (len == 0 ? "\"" : " ..."));
}

bool
_sdp_data_print(const uint8_t *next, const uint8_t *end, int indent)
{
	size_t len;

	while (next < end) {
		if (next + 1 > end)
			return false;

		switch (*next++) {
		case SDP_DATA_NIL:
			_sdp_put(indent, "nil", "");
			break;

		case SDP_DATA_BOOL:
			if (next + 1 > end)
				return false;

			_sdp_put(indent, "bool", "%s",
			    (*next == 0x00 ? "false" : "true"));

			next += 1;
			break;

		case SDP_DATA_INT8:
			if (next + 1 > end)
				return false;

			_sdp_put(indent, "int8", "%" PRId8,
			    *(const int8_t *)next);
			next += 1;
			break;

		case SDP_DATA_UINT8:
			if (next + 1 > end)
				return false;

			_sdp_put(indent, "uint8", "0x%02" PRIx8,
			    *next);
			next += 1;
			break;

		case SDP_DATA_INT16:
			if (next + 2 > end)
				return false;

			_sdp_put(indent, "int16", "%" PRId16,
			    (int16_t)be16dec(next));
			next += 2;
			break;

		case SDP_DATA_UINT16:
			if (next + 2 > end)
				return false;

			_sdp_put(indent, "uint16", "0x%04" PRIx16,
			    be16dec(next));
			next += 2;
			break;

		case SDP_DATA_UUID16:
			if (next + 2 > end)
				return false;

			_sdp_put(indent, "uuid16", "0x%04" PRIx16,
			    be16dec(next));
			next += 2;
			break;

		case SDP_DATA_INT32:
			if (next + 4 > end)
				return false;

			_sdp_put(indent, "int32", "%" PRId32,
			    (int32_t)be32dec(next));
			next += 4;
			break;

		case SDP_DATA_UINT32:
			if (next + 4 > end)
				return false;

			_sdp_put(indent, "uint32", "0x%08" PRIx32,
			    be32dec(next));
			next += 4;
			break;

		case SDP_DATA_UUID32:
			if (next + 4 > end)
				return false;

			_sdp_put(indent, "uuid32", "0x%08" PRIx32,
			    be32dec(next));
			next += 4;
			break;

		case SDP_DATA_INT64:
			if (next + 8 > end)
				return false;

			_sdp_put(indent, "int64", "%" PRId64,
			    (int64_t)be64dec(next));
			next += 8;
			break;

		case SDP_DATA_UINT64:
			if (next + 8 > end)
				return false;

			_sdp_put(indent, "uint64", "0x%016" PRIx64,
			    be64dec(next));
			next += 8;
			break;

		case SDP_DATA_INT128:
			if (next + 16 > end)
				return false;

			_sdp_put(indent, "int128",
				"0x%02x%02x%02x%02x%02x%02x%02x%02x"
				"%02x%02x%02x%02x%02x%02x%02x%02x",
				next[0], next[1], next[2], next[3],
				next[4], next[5], next[6], next[7],
				next[8], next[9], next[10], next[11],
				next[12], next[13], next[14], next[15]);
			next += 16;
			break;

		case SDP_DATA_UINT128:
			if (next + 16 > end)
				return false;

			_sdp_put(indent, "uint128",
				"0x%02x%02x%02x%02x%02x%02x%02x%02x"
				"%02x%02x%02x%02x%02x%02x%02x%02x",
				next[0], next[1], next[2], next[3],
				next[4], next[5], next[6], next[7],
				next[8], next[9], next[10], next[11],
				next[12], next[13], next[14], next[15]);
			next += 16;
			break;

		case SDP_DATA_UUID128:
			if (next + 16 > end)
				return false;

			_sdp_put(indent, "uuid128",
				"%02x%02x%02x%02x-"
				"%02x%02x-"
				"%02x%02x-"
				"%02x%02x-"
				"%02x%02x%02x%02x%02x%02x",
				next[0], next[1], next[2], next[3],
				next[4], next[5],
				next[6], next[7],
				next[8], next[9],
				next[10], next[11], next[12],
				next[13], next[14], next[15]);
			next += 16;
			break;

		case SDP_DATA_STR8:
			if (next + 1 > end)
				return false;

			len = *next;
			next += 1;

			if (next + len > end)
				return false;

			_sdp_putstr(indent, VIS_CSTYLE, "str8", next, len);
			next += len;
			break;

		case SDP_DATA_URL8:
			if (next + 1 > end)
				return false;

			len = *next;
			next += 1;

			if (next + len > end)
				return false;

			_sdp_putstr(indent, VIS_HTTPSTYLE, "url8", next, len);
			next += len;
			break;

		case SDP_DATA_STR16:
			if (next + 2 > end)
				return false;

			len = be16dec(next);
			next += 2;

			if (next + len > end)
				return false;

			_sdp_putstr(indent, VIS_CSTYLE, "str16", next, len);
			next += len;
			break;

		case SDP_DATA_URL16:
			if (next + 2 > end)
				return false;

			len = be16dec(next);
			next += 2;

			if (next + len > end)
				return false;

			_sdp_putstr(indent, VIS_HTTPSTYLE, "url16", next, len);
			next += len;
			break;

		case SDP_DATA_STR32:
			if (next + 4 > end)
				return false;

			len = be32dec(next);
			next += 4;

			if (next + len > end)
				return false;

			_sdp_putstr(indent, VIS_CSTYLE, "str32", next, len);
			next += len;
			break;

		case SDP_DATA_URL32:
			if (next + 4 > end)
				return false;

			len = be32dec(next);
			next += 4;

			if (next + len > end)
				return false;

			_sdp_putstr(indent, VIS_HTTPSTYLE, "url32", next, len);
			next += len;
			break;

		case SDP_DATA_SEQ8:
			if (next + 1 > end)
				return false;

			len = *next;
			next += 1;

			if (next + len > end)
				return false;

			printf("%*sseq8(%zu)\n", indent, "", len);
			if (!_sdp_data_print(next, next + len, indent + 1))
				return false;

			next += len;
			break;

		case SDP_DATA_ALT8:
			if (next + 1 > end)
				return false;

			len = *next;
			next += 1;

			if (next + len > end)
				return false;

			printf("%*salt8(%zu)\n", indent, "", len);
			if (!_sdp_data_print(next, next + len, indent + 1))
				return false;

			next += len;
			break;

		case SDP_DATA_SEQ16:
			if (next + 2 > end)
				return false;

			len = be16dec(next);
			next += 2;

			if (next + len > end)
				return false;

			printf("%*sseq16(%zu)\n", indent, "", len);
			if (!_sdp_data_print(next, next + len, indent + 1))
				return false;

			next += len;
			break;

		case SDP_DATA_ALT16:
			if (next + 2 > end)
				return false;

			len = be16dec(next);
			next += 2;

			if (next + len > end)
				return false;

			printf("%*salt16(%zu)\n", indent, "", len);
			if (!_sdp_data_print(next, next + len, indent + 1))
				return false;

			next += len;
			break;

		case SDP_DATA_SEQ32:
			if (next + 4 > end)
				return false;

			len = be32dec(next);
			next += 4;

			if (next + len > end)
				return false;

			printf("%*sseq32(%zu)\n", indent, "", len);
			if (!_sdp_data_print(next, next + len, indent + 1))
				return false;

			next += len;
			break;

		case SDP_DATA_ALT32:
			if (next + 4 > end)
				return false;

			len = be32dec(next);
			next += 4;

			if (next + len > end)
				return false;

			printf("%*salt32(%zu)\n", indent, "", len);
			if (!_sdp_data_print(next, next + len, indent + 1))
				return false;

			next += len;
			break;

		default:
			return false;
		}
	}

	return true;
}

void
sdp_data_print(const sdp_data_t *data, int indent)
{

	if (!_sdp_data_print(data->next, data->end, indent))
		printf("SDP data error\n");
}
