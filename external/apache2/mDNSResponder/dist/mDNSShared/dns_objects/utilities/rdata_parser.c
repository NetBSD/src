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
#include "rdata_parser.h"
#include "domain_name_labels.h"
#include "dns_common.h"

#include "dns_assert_macros.h"
#include "mdns_strict.h"

//======================================================================================================================
// MARK: - Private Functions

static bool
type_bit_maps_check_length(const uint8_t * const type_bit_maps, const uint16_t type_bit_maps_len)
{
	const uint8_t *ptr = type_bit_maps;
	const uint8_t * const ptr_limit = ptr + type_bit_maps_len;

	// Ensure each type bit map window is within the limit.
	uint16_t window_block_size;
	for (; ptr + 1 < ptr_limit; ptr += window_block_size) {
		window_block_size = 1 + 1 + *(ptr + 1);
	}
	require_return_value(ptr == ptr_limit, false);

	return true;
}

//======================================================================================================================
// MARK: - CNAME Parser

typedef struct rdata_cname_s {
	uint8_t	canonical_name[0];
} rdata_cname_t;

check_compile_time(offsetof(rdata_cname_t, canonical_name) == 0);

//======================================================================================================================

const uint8_t *
rdata_parser_cname_get_canonical_name(const uint8_t * const rdata)
{
	return ((const rdata_cname_t *)rdata)->canonical_name;
}

//======================================================================================================================

bool
rdata_parser_cname_check_validity(const uint8_t * const rdata, const uint16_t rdata_len)
{
	const size_t cname_length = domain_name_labels_length_with_limit(rdata, rdata + rdata_len);
	return (cname_length != 0);
}

//======================================================================================================================
// MARK: - SOA Parser

typedef struct rdata_soa_s {
	// uint8_t		primary_name_server[0];		// The domain name of the primary name server.
	// uint8_t		mailbox_name[0];			// The mail of the administrator in domain name format.
	uint32_t		serial;						// The serial number of the original copy of the zone.
	uint32_t		refresh;					// The time interval before the zone should refreshed.
	uint32_t		retry;						// The time interval before the failed refresh should be retried.
	uint32_t		expire;						// The time interval before the zone is no longer authoritative.
	uint32_t		minimum;					// The minimum TTL should be used for any RR from this zone.
} rdata_soa_t;

check_compile_time(offsetof(rdata_soa_t, serial)	== 0);
check_compile_time(offsetof(rdata_soa_t, refresh)	== 4);
check_compile_time(offsetof(rdata_soa_t, retry)		== 8);
check_compile_time(offsetof(rdata_soa_t, expire)	== 12);
check_compile_time(offsetof(rdata_soa_t, minimum)	== 16);
check_compile_time(sizeof(rdata_soa_t) == 20);

//======================================================================================================================

uint32_t
rdata_parser_soa_get_minimum_ttl(const uint8_t * const rdata)
{
	const uint8_t * const primary_name_server = rdata;
	const size_t primary_name_server_len = domain_name_labels_length(primary_name_server);

	const uint8_t * const mailbox_name = rdata + primary_name_server_len;
	const size_t mailbox_name_len = domain_name_labels_length(mailbox_name);

	const uint8_t * const minimum_ttl_bytes = rdata + primary_name_server_len + mailbox_name_len + offsetof(rdata_soa_t, minimum);
	return get_uint32_from_bytes(minimum_ttl_bytes);
}

//======================================================================================================================

bool
rdata_parser_soa_check_validity(const uint8_t * const rdata, const uint16_t rdata_len)
{
	const uint8_t * const rdata_limit = rdata + rdata_len;
	// Minimal size of the SOA rdata: <root domain name as the primary name server> + <root domain name as the mailbox>
	// + <size of the remaining fixed length members>
	const uint16_t min_rdata_len_soa = 1 + 1 + sizeof(rdata_soa_t);
	require_return_value(rdata_len >= min_rdata_len_soa, false);

	const uint8_t * const primary_name_server = rdata;
	const size_t primary_name_server_len = domain_name_labels_length_with_limit(primary_name_server, rdata_limit);
	// The shortest domain name is <root>.
	require_return_value(primary_name_server_len >= 1, false);
	require_return_value(primary_name_server_len + sizeof(rdata_soa_t) < rdata_len, false);

	const uint8_t * const mailbox_name = rdata + primary_name_server_len;
	const size_t mailbox_name_len = domain_name_labels_length_with_limit(mailbox_name, rdata_limit);
	require_return_value(mailbox_name_len >= 1, false);

	require_return_value(primary_name_server_len + mailbox_name_len + sizeof(rdata_soa_t) == rdata_len, false);

	return true;
}

