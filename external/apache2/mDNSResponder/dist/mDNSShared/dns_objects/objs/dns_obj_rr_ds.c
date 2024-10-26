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
#include "dns_obj_rr_ds.h"
#include "dns_obj_rr_dnskey.h"
#include "dns_obj_rr_private.h"
#include "rdata_parser.h"
#include <string.h>

#include "dns_assert_macros.h"
#include "mdns_strict.h"

//======================================================================================================================
// MARK: - DNSSEC DS Resource Record Kind Definition

struct dns_obj_rr_ds_s {
	struct dns_obj_rr_s	base;		// The reference count and kind support base.
};

char * NULLABLE
_dns_obj_rr_ds_copy_rdata_rfc_description(dns_obj_rr_any_t me, dns_obj_error_t * NULLABLE out_error);

// dns_obj_rr_ds_t is a subkind of dns_obj_rr_t, and it always have DNS type: kDNSRecordType_DS.
DNS_OBJECT_SUBKIND_DEFINE_ABSTRUCT(rr, ds,
	.rr_type = kDNSRecordType_DS,
	.copy_rdata_rfc_description_method = _dns_obj_rr_ds_copy_rdata_rfc_description
);

//======================================================================================================================
// MARK: - Local Prototypes

static bool
ds_digest_type_get_priority(uint8_t digest_type);

//======================================================================================================================
// MARK: - DNSSEC DS Resource Record Public Methods

dns_obj_rr_ds_t
dns_obj_rr_ds_create(const uint8_t * const name, const uint16_t class, const uint8_t * const rdata,
	const uint16_t rdata_len, const bool allocate, dns_obj_error_t * const out_error)
{
	dns_obj_error_t err;
	dns_obj_rr_ds_t ds = NULL;
	dns_obj_rr_ds_t obj = NULL;

	const bool valid = rdata_parser_ds_check_validity(rdata, rdata_len);
	require_action(valid, exit, err = DNS_OBJ_ERROR_PARAM_ERR);

	obj = _dns_obj_rr_ds_new();
	require_action(obj != NULL, exit, err = DNS_OBJ_ERROR_NO_MEMORY);

	_dns_obj_rr_ds_kind.dns_obj_rr_init_fields(&obj->base, name, _dns_obj_rr_ds_kind.rr_type, class, rdata,
		rdata_len, allocate, _dns_obj_rr_ds_kind.copy_rdata_rfc_description_method, &err);
	require_noerr(err, exit);

	ds = obj;
	dns_obj_retain(ds);
	err = DNS_OBJ_ERROR_NO_ERROR;

exit:
	if (out_error != NULL) {
		*out_error = err;
	}
	MDNS_DISPOSE_DNS_OBJ(obj);
	return ds;
}

//======================================================================================================================

uint16_t
dns_obj_rr_ds_get_key_tag(const dns_obj_rr_ds_t me)
{
	const uint8_t * const rdata = dns_obj_rr_get_rdata(me);
	return rdata_parser_ds_get_key_tag(rdata);
}

//======================================================================================================================

uint8_t
dns_obj_rr_ds_get_algorithm(const dns_obj_rr_ds_t me)
{
	const uint8_t * const rdata = dns_obj_rr_get_rdata(me);
	return rdata_parser_ds_get_algorithm(rdata);
}

//======================================================================================================================

uint8_t
dns_obj_rr_ds_get_digest_type(const dns_obj_rr_ds_t me)
{
	const uint8_t * const rdata = dns_obj_rr_get_rdata(me);
	return rdata_parser_ds_get_digest_type(rdata);
}

//======================================================================================================================

const uint8_t *
dns_obj_rr_ds_get_digest(const dns_obj_rr_ds_t me)
{
	const uint8_t * const rdata = dns_obj_rr_get_rdata(me);
	return rdata_parser_ds_get_digest(rdata);
}

//======================================================================================================================

uint16_t
dns_obj_rr_ds_get_digest_length(const dns_obj_rr_ds_t me)
{
	return rdata_parser_ds_get_digest_length(dns_obj_rr_get_rdata_len(me));
}

