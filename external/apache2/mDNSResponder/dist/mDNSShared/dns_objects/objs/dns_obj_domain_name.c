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
#include "dns_obj_domain_name.h"
#include "dns_obj_crypto.h"
#include "domain_name_labels.h"
#include "base_encoding.h"
#include "string.h"	// For memcpy().

#include "dns_assert_macros.h"
#include "mdns_strict.h"

//======================================================================================================================
// MARK: - DNS Domain Name Kind Definition

struct dns_obj_domain_name_s {
	struct ref_count_obj_s		base;					// The reference count and kind support base.
	union {
		const uint8_t *			const_labels;			// The pointer to a domain name labels that is valid for the life time of this domain name object.
		uint8_t *				allocated_labels;		// The pointer to a domain name labels that is allocated when initializing this domain name object.
	} labels_u;
	size_t						length;					// The length of the domain name labels.
	uint32_t					hash_value;				// The hash value of the domain name labels.
	bool						hash_computed;			// The boolean value to indicate if the hash value has been computed for this domain name object.
	bool						allocated_memory;		// Indicate whether the domain name labels point to an external memory or is allocated.
	dns_obj_domain_name_t		nsec3_hashed_name;		// The hashed domain name if it has been set.
};

DNS_OBJECT_DEFINE_FULL(domain_name);	// Define domain name object as a DNSSEC object.

//======================================================================================================================
// MARK: - Local Prototypes

static dns_obj_error_t
_dns_obj_domain_name_init_fields(dns_obj_domain_name_t domain_name, const uint8_t *labels, bool allocate_memory);

static dns_obj_error_t
dns_obj_domain_name_set_nsec3_hashed_name(dns_obj_domain_name_t NONNULL me, dns_obj_rr_nsec3_t NONNULL nsec3);

//======================================================================================================================
// MARK: - DNSSEC Domain Name Public Methods

dns_obj_domain_name_t
dns_obj_domain_name_create_with_labels(const uint8_t * const labels, const bool allocate_memory,
	dns_obj_error_t * const out_error)
{
	dns_obj_error_t err;
	dns_obj_domain_name_t domain_name = NULL;
	dns_obj_domain_name_t obj = NULL;

	obj = _dns_obj_domain_name_new();
	require_action(obj != NULL, exit, err = DNS_OBJ_ERROR_NO_MEMORY);

	err = _dns_obj_domain_name_init_fields(obj, labels, allocate_memory);
	require_noerr(err, exit);

	domain_name = obj;
	obj = NULL;
	err = DNS_OBJ_ERROR_NO_ERROR;

exit:
	if (out_error != NULL) {
		*out_error = err;
	}
	MDNS_DISPOSE_DNS_OBJ(obj);
	return domain_name;
}

//======================================================================================================================

dns_obj_domain_name_t
dns_obj_domain_name_create_concatenation(const dns_obj_domain_name_t front_domain,
	const dns_obj_domain_name_t end_domain, dns_obj_error_t * const out_error)
{
	return dns_obj_domain_name_create_concatenation_with_subdomain(dns_obj_domain_name_get_labels(front_domain),
																	  end_domain, out_error);
}

//======================================================================================================================

dns_obj_domain_name_t
dns_obj_domain_name_create_concatenation_with_subdomain(const uint8_t * const subdomain_in_labels,
	const dns_obj_domain_name_t end_domain, dns_obj_error_t * const out_error)
{
	dns_obj_error_t err;
	uint8_t concatenation_in_labels[MAX_DOMAIN_NAME];
	dns_obj_domain_name_t concatenation = NULL;

	domain_name_labels_concatenate(subdomain_in_labels, dns_obj_domain_name_get_labels(end_domain),
		concatenation_in_labels, sizeof(concatenation_in_labels), &err);
	require_noerr(err, exit);

	concatenation = dns_obj_domain_name_create_with_labels(concatenation_in_labels, true, &err);
	require_noerr(err, exit);

exit:
	if (out_error != NULL) {
		*out_error = err;
	}
	return concatenation;
}

//======================================================================================================================