//======================================================================================================================
// MARK: - SRV Parser

typedef struct rdata_srv_s {
	uint16_t priority;
	uint16_t weight;
	uint16_t port;
	uint8_t target[0];
} rdata_srv_t;

check_compile_time(offsetof(rdata_srv_t, priority)		== 0);
check_compile_time(offsetof(rdata_srv_t, weight)		== 2);
check_compile_time(offsetof(rdata_srv_t, port)			== 4);
check_compile_time(offsetof(rdata_srv_t, target)		== 6);
check_compile_time(sizeof(rdata_srv_t) == 6);

//======================================================================================================================

uint16_t
rdata_parser_srv_get_priority(const uint8_t * const rdata)
{
	return get_uint16_from_bytes(rdata);
}

//======================================================================================================================

uint16_t
rdata_parser_srv_get_weight(const uint8_t * const rdata)
{
	return get_uint16_from_bytes(rdata + offsetof(rdata_srv_t, weight));
}

//======================================================================================================================

uint16_t
rdata_parser_srv_get_port(const uint8_t * const rdata)
{
	return get_uint16_from_bytes(rdata + offsetof(rdata_srv_t, port));
}

//======================================================================================================================

const uint8_t *
rdata_parser_srv_get_target(const uint8_t * const rdata)
{
	return rdata + offsetof(rdata_srv_t, target);
}

//======================================================================================================================

bool
rdata_parser_srv_check_validity(const uint8_t * const UNUSED rdata, const uint16_t rdata_len)
{
	return rdata_len > offsetof(rdata_srv_t, target);
}

//======================================================================================================================
// MARK: - NSEC Parser

typedef struct rdata_nsec_s {
	uint8_t		next_domain_name[0];	// The next domain that exists in the zone.
	// uint8_t	type_bit_maps[0];		// The type bit map that indicates what DNS types are covered by the current NSEC record.
} rdata_nsec_t;

check_compile_time(offsetof(rdata_nsec_t, next_domain_name)	== 0);

//======================================================================================================================

const uint8_t *
rdata_parser_nsec_get_next_domain_name(const uint8_t * const rdata)
{
	return rdata;
}

//======================================================================================================================

const uint8_t *
rdata_parser_nsec_get_type_bit_maps(const uint8_t * const rdata, const uint16_t rdata_len,
	uint16_t * const out_type_bit_maps_len)
{
	const uint8_t * const next_domain_name = rdata_parser_nsec_get_next_domain_name(rdata);
	const size_t next_domain_name_len = domain_name_labels_length(next_domain_name);

	*out_type_bit_maps_len = rdata_len - (uint16_t)next_domain_name_len;

	return rdata + next_domain_name_len;
}

//======================================================================================================================

bool
rdata_parser_nsec_check_validity(const uint8_t * const rdata, const uint16_t rdata_len)
{
	// One byte to encode window block number.
	// One byte to encode bitmap length.
	// One byte to encode bitmap. (which means bitmap length is 1)
	const uint16_t min_type_bit_maps_len = 1 + 1 + 1;
	//The minimal size: One root domain name label + the minimal type bit maps.
	const uint16_t min_rdata_len_nsec = 1 + min_type_bit_maps_len;
	require_return_value(rdata_len >= min_rdata_len_nsec, false);

	const uint8_t * const next_domain_name = rdata_parser_nsec_get_next_domain_name(rdata);
	const size_t next_domain_name_len = domain_name_labels_length_with_limit(next_domain_name, rdata + rdata_len);
	// The shortest domain name is <root>.
	require_return_value(next_domain_name_len >= 1, false);
	// The type bit maps length has to be greater than 0.
	require_return_value(next_domain_name_len < rdata_len, false);

	// Check type bit maps format.
	uint16_t type_bit_maps_len;
	const uint8_t *type_bit_maps = rdata_parser_nsec_get_type_bit_maps(rdata, rdata_len, &type_bit_maps_len);
	const bool type_bit_maps_is_valid = type_bit_maps_check_length(type_bit_maps, type_bit_maps_len);
	require_return_value(type_bit_maps_is_valid, false);

	return true;
}

