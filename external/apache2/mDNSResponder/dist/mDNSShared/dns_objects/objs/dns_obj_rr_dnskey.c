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
#include "dns_obj_rr_dnskey.h"
#include "dns_obj_rr_ds.h"
#include "dns_obj_rr_private.h"
#include "dns_obj_crypto.h"
#include "dns_obj.h"
#include "dns_common.h"
#include "rdata_parser.h"
#include "base_encoding.h"

#include "dns_assert_macros.h"
#include "mdns_strict.h"

//======================================================================================================================
// MARK: - DNS DNSKEY Resource Record Kind Definition

struct dns_obj_rr_dnskey_s {
	struct dns_obj_rr_s	base;		// The reference count and kind support base.
	uint16_t				key_tag;	// The key tag of the DNSKEY record that can be used to match RRSIG/DS records.
};

char * NULLABLE
_dns_obj_rr_dnskey_copy_rdata_rfc_description(dns_obj_rr_any_t me, dns_obj_error_t * NULLABLE out_error);

// dns_obj_rr_dnskey_t is a subkind of dns_obj_rr_t, and it always have DNS type: kDNSRecordType_DNSKEY.
DNS_OBJECT_SUBKIND_DEFINE_ABSTRUCT(rr, dnskey,
	.rr_type = kDNSRecordType_DNSKEY,
	.copy_rdata_rfc_description_method = _dns_obj_rr_dnskey_copy_rdata_rfc_description
);

//======================================================================================================================
// MARK: - Local Prototypes

static uint16_t
dns_obj_rr_dnskey_compute_key_tag(const uint8_t * NONNULL rdata, uint16_t rdata_len);

//======================================================================================================================
// MARK: - DNS DNSKEY Resource Record Public Methods

dns_obj_rr_dnskey_t
dns_obj_rr_dnskey_create(const uint8_t * const name, const uint16_t class, const uint8_t * const rdata,
	const uint16_t rdata_len, const bool allocate, dns_obj_error_t * const out_error)
{
	dns_obj_error_t err;
	dns_obj_rr_dnskey_t dnskey = NULL;
	dns_obj_rr_dnskey_t obj = NULL;

	const bool valid = rdata_parser_dnskey_check_validity(rdata, rdata_len);
	require_action(valid, exit, err = DNS_OBJ_ERROR_PARAM_ERR);

	obj = _dns_obj_rr_dnskey_new();
	require_action(obj != NULL, exit, err = DNS_OBJ_ERROR_NO_MEMORY);

	_dns_obj_rr_dnskey_kind.dns_obj_rr_init_fields(&obj->base, name, _dns_obj_rr_dnskey_kind.rr_type,
		class, rdata, rdata_len, allocate, _dns_obj_rr_dnskey_kind.copy_rdata_rfc_description_method, &err);
	require_noerr(err, exit);

	obj->key_tag = dns_obj_rr_dnskey_compute_key_tag(rdata, rdata_len);

	dnskey = obj;
	dns_obj_retain(dnskey);
	err = DNS_OBJ_ERROR_NO_ERROR;

exit:
	if (out_error != NULL) {
		*out_error = err;
	}
	MDNS_DISPOSE_DNS_OBJ(obj);
	return dnskey;
}

//======================================================================================================================

uint16_t
dns_obj_rr_dnskey_get_flags(const dns_obj_rr_dnskey_t me)
{
	const uint8_t * const rdata = dns_obj_rr_get_rdata(me);
	return rdata_parser_dnskey_get_flags(rdata);
}

//======================================================================================================================

uint8_t
dns_obj_rr_dnskey_get_protocol(const dns_obj_rr_dnskey_t me)
{
	const uint8_t * const rdata = dns_obj_rr_get_rdata(me);
	return rdata_parser_dnskey_get_protocol(rdata);
}

//======================================================================================================================

uint8_t
dns_obj_rr_dnskey_get_algorithm(const dns_obj_rr_dnskey_t me)
{
	const uint8_t * const rdata = dns_obj_rr_get_rdata(me);
	return rdata_parser_dnskey_get_algorithm(rdata);
}

//======================================================================================================================

const uint8_t *
dns_obj_rr_dnskey_get_public_key(const dns_obj_rr_dnskey_t me)
{
	const uint8_t * const rdata = dns_obj_rr_get_rdata(me);
	return rdata_parser_dnskey_get_public_key(rdata);
}

//======================================================================================================================

uint16_t
dns_obj_rr_dnskey_get_public_key_size(const dns_obj_rr_dnskey_t me)
{
	return rdata_parser_dnskey_get_public_key_size(dns_obj_rr_get_rdata_len(me));
}

//======================================================================================================================

uint16_t
dns_obj_rr_dnskey_get_key_tag(const dns_obj_rr_dnskey_t me)
{
	return me->key_tag;
}

//======================================================================================================================

bool
dns_obj_rr_dnskey_is_zone_key(const dns_obj_rr_dnskey_t me)
{
	const uint16_t flags = dns_obj_rr_dnskey_get_flags(me);
	return (flags & DNSKEY_FLAG_ZONE_KEY) != 0;
}