// See <https://datatracker.ietf.org/doc/html/rfc4035#section-5.3.2> for how canonical name is calculated.
dns_obj_domain_name_t
dns_obj_domain_name_create_canonical(const dns_obj_domain_name_t me, const uint8_t rrsig_labels,
	dns_obj_error_t * const out_error)
{
	dns_obj_error_t err;
	dns_obj_domain_name_t canonical = NULL;
	dns_obj_domain_name_t rightmost = NULL;

	// let fqdn = RRset's fully qualified domain name in canonical form
	const dns_obj_domain_name_t fqdn = me;
	// let fqdn_labels = Label count of the fqdn above.
	const size_t fqdn_labels = dns_obj_domain_name_get_label_count(fqdn);

	// if rrsig_labels > fqdn_labels, the RRSIG RR did not pass the necessary validation checks and MUST NOT be used to
	// authenticate this RRset.
	require_action(rrsig_labels <= fqdn_labels, exit, err = DNS_OBJ_ERROR_RANGE_ERR);

	if (rrsig_labels == fqdn_labels) {
		// if rrsig_labels = fqdn_labels,
		// result = fqdn
		canonical = me;
		dns_obj_retain(canonical);
	} else {
		// if rrsig_labels < fqdn_labels,
		// result = "*." | the rightmost rrsig_label labels of the fqdn
		const uint8_t asterisk_labels[] = {1, '*', 0};
		rightmost = dns_obj_domain_name_copy_parent_domain(fqdn, fqdn_labels - rrsig_labels, &err);
		require_noerr(err, exit);

		canonical = dns_obj_domain_name_create_concatenation_with_subdomain(asterisk_labels, rightmost, &err);
		require_noerr(err, exit);
	}

	err = DNS_OBJ_ERROR_NO_ERROR;

exit:
	if (out_error != NULL) {
		*out_error = err;
	}
	MDNS_DISPOSE_DNS_OBJ(rightmost);
	return canonical;
}

//======================================================================================================================

dns_obj_domain_name_t
dns_obj_domain_name_create_with_cstring(const char * const name_cstring, dns_obj_error_t * const out_error)
{
	dns_obj_error_t err;
	uint8_t labels[MAX_DOMAIN_NAME];
	dns_obj_domain_name_t name = NULL;

	err = cstring_to_domain_name_labels(name_cstring, labels, NULL);
	require_noerr(err, exit);

	name = dns_obj_domain_name_create_with_labels(labels, true, &err);
	require_noerr(err, exit);

exit:
	if (out_error != NULL) {
		*out_error = err;
	}
	return name;
}

//======================================================================================================================

dns_obj_domain_name_t
dns_obj_domain_name_create_copy(const dns_obj_domain_name_t name, dns_obj_error_t * const out_error)
{
	return dns_obj_domain_name_create_with_labels(dns_obj_domain_name_get_labels(name), true, out_error);
}

//======================================================================================================================

const uint8_t *
dns_obj_domain_name_get_labels(const dns_obj_domain_name_t me)
{
	return me->allocated_memory ? me->labels_u.allocated_labels : me->labels_u.const_labels;
}

//======================================================================================================================

size_t
dns_obj_domain_name_get_label_count(const dns_obj_domain_name_t me)
{
	const uint8_t * const my_labels = dns_obj_domain_name_get_labels(me);
	return domain_name_labels_count_label(my_labels);
}

//======================================================================================================================

size_t
dns_obj_domain_name_get_length(const dns_obj_domain_name_t me)
{
	return me->length;
}

//======================================================================================================================

dns_obj_error_t
dns_obj_domain_name_to_cstring(const dns_obj_domain_name_t me, char out_name_cstring[static const MAX_ESCAPED_DOMAIN_NAME])
{
	return domain_name_labels_to_cstring(dns_obj_domain_name_get_labels(me), dns_obj_domain_name_get_length(me), out_name_cstring);
}

//======================================================================================================================

bool
dns_obj_domain_name_is_root(const dns_obj_domain_name_t me)
{
	const uint8_t * const my_labels = dns_obj_domain_name_get_labels(me);
	return my_labels[0] == 0;
}

//======================================================================================================================