//======================================================================================================================

// For the detail of type bit maps and how the dns type is checked, go to:
// [RFC 4034 4.1.2. The Type Bit Maps Field](https://datatracker.ietf.org/doc/html/rfc4034#section-4.1.2)
bool
rdata_parser_type_bit_maps_cover_dns_type(const uint8_t * const type_bit_maps, const uint16_t type_bit_maps_len,
	const uint16_t type)
{
	bool covers;
	const uint8_t window_block = type / 256;
	const uint8_t offset_in_window_block = (uint8_t)(type % 256);
	const uint8_t *ptr = type_bit_maps;
	const uint8_t * const ptr_limit = ptr + type_bit_maps_len;

	uint16_t window_block_size;
	covers = false;
	for (; ptr + 1 < ptr_limit; ptr += window_block_size) {
		const uint8_t current_window_block = *ptr;
		const uint8_t current_bitmap_length = *(ptr + 1);
		const uint32_t current_bitmap_bit_count = current_bitmap_length * 8;
		const uint8_t * const current_bitmap = ptr + 2;
		const uint8_t mask_to_match = (uint8_t)(1 << (7 - (offset_in_window_block % 8)));

		// One octet to encode window block number.
		// One octet to encode bitmap length.
		// and current_bitmap_length.
		window_block_size = 1 + 1 + current_bitmap_length;
		if (ptr + window_block_size > ptr_limit) { // Ensure that the entire window is within the range.
			break;
		}

		if (current_window_block != window_block) {
			continue;
		}

		if (offset_in_window_block >= current_bitmap_bit_count) {
			continue;
		}

		if ((current_bitmap[offset_in_window_block / 8] & mask_to_match) != 0) {
			covers = true;
		}
	}

	return covers;
}

//======================================================================================================================
// MARK: - DS Parser

typedef struct rdata_ds_s {
	uint16_t	key_tag;		// The key tag of the corresponding DNSKEY that this DS record validates.
	uint8_t		algorithm;		// The algorithm of the corresponding DNSKEY that this DS record validates.
	uint8_t		digest_type;	// The digest type that the DS record uses to generate the DNSKEY digest.
	uint8_t		digest[0];		// The digest in bytes.
} rdata_ds_t;

check_compile_time(sizeof(rdata_ds_t)					== 4);
check_compile_time(offsetof(rdata_ds_t, key_tag)		== 0);
check_compile_time(offsetof(rdata_ds_t, algorithm)		== 2);
check_compile_time(offsetof(rdata_ds_t, digest_type)	== 3);
check_compile_time(offsetof(rdata_ds_t, digest)			== 4);

//======================================================================================================================

uint16_t
rdata_parser_ds_get_key_tag(const uint8_t * const rdata)
{
	return get_uint16_from_bytes(rdata);
}

//======================================================================================================================

uint8_t
rdata_parser_ds_get_algorithm(const uint8_t * const rdata)
{
	return rdata[offsetof(rdata_ds_t, algorithm)];
}

//======================================================================================================================

uint8_t
rdata_parser_ds_get_digest_type(const uint8_t * const rdata)
{
	return rdata[offsetof(rdata_ds_t, digest_type)];
}

//======================================================================================================================

const uint8_t *
rdata_parser_ds_get_digest(const uint8_t * const rdata)
{
	return rdata + offsetof(rdata_ds_t, digest);
}

//======================================================================================================================

uint16_t
rdata_parser_ds_get_digest_length(const uint16_t rdata_len)
{
	require_return_value(rdata_len >= offsetof(rdata_ds_t, digest), 0);
	return rdata_len - offsetof(rdata_ds_t, digest);
}

//======================================================================================================================

bool
rdata_parser_ds_check_validity(const uint8_t * const UNUSED rdata, const uint16_t rdata_len)
{
	return rdata_len > offsetof(rdata_ds_t, digest);
}

