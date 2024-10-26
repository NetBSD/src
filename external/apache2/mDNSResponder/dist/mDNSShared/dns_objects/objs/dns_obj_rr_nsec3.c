/*
 * Copyright (c) 2022-2023 Apple Inc. All rights reserved.
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
#include "dns_obj_rr_nsec3.h"
#include "dns_obj_rr_private.h"
#include "domain_name_labels.h"
#include "rdata_parser.h"
#include "base_encoding.h"

#include "dns_assert_macros.h"
#include "mdns_strict.h"

//======================================================================================================================
// MARK: - DNSSEC NSEC3 Resource Record Kind Definition

struct dns_obj_rr_nsec3_s {
	struct dns_obj_rr_s				base;					// The reference count and kind support base.
	dns_obj_domain_name_t			next_hashed_owner_name;	// The domain name object of the next owner name.
	bool							last_nsec3;				// If this NSEC3 record is the last one in the zone.
};

// dns_obj_rr_nsec3_t is a subkind of dns_obj_rr_t, and it always have DNS type: kDNSRecordType_NSEC3.
DNS_OBJECT_SUBKIND_DEFINE_FULL(rr, nsec3,
	.rr_type = kDNSRecordType_NSEC3,
	.copy_rdata_rfc_description_method = NULL
);

//======================================================================================================================
// MARK: - DNSSEC NSEC3 Resource Record Local Prototypes

static void
dns_obj_rr_nsec3_init_fields(dns_obj_rr_nsec3_t NONNULL me, const uint8_t * NONNULL name,
	const uint8_t * NONNULL rdata, dns_obj_error_t * NONNULL out_error);

static bool
dns_obj_rr_nsec3_check_hash_algorithm(dns_obj_rr_nsec3_t NONNULL me);

static bool
dns_obj_rr_nsec3_check_flags(dns_obj_rr_nsec3_t NONNULL me);

//======================================================================================================================
// MARK: - DNSSEC NSEC3 Resource Record Public Methods

dns_obj_rr_nsec3_t
dns_obj_rr_nsec3_create(const uint8_t * const name, const uint8_t * const rdata, const uint16_t rdata_len,
	const bool allocate_memory, dns_obj_error_t * const out_error)
{
	dns_obj_error_t err;
	dns_obj_rr_nsec3_t nsec3 = NULL;
	dns_obj_rr_nsec3_t obj = NULL;

	const size_t name_label_count = domain_name_labels_count_label(name);
	require_action(name_label_count > 1, exit, err = DNS_OBJ_ERROR_PARAM_ERR);

	const bool valid = rdata_parser_nsec3_check_validity(rdata, rdata_len);
	require_action(valid, exit, err = DNS_OBJ_ERROR_PARAM_ERR);

	obj = _dns_obj_rr_nsec3_new();
	require_action(obj != NULL, exit, err = DNS_OBJ_ERROR_NO_MEMORY);

	_dns_obj_rr_nsec3_kind.dns_obj_rr_init_fields(&obj->base, name, _dns_obj_rr_nsec3_kind.rr_type,
		kDNSClassType_IN, rdata, rdata_len, allocate_memory, _dns_obj_rr_nsec3_kind.copy_rdata_rfc_description_method, &err);
	require_noerr(err, exit);

	dns_obj_rr_nsec3_init_fields(obj, name, rdata, &err);
	require_noerr(err, exit);

	nsec3 = obj;
	obj = NULL;
	err = DNS_OBJ_ERROR_NO_ERROR;

exit:
	if (out_error != NULL) {
		*out_error = err;
	}
	MDNS_DISPOSE_DNS_OBJ(obj);
	return nsec3;
}

//======================================================================================================================

dns_obj_domain_name_t
dns_obj_rr_nsec3_get_current_owner_name(const dns_obj_rr_nsec3_t me)
{
	return dns_obj_rr_get_name(me);
}

//======================================================================================================================

uint8_t
dns_obj_rr_nsec3_get_hash_algorithm(const dns_obj_rr_nsec3_t me)
{
	const uint8_t * const rdata = dns_obj_rr_get_rdata(me);
	return rdata_parser_nsec3_get_hash_algorithm(rdata);
}

//======================================================================================================================

uint8_t
dns_obj_rr_nsec3_get_flags(const dns_obj_rr_nsec3_t me)
{
	const uint8_t * const rdata = dns_obj_rr_get_rdata(me);
	return rdata_parser_nsec3_get_flags(rdata);
}

//======================================================================================================================

uint16_t
dns_obj_rr_nsec3_get_iterations(const dns_obj_rr_nsec3_t me)
{
	const uint8_t * const rdata = dns_obj_rr_get_rdata(me);
	return rdata_parser_nsec3_get_iterations(rdata);
}

//======================================================================================================================

uint8_t
dns_obj_rr_nsec3_get_salt_length(const dns_obj_rr_nsec3_t me)
{
	const uint8_t * const rdata = dns_obj_rr_get_rdata(me);
	return rdata_parser_nsec3_get_salt_length(rdata);
}

//======================================================================================================================

const uint8_t *
dns_obj_rr_nsec3_get_salt(const dns_obj_rr_nsec3_t me, uint8_t * const out_salt_length)
{
	const uint8_t * const rdata = dns_obj_rr_get_rdata(me);
	*out_salt_length = rdata_parser_nsec3_get_salt_length(rdata);
	return rdata_parser_nsec3_get_salt(rdata);
}

//======================================================================================================================

const uint8_t *
dns_obj_rr_nsec3_get_next_hashed_owner_name_in_binary(const dns_obj_rr_nsec3_t me,
	uint8_t * const out_hash_length)
{
	const uint8_t * const rdata = dns_obj_rr_get_rdata(me);
	*out_hash_length = rdata_parser_nsec3_get_hash_length(rdata);
	return rdata_parser_nsec3_get_next_hashed_owner_name(rdata);
}

//======================================================================================================================

dns_obj_domain_name_t
dns_obj_rr_nsec3_get_next_hashed_owner_name(const dns_obj_rr_nsec3_t me)
{
	dns_obj_domain_name_t next_hashed_owner_name = me->next_hashed_owner_name;
	return next_hashed_owner_name;
}

//======================================================================================================================

bool
dns_obj_rr_nsec3_get_opt_out_enabled(const dns_obj_rr_nsec3_t me)
{
	return (dns_obj_rr_nsec3_get_flags(me) & NSEC3_FLAG_OPT_OUT) != 0;
}

//======================================================================================================================

bool
dns_obj_rr_nsec3_covers_dns_type(dns_obj_rr_nsec3_t NONNULL nsec3, const uint16_t type)
{
	const uint8_t * const rdata = dns_obj_rr_get_rdata(nsec3);
	const uint16_t rdata_len = dns_obj_rr_get_rdata_len(nsec3);

	uint16_t type_bit_maps_len;
	const uint8_t * const type_bit_maps = rdata_parser_nsec3_get_type_bit_maps(rdata, rdata_len, &type_bit_maps_len);

	return rdata_parser_type_bit_maps_cover_dns_type(type_bit_maps, type_bit_maps_len, type);
}

//======================================================================================================================

bool
dns_obj_rr_nsec3_have_same_closest_parent(const dns_obj_rr_nsec3_t me, const dns_obj_rr_nsec3_t other)
{
	const uint8_t * const my_labels = dns_obj_rr_get_name_in_labels(me);
	const uint8_t * const others_labels = dns_obj_rr_get_name_in_labels(other);

	const uint8_t * const my_parent = domain_name_labels_get_parent(my_labels, 1);
	require_return_value(my_parent != NULL, false);

	const uint8_t * const others_parent = domain_name_labels_get_parent(others_labels, 1);
	require_return_value(others_parent != NULL, false);

	const compare_result_t result = domain_name_labels_canonical_compare(my_parent, others_parent, true);

	return (result == compare_result_equal);
}

//======================================================================================================================

bool
dns_obj_rr_nsec3_has_same_nsec3_parameters(const dns_obj_rr_nsec3_t me, const dns_obj_rr_nsec3_t other)
{
	if (me == other) {
		return true;
	}

	if (dns_obj_rr_nsec3_get_hash_algorithm(me) != dns_obj_rr_nsec3_get_hash_algorithm(other) ||
		dns_obj_rr_nsec3_get_iterations(me) != dns_obj_rr_nsec3_get_iterations(other) ||
		dns_obj_rr_nsec3_get_salt_length(me) != dns_obj_rr_nsec3_get_salt_length(other)) {
		return false;
	}

	uint8_t my_salt_len;
	const uint8_t * const my_salt = dns_obj_rr_nsec3_get_salt(me, &my_salt_len);

	uint8_t others_salt_len;
	const uint8_t * const others_salt = dns_obj_rr_nsec3_get_salt(other, &others_salt_len);

	return memcmp(my_salt, others_salt, my_salt_len) == 0;
}

//======================================================================================================================

bool
dns_obj_rr_nsec3_has_reasonable_iterations(const dns_obj_rr_nsec3_t me)
{
	// Validating resolvers SHOULD return an insecure response when processing NSEC3 records with iterations larger
	// than 100.
	// See https://datatracker.ietf.org/doc/html/rfc9276
	const uint16_t max_reasonable_iterations = 100;
	return dns_obj_rr_nsec3_get_iterations(me) <= max_reasonable_iterations;
}

//======================================================================================================================

bool
dns_obj_rr_nsec3_should_be_ignored(const dns_obj_rr_nsec3_t me)
{
	// A validator MUST ignore NSEC3 RRs with unknown hash types.
	if (!dns_obj_rr_nsec3_check_hash_algorithm(me)) {
		return true;
	}

	// A validator MUST ignore NSEC3 RRs with a Flag fields value other than zero or one.
	if (!dns_obj_rr_nsec3_check_flags(me)) {
		return true;
	}

	return false;
}

//======================================================================================================================

bool
dns_obj_rr_nsec3_asserts_name_exists(const dns_obj_rr_nsec3_t me, const dns_obj_domain_name_t name,
	const uint16_t qclass)
{
	// The class has to be the same.
	if (dns_obj_rr_get_class(me) != qclass) {
		return false;
	}

	dns_obj_domain_name_t hashed_name = dns_obj_domain_name_get_nsec3_hashed_name(name, me);
	require_return_value(hashed_name != NULL, false);

	// The current owner name or the next owner name of the NSEC3 has to match the hashed name.
	const bool equal_to_current = dns_obj_equal(dns_obj_rr_nsec3_get_current_owner_name(me), hashed_name);
	const bool equal_to_next = dns_obj_equal(dns_obj_rr_nsec3_get_next_hashed_owner_name(me), hashed_name);
	return (equal_to_current || equal_to_next);
}

//======================================================================================================================

bool
dns_obj_rr_nsec3_asserts_name_exists_data_does_not_exist(const dns_obj_rr_nsec3_t me,
	const dns_obj_domain_name_t name, const uint16_t qclass, const uint16_t qtype)
{
	// The class has to be the same.
	if (dns_obj_rr_get_class(me) != qclass) {
		return false;
	}

	// The type bit map must not cover the type.
	if (dns_obj_rr_nsec3_covers_dns_type(me, qtype)) {
		return false;
	}

	// [Check for CNAME](https://datatracker.ietf.org/doc/html/rfc6840#section-4.3)
	// When validating a NOERROR/NODATA response, validators MUST check the CNAME bit in the matching NSEC or NSEC3 RR's
	// type bitmap in addition to the bit for the query type.
	// When we are not asking for CNAME record, if the returned NSEC3 only covers CNAME (it always covers RRSIG and NSEC3),
	// this NSEC does not prove that the name we asked for has no data. We need to follow the CNAME to the end and get
	// NSEC3 there to prove that.
	if (dns_obj_rr_nsec3_covers_dns_type(me, kDNSRecordType_CNAME)) {
		return false;
	}

	dns_obj_domain_name_t hashed_name = dns_obj_domain_name_get_nsec3_hashed_name(name, me);
	require_return_value(hashed_name != NULL, false);

	// The name of the NSEC3 has to match the hashed name.
	return dns_obj_equal(dns_obj_rr_get_name(me), hashed_name);
}

//======================================================================================================================

bool
dns_obj_rr_nsec3_asserts_name_does_not_exist(const dns_obj_rr_nsec3_t me, const dns_obj_domain_name_t name,
	const uint16_t qclass)
{
	// The class has to be the same.
	if (dns_obj_rr_get_class(me) != qclass) {
		return false;
	}

	// The name does not exist if it appears in-between the current hashed owner name and the next hashed owner name.
	const dns_obj_domain_name_t current_hashed_owner_name = dns_obj_rr_get_name(me);
	const dns_obj_domain_name_t next_hashed_owner_name = dns_obj_rr_nsec3_get_next_hashed_owner_name(me);

	// Get the base32 hex encoded hash name for the domain name that will be compared.
	dns_obj_domain_name_t hashed_name = dns_obj_domain_name_get_nsec3_hashed_name(name, me);
	require_return_value(hashed_name != NULL, false);

	bool name_not_exists;
	if (!me->last_nsec3) {
		// If the NSEC3 record is not the last one in the zone.
		name_not_exists = (dns_obj_compare(current_hashed_owner_name, hashed_name) == compare_result_less &&
						   dns_obj_compare(hashed_name, next_hashed_owner_name) == compare_result_less);
	} else {
		// If the NSEC3 record is the last one in the zone.
		// If the the hashed question name appears after the current hashed owner name and appears before the next
		// hashed owner name, then the question is in the gap defined by this NSEC3 record. Thus it does not exist.
		name_not_exists = (dns_obj_compare(current_hashed_owner_name, hashed_name) == compare_result_less ||
						   dns_obj_compare(hashed_name, next_hashed_owner_name) == compare_result_less);
	}

	return name_not_exists;
}

//======================================================================================================================

bool
dns_obj_rr_nsec3_is_usable_for_insecure_validation(const dns_obj_rr_nsec3_t me)
{
	// See <https://datatracker.ietf.org/doc/html/rfc6840#section-4.4>
	// When proving a delegation is not secure, needs to check for the absence of the DS and SOA bits in the NSEC3 type bitmap.
	// The validator also MUST check for the presence of the NS bit in the matching NSEC3 RR (proving that there is, indeed, a delegation).
	return (!dns_obj_rr_nsec3_covers_dns_type(me, kDNSRecordType_DS) &&
			!dns_obj_rr_nsec3_covers_dns_type(me, kDNSRecordType_SOA) &&
			dns_obj_rr_nsec3_covers_dns_type(me, kDNSRecordType_NS));
}

//======================================================================================================================
// MARK: - DNSSEC Resource Record Private Methods

static compare_result_t
_dns_obj_rr_nsec3_compare(const dns_obj_rr_nsec3_t me, const dns_obj_rr_nsec3_t other,
	const bool check_equality_only)
{
	if (check_equality_only) {
		// Let the comparator of the super kind to do comparison if there is one.
		return compare_result_unknown;
	}

	const bool have_same_parent = dns_obj_rr_nsec3_have_same_closest_parent(me, other);
	if (!have_same_parent) {
		// If two NSEC3s have different parent, then they are not equal and cannot be compared.
		return compare_result_notequal;
	}

	const uint8_t * const my_labels = dns_obj_rr_get_name_in_labels(me);
	const uint8_t * const others_labels = dns_obj_rr_get_name_in_labels(other);

	return domain_name_label_canonical_compare(my_labels, others_labels, false);
}

//======================================================================================================================

static void
_dns_obj_rr_nsec3_finalize(const dns_obj_rr_nsec3_t me)
{
	MDNS_DISPOSE_DNS_OBJ(me->next_hashed_owner_name);
}

//======================================================================================================================

static void
dns_obj_rr_nsec3_init_fields(const dns_obj_rr_nsec3_t me, const uint8_t * const name, const uint8_t * const rdata,
	dns_obj_error_t * const out_error)
{
	dns_obj_error_t err;
	uint8_t base32_hex_labels_buf[MAX_DOMAIN_NAME];
	uint8_t next_hashed_owner_name_in_labels[MAX_DOMAIN_NAME];

	const uint8_t hash_length = rdata_parser_nsec3_get_hash_length(rdata);
	const uint8_t * const next_owner_name_in_binary = rdata_parser_nsec3_get_next_hashed_owner_name(rdata);

	// Validate the length of the base 32 hex encoding string.
	const size_t base32_hex_str_len = base_x_get_encoded_string_length(base_encoding_type_base32_hex_without_padding, hash_length);
	require_action(base32_hex_str_len <= MAX_DOMAIN_LABEL, exit, err = DNS_OBJ_ERROR_PARAM_ERR);

	// Encode the binary hash value to base 32 hex string.
	base32_hex_labels_buf[0] = (uint8_t)base32_hex_str_len;
	base_x_encode(base_encoding_type_base32_hex_without_padding, next_owner_name_in_binary, hash_length, (char *)base32_hex_labels_buf + 1);

	// Since the next hashed owner name does not have parent domain appended, we need to get it from the current owner
	// name.
	const uint8_t * const current_owner_name_labels = name;
	const uint8_t * const parent = domain_name_labels_get_parent(current_owner_name_labels, 1);
	require_action(parent != NULL, exit, err = DNS_OBJ_ERROR_UNEXPECTED_ERR);

	// Append the zone domain name labels to the base32 hex encoded next hashed owner name.
	domain_name_labels_concatenate(base32_hex_labels_buf, parent, next_hashed_owner_name_in_labels,
		sizeof(next_hashed_owner_name_in_labels), &err);
	require_noerr(err, exit);

	// Make the next hashed owner name a domain name object.
	me->next_hashed_owner_name = dns_obj_domain_name_create_with_labels(next_hashed_owner_name_in_labels, true, &err);
	require_noerr(err, exit);

	const compare_result_t compare_result = dns_obj_compare(dns_obj_rr_get_name(me), me->next_hashed_owner_name);
	const bool last_nsec3 = (compare_result == compare_result_greater || compare_result == compare_result_equal);
	me->last_nsec3 = last_nsec3;

exit:
	*out_error = err;
}

//======================================================================================================================

static bool
dns_obj_rr_nsec3_check_hash_algorithm(const dns_obj_rr_nsec3_t me)
{
	return dns_obj_rr_nsec3_get_hash_algorithm(me) == NSEC3_HASH_ALGORITHM_SHA_1;
}

//======================================================================================================================

static bool
dns_obj_rr_nsec3_check_flags(const dns_obj_rr_nsec3_t me)
{
	const uint8_t flags = dns_obj_rr_nsec3_get_flags(me);
	const uint8_t other_flags = (flags & (~NSEC3_FLAG_OPT_OUT));
	return (other_flags == 0);
}