bool
dns_obj_domain_name_is_single_label(const dns_obj_domain_name_t me)
{
	const uint8_t * const my_labels = dns_obj_domain_name_get_labels(me);
	return ((my_labels[0] + 2) == dns_obj_domain_name_get_length(me));
}

//======================================================================================================================

bool
dns_obj_domain_name_is_sub_domain_of(const dns_obj_domain_name_t me, const dns_obj_domain_name_t parent)
{
	const uint8_t * const my_labels = dns_obj_domain_name_get_labels(me);
	const uint8_t * const parents_labels = dns_obj_domain_name_get_labels(parent);
	return domain_name_labels_is_sub_labels_of(my_labels, parents_labels);
}

//======================================================================================================================

dns_obj_domain_name_t
dns_obj_domain_name_copy_parent_domain(const dns_obj_domain_name_t me, const size_t index, dns_obj_error_t * const out_error)
{
	dns_obj_error_t err;
	dns_obj_domain_name_t parent = NULL;
	const uint8_t * const my_labels = dns_obj_domain_name_get_labels(me);
	const uint8_t * const parents_labels = domain_name_labels_get_parent(my_labels, index);
	require_action(parents_labels != NULL, exit, err = DNS_OBJ_ERROR_RANGE_ERR);


	parent = dns_obj_domain_name_create_with_labels(parents_labels, true, &err);
	require_noerr(err, exit);

	err = DNS_OBJ_ERROR_NO_ERROR;
exit:
	if (out_error != NULL) {
		*out_error = err;
	}
	return parent;
}

//======================================================================================================================

dns_obj_domain_name_t
dns_obj_domain_name_copy_closest_common_ancestor(const dns_obj_domain_name_t me,
	const dns_obj_domain_name_t other, dns_obj_error_t * const out_error)
{
	// The closest common ancestor of "wwwtest.ietf.org." and "xml2rfc.ietf.org." is "ietf.org.".
	// The closest common ancestor of "*.wildcard.dnssec.qdeng.io." and "a.wildcard.dnssec.qdeng.io." is "wildcard.dnssec.qdeng.io.".
	const uint8_t * const closest_common_ancestor_labels =
		domain_name_labels_get_closest_common_ancestor(dns_obj_domain_name_get_labels(me),
													   dns_obj_domain_name_get_labels(other));

	dns_obj_error_t err;
	const dns_obj_domain_name_t closest_common_ancestor =
		dns_obj_domain_name_create_with_labels(closest_common_ancestor_labels, true, &err);

	if (out_error != NULL) {
		*out_error = err;
	}
	return closest_common_ancestor;
}

//======================================================================================================================

bool
dns_obj_domain_name_is_wildcard_domain_name(const dns_obj_domain_name_t NONNULL me)
{
	const uint8_t asterisk_label[] = {1, '*'};
	if (dns_obj_domain_name_get_length(me) <= sizeof(asterisk_label)) {
		verify(dns_obj_domain_name_is_root(me));
		return false;
	}

	const uint8_t * const wildcard_in_labels = dns_obj_domain_name_get_labels(me);
	const bool is_wildcard_domain_name = (memcmp(wildcard_in_labels, asterisk_label, sizeof(asterisk_label)) == 0);

	return is_wildcard_domain_name;
}

//======================================================================================================================

bool
dns_obj_domain_name_is_a_wildcard_expansion(const dns_obj_domain_name_t me,
	const dns_obj_domain_name_t wildcard)
{
	// An expansion of wildcard (me) cannot be a wildcard domain name.
	if (dns_obj_domain_name_is_wildcard_domain_name(me)) {
		return false;
	}

	// A wildcard has to be a wildcard domain name.
	if (!dns_obj_domain_name_is_wildcard_domain_name(wildcard)) {
		return false;
	}

	// Wildcard expansion can only happen on the leading label.
	const uint8_t * const closest_encloser = domain_name_labels_get_parent(dns_obj_domain_name_get_labels(wildcard), 1);
	// closest_encloser is ensured to be non-null because the wildcard check above.

	// www.appleweb.apple.com. is an expansion of *, *.com., *.apple.com., *.appleweb.apple.com.
	const uint8_t * const my_name_in_labels = dns_obj_domain_name_get_labels(me);
	return domain_name_labels_is_sub_labels_of(my_name_in_labels, closest_encloser);
}