//======================================================================================================================
// MARK: - RRSIG Parser

typedef struct rdata_rrsig_s {
	uint16_t	type_covered;			// Indicates which DNS type RRSIG covers.
	uint8_t		algorithm;				// The DNSKEY algorithm that is used to sign the data.
	uint8_t		labels;					// The number of labels in the RRSIG owner name, is used to check wild matching.
	uint32_t	original_ttl;			// The original TTL of the records that are covered by the RRSIG, it is used to
										// reconstruct the signed data.
	uint32_t	signature_expiration;	// The epoch time when the RRSIG expires.
	uint32_t	signature_inception;	// The epoch time when the RRSIG should start to be valid to validate.
	uint16_t	key_tag;				// The key tag that identifies which DNSKEY it uses to generate the current RRSIG.
	uint8_t		signer_name[0];			// The signer name.
	// uint8_t	signature[0];			// The signature data in bytes.
} rdata_rrsig_t;

check_compile_time(offsetof(rdata_rrsig_t, type_covered)			== 0);
check_compile_time(offsetof(rdata_rrsig_t, algorithm)				== 2);
check_compile_time(offsetof(rdata_rrsig_t, labels)					== 3);
check_compile_time(offsetof(rdata_rrsig_t, original_ttl)			== 4);
check_compile_time(offsetof(rdata_rrsig_t, signature_expiration)	== 8);
check_compile_time(offsetof(rdata_rrsig_t, signature_inception)		== 12);
check_compile_time(offsetof(rdata_rrsig_t, key_tag)					== 16);
check_compile_time(offsetof(rdata_rrsig_t, signer_name)				== 18);

//======================================================================================================================

uint16_t
rdata_parser_rrsig_get_type_covered(const uint8_t * const rdata)
{
	return get_uint16_from_bytes(rdata + offsetof(rdata_rrsig_t, type_covered));
}

//======================================================================================================================

uint8_t
rdata_parser_rrsig_get_algorithm(const uint8_t * const rdata)
{
	return rdata[offsetof(rdata_rrsig_t, algorithm)];
}

//======================================================================================================================

uint8_t
rdata_parser_rrsig_get_labels(const uint8_t * const rdata)
{
	return rdata[offsetof(rdata_rrsig_t, labels)];
}

//======================================================================================================================

uint32_t
rdata_parser_rrsig_get_original_ttl(const uint8_t * const rdata)
{
	return get_uint32_from_bytes(rdata + offsetof(rdata_rrsig_t, original_ttl));
}

//======================================================================================================================

uint32_t
rdata_parser_rrsig_get_signature_expiration(const uint8_t * const rdata)
{
	return get_uint32_from_bytes(rdata + offsetof(rdata_rrsig_t, signature_expiration));
}

//======================================================================================================================

uint32_t
rdata_parser_rrsig_get_signature_inception(const uint8_t * const rdata)
{
	return get_uint32_from_bytes(rdata + offsetof(rdata_rrsig_t, signature_inception));
}

//======================================================================================================================

uint16_t
rdata_parser_rrsig_get_key_tag(const uint8_t * const rdata)
{
	return get_uint16_from_bytes(rdata + offsetof(rdata_rrsig_t, key_tag));
}

//======================================================================================================================

const uint8_t *
rdata_parser_rrsig_get_signer_name(const uint8_t * const rdata)
{
	return rdata + offsetof(rdata_rrsig_t, signer_name);
}

//======================================================================================================================

const uint8_t *
rdata_parser_rrsig_get_signature(const uint8_t * const rdata, const uint16_t rdata_len,
	uint16_t * const out_signature_len)
{
	const rdata_rrsig_t * const rrsig = (const rdata_rrsig_t *)rdata;
	const uint8_t * const signer_name = rrsig->signer_name;
	const size_t signer_name_len = domain_name_labels_length(signer_name);

	*out_signature_len = rdata_len - offsetof(rdata_rrsig_t, signer_name) - (uint16_t)signer_name_len;

	return rdata + offsetof(rdata_rrsig_t, signer_name) + signer_name_len;;
}

//======================================================================================================================

