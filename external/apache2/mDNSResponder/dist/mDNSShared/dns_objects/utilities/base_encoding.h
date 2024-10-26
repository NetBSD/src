/*
 * Copyright (c) 2021 Apple Inc. All rights reserved.
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

#ifndef BASE_ENCODING_H
#define BASE_ENCODING_H

//======================================================================================================================
// MARK: - Headers

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "nullability.h"

//======================================================================================================================
// MARK: - Base Encoding Type

typedef enum base_encoding_type {
	base_encoding_type_base64,						// Base 64 encoding.
	base_encoding_type_base32_hex_with_padding,		// Base 32 HEX encoding.
	base_encoding_type_base32_hex_without_padding,	// Base 32 HEX encoding used by NSEC3.
} base_encoding_type_t;

//======================================================================================================================
// MARK: - Macros

#define BASE32_HEX_OUTPUT_SIZE(INPUT_SIZE) (((INPUT_SIZE) + 4) / 5 * 8)

//======================================================================================================================
// MARK: - Function Declarations

/*!
 *	@brief
 *		Encode the binary bytes to text by using the specified schemes.
 *
 *	@param type
 *		The binary bytes to text schemes:
 *		base_encoding_type_base64 for the base 64 encoding,
 *		base_encoding_type_base32_hex_with_padding for the normal base 32 hex encoding.
 *		base_encoding_type_base32_hex_without_padding for the base 32 hex encoding that does not include the padding.
 *
 *	@param data
 *		The binary data to be encoded.
 *
 *	@param data_len
 *		The length of the binary data to be encoded.
 *
 *	@param out_encoded_str_buf
 *		The output buffer to hold the encoding result, if it is not NULL. Otherwise, a memory will be allocated to hold the encoding result.
 *
 *	@result
 *		The encoded text if no memory failure occurs. Otherwise, NULL.
 */
char * NULLABLE
base_x_encode(base_encoding_type_t type, const uint8_t * NONNULL data, size_t data_len,
	char * NULLABLE out_encoded_str_buf);

/*!
 *	@brief
 *		Get the length of the encoded text for a given binary bytes to text schemes.
 *
 *	@param type
 *		The binary bytes to text schemes:
 *		base_encoding_type_base64 for the base 64 encoding,
 *		base_encoding_type_base32_hex_with_padding for the normal base 32 hex encoding.
 *		base_encoding_type_base32_hex_without_padding for the base 32 hex encoding that does not include the padding.
 *
 *	@param data_len
 *		The length of the binary data to be encoded.
 *
 *	@result
 *		The length of the encoded text if the encoding text length is less than or equal to the maximum size of <code>size_t</code>.
 */
size_t
base_x_get_encoded_string_length(base_encoding_type_t type, size_t data_len);

#endif // BASE_ENCODING_H
