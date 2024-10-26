/*
 * Copyright (c) 2021-2022 Apple Inc. All rights reserved.
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


//======================================================================================================================
// MARK: - Headers

#include "dns_obj_log.h"
#include "domain_name_labels.h"
#include <string.h> // For memcpy().

#include "dns_assert_macros.h"
#include "mdns_strict.h"

//======================================================================================================================
// MARK: - Macros

#define _IS_UPPER_CASE_ALPHABET(C) (((C) >= 'A') && ((C) <= 'Z'))
#define _TO_LOWER_CASE_ALPHABET(C) (_IS_UPPER_CASE_ALPHABET(C) ? ((C) + ('a' - 'A')) : (C))

//======================================================================================================================
// MARK: - Local Prototypes

size_t
_domain_name_labels_length_with_limit(const uint8_t * NONNULL labels, const uint8_t * NULLABLE limit);

//======================================================================================================================
// MARK: - Public Functions

uint8_t *
domain_name_labels_create(const uint8_t * const labels, const bool to_lower_case, dns_obj_error_t * const out_error)
{
	dns_obj_error_t err;
	uint8_t *new_name;

	size_t len = domain_name_labels_length(labels);
	require_action(len <= MAX_DOMAIN_NAME, exit, new_name = NULL; err = DNS_OBJ_ERROR_OVER_RUN);

	new_name = mdns_malloc(len);
	require_action(new_name != NULL, exit, err = DNS_OBJ_ERROR_NO_MEMORY);

	memcpy(new_name, labels, len);
	if (to_lower_case) {
		domain_name_labels_to_lower_case(new_name);
	}
	err = DNS_OBJ_ERROR_NO_ERROR;

exit:
	if (out_error != NULL) {
		*out_error = err;
	}
	return new_name;
}

//======================================================================================================================

size_t
domain_name_labels_length(const uint8_t * const labels)
{
	return _domain_name_labels_length_with_limit(labels, NULL);
}

//======================================================================================================================

size_t
domain_name_labels_length_with_limit(const uint8_t * const labels, const uint8_t * const limit)
{
	return _domain_name_labels_length_with_limit(labels, limit);
}

//======================================================================================================================

size_t
domain_name_labels_count_label(const uint8_t * const labels)
{
	const uint8_t *label;
	uint32_t label_len;

	// Not count root label.
	size_t label_count = 0;
	for (label = labels; (label_len = *label) != 0; label = &label[label_len + 1]) {
		label_count++;
	}

	return label_count;
}

//======================================================================================================================

const uint8_t * NULLABLE
domain_name_labels_get_parent(const uint8_t * const labels, const size_t index)
{
	const size_t label_count = domain_name_labels_count_label(labels);
	require_return_value(index <= label_count, NULL);

	size_t index_to_count = index;
	const uint8_t *label;
	uint32_t label_len;

	for (label = labels; (label_len = *label) != 0 && index_to_count > 0; label = &label[label_len + 1]) {
		index_to_count--;
	}

	return label;
}

//======================================================================================================================

const uint8_t *
domain_name_labels_get_closest_common_ancestor(const uint8_t * const labels_1, const uint8_t * const labels_2)
{
	if (labels_1 == labels_2) {
		return labels_1;
	}

	const size_t labels_1_label_count = domain_name_labels_count_label(labels_1);
	const size_t labels_2_label_count = domain_name_labels_count_label(labels_2);

	const uint8_t * closest_common_ancestor_labels = labels_1 + domain_name_labels_length(labels_1) - 1;
	for (size_t i = 1; i <= labels_1_label_count && i <= labels_2_label_count; i++) {
		const uint8_t * const labels_1_current_label = domain_name_labels_get_parent(labels_1, labels_1_label_count - i);
		const uint8_t * const labels_2_current_label = domain_name_labels_get_parent(labels_2, labels_2_label_count - i);

		const bool label_equal = (domain_name_label_canonical_compare(labels_1_current_label, labels_2_current_label, true) == compare_result_equal);
		if (!label_equal) {
			break;
		}

		closest_common_ancestor_labels = labels_1_current_label;
	}

	return closest_common_ancestor_labels;
}

//======================================================================================================================

bool
domain_name_labels_is_sub_labels_of(const uint8_t * NONNULL sub_labels, const uint8_t * NONNULL labels)
{
	const size_t sub_labels_label_count = domain_name_labels_count_label(sub_labels);
	const size_t labels_label_count = domain_name_labels_count_label(labels);

	if (sub_labels_label_count <= labels_label_count) {
		return false;
	}
	require_return_value(sub_labels_label_count - labels_label_count <= UINT8_MAX, false);

	const uint8_t parent_labels_index = (uint8_t)(sub_labels_label_count - labels_label_count);
	const uint8_t * const parent_labels = domain_name_labels_get_parent(sub_labels, parent_labels_index);
	require_return_value(parent_labels != NULL, false);

	return domain_name_labels_canonical_compare(parent_labels, labels, true) == compare_result_equal;
}

//======================================================================================================================

bool
domain_name_labels_is_root(const uint8_t * const labels)
{
	return labels[0] == 0;
}

//======================================================================================================================

compare_result_t
domain_name_label_canonical_compare(const uint8_t * const label1, const uint8_t * const label2, const bool check_equality_only)
{
	compare_result_t result;
	const uint8_t label1_len = *label1;
	const uint8_t label2_len = *label2;
	if (check_equality_only && label1_len != label2_len) {
		result = compare_result_notequal;
		goto exit;
	}

	const uint8_t length_to_compare = MIN(label1_len, label2_len);
	const uint8_t * const label1_bytes = label1 + 1;
	const uint8_t * const label2_bytes = label2 + 1;

	for (uint32_t i = 0; i < length_to_compare; i++) {
		const uint8_t ch1 = _TO_LOWER_CASE_ALPHABET(label1_bytes[i]);
		const uint8_t ch2 = _TO_LOWER_CASE_ALPHABET(label2_bytes[i]);
		if (ch1 < ch2) {
			result = compare_result_less;
			goto exit;
		} else if (ch1 > ch2) {
			result = compare_result_greater;
			goto exit;
		}
	}

	if (label1_len < label2_len) {
		result = compare_result_less;
	} else if (label1_len > label2_len) {
		result = compare_result_greater;
	} else {
		result = compare_result_equal;
	}

	if (check_equality_only && result != compare_result_equal && result != compare_result_unknown) {
		result = compare_result_notequal;
	}

exit:
	return result;
}

//======================================================================================================================

compare_result_t
domain_name_labels_canonical_compare(const uint8_t * const labels1, const uint8_t * const labels2, const bool check_equality_only)
{
	compare_result_t result;
	const size_t labels1_len = domain_name_labels_length(labels1);
	const size_t labels2_len = domain_name_labels_length(labels2);
	require_action(labels1_len <= MAX_DOMAIN_NAME && labels2_len <= MAX_DOMAIN_NAME, exit, result = compare_result_unknown);

	if (check_equality_only && labels1_len != labels2_len) {
		result = compare_result_notequal;
		goto exit;
	}

	const uint8_t * const labels1_limit = labels1 + labels1_len;
	const uint8_t * const labels2_limit = labels2 + labels2_len;

	const uint8_t *labels1_labels_len_ptrs[MAX_DOMAIN_NAME];
	const uint8_t *labels2_labels_len_ptrs[MAX_DOMAIN_NAME];
	uint32_t labels1_label_count = 0;
	uint32_t labels2_label_count = 0;

	for (const uint8_t *ptr = labels1; ptr < labels1_limit; ptr += 1 + *ptr) {
		labels1_labels_len_ptrs[labels1_label_count++] = ptr;
	}

	for (const uint8_t *ptr = labels2; ptr < labels2_limit; ptr += 1 + *ptr) {
		labels2_labels_len_ptrs[labels2_label_count++] = ptr;
	}

	if (check_equality_only) {
		if (labels1_label_count != labels2_label_count) {
			result = compare_result_notequal;
			goto exit;
		}

		for (uint32_t i = 0; i < labels1_label_count; i++) {
			const uint8_t labels1_label_len = *labels1_labels_len_ptrs[i];
			const uint8_t labels2_label_len = *labels2_labels_len_ptrs[i];
			if (labels1_label_len != labels2_label_len) {
				result = compare_result_notequal;
				goto exit;
			}
		}
	}

	while (labels1_label_count > 0 && labels2_label_count > 0) {
		const uint8_t * const labels1_label = labels1_labels_len_ptrs[labels1_label_count - 1];
		const uint8_t * const labels2_label = labels2_labels_len_ptrs[labels2_label_count - 1];

		compare_result_t label_result = domain_name_label_canonical_compare(labels1_label, labels2_label, check_equality_only);
		if (label_result != compare_result_equal) {
			result = label_result;
			goto exit;
		}

		labels1_label_count--;
		labels2_label_count--;
	}

	if (labels1_label_count == 0 && labels2_label_count == 0) {
		result = compare_result_equal;
	} else if (labels1_label_count == 0) {
		// labels2_label_count > 0
		result = compare_result_less;
	} else if (labels2_label_count == 0) {
		// labels1_label_count > 0
		result = compare_result_greater;
	} else {
		// Should never happen.
		result = compare_result_unknown;
	}

exit:
	if (check_equality_only && result != compare_result_equal && result != compare_result_unknown) {
		result = compare_result_notequal;
	}
	return result;
}

//======================================================================================================================

void
domain_name_labels_to_lower_case(uint8_t * const name)
{
	uint8_t *ptr;
	int32_t label_len;

	for (ptr = name, label_len = *ptr; label_len != 0; label_len = *ptr) {
		ptr++;
		while (label_len > 0) {
			label_len--;
			if (_IS_UPPER_CASE_ALPHABET(*ptr)) {
				*ptr += 'a' - 'A';
			}
			ptr++;
		}
	}
}

//======================================================================================================================

bool
domain_name_labels_contains_upper_case(const uint8_t * const name)
{
	const uint8_t *ptr;
	int32_t label_len;

	for (ptr = name, label_len = *ptr; label_len != 0; label_len = *ptr) {
		ptr++;
		while (label_len > 0) {
			label_len--;
			if (_IS_UPPER_CASE_ALPHABET(*ptr)) {
				return true;
			}
			ptr++;
		}
	}

	return false;
}

//======================================================================================================================
// The FNV-1a hash function is used to hash each octet of the domain name's labels, except that uppercase ASCII
// letters are treated as lowercase. See <https://tools.ietf.org/html/draft-eastlake-fnv-17.html#section-2> for the
// definition of the general FNV-1a hash function.

uint32_t
domain_name_labels_compute_hash(const uint8_t * const name)
{
	// FNV 32-bit constants. See <https://tools.ietf.org/html/draft-eastlake-fnv-17#section-5>.
	static const uint32_t fnv_32_bit_offset_basis = 0x811C9DC5;
	static const uint32_t fnv_32_bit_prime = 0x01000193;
	const size_t len = domain_name_labels_length(name);

	uint32_t hash = fnv_32_bit_offset_basis;
	for (size_t i = 0; i < len; i++) {
		hash ^= (uint8_t)_TO_LOWER_CASE_ALPHABET(name[i]);
		hash *= fnv_32_bit_prime;
	}

	return hash;
}

//======================================================================================================================

uint8_t *
domain_name_labels_concatenate(const uint8_t * const front_labels, const uint8_t * const end_labels,
	uint8_t * const out_concatenation, const size_t max_concatenation_len, dns_obj_error_t * const out_error)
{
	dns_obj_error_t err;
	uint8_t *concatenation = NULL;

	const size_t front_labels_len = domain_name_labels_length(front_labels);
	const size_t end_labels_len = domain_name_labels_length(end_labels);
	require_action(front_labels_len - 1 + end_labels_len <= max_concatenation_len, exit, err = DNS_OBJ_ERROR_OVER_RUN);

	memcpy(out_concatenation, front_labels, front_labels_len);
	memcpy(out_concatenation + front_labels_len - 1, end_labels, end_labels_len);
	concatenation = out_concatenation;
	err = DNS_OBJ_ERROR_NO_ERROR;

exit:
	if (out_error != NULL) {
		*out_error = err;
	}
	return concatenation;
}

//======================================================================================================================

#define _IS_PRINTABLE_ASCII(CH)	(((CH) >= 32) && ((CH) <= 126))

dns_obj_error_t
domain_name_labels_to_cstring(const uint8_t * const labels, const size_t labels_len,
	char out_name_cstring[static const MAX_ESCAPED_DOMAIN_NAME])
{
	dns_obj_error_t err;
	const uint8_t *label;
	uint8_t label_len;
	const uint8_t *next_label;
	char *dst;

	dst = out_name_cstring;
	const uint8_t * const limit = labels + labels_len;
	for (label = labels; (label_len = label[0]) != 0; label = next_label) {

		const uint8_t *src;

		require_action(label_len <= MAX_DOMAIN_LABEL, exit, err = DNS_OBJ_ERROR_MALFORMED_ERR);

		next_label = &label[1 + label_len];
		require_action((next_label - labels) < MAX_DOMAIN_NAME, exit, err = DNS_OBJ_ERROR_MALFORMED_ERR);
		require_action(next_label < limit, exit, err = DNS_OBJ_ERROR_UNDER_RUN);

		for (src = &label[1]; src < next_label; src++) {
			if (_IS_PRINTABLE_ASCII(*src)) {
				const char ch = (char) *src;
				if ((ch == '.') || (ch == '\\') || (ch == ' ')) {
					*dst++ = '\\';
				}
				*dst++ = ch;
			} else {
				*dst++ = '\\';
				*dst++ = '0' + (  *src / 100);
				*dst++ = '0' + (( *src /  10) % 10);
				*dst++ = '0' + (  *src        % 10);
			}
		}
		*dst++ = '.';
	}

	// At this point, label points to the root label.
	// If the root label was the only label, then write a dot for it.
	if (label == labels) {
		*dst++ = '.';
	}
	*dst = '\0';
	err = DNS_OBJ_ERROR_NO_ERROR;

exit:
	return err;
}

//======================================================================================================================

dns_obj_error_t
cstring_to_domain_name_labels(const char * const name_cstring, uint8_t out_labels[static const MAX_DOMAIN_NAME], uint8_t ** const out_end)
{
	out_labels[0] = 0;
	return domain_name_labels_append_cstring(out_labels, name_cstring, out_end);
}

//======================================================================================================================

dns_obj_error_t
domain_name_labels_append_cstring(uint8_t labels[static const MAX_DOMAIN_NAME], const char * const name_cstring, uint8_t ** const out_end)
{
	dns_obj_error_t err;
	const char * src;
	uint8_t * root;
	const uint8_t * const limit = labels + MAX_DOMAIN_NAME;

	for (root = labels; (root < limit) && (*root != 0); root += (1 + *root)) {
		// Left empty intentionally to iterate to the end of the domain name labels.
	}
	require_action(root < limit, exit, err = DNS_OBJ_ERROR_MALFORMED_ERR);

	src = name_cstring;
	if ((src[0] == '.') && (src[1] == '\0')) {
		++src;
	}

	while (*src != 0) {
		uint8_t * const label = root;
		const uint8_t * const label_limit = MIN(&label[1 + MAX_DOMAIN_LABEL], limit - 1);
		uint8_t *dst;
		int ch;
		size_t label_len;

		dst = &label[1];
		while (*src != 0 && ((ch = *src++) != '.')) {
			if (ch == '\\') {
				require_action(*src != '\0', exit, err = DNS_OBJ_ERROR_UNDER_RUN);
				ch = *src++;
				if (isdigit_safe(ch) && isdigit_safe(src[0]) && isdigit_safe(src[1])) {
					const int decimal = ((ch - '0') * 100) + ((src[0] - '0') * 10) + (src[1] - '0');
					if (decimal <= 255) {
						ch = decimal;
						src += 2;
					}
				}
			}
			require_action(dst < label_limit, exit, err = DNS_OBJ_ERROR_OVER_RUN);
			*dst++ = (uint8_t)ch;
		}

		label_len = (size_t)(dst - &label[1]);
		require_action(label_len > 0, exit, err = DNS_OBJ_ERROR_MALFORMED_ERR);

		label[0] = (uint8_t) label_len;
		root = dst;
		*root = 0;
	}
	if (out_end != NULL) {
		*out_end = root + 1;
	}
	err = DNS_OBJ_ERROR_NO_ERROR;

exit:
	return err;
}

//======================================================================================================================
// MARK: - Private Functions

size_t
_domain_name_labels_length_with_limit(const uint8_t * const labels, const uint8_t * const limit)
{
	const uint8_t * label;
	uint32_t label_len;

	for (label = labels; (label_len = *label) != 0; label = &label[label_len + 1]) {
		if (limit != NULL && label + 1 + label_len > limit) {
			return 0;
		}
	}

	return ((size_t)(label - labels) + 1);
}