bool
rdata_parser_rrsig_check_validity(const uint8_t * const rdata, const uint16_t rdata_len)
{
	// Minimal size of the RRSIG rdata: <all the fields before signer_name> + <1 byte root domain> + <1 byte signature>
	const uint16_t min_rdata_len_rrsig = offsetof(rdata_rrsig_t, signer_name) + 1 + 1;
	require_return_value(rdata_len >= min_rdata_len_rrsig, false);

	const uint8_t * const signer_name = rdata_parser_rrsig_get_signer_name(rdata);
	const size_t signer_name_len = domain_name_labels_length_with_limit(signer_name, rdata + rdata_len);
	// The shortest domain name is <root>.
	require_return_value(signer_name_len >= 1, false);
	// The signature length has to be greater than 0.
	require_return_value(signer_name_len < rdata_len - offsetof(rdata_rrsig_t, signer_name), false);

	return true;
}

//======================================================================================================================
// MARK: - DNSKEY Parser

typedef struct rdata_dnskey_s {
	uint16_t	flags;			// The flags of the DNSKEY record.
	uint8_t		protocol;		// The protocol of the DNSKEY record.
	uint8_t		algorithm;		// The algorithm of the DNSKEY record.
	uint8_t		public_key[0];	// The public key bytes of the DNSKEY record.
} rdata_dnskey_t;

check_compile_time(sizeof(rdata_dnskey_t)				== 4);
check_compile_time(offsetof(rdata_dnskey_t, flags)		== 0);
check_compile_time(offsetof(rdata_dnskey_t, protocol)	== 2);
check_compile_time(offsetof(rdata_dnskey_t, algorithm)	== 3);
check_compile_time(offsetof(rdata_dnskey_t, public_key)	== 4);

//======================================================================================================================

uint16_t
rdata_parser_dnskey_get_flags(const uint8_t * const rdata)
{
	return get_uint16_from_bytes(rdata);
}

//======================================================================================================================

uint8_t
rdata_parser_dnskey_get_protocol(const uint8_t * const rdata)
{
	return rdata[offsetof(rdata_dnskey_t, protocol)];
}

//======================================================================================================================

uint8_t
rdata_parser_dnskey_get_algorithm(const uint8_t * const rdata)
{
	return rdata[offsetof(rdata_dnskey_t, algorithm)];
}

//======================================================================================================================

const uint8_t *
rdata_parser_dnskey_get_public_key(const uint8_t * const rdata)
{
	return rdata + offsetof(rdata_dnskey_t, public_key);
}

//======================================================================================================================

uint16_t
rdata_parser_dnskey_get_public_key_size(const uint16_t rdata_len)
{
	require_return_value(rdata_len >= offsetof(rdata_dnskey_t, public_key), 0);
	return rdata_len - offsetof(rdata_dnskey_t, public_key);
}

//======================================================================================================================

bool
rdata_parser_dnskey_check_validity(const uint8_t * const UNUSED rdata, const uint16_t rdata_len)
{
	return rdata_len > offsetof(rdata_dnskey_t, public_key);
}

//======================================================================================================================
// MARK: - NSEC3 Parser

typedef struct rdata_nsec3_s {
	uint8_t		hash_algorithm;				// The hash algorithm used to generate the next hashed owner name.
	uint8_t		flags;						// The NSEC flags.
	uint16_t	iterations;					// The number of extra hash iteration applied when generating the hash.
	uint8_t		salt_length;				// The length of the salt below.
	uint8_t		salt[0];					// The salt added to the hash computation.
	// uint8_t	hash_length;				// The length of the hash value bytes.
	// uint8_t	next_hashed_owner_name[0];	// The hash value in binary format.
	// uint8_t	type_bit_maps[0];			// The type bit maps that indicate what DNS types are covered by the current NSEC3 record.
} rdata_nsec3_t;

check_compile_time(offsetof(rdata_nsec3_t, hash_algorithm)	== 0);
check_compile_time(offsetof(rdata_nsec3_t, flags)			== 1);
check_compile_time(offsetof(rdata_nsec3_t, iterations)		== 2);
check_compile_time(offsetof(rdata_nsec3_t, salt_length)		== 4);
check_compile_time(offsetof(rdata_nsec3_t, salt)			== 5);

