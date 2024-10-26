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

#ifndef DOMAIN_NAME_LABELS_H
#define DOMAIN_NAME_LABELS_H

//======================================================================================================================
// MARK: - Headers

#include "dns_common.h"
#include <stdlib.h>	// For size_t.
#include <stdint.h>
#include <stdbool.h>

#include "nullability.h"

//======================================================================================================================
// MARK: - Function Declarations

/*!
 *	@brief
 *		Create a new uncompressed domain name labels as a copy of an existing uncompressed domain name labels.
 *
 *	@param labels
 *		The uncompressed domain name labels.
 *
 *	@param to_lower_case
 *		The boolean value to indicate whether to convert all alphabets in <code>labels</code> to lower case characters.
 *
 *	@param out_error
 *		The pointer to the error value indicates the success of the function call or the error encountered.
 *
 *	@result
 *		The domain name labels object created, or NULL if error happens during creation. <code>out_error</code> will be set to the error encountered if it is not
 *		NULL.
 */
uint8_t * NULLABLE
domain_name_labels_create(const uint8_t * NONNULL labels, bool to_lower_case, dns_obj_error_t * NULLABLE out_error);

/*!
 *	@brief
 *		Get the length of the uncompressed domain name labels
 *
 *	@param labels
 *		The uncompressed domain name labels.
 *
 *	@result
 *		The length of the domain name labels.
 */
size_t
domain_name_labels_length(const uint8_t * NONNULL labels);

/*!
 *	@brief
 *		Get the length of the uncompressed domain name labels, but do not check the bytes over <code>limit</code>
 *
 *	@param labels
 *		The uncompressed domain name labels.
 *
 *	@param limit
 *		The bytes checking limit.
 *
 *	@result
 *		The length of the domain name labels or 0 if the domain name labels runs over <code>limit</code>.
 */
size_t
domain_name_labels_length_with_limit(const uint8_t * NONNULL labels, const uint8_t * NONNULL limit);

/*!
 *	@brief
 *		Get the number of label in the domain name labels.
 *
 *	@param labels
 *		The domain name labels to count.
 *
 *	@result
 *		 The number of domain name label.
 */
size_t
domain_name_labels_count_label(const uint8_t * NONNULL labels);

/*!
 *	@brief
 *		Get the parent domain of the domain name labels, the level of the parent is specified by <code>index</code>.
 *
 *	@param labels
 *		The domain name labels to iterate.
 *
 *	@param index
 *		The level of the parent domain, for example, for domain name labels: www.apple.com.
 *		index 0 means www.apple.com.
 *		index 1 means apple.com.
 *		index 2 means com.
 *		index 3 means <root>.
 *		index 4 or more means nothing.
 *
 *	@result
 *		The parent domain name labels if index is less than or equal to <code>domain_name_labels_count_label(labels)</code>, otherwise, NULL.
 */
const uint8_t * NULLABLE
domain_name_labels_get_parent(const uint8_t * NONNULL labels, size_t index);

/*!
 *	@brief
 *		Get the closest common ancestor of <code>labels_1</code> and <code>labels_2</code>.
 *
 *	@param labels_1
 *		The domain name labels.
 *
 *	@param labels_2
 *		The domain name labels.
 *
 *	@result
 *		The closest common ancestor of two domain name labels.
 *
 *	@discussion
 *		The closest common ancestor of two domain names is the longest name matching of them starting from the root.
 *		The returned domain name labels is guaranteed to be valid if <code>labels_1</code> is valid.
 */
const uint8_t * NONNULL
domain_name_labels_get_closest_common_ancestor(const uint8_t * NONNULL labels_1, const uint8_t * NONNULL labels_2);

/*!
 *	@brief
 *		Check if <code>sub_labels</code> is a sub labels of <code>labels</code>.
 *
 *	@param sub_labels
 *		The child labels.
 *
 *	@param labels
 *		The parent labels.
 *
 *	@result
 *		True if <code>sub_labels</code> is a sub labels of <code>labels</code>, otherwise, false.
 *
 *	@discussion
 *		If the <code>sub_labels</code> has to be the real subdomain labels of <code>labels</code> to get true result. If the two labels are equal,  the result
 *		would be false since domain name labels cannot be the subdomain of itself.
 */
bool
domain_name_labels_is_sub_labels_of(const uint8_t * NONNULL sub_labels, const uint8_t * NONNULL labels);

/*!
 *	@brief
 *		Check if the domain name labels is a root label.
 *
 *	@param labels
 *		The domain name labels.
 *
 *	@result
 *		True if it is root label, otherwise, false.
 */
bool
domain_name_labels_is_root(const uint8_t * NONNULL labels);

/*!
 *	@brief
 *		Compare the domain name label canonically.
 *
 *	@param label1
 *		The uncompressed domain name label.
 *
 *	@param label2
 *		The uncompressed domain name label.
 *
 *	@result
 *		<code>compare_result_less</code> if <code>label1</code> is less than <code>label2</code>.
 *		<code>compare_result_equal</code> if <code>label1</code> is equal to <code>label2</code>.
 *		<code>compare_result_greater</code> if <code>label1</code> is greater than <code>label2</code>.
 *		<code>compare_result_notequal</code> if <code>check_equality_only</code> is true, and <code>label1</code> is not equal to <code>label2</code>.
 */
compare_result_t
domain_name_label_canonical_compare(const uint8_t * NONNULL label1, const uint8_t * NONNULL label2, bool check_equality_only);