//======================================================================================================================

dns_obj_error_t
dns_obj_domain_name_set_nsec3_hashed_name_with_params(const dns_obj_domain_name_t me, const uint8_t algorithm,
	const uint16_t iterations, const uint8_t * const salt, const uint8_t salt_length, const uint8_t * const zone_domain)
{
	dns_obj_error_t err;
	digest_type_t digest_type;
	uint8_t *lower_case_labels = NULL;

	switch (algorithm) {
		case NSEC3_HASH_ALGORITHM_SHA_1:
			digest_type = DIGEST_SHA_1;
			err = DNS_OBJ_ERROR_NO_ERROR;
			break;
		default:
			digest_type = DIGEST_UNSUPPORTED;
			err = DNS_OBJ_ERROR_UNSUPPORTED_ERR;
			break;
	}
	require_noerr(err, exit);

	// Hash is calculated with all lower case domain name.
	lower_case_labels = domain_name_labels_create(dns_obj_domain_name_get_labels(me), true, &err);
	require_noerr(err, exit);
	const size_t lower_case_labels_len = domain_name_labels_length(lower_case_labels);

	// Calculate the binary format of the hash value.
	uint8_t digest[MAX_HASHED_NAME_BUFF_SIZE];
	err = dns_obj_compute_nsec3_digest(digest_type, lower_case_labels, lower_case_labels_len, iterations, salt, salt_length, digest);
	const size_t digest_len = dns_obj_data_compute_digest_get_output_size(digest_type);

	// Convert the binary hash value to a base32 hex string, and convert it to a domain name labels.
	char base32_hex[1 + BASE32_HEX_OUTPUT_SIZE(MAX_HASHED_NAME_OUTPUT_SIZE)];
	const size_t base32_encoded_str_len = base_x_get_encoded_string_length(base_encoding_type_base32_hex_without_padding, digest_len);
	require_action(base32_encoded_str_len <= MAX_DOMAIN_LABEL, exit, err = DNS_OBJ_ERROR_OVER_RUN);

	base32_hex[0] = (char)base32_encoded_str_len;
	base_x_encode(base_encoding_type_base32_hex_without_padding, digest, digest_len, base32_hex + 1);
	const uint8_t * const base32_hex_as_labels = (uint8_t *)base32_hex;

	// Append the zone domain after the base32 hex encoded label.
	uint8_t my_nsec3_hashed_name_in_labels[MAX_DOMAIN_NAME];
	domain_name_labels_concatenate(base32_hex_as_labels, zone_domain, my_nsec3_hashed_name_in_labels, sizeof(my_nsec3_hashed_name_in_labels), &err);
	require_noerr(err, exit);

	// Create the domain name object based on the final domain name labels.
	dns_obj_domain_name_t my_nsec3_hashed_name = dns_obj_domain_name_create_with_labels(my_nsec3_hashed_name_in_labels, true, &err);
	require_noerr(err, exit);

	me->nsec3_hashed_name = my_nsec3_hashed_name;
	err = DNS_OBJ_ERROR_NO_ERROR;

exit:
	mdns_free(lower_case_labels);
	return err;
}

//======================================================================================================================

dns_obj_domain_name_t
dns_obj_domain_name_get_nsec3_hashed_name(const dns_obj_domain_name_t me, const dns_obj_rr_nsec3_t nsec3)
{
	dns_obj_domain_name_t hashed_name = me->nsec3_hashed_name;
	if (hashed_name == NULL && nsec3 != NULL) {
		const dns_obj_error_t err = dns_obj_domain_name_set_nsec3_hashed_name(me, nsec3);
		require_noerr_return_value(err, NULL);
		hashed_name = me->nsec3_hashed_name;
	}

	return hashed_name;
}

//======================================================================================================================

void
dns_obj_domain_name_clear_nsec3_hashed_name(const dns_obj_domain_name_t me)
{
	MDNS_DISPOSE_DNS_OBJ(me->nsec3_hashed_name);
}