//======================================================================================================================

bool
dns_obj_rr_dnskey_is_secure_entry_point(const dns_obj_rr_dnskey_t me)
{
	const uint16_t flags = dns_obj_rr_dnskey_get_flags(me);
	return (flags & DNSKEY_FLAG_SECURITY_ENTRY_POINT) != 0;
}

//======================================================================================================================

bool
dns_obj_rr_dnskey_has_supported_algorithm(const dns_obj_rr_dnskey_t me)
{
	const uint8_t algorithm = dns_obj_rr_dnskey_get_algorithm(me);
	const uint16_t priority = dns_obj_rr_dnskey_algorithm_get_priority(algorithm);
	return (priority != 0);
}

//======================================================================================================================

bool
dns_obj_rr_dnskey_is_valid_for_dnssec(const dns_obj_rr_dnskey_t me, dns_obj_error_t * const out_error)
{
	dns_obj_error_t err;

	const bool zone_key = dns_obj_rr_dnskey_is_zone_key(me);
	require_action_quiet(zone_key, exit, err = DNS_OBJ_ERROR_UNSUPPORTED_ERR);

	const bool dns_protocol = dns_obj_rr_dnskey_get_protocol(me);
	require_action_quiet(dns_protocol, exit, err = DNS_OBJ_ERROR_UNSUPPORTED_ERR);

	const bool supported_algorithm = dns_obj_rr_dnskey_has_supported_algorithm(me);
	require_action_quiet(supported_algorithm, exit, err = DNS_OBJ_ERROR_UNSUPPORTED_ERR);

	err = DNS_OBJ_ERROR_NO_ERROR;
exit:
	if (out_error != NULL) {
		*out_error = err;
	}
	return err == DNS_OBJ_ERROR_NO_ERROR;
}

//======================================================================================================================

dns_obj_error_t
dns_obj_rr_dnskey_compute_digest(const dns_obj_rr_dnskey_t me, const uint8_t ds_digest_type,
	uint8_t out_digest[static const MAX_DIGEST_OUTPUT_SIZE], size_t * const out_digest_size)
{
	dns_obj_error_t err;
	*out_digest_size = 0;

	digest_type_t digest_type = dns_obj_rr_ds_digest_type_to_digest_type_enum(ds_digest_type);
	require_action(digest_type != DIGEST_UNSUPPORTED, exit, err = DNS_OBJ_ERROR_UNSUPPORTED_ERR);

	// Digest = DigestFunction(name | rdata)
	dns_obj_digest_ctx_t ctx;
	err = dns_obj_data_compute_digest_init(&ctx, digest_type);
	require_noerr(err, exit);

	const dns_obj_domain_name_t my_name = dns_obj_rr_get_name(me);
	err = dns_obj_data_compute_digest_update(&ctx, dns_obj_domain_name_get_labels(my_name), dns_obj_domain_name_get_length(my_name));
	require_noerr(err, exit);

	const uint8_t * const rdata = dns_obj_rr_get_rdata(me);
	err = dns_obj_data_compute_digest_update(&ctx, rdata, dns_obj_rr_get_rdata_len(me));
	require_noerr(err, exit);

	err = dns_obj_data_compute_digest_final(&ctx, out_digest);
	require_noerr(err, exit);
	*out_digest_size = dns_obj_data_compute_digest_get_output_size(digest_type);

	err = DNS_OBJ_ERROR_NO_ERROR;
exit:
	return err;
}

//======================================================================================================================
// MARK: - DNSKEY Public Functions