//======================================================================================================================

uint8_t
rdata_parser_nsec3_get_hash_algorithm(const uint8_t * const rdata)
{
	return rdata[offsetof(rdata_nsec3_t, hash_algorithm)];
}

//======================================================================================================================

uint8_t
rdata_parser_nsec3_get_flags(const uint8_t * const rdata)
{
	return rdata[offsetof(rdata_nsec3_t, flags)];
}

//======================================================================================================================

uint16_t
rdata_parser_nsec3_get_iterations(const uint8_t * const rdata)
{
	return get_uint16_from_bytes(rdata + offsetof(rdata_nsec3_t, iterations));
}

//======================================================================================================================

uint8_t
rdata_parser_nsec3_get_salt_length(const uint8_t * const rdata)
{
	return rdata[offsetof(rdata_nsec3_t, salt_length)];
}

//======================================================================================================================

const uint8_t *
rdata_parser_nsec3_get_salt(const uint8_t * const rdata)
{
	return rdata + offsetof(rdata_nsec3_t, salt);
}

//======================================================================================================================

uint8_t
rdata_parser_nsec3_get_hash_length(const uint8_t * const rdata)
{
	const uint8_t salt_length = rdata_parser_nsec3_get_salt_length(rdata);
	return rdata[offsetof(rdata_nsec3_t, salt) + salt_length];
}

//======================================================================================================================

const uint8_t *
rdata_parser_nsec3_get_next_hashed_owner_name(const uint8_t * const rdata)
{
	const uint8_t salt_length = rdata_parser_nsec3_get_salt_length(rdata);
	return (rdata + offsetof(rdata_nsec3_t, salt) + salt_length + sizeof(uint8_t));
}

//======================================================================================================================

const uint8_t *
rdata_parser_nsec3_get_type_bit_maps(const uint8_t * const rdata, const uint16_t rdata_len,
	uint16_t * const out_type_bit_maps_len)
{
	const uint8_t hash_length = rdata_parser_nsec3_get_hash_length(rdata);
	const uint8_t * const type_bit_maps = rdata_parser_nsec3_get_next_hashed_owner_name(rdata) + hash_length;
	*out_type_bit_maps_len = (uint16_t)(rdata + rdata_len - type_bit_maps);

	return type_bit_maps;
}

//======================================================================================================================

bool
rdata_parser_nsec3_check_validity(const uint8_t * const rdata, const uint16_t rdata_len)
{
	// Since NSEC3 does not need to cover itself, it is possible for a NSEC3 to have no type bit map, for example, the
	// NSEC3 record of the empty nonterminal.
	// 1 byte <hash algorithm> + 1 byte <flags> + 2 bytes <iterations> + 1 byte <salt length>
	// + 0 byte <salt, which means no salt> + 1 byte <hash length> + 1 byte <minimal hash length> + 0 bytes <minimal type bit maps>.
	const uint16_t min_rdata_len_nsec3 = offsetof(rdata_nsec3_t, salt) + sizeof(uint8_t) + 1;
	require_return_value(rdata_len >= min_rdata_len_nsec3, false);

	const uint8_t * const limit = rdata + rdata_len;

	// Check if the salt is within the limit.
	const uint8_t * const salt = rdata_parser_nsec3_get_salt(rdata);
	const uint8_t salt_len = rdata_parser_nsec3_get_salt_length(rdata);
	require_return_value(salt + salt_len < limit, false);

	// Check if the hash value is within the limit.
	const uint8_t * const next_hashed_owner_name = rdata_parser_nsec3_get_next_hashed_owner_name(rdata);
	const uint8_t hash_len = rdata_parser_nsec3_get_hash_length(rdata);
	require_return_value(next_hashed_owner_name + hash_len <= limit, false);

	// Check type bit maps format.
	uint16_t type_bit_maps_len;
	const uint8_t * const type_bit_maps = rdata_parser_nsec3_get_type_bit_maps(rdata, rdata_len, &type_bit_maps_len);
	const bool type_bit_maps_is_valid = type_bit_maps_check_length(type_bit_maps, type_bit_maps_len);
	require_return_value(type_bit_maps_is_valid, false);

	return true;
}