//======================================================================================================================
// MARK: - DNSSEC Domain Name Private Methods

static compare_result_t
_dns_obj_domain_name_compare(const dns_obj_domain_name_t me, const dns_obj_domain_name_t other,
	const bool check_equality_only)
{
	const uint8_t * const my_labels = dns_obj_domain_name_get_labels(me);
	const uint8_t * const others_labels = dns_obj_domain_name_get_labels(other);

	if (me == other) {
		return compare_result_equal;
	}

	if (me->length == other->length) {
		if (!me->hash_computed) {
			me->hash_value = domain_name_labels_compute_hash(my_labels);
			me->hash_computed = true;
		}

		if (!other->hash_computed) {
			other->hash_value = domain_name_labels_compute_hash(others_labels);
			other->hash_computed = true;
		}

		if (me->hash_value != other->hash_value && check_equality_only) {
			// If the hash values are not equal, the two domain name object cannot be equal.
			return compare_result_notequal;
		}
	} else { // me->length != other->length
		if (check_equality_only) {
			// If the lengths are not equal, the two domain name object cannot be equal.
			return compare_result_notequal;
		}
		// Continue if we want know the exact order of the two objects.
	}

	return domain_name_labels_canonical_compare(my_labels, others_labels, check_equality_only);
}

//======================================================================================================================

static void
_dns_obj_domain_name_finalize(const dns_obj_domain_name_t me)
{
	if (me->allocated_memory) {
		// Only free the domain name labels when the memory associated with it is allocated.
		mdns_free(me->labels_u.allocated_labels);
	}
	MDNS_DISPOSE_DNS_OBJ(me->nsec3_hashed_name);
}

//======================================================================================================================
// MARK: - Local Functions

static dns_obj_error_t
_dns_obj_domain_name_init_fields(const dns_obj_domain_name_t me, const uint8_t * labels,
	bool allocate_memory)
{
	dns_obj_error_t err;
	uint8_t *allocated_labels = NULL;
	size_t length;

	if (!allocate_memory && !domain_name_labels_contains_upper_case(labels)) {
		// If the original domain name labels contains upper case characters, to make it consistent with the following
		// name manipulation that requires all lower-case labels, we convert it to lower case by allocating a new memory
		// region for it.
		me->labels_u.const_labels = labels;
		length = domain_name_labels_length(labels);
		me->allocated_memory = false;
	} else {
		allocated_labels = domain_name_labels_create(labels, true, &err);
		require_noerr(err, exit);

		length = domain_name_labels_length(allocated_labels);

		me->labels_u.allocated_labels = allocated_labels;
		allocated_labels = NULL;

		me->allocated_memory = true;
	}

	me->length = length;
	me->hash_value = 0;
	me->hash_computed = false;
	me->nsec3_hashed_name = NULL;
	err = DNS_OBJ_ERROR_NO_ERROR;

exit:
	mdns_free(allocated_labels);
	return err;
}

//======================================================================================================================

static dns_obj_error_t
dns_obj_domain_name_set_nsec3_hashed_name(const dns_obj_domain_name_t me, const dns_obj_rr_nsec3_t nsec3)
{
	dns_obj_error_t err;

	uint8_t salt_length;
	const uint8_t algorithm = dns_obj_rr_nsec3_get_hash_algorithm(nsec3);
	const uint16_t iterations = dns_obj_rr_nsec3_get_iterations(nsec3);
	const uint8_t * const salt = dns_obj_rr_nsec3_get_salt(nsec3, &salt_length);

	const dns_obj_domain_name_t current_owner_name = dns_obj_rr_nsec3_get_current_owner_name(nsec3);
	require_action(dns_obj_domain_name_get_label_count(current_owner_name) >= 1, exit, err = DNS_OBJ_ERROR_MALFORMED_ERR);
	const uint8_t * const zone_domain = domain_name_labels_get_parent(dns_obj_domain_name_get_labels(current_owner_name), 1);

	err = dns_obj_domain_name_set_nsec3_hashed_name_with_params(me, algorithm, iterations, salt, salt_length, zone_domain);

exit:
	return err;
}
