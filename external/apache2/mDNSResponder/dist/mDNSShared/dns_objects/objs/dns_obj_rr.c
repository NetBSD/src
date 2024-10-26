/*
 * Copyright (c) 2022 Apple Inc. All rights reserved.
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
#include "dns_obj_rr_private.h"
#include "domain_name_labels.h"
#include "dns_common.h"
#include <string.h>	// For memcmp().

#include "dns_assert_macros.h"
#include "mdns_strict.h"

//======================================================================================================================
// MARK: - DNS Resource Record Kind Definition

// The definition of the resource record object is in `dns_obj_rr_private.h`
DNS_OBJECT_DEFINE_FULL(rr);

//======================================================================================================================
// MARK: - Local Prototypes

static uint8_t *
dns_obj_rr_create_signed_data(dns_obj_rr_t NONNULL me, size_t * NONNULL signed_data_len);

//======================================================================================================================
// MARK: - DNSSEC Resource Record Public Methods

dns_obj_rr_t
dns_obj_rr_create(const uint8_t * const name, const uint16_t type, const uint16_t class,
	const uint8_t * const rdata, const uint16_t rdata_len, const bool allocate_memory, dns_obj_error_t * const out_error)
{
	dns_obj_error_t err;
	dns_obj_rr_t record = NULL;
	dns_obj_rr_t obj = NULL;

	obj = _dns_obj_rr_new();
	require_action(obj != NULL, exit, err = DNS_OBJ_ERROR_NO_MEMORY);

	dns_obj_rr_init_fields(obj, name, type, class, rdata, rdata_len, allocate_memory, NULL, &err);
	require_noerr(err, exit);

	record = obj;
	obj = NULL;
	err = DNS_OBJ_ERROR_NO_ERROR;

exit:
	if (out_error != NULL) {
		*out_error = err;
	}
	MDNS_DISPOSE_DNS_OBJ(obj);
	return record;
}

//======================================================================================================================

dns_obj_domain_name_t
dns_obj_rr_get_name(const dns_obj_rr_t me)
{
	return me->name;
}

//======================================================================================================================

const uint8_t *
dns_obj_rr_get_name_in_labels(const dns_obj_rr_t me)
{
	const dns_obj_domain_name_t name = me->name;
	return dns_obj_domain_name_get_labels(name);
}

//======================================================================================================================

uint16_t
dns_obj_rr_get_type(const dns_obj_rr_t me)
{
	return me->type;
}

//======================================================================================================================

uint16_t
dns_obj_rr_get_class(const dns_obj_rr_t me)
{
	return me->class;
}

//======================================================================================================================

uint16_t
dns_obj_rr_get_rdata_len(const dns_obj_rr_t me)
{
	return me->rdata_len;
}

//======================================================================================================================

const uint8_t *
dns_obj_rr_get_rdata(const dns_obj_rr_t me)
{
	if (!me->allocated_memory) {
		return me->rdata_u.const_rdata;
	} else {
		return me->rdata_u.allocated_rdata;
	}
}

//======================================================================================================================

uint32_t
dns_obj_rr_get_ttl(const dns_obj_rr_t me)
{
	return me->ttl;
}

//======================================================================================================================

const uint8_t *
dns_obj_rr_get_signed_data(const dns_obj_rr_t me)
{
	if (me->signed_data == NULL) {
		size_t signed_data_len;
		me->signed_data = dns_obj_rr_create_signed_data(me, &signed_data_len);
		if (me->signed_data != NULL) {
			me->signed_data_len = signed_data_len;
		}
	}

	return me->signed_data;
}

//======================================================================================================================

size_t
dns_obj_rr_get_signed_data_len(const dns_obj_rr_t me)
{
	// Call dns_obj_rr_get_signed_data() will also set signed_data_len.
	dns_obj_rr_get_signed_data(me);
	return me->signed_data_len;
}

//======================================================================================================================

char *
dns_obj_rr_copy_rdata_rfc_description(const dns_obj_rr_t me, dns_obj_error_t * const out_error)
{
	if (me->copy_rdata_rfc_description_method != NULL) {
		return me->copy_rdata_rfc_description_method(me, out_error);
	}

	// "65535 <Hex Dump>"
	dns_obj_error_t err;
	const size_t max_len = strlen("65535") + 1 + 2 * dns_obj_rr_get_rdata_len(me) + 1;
	char *buffer = mdns_calloc(1, max_len);
	require_action(buffer != NULL, exit, err = DNS_OBJ_ERROR_NO_MEMORY);

	char *ptr = buffer;
	const char * const limit = buffer + max_len;
	ptr += snprintf(ptr, (size_t)(limit - ptr), "%u ", dns_obj_rr_get_rdata_len(me));

	const char * const end = put_hex_from_bytes(dns_obj_rr_get_rdata(me), dns_obj_rr_get_rdata_len(me), ptr, (size_t)(limit - ptr));
	require_action(end != ptr, exit, err = DNS_OBJ_ERROR_OVER_RUN);
	err = DNS_OBJ_ERROR_NO_ERROR;

exit:
	if (out_error != NULL) {
		*out_error = err;
	}
	return buffer;
}

//======================================================================================================================

void
dns_obj_rr_clear_comparison_attributes(const dns_obj_rr_t me)
{
	me->original_ttl = 0;
	me->rrsig_labels = 0;
	me->signed_data_len = 0;
	mdns_free(me->signed_data);
}

//======================================================================================================================

void
dns_obj_rr_set_comparison_attributes(const dns_obj_rr_t me, const uint32_t original_ttl, const uint8_t rrsig_labels)
{
	me->original_ttl = original_ttl;
	me->rrsig_labels = rrsig_labels;
}

//======================================================================================================================

void
dns_obj_rrs_set_comparison_attributes(dns_obj_rr_t * const rrs, const size_t rr_count, const uint32_t original_ttl,
	const uint8_t rrsig_labels)
{
	for (size_t i = 0; i < rr_count; i++) {
		dns_obj_rr_set_comparison_attributes(rrs[i], original_ttl, rrsig_labels);
	}
}

//======================================================================================================================

void
dns_obj_rrs_clear_comparison_attributes(dns_obj_rr_t * const rrs, const size_t rr_count)
{
	for (size_t i = 0; i < rr_count; i++) {
		dns_obj_rr_clear_comparison_attributes(rrs[i]);
	}
}

//======================================================================================================================

bool
dns_obj_rr_equal_to_raw_data(const dns_obj_rr_t me, const uint8_t * const name, const uint16_t type,
	const uint16_t class, const uint8_t * const rdata, const uint16_t rdata_len)
{
	if (type != dns_obj_rr_get_type(me)) {
		return false;
	}

	if (class != dns_obj_rr_get_class(me)) {
		return false;
	}

	if (rdata_len != dns_obj_rr_get_rdata_len(me)) {
		return false;
	}

	if (domain_name_labels_canonical_compare(name, dns_obj_rr_get_name_in_labels(me), true) != compare_result_equal) {
		return false;
	}

	const bool points_to_same_memory = (rdata == dns_obj_rr_get_rdata(me));
	if (!points_to_same_memory && memcmp(rdata, dns_obj_rr_get_rdata(me), rdata_len) != 0) {
		return false;
	}

	return true;
}

//======================================================================================================================

bool
dns_obj_rrs_belong_to_one_rrset(dns_obj_rr_t * const rrs, const size_t count)
{
	require_return_value(count > 0, false);

	const uint16_t class = dns_obj_rr_get_class(rrs[0]);
	const uint16_t type = dns_obj_rr_get_type(rrs[0]);
	const dns_obj_domain_name_t name = dns_obj_rr_get_name(rrs[0]);

	for (size_t i = 1; i < count; i++) {
		const dns_obj_rr_t rr = rrs[i];
		if (dns_obj_rr_get_class(rr) != class) {
			return false;
		}
		if (dns_obj_rr_get_type(rr) != type) {
			return false;
		}
		if (!dns_obj_equal(dns_obj_rr_get_name(rr), name)) {
			return false;
		}
	}

	return true;
}

//======================================================================================================================
// MARK: - DNSSEC Resource Record Private Methods

static compare_result_t
_dns_obj_rr_compare(const dns_obj_rr_t me, const dns_obj_rr_t other, const bool check_equality_only)
{
	if (check_equality_only) {
		if (me->class != other->class) {
			return compare_result_notequal;
		}

		if (me->type != other->type) {
			return compare_result_notequal;
		}

		if (me->rdata_len != other->rdata_len) {
			return compare_result_notequal;
		}

		if (!dns_obj_equal(me->name, other->name)) {
			return compare_result_notequal;
		}

		const uint8_t * const my_data = dns_obj_rr_get_rdata(me);
		const uint8_t * const others_data = dns_obj_rr_get_rdata(other);

		const bool rdata_equal = (memcmp(my_data, others_data, me->rdata_len) == 0);

		return rdata_equal ? compare_result_equal : compare_result_notequal;
	} else {

		// <https://datatracker.ietf.org/doc/html/rfc4034#section-6.3> Canonical RR Ordering within an RRset:
		// For the purposes of DNS security, RRs with the same owner name, class, and type are sorted by treating the
		// RDATA portion of the canonical form of each RR as a left-justified unsigned octet sequence in which the
		// absence of an octet sorts before a zero octet.

		if (!dns_obj_equal(dns_obj_rr_get_name(me), dns_obj_rr_get_name(other))) {
			return compare_result_unknown;
		}

		if (dns_obj_rr_get_class(me) != dns_obj_rr_get_class(other)) {
			return compare_result_unknown;
		}

		if (dns_obj_rr_get_type(me) != dns_obj_rr_get_type(other)) {
			return compare_result_unknown;
		}

		const uint8_t * const my_rdata = dns_obj_rr_get_rdata(me);
		const size_t my_rdata_len = dns_obj_rr_get_rdata_len(me);
		const uint8_t * const others_rdata = dns_obj_rr_get_rdata(other);
		const size_t others_rdata_len = dns_obj_rr_get_rdata_len(other);
		const size_t len_to_compare = MIN(my_rdata_len, others_rdata_len);

		const int compare_result = memcmp(my_rdata, others_rdata, len_to_compare);
		if (compare_result < 0) {
			return compare_result_less;
		} else if (compare_result > 0) {
			return compare_result_greater;
		} else {
			// The absence of an octet sorts before a zero octet.
			if (my_rdata_len < others_rdata_len) {
				return compare_result_less;
			} else if (my_rdata_len > others_rdata_len) {
				return compare_result_greater;
			} else {
				return compare_result_equal;
			}
		}
	}
}

//======================================================================================================================

static void
_dns_obj_rr_finalize(const dns_obj_rr_t me)
{
	MDNS_DISPOSE_DNS_OBJ(me->name);
	if (me->allocated_memory) {
		mdns_free(me->rdata_u.allocated_rdata);
	}
	mdns_free(me->signed_data);
}

// =====================================================================================================================
// MARK: - Resource Record Project Private Functions

void
dns_obj_rr_init_fields(const dns_obj_rr_t me, const uint8_t * const name, const uint16_t type,
	const uint16_t class, const uint8_t * const rdata, const uint16_t rdata_len, const bool allocate_memory,
	const dns_obj_rr_copy_rdata_rfc_description_f copy_rdata_rfc_description_method, dns_obj_error_t * const out_error)
{
	dns_obj_error_t err;
	uint8_t *allocated_rdata = NULL;
	dns_obj_domain_name_t name_obj = NULL;

	name_obj = dns_obj_domain_name_create_with_labels(name, allocate_memory, &err);
	require_noerr(err, exit);

	me->allocated_memory = allocate_memory;
	if (!me->allocated_memory) {
		me->rdata_u.const_rdata = rdata;
	} else {
		if (rdata_len != 0) {
			require_action(rdata != NULL, exit, err = DNS_OBJ_ERROR_PARAM_ERR);
			allocated_rdata = mdns_malloc(rdata_len);
			require_action(allocated_rdata != NULL, exit, err = DNS_OBJ_ERROR_NO_MEMORY);
			memcpy(allocated_rdata, rdata, rdata_len);
		}
		me->rdata_u.allocated_rdata = allocated_rdata;
		allocated_rdata = NULL;
	}

	me->name = name_obj;
	name_obj = NULL;

	me->type = type;
	me->class = class;
	me->rdata_len = rdata_len;
	me->ttl = MAX_UNICAST_TTL_IN_SECONDS;
	me->copy_rdata_rfc_description_method = copy_rdata_rfc_description_method;
	err = DNS_OBJ_ERROR_NO_ERROR;

exit:
	if (out_error != NULL) {
		*out_error = err;
	}
	mdns_free(allocated_rdata);
	MDNS_DISPOSE_DNS_OBJ(name_obj);
}

//======================================================================================================================
// MARK: - Local Functions

static uint8_t *
dns_obj_rr_create_signed_data(const dns_obj_rr_t me, size_t * const out_signed_data_len)
{
	// RR(i) = name | type | class | OrigTTL | RDATA length | RDATA
	dns_obj_error_t err;
	dns_obj_domain_name_t name_to_sign = NULL;
	uint8_t *signed_data = NULL;

	require_return_value(me->original_ttl != 0, NULL);

	const dns_obj_domain_name_t name = dns_obj_rr_get_name(me);
	const uint16_t type = dns_obj_rr_get_type(me);
	const uint16_t class = dns_obj_rr_get_class(me);
	const uint16_t rdata_len = dns_obj_rr_get_rdata_len(me);
	const uint32_t original_ttl = me->original_ttl;

	// Get the name in canonical form.
	name_to_sign = dns_obj_domain_name_create_canonical(name, me->rrsig_labels, &err);
	require_noerr(err, exit);

	// Pre-calculate the length of the signed data.
	size_t signed_data_len = dns_obj_domain_name_get_length(name_to_sign);
	signed_data_len += sizeof(type);
	signed_data_len += sizeof(class);
	signed_data_len += sizeof(original_ttl);
	signed_data_len += sizeof(rdata_len);
	signed_data_len += rdata_len;

	signed_data = mdns_malloc(signed_data_len);
	require(signed_data != NULL, exit);
	*out_signed_data_len = signed_data_len;

	uint8_t *ptr = signed_data;

	// RR(i) += name
	memcpy(ptr, dns_obj_domain_name_get_labels(name_to_sign), dns_obj_domain_name_get_length(name_to_sign));
	ptr += dns_obj_domain_name_get_length(name_to_sign);

	// RR(i) += type
	put_uint16_to_bytes(type, &ptr);

	// RR(i) += class
	put_uint16_to_bytes(class, &ptr);

	// RR(i) += OrigTTL
	put_uint32_to_bytes(original_ttl, &ptr);

	// RR(i) += RDATA length
	put_uint16_to_bytes(rdata_len, &ptr);

	// RR(i) += RDATA
	memcpy(ptr, dns_obj_rr_get_rdata(me), rdata_len);

exit:
	MDNS_DISPOSE_DNS_OBJ(name_to_sign);
	return signed_data;
}