uint16_t
dns_obj_rr_dnskey_algorithm_get_priority(const uint8_t algorithm)
{
	// Get from <https://datatracker.ietf.org/doc/html/rfc8624#section-3.1>

	uint16_t priority;

	//			+--------+--------------------+-----------------+-------------------+
	//			| Number | Mnemonics          | DNS Signing  | DNS Validation |
	//			+--------+--------------------+-----------------+-------------------+
	switch (algorithm) {
		case DNSKEY_ALGORITHM_RSASHA1:
			//	| 5      | RSASHA1            | NOT RECOMMENDED | MUST              |
			priority = 1;
			break;
		case DNSKEY_ALGORITHM_RSASHA1_NSEC3_SHA1:
			//	| 7      | RSASHA1-NSEC3-SHA1 | NOT RECOMMENDED | MUST              |
			priority = 2;
			break;
		case DNSKEY_ALGORITHM_RSASHA256:
			//	| 8      | RSASHA256          | MUST            | MUST              |
			priority = 3;
			break;
		case DNSKEY_ALGORITHM_RSASHA512:
			//	| 10     | RSASHA512          | NOT RECOMMENDED | MUST              |
			priority = 4;
			break;
		case DNSKEY_ALGORITHM_ECDSAP256SHA256:
			//	| 13     | ECDSAP256SHA256    | MUST            | MUST              |
			priority = 5;
			break;
		case DNSKEY_ALGORITHM_ECDSAP384SHA384:
			//	| 14     | ECDSAP384SHA384    | MAY             | RECOMMENDED       |
			priority = 6;
			break;
		case DNSKEY_ALGORITHM_ED25519:
			//	| 15     | ED25519            | RECOMMENDED     | RECOMMENDED       |
			priority = 7;
			break;
		case DNSKEY_ALGORITHM_ED448:
			//	| 16     | ED448              | MAY             | RECOMMENDED       |
			priority = 8;
			break;
		default:
			//	| 1      | RSAMD5             | MUST NOT        | MUST NOT          |
			//	| 3      | DSA                | MUST NOT        | MUST NOT          |
			//	| 6      | DSA-NSEC3-SHA1     | MUST NOT        | MUST NOT          |
			//	| 12     | ECC-GOST           | MUST NOT        | MAY               |
			//	| 0      | Delete             | N/A             | N/A               |
			//	| 2      | Diffie-Hellman     | N/A             | NOT SUPPORTED     |
			//	| 4      | Reserved           | N/A             | N/A               |
			//	| 9      | Reserved           | N/A             | N/A               |
			//	| 11     | Reserved           | N/A             | N/A               |
			//	| 17-122 | Unassigned         | N/A             | N/A               |
			//	| 123-251| Reserved           | N/A             | N/A               |
			//	| 252    | Indirect Keys      | N/A             | N/A               |
			//	| 253    | Private algorithm  | N/A             | N/A               |
			//	| 254    | Private algorithm OID| N/A           | N/A               |
			//	| 255    | Reserved           | N/A             | N/A               |
			priority = 0;
			break;
	}

	return priority;
}

//======================================================================================================================
// MARK: - DNS DNSKEY Resource Record Private Methods

char *
_dns_obj_rr_dnskey_copy_rdata_rfc_description(const dns_obj_rr_dnskey_t me, dns_obj_error_t * const out_error)
{
	dns_obj_error_t err;
	char *description = NULL;

	// Get the public key base64 description length.
	const size_t public_key_base64_len = base_x_get_encoded_string_length(base_encoding_type_base64, dns_obj_rr_dnskey_get_public_key_size(me));

	// Calculate the buffer that holds the DNSKEY description.
	// <Flags> + ' ' + <Protocol> + ' ' + <Algorithm> + ' ' + <Key Tag> + ' ' + <Public Key In Base64> + '\0'
	char *const fake_buffer = NULL;
	const size_t buffer_len = (size_t)snprintf(fake_buffer, 0, "%u %u %u  (Key Tag: %u)",
		dns_obj_rr_dnskey_get_flags(me), dns_obj_rr_dnskey_get_protocol(me),
		dns_obj_rr_dnskey_get_algorithm(me), dns_obj_rr_dnskey_get_key_tag(me)) + public_key_base64_len + 1;

	// Create the buffer.
	description = mdns_calloc(1, buffer_len);
	require_action(description != NULL, exit, err = DNS_OBJ_ERROR_NO_MEMORY);

	// Put the value in.
	char *ptr = description;
	const char * const limit = description + buffer_len;
	int ret;
	ret = snprintf(ptr, (size_t)(limit - ptr), "%u %u %u ", dns_obj_rr_dnskey_get_flags(me), dns_obj_rr_dnskey_get_protocol(me),
		dns_obj_rr_dnskey_get_algorithm(me));
	require_action(ret > 0, exit, err = DNS_OBJ_ERROR_UNKNOWN_ERR);
	ptr += ret;

	const char * const public_key_base64 = base_x_encode(base_encoding_type_base64, dns_obj_rr_dnskey_get_public_key(me),
		dns_obj_rr_dnskey_get_public_key_size(me), ptr);
	require_action(public_key_base64 != NULL, exit, err = DNS_OBJ_ERROR_UNKNOWN_ERR);
	ptr += public_key_base64_len;

	ret = snprintf(ptr, (size_t)(limit - ptr), " (Key Tag: %u)", dns_obj_rr_dnskey_get_key_tag(me));
	require_action(ret > 0, exit, err = DNS_OBJ_ERROR_UNKNOWN_ERR);

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

// Based on reference implementation from <https://tools.ietf.org/html/rfc4034#appendix-B>.
static uint16_t
dns_obj_rr_dnskey_compute_key_tag(const uint8_t * const rdata, const uint16_t rdata_len)
{
	uint32_t accumulator = 0;

	// We do not support DNSKEY_ALGORITHM_RSAMD5 since it is unsafe, and we did not implement the key tag computation
	// algorithm for DNSKEY_ALGORITHM_RSAMD5.
	require_return_value(rdata_parser_dnskey_get_algorithm(rdata) != DNSKEY_ALGORITHM_RSAMD5, 0);

	for (size_t i = 0; i < rdata_len; i++) {
		accumulator += (i & 1) ? rdata[i] : (uint32_t)(rdata[i] << 8);
	}
	accumulator += (accumulator >> 16) & UINT32_C(0xFFFF);
	return (accumulator & UINT32_C(0xFFFF));
}