//======================================================================================================================

bool
dns_obj_rr_ds_refers_to_supported_key_algorithm(const dns_obj_rr_ds_t me)
{
	const uint8_t algorithm = dns_obj_rr_ds_get_algorithm(me);
	const uint16_t priority = dns_obj_rr_dnskey_algorithm_get_priority(algorithm);
	return (priority != 0);
}

//======================================================================================================================

bool
dns_obj_rr_ds_is_valid_for_dnssec(const dns_obj_rr_ds_t me, dns_obj_error_t *const out_error)
{
	dns_obj_error_t err;

	// The algorithm of the DNSKEY must be something we support.
	const uint16_t algorithm_priority = dns_obj_rr_dnskey_algorithm_get_priority(dns_obj_rr_ds_get_algorithm(me));
	require_action_quiet(algorithm_priority != 0, exit, err = DNS_OBJ_ERROR_UNSUPPORTED_ERR);

	// The digest type in the DS must be something we support.
	const uint8_t digest_type_priority = ds_digest_type_get_priority(dns_obj_rr_ds_get_digest_type(me));
	require_action_quiet(digest_type_priority != 0, exit, err = DNS_OBJ_ERROR_UNSUPPORTED_ERR);

	err = DNS_OBJ_ERROR_NO_ERROR;
exit:
	if (out_error != NULL) {
		*out_error = err;
	}
	return err == DNS_OBJ_ERROR_NO_ERROR;
}

//======================================================================================================================

bool
dns_obj_rr_ds_validates_dnskey(const dns_obj_rr_ds_t me, const dns_obj_rr_dnskey_t dnskey, dns_obj_error_t * const out_error)
{
	dns_obj_error_t err;
	bool validates = false;

	// The DS and DNSKEY must have the same key tag.
	validates = (dns_obj_rr_ds_get_key_tag(me) == dns_obj_rr_dnskey_get_key_tag(dnskey));
	require_action_quiet(validates, exit, err = DNS_OBJ_ERROR_MISMATCH_ERR);

	// The DS must be valid for DNSSEC validation.
	validates = dns_obj_rr_ds_is_valid_for_dnssec(me, &err);
	require_quiet(validates, exit);

	// The DNSKEY must be valid for DNSSEC validation.
	validates = dns_obj_rr_dnskey_is_valid_for_dnssec(dnskey, &err);
	require_quiet(validates, exit);

	// The algorithm in both DS and DNSKEY must be the same.
	const uint8_t algorithm_in_ds = dns_obj_rr_ds_get_algorithm(me);
	const uint8_t algorithm_in_dnskey = dns_obj_rr_dnskey_get_algorithm(dnskey);
	validates = (algorithm_in_ds == algorithm_in_dnskey);
	require_action_quiet(validates, exit, err = DNS_OBJ_ERROR_MISMATCH_ERR);

	// The owner name of the DS and DNSKEY must be the same.
	const dns_obj_domain_name_t ds_owner_name = dns_obj_rr_get_name(me);
	const dns_obj_domain_name_t dnskey_owner_name = dns_obj_rr_get_name(dnskey);
	validates = dns_obj_equal(ds_owner_name, dnskey_owner_name);
	require_action_quiet(validates, exit, err = DNS_OBJ_ERROR_MISMATCH_ERR);

	// The digest contained in the DS and the digest of DNSKEY must match.
	uint8_t digest[MAX_DIGEST_OUTPUT_SIZE];
	size_t digest_size;
	const uint8_t digest_type = dns_obj_rr_ds_get_digest_type(me);
	err = dns_obj_rr_dnskey_compute_digest(dnskey, digest_type, digest, &digest_size);
	require_noerr_action(err, exit, validates = false);

	validates = (dns_obj_rr_ds_get_digest_length(me) == digest_size);
	require_action_quiet(validates, exit, err = DNS_OBJ_ERROR_AUTHENTICATION_ERR);

	validates = (memcmp(dns_obj_rr_ds_get_digest(me), digest, digest_size) == 0);
	require_action_quiet(validates, exit, err = DNS_OBJ_ERROR_AUTHENTICATION_ERR);

	validates = true;
	err = DNS_OBJ_ERROR_NO_ERROR;
exit:
	if (out_error != NULL) {
		*out_error = err;
	}
	return validates;
}

