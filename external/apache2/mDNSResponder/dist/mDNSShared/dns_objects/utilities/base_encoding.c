/*
 * Copyright (c) 2021-2024 Apple Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "mDNSFeatures.h"
#if MDNSRESPONDER_SUPPORTS(APPLE, DNSSECv2)

//======================================================================================================================
// MARK: - Headers

#include "dns_obj_log.h"
#include "base_encoding.h"
#include "dns_common.h"
#include <string.h>	// For memcpy().

#include "dns_assert_macros.h"
#include "mdns_strict.h"

//======================================================================================================================
// MARK: - Constants

static const char b64_table[] = {
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
	'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
	'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
	'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
	'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
	'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
	'w', 'x', 'y', 'z', '0', '1', '2', '3',
	'4', '5', '6', '7', '8', '9', '+', '/'
};

static const char b32_hex_table[] = {
	'0', '1', '2', '3', '4', '5', '6', '7',
	'8', '9', 'A', 'B', 'C', 'D', 'E', 'F',
	'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
	'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V'
};

#define MAX_LENGTH_B64_ENCODING_DATA (SIZE_MAX * 3 / 4)
#define MAX_LENGTH_B32_HEX_ENCODING_DATA (SIZE_MAX * 5 / 8)

//======================================================================================================================
// MARK: - Local Prototypes

static void
base_64_encode(const uint8_t * NONNULL data, size_t data_len, char * NONNULL out_encoded_str);

static void
base_32_hex_encode(const uint8_t * NONNULL data, size_t data_len, bool no_padding, char * NONNULL out_encoded_str);

//======================================================================================================================
// MARK: - Public Functions

char *
base_x_encode(const base_encoding_type_t type, const uint8_t * const data, const size_t data_len,
	char * const out_encoded_str_buf)
{
	const size_t encoded_str_len = base_x_get_encoded_string_length(type, data_len);

	char *encoded_str;
	if (out_encoded_str_buf == NULL) {
		encoded_str = mdns_malloc(encoded_str_len + 1);
		require_return_value(encoded_str != NULL, NULL);
	} else {
		encoded_str = out_encoded_str_buf;
	}

	encoded_str[encoded_str_len] = '\0';

	switch (type) {
		case base_encoding_type_base64:
			base_64_encode(data, data_len, encoded_str);
			break;
		case base_encoding_type_base32_hex_with_padding:
			base_32_hex_encode(data, data_len, false, encoded_str);
			break;
		case base_encoding_type_base32_hex_without_padding:
			base_32_hex_encode(data, data_len, true, encoded_str);
			break;
		MDNS_COVERED_SWITCH_DEFAULT:
			break;
	}

	return encoded_str;
}

//======================================================================================================================

size_t
base_x_get_encoded_string_length(const base_encoding_type_t type, const size_t data_len)
{
	size_t encoded_str_len;
	switch (type) {
		case base_encoding_type_base64:
			require_action(data_len < MAX_LENGTH_B64_ENCODING_DATA, exit, encoded_str_len = 0);
			encoded_str_len = (data_len + 2) / 3 * 4;
			break;
		case base_encoding_type_base32_hex_with_padding:
			require_action(data_len < MAX_LENGTH_B32_HEX_ENCODING_DATA, exit, encoded_str_len = 0);
			encoded_str_len = (data_len + 4) / 5 * 8;
			break;
		case base_encoding_type_base32_hex_without_padding:
			require_action(data_len < MAX_LENGTH_B32_HEX_ENCODING_DATA, exit, encoded_str_len = 0);
			encoded_str_len = data_len / 5 * 8;
			switch (data_len % 5) {
				case 0:
					// encoded_str_len += 0;
					break;
				case 1:
					encoded_str_len += 2;
					break;
				case 2:
					encoded_str_len += 4;
					break;
				case 3:
					encoded_str_len += 5;
					break;
				case 4:
					encoded_str_len += 7;
					break;
				MDNS_COVERED_SWITCH_DEFAULT:
					encoded_str_len = 0;
					break;
			}
			break;
		MDNS_COVERED_SWITCH_DEFAULT:
			encoded_str_len = 0;
			break;
	}

exit:
	return encoded_str_len;
}

//======================================================================================================================
// MARK: - Private Functions

static void
base_64_encode(const uint8_t * const data, const size_t data_len, char * const out_encoded_str)
{
	const uint8_t * data_ptr = data;
	const uint8_t * const data_ptr_limit = data_ptr + data_len;
	char * encoded_str_ptr = out_encoded_str;

	while (data_ptr < data_ptr_limit) {
		char encoded_buf[4];
		uint32_t encoded_size = 0;

		const size_t remain = (size_t)(data_ptr_limit - data_ptr);
		uint32_t quantum = 0;

		// Get 24 bits from 3 bytes.
		switch (remain) {
			default:
			case 3:	quantum |= (uint32_t)data_ptr[2];			// bits 16 - 23
			case 2:	quantum |= ((uint32_t)data_ptr[1]) << 8;	// bits 8 - 15
			case 1:	quantum |= ((uint32_t)data_ptr[0]) << 16;	// bits 0 - 7
		}

		// Advance the data pointer.
		data_ptr += MIN(remain, 3);

		// Convert 24 bits to 4 characters.
		switch (remain) {
			default:
			case 3:
				encoded_buf[3] = b64_table[quantum & 0x3F];
				encoded_size = 4;
			case 2:
				encoded_buf[2] = b64_table[(quantum >> 6) & 0x3F];
				if (encoded_size == 0) encoded_size = 3;
			case 1:
				encoded_buf[1] = b64_table[(quantum >> 12) & 0x3F];
				encoded_buf[0] = b64_table[(quantum >> 18) & 0x3F];
				if (encoded_size == 0) encoded_size = 2;
		}

		// Fill the padding with '='.
		for (size_t i = encoded_size; i < sizeof(encoded_buf); i++) {
			encoded_buf[i] = '=';
		}

		// Move the current encoded string chunk to the returned buffer.
		memcpy(encoded_str_ptr, encoded_buf, sizeof(encoded_buf));
		encoded_str_ptr += sizeof(encoded_buf);
	}
}

//======================================================================================================================

static void
base_32_hex_encode(const uint8_t * const data, const size_t data_len, const bool no_padding, char * const out_encoded_str)
{
	const uint8_t * data_ptr = data;
	const uint8_t * const data_ptr_limit = data_ptr + data_len;
	char * encoded_str_ptr = out_encoded_str;

	while (data_ptr < data_ptr_limit) {
		char encoded_buf[8];
		uint32_t encoded_size = 0;

		size_t remain = (size_t)(data_ptr_limit - data_ptr);
		uint64_t quantum = 0;

		// Get 40 bits from 8 bytes.
		switch (remain) {
			default:
			case 5: quantum |= (uint64_t)data_ptr[4];			// bits 32 - 39
			case 4: quantum |= ((uint64_t)data_ptr[3]) << 8;	// bits 24 - 32
			case 3: quantum |= ((uint64_t)data_ptr[2]) << 16;	// bits 16 - 23
			case 2: quantum |= ((uint64_t)data_ptr[1]) << 24;	// bits  8 - 15
			case 1: quantum |= ((uint64_t)data_ptr[0]) << 32;	// bits  0 -  7
		}

		// Advance the data pointer.
		data_ptr += MIN(remain, 5);

		// Convert 40 bits to 8 characters.
		switch (remain) {
			default:
			case 5:
				encoded_buf[7] = b32_hex_table[quantum & 0x1F];
				encoded_size = 8;
			case 4:
				encoded_buf[6] = b32_hex_table[(quantum >> 5) & 0x1F];
				encoded_buf[5] = b32_hex_table[(quantum >> 10) & 0x1F];
				if (encoded_size == 0) encoded_size = 7;
			case 3:
				encoded_buf[4] = b32_hex_table[(quantum >> 15) & 0x1F];
				if (encoded_size == 0) encoded_size = 5;
			case 2:
				encoded_buf[3] = b32_hex_table[(quantum >> 20) & 0x1F];
				encoded_buf[2] = b32_hex_table[(quantum >> 25) & 0x1F];
				if (encoded_size == 0) encoded_size = 4;
			case 1:
				encoded_buf[1] = b32_hex_table[(quantum >> 30) & 0x1F];
				encoded_buf[0] = b32_hex_table[(quantum >> 35) & 0x1F];
				if (encoded_size == 0) encoded_size = 2;
		}

		if (!no_padding) {
			// Fill the padding with '='.
			for (size_t i = encoded_size; i < sizeof(encoded_buf); i++) {
				encoded_buf[i] = '=';
			}

			encoded_size = sizeof(encoded_buf);
		}

		// Move the current encoded string chunk to the returned buffer.
		memcpy(encoded_str_ptr, encoded_buf, encoded_size);
		encoded_str_ptr += encoded_size;
	}
}

#else

extern int _this_declaration_avoids_iso_c_empty_translation_unit_warning;

#endif // MDNSRESPONDER_SUPPORTS(APPLE, DNSSECv2)