/*!
 *	@brief
 *		Compare the domain name labels canonically.
 *
 *	@param labels1
 *		The uncompressed domain name labels.
 *
 *	@param labels2
 *		The uncompressed domain name labels.
 *
 *	@result
 *		<code>compare_result_less</code> if <code>labels1</code> is less than <code>labels2</code>.
 *		<code>compare_result_equal</code> if <code>labels1</code> is equal to <code>labels2</code>.
 *		<code>compare_result_greater</code> if <code>labels1</code> is greater than <code>labels2</code>.
 *		<code>compare_result_notequal</code> if <code>check_equality_only</code> is true, and <code>labels1</code> is not equal to <code>labels2</code>.
 */
compare_result_t
domain_name_labels_canonical_compare(const uint8_t * NONNULL labels1, const uint8_t * NONNULL labels2, bool check_equality_only);

/*!
 *	@brief
 *		Convert the domain name labels to all lower case.
 *
 *	@param labels
 *		The domain name labels to convert.
 */
void
domain_name_labels_to_lower_case(uint8_t * NONNULL labels);

/*!
 *	@brief
 *		Check if the domain name labels contains any upper case character.
 *
 *	@param labels
 *		The domain name labels to check.
 *
 *	@result
 *		True if the domain name labels contains upper case character. Otherwise, false.
 */
bool
domain_name_labels_contains_upper_case(const uint8_t * NONNULL labels);

/*!
 *	@brief
 *		Compute the hash for the current domain name labels.
 *
 *	@param labels
 *		The domain name labels to compute the hash value.
 *
 *	@result
 *		The hash value.
 *
 *	@discussion
 *		Currently the FNV-1a hash function is used to generate the hash, See
 *		<https://tools.ietf.org/html/draft-eastlake-fnv-17.html#section-2> for the details of  FNV-1a hash function .
 */
uint32_t
domain_name_labels_compute_hash(const uint8_t * NONNULL labels);

/*!
 *	@brief
 *		Concatenate two domain name labels together to form a new domain name labels.
 *
 *	@param front_labels
 *		The left part of the new domain name labels.
 *
 *	@param end_labels
 *		The right part of the new domain name labels.
 *
 *	@param out_concatenation
 *		The pointer to the domain name labels buffer that saves the newly concatenated domain name labels.
 *
 *	@param max_concatenation_len
 *		The maximum size of the output buffer.
 *
 *	@param out_err
 *		The pointer to the error value indicates the success of the function call or the error encountered.
 */
uint8_t * NULLABLE
domain_name_labels_concatenate(const uint8_t * NONNULL front_labels, const uint8_t * NONNULL end_labels,
	uint8_t * NONNULL out_concatenation, size_t max_concatenation_len, dns_obj_error_t * NULLABLE out_err);

/*!
 *	@brief
 *		Convert DNS domain name labels into domain name C string.
 *
 *	@param labels
 *		The domain name labels.
 *
 *	@param labels_len
 *		the length of the domain name labels to be converted.
 *
 *	@param out_name_cstring
 *		The output buffer that will be used to store the converted domain name C string. It has to be no less than <code>MAX_ESCAPED_DOMAIN_NAME</code>,
 *		or the result will be undefined.
 *
 *	@result
 *		The error value indicates the success of the function call or the error encountered. If the value is <code>DNSSEC_ERROR_NO_ERROR</code>,
 *		<code>out_name_cstring</code> will be filled with the converted domain name C string.
 */
dns_obj_error_t
domain_name_labels_to_cstring(const uint8_t * NONNULL labels, size_t labels_len, char out_name_cstring[static MAX_ESCAPED_DOMAIN_NAME]);

/*!
 *	@brief
 *		Convert the domain name C string to DNS domain name labels.
 *
 *	@param name_cstring
 *		The domain name C string.
 *
 *	@param out_labels
 *		The output buffer that will be used to store the converted domain name labels. It has to be longer than <code>MAX_DOMAIN_NAME</code>, or the result
 *		will be undefined.
 *
 *	@param out_end
 *		if <code>out_end</code> is non-null, then the pointer to the end of the converted domain name labels will be put inside.
 *
 *	@result
 *		The error value indicates the success of the function call or the error encountered. If the value is <code>DNSSEC_ERROR_NO_ERROR</code>,
 *		<code>out_labels</code> will be filled with the converted domain name labels. if <code>out_end</code> is non-null, then the pointer to the end of the
 *		converted domain name labels will be put inside.
 */
dns_obj_error_t
cstring_to_domain_name_labels(const char * NONNULL name_cstring, uint8_t out_labels[static MAX_DOMAIN_NAME], uint8_t * NULLABLE * NULLABLE out_end);

/*!
 *	@brief
 *		Append the domain label(s) C string to the original domain name labels.
 *
 *	@param out_labels
 *		The original domain name labels, the domain name labels after appending will also be here.
 *
 *	@param name_cstring
 *		The domain name label(s) in C string format that will be appended to <code>out_labels</code>.
 *
 *	@param out_end
 *		if <code>out_end</code> is non-null, then the pointer to the end of the appended domain name labels will be put inside.
 *
 *	@result
 *		The error value indicates the success of the function call or the error encountered. If the value is <code>DNSSEC_ERROR_NO_ERROR</code>,
 *		<code>out_labels</code> will be filled with the appended domain name labels. if <code>out_end</code> is non-null, then the pointer to the end of the
 *		domain name labels will be put inside.
 */
dns_obj_error_t
domain_name_labels_append_cstring(uint8_t out_labels[static MAX_DOMAIN_NAME], const char * NONNULL name_cstring, uint8_t * NULLABLE * NULLABLE out_end);

#endif // DOMAIN_NAME_LABELS_H