//======================================================================================================================
// MARK: - DS Public Functions

digest_type_t
dns_obj_rr_ds_digest_type_to_digest_type_enum(const uint16_t ds_digest_type)
{
	digest_type_t digest_type;

	switch (ds_digest_type) {
		case DS_DIGEST_SHA_1:
			digest_type = DIGEST_SHA_1;
			break;
		case DS_DIGEST_SHA_256:
			digest_type = DIGEST_SHA_256;
			break;
		case DS_DIGEST_SHA_384:
			digest_type = DIGEST_SHA_384;
			break;
		default:
			digest_type = DIGEST_UNSUPPORTED;
			break;
	}

	return digest_type;
}

//======================================================================================================================
// MARK: -  DNSSEC DS Resource Record Private Methods

char *
_dns_obj_rr_ds_copy_rdata_rfc_description(const dns_obj_rr_ds_t me, dns_obj_error_t * const out_error)
{
	dns_obj_error_t err;
	char *description = NULL;

	// Calculate the buffer that holds the DS description.
	// <Key Tag> + ' ' + <Algorithm> + ' ' + <Digest Type> + ' ' + <Digest In Hex> + '\0'
	char *const fake_buffer = NULL;
	const size_t buffer_len = (size_t)snprintf(fake_buffer, 0, "%u %u %u ", dns_obj_rr_ds_get_key_tag(me), dns_obj_rr_ds_get_algorithm(me), dns_obj_rr_ds_get_digest_type(me))
		+ 2 * dns_obj_rr_ds_get_digest_length(me) + 1;

	// Create the buffer.
	description = mdns_calloc(1, buffer_len);
	require_action(description != NULL, exit, err = DNS_OBJ_ERROR_NO_MEMORY);

	// Put the value in.
	char *ptr = description;
	const char * const limit = description + buffer_len;
	const int ret = snprintf(ptr, (size_t)(limit - ptr), "%u %u %u ", dns_obj_rr_ds_get_key_tag(me), dns_obj_rr_ds_get_algorithm(me),
		dns_obj_rr_ds_get_digest_type(me));
	require_action(ret > 0, exit, err = DNS_OBJ_ERROR_UNKNOWN_ERR);

	ptr += ret;
	const char * const end = put_hex_from_bytes(dns_obj_rr_ds_get_digest(me), dns_obj_rr_ds_get_digest_length(me), ptr, (size_t)(limit - ptr));
	require_action(end != ptr, exit, err = DNS_OBJ_ERROR_OVER_RUN);
	err = DNS_OBJ_ERROR_NO_ERROR;

exit:
	if (out_error != NULL) {
		*out_error = err;
	}
	if (err != DNS_OBJ_ERROR_NO_ERROR) {
		mdns_free(description);
	}
	return description;
}

//======================================================================================================================
// MARK: - Local Functions

static bool
ds_digest_type_get_priority(const uint8_t digest_type)
{
	// Get from <https://datatracker.ietf.org/doc/html/rfc8624#section-3.3>

	uint8_t priority;

	//			+--------+-----------------+-------------------+-------------------+
	//			| Number | Mnemonics       | DNSSEC Delegation | DNSSEC Validation |
	//			+--------+-----------------+-------------------+-------------------+
	switch (digest_type) {
		case DS_DIGEST_SHA_1:
			//	| 1      | SHA-1           | MUST NOT          | MUST              |
			priority = 1;
			break;
		case DS_DIGEST_SHA_256:
			//	| 2      | SHA-256         | MUST              | MUST              |
			priority = 2;
			break;
		case DS_DIGEST_SHA_384:
			//	| 4      | SHA-384         | MAY               | RECOMMENDED       |
			priority = 3;
			break;
		default:
			//	| 0      | NULL (CDS only) | MUST NOT [*]      | MUST NOT [*]      |
			priority = 0;
			break;
	}

	return priority;
}
