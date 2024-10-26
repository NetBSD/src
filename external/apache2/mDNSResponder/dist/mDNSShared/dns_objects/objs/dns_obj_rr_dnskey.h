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

#ifndef DNS_OBJ_RR_DNSKEY_H
#define DNS_OBJ_RR_DNSKEY_H

//======================================================================================================================
// MARK: - Headers

#include "dns_obj.h"
#include "dns_obj_crypto.h"
#include "dns_common.h"
#include <stdint.h>
#include <stdbool.h>

#include "nullability.h"

//======================================================================================================================
// MARK: - Object Reference Definition

DNS_OBJECT_SUBKIND_TYPEDEF_OPAQUE_POINTER(rr, dnskey);

//======================================================================================================================
// MARK: - Object Constants

// DNSKEY flags
// Take from <https://tools.ietf.org/html/rfc4034#section-2.1.1>.
#define DNSKEY_FLAG_ZONE_KEY				(1U << (15 - 7))	// MSB bit 7
#define DNSKEY_FLAG_SECURITY_ENTRY_POINT	(1U << (15 - 15))	// MSB bit 15

// DNSKEY algorithms
// Taken from <https://www.iana.org/assignments/dns-sec-alg-numbers/dns-sec-alg-numbers.xhtml>.
typedef enum dnskey_algorithm_type {
	DNSKEY_ALGORITHM_DELETE				= 0,
	DNSKEY_ALGORITHM_RSAMD5				= 1,
	DNSKEY_ALGORITHM_DH					= 2,
	DNSKEY_ALGORITHM_DSA				= 3,
	// Reserved 4
	DNSKEY_ALGORITHM_RSASHA1			= 5,
	DNSKEY_ALGORITHM_DSA_NSEC3_SHA1		= 6,
	DNSKEY_ALGORITHM_RSASHA1_NSEC3_SHA1 = 7,
	DNSKEY_ALGORITHM_RSASHA256			= 8,
	// Reserved 9
	DNSKEY_ALGORITHM_RSASHA512			= 10,
	// Reserved 11
	DNSKEY_ALGORITHM_ECC_GOST			= 12,
	DNSKEY_ALGORITHM_ECDSAP256SHA256	= 13,
	DNSKEY_ALGORITHM_ECDSAP384SHA384	= 14,
	DNSKEY_ALGORITHM_ED25519			= 15,
	DNSKEY_ALGORITHM_ED448				= 16,
	// Unassigned 17 - 122
	// Reserved 123 - 251
	DNSKEY_ALGORITHM_INDIRECT			= 252,
	DNSKEY_ALGORITHM_PRIVATEDNS			= 253,
	DNSKEY_ALGORITHM_PRIVATEOID			= 254
	// Reserved 255
} dnskey_algorithm_type_t;

// DNSKEY protocol field
// Taken from <https://tools.ietf.org/html/rfc4034#section-2.1.2>.
#define DNSKEY_PROTOCOL_DNSSEC			3

//======================================================================================================================
// MARK: - Object Methods

/*!
 *	@brief
 *		Create an DNSKEY resource record object.
 *
 *	@param name
 *		The name of the DNSKEY resource record in domain name labels.
 *
 *	@param class
 *		The class of the DNSKEY record.
 *
 *	@param rdata
 *		The pointer to the rdata of the record, when it is NULL, it is negative response.
 *
 *	@param rdata_len
 *		The length of the rdata, when <code>rdata</code> is NULL, it should be zero.
 *
 *	@param allocate
 *		The boolean value to indicate whether to allocate new memory and copy all rdata from the memory region pointed by <code>name</code>,
 *		<code>rdata</code>. If it is false, the caller is required to ensure that <code>name</code> and <code>rdata</code> are always valid during the life time
 *		of this DNSKEY resource record object.
 *
 *	@param out_error
 *		The pointer to the error value indicates the success of the function call or the error encountered.
 *
 *	@result
 *		The DNSKEY resource record object created, or NULL if error happens during creation. <code>out_error</code> will be set to the error encountered if it is not NULL.
 */
dns_obj_rr_dnskey_t NULLABLE
dns_obj_rr_dnskey_create(const uint8_t * NONNULL name, uint16_t class, const uint8_t * NONNULL rdata,
	uint16_t rdata_len, bool allocate, dns_obj_error_t * NULLABLE out_error);

/*!
 *	@brief
 *		Get the flags of the DNSKEY resource record object.
 *
 *	@param dnskey
 *		The DNSKEY resource record object.
 *
 *	@result
 *		The flags of DNSKEY object.
 */
uint16_t
dns_obj_rr_dnskey_get_flags(dns_obj_rr_dnskey_t NONNULL dnskey);

/*!
 *	@brief
 *		Get the protocol of the DNSKEY resource record object.
 *
 *	@param dnskey
 *		The DNSKEY resource record object.
 *
 *	@result
 *		The protocol of DNSKEY object.
 */
uint8_t
dns_obj_rr_dnskey_get_protocol(dns_obj_rr_dnskey_t NONNULL dnskey);

/*!
 *	@brief
 *		Get the algorithm of the DNSKEY resource record object.
 *
 *	@param dnskey
 *		The DNSKEY resource record object.
 *
 *	@result
 *		The algorithm of DNSKEY object.
 */
uint8_t
dns_obj_rr_dnskey_get_algorithm(dns_obj_rr_dnskey_t NONNULL dnskey);

/*!
 *	@brief
 *		Get the public key contained in the DNSKEY resource record object.
 *
 *	@param dnskey
 *		The DNSKEY resource record object.
 *
 *	@result
 *		The public key in bytes.
 */
const uint8_t * NONNULL
dns_obj_rr_dnskey_get_public_key(dns_obj_rr_dnskey_t NONNULL dnskey);

/*!
 *	@brief
 *		Get the size of the public key in the DNSKEY resource record object.
 *
 *	@param dnskey
 *		The DNSKEY resource record object.
 *
 *	@result
 *		The public key size in bytes.
 */
uint16_t
dns_obj_rr_dnskey_get_public_key_size(dns_obj_rr_dnskey_t NONNULL dnskey);

/*!
 *	@brief
 *		Get the key tag of the public key in the DNSKEY resource record object.
 *
 *	@param dnskey
 *		The DNSKEY resource record object.
 *
 *	@result
 *		The key tag of the corresponding public key, which can be used to match RRSIG and DS records.
 */
uint16_t
dns_obj_rr_dnskey_get_key_tag(dns_obj_rr_dnskey_t NONNULL dnskey);

/*!
 *	@brief
 *		Check if the DNSKEY is a zone key that is used by DNSSEC to validates the signature.
 *
 *	@param dnskey
 *		The DNSKEY resource record object.
 *
 *	@result
 *		A boolean value to indicate whether the DNSKEY is a zone key.
 *
 *	@discussion
 *		If a DNSKEY is not a zone key, then this DNSKEY cannot be used for DNSSEC validation.
 */
bool
dns_obj_rr_dnskey_is_zone_key(dns_obj_rr_dnskey_t NONNULL dnskey);

/*!
 *	@brief
 *		Check if the DNSKEY is a secure entry point that is used by DNSSEC to validates the signature of the zone signing key, in other words, check if the
 *		DNSKEY contains the public key part of the key signing key. The DS record usually validates the DNSKEY that is a secure entry point.
 *
 *	@param dnskey
 *		The DNSKEY resource record object.
 *
 *	@result
 *		A boolean value to indicate whether the DNSKEY is a secure entry point.
 *
 *	@discussion
 *		The information about whether the DNSKEY is a secure entry point or not must not be used in the process validation. It is provided as a debugging tool. A
 *		DNSKEY that is not a secure entry point can also be used as a key signing key.
 */
bool
dns_obj_rr_dnskey_is_secure_entry_point(dns_obj_rr_dnskey_t NONNULL dnskey);

/*!
 *	@brief
 *		Check if the algorithm that this DNSKEY resource record object uses is supported by the current implementation.
 *
 *	@param dnskey
 *		The DNSKEY resource record object.
 *
 *	@result
 *		A boolean value that indicates whether the DNSKEY uses a supported algorithm.
 *
 *	@discussion
 *		We decide whether to support a DNSKEY algorithm based on the recommendations from <>
 */
bool
dns_obj_rr_dnskey_has_supported_algorithm(dns_obj_rr_dnskey_t NONNULL dnskey);

/*!
 *	@brief
 *		Check if the specified DNSKEY resource record object is valid to be used for DNSSEC validation.
 *
 *	@param dnskey
 *		The DNSKEY resource record object.
 *
 *	@param out_error
 *		The pointer to the error value indicates the success of the function call or the error that shows why the DNSKEY object is not valid for DNSSEC.
 *
 *	@result
 *		A boolean value to indicate whether the DNSKEY object can be used for DNSSEC.
 */
bool
dns_obj_rr_dnskey_is_valid_for_dnssec(dns_obj_rr_dnskey_t NONNULL dnskey, dns_obj_error_t * NULLABLE out_error);

/*!
 *	@brief
 *		Compute the digest of the DNSKEY resource record object based on the specified digest type.
 *
 *	@param dnskey
 *		The DNSKEY resource record object.
 *
 *	@param ds_digest_type
 *		The digest type value being specified in the DS resource record.
 *
 *	@param out_digest
 *		The output buffer that will be filled with the digest computed, its size in bytes must be greater than <code>MAX_DIGEST_OUTPUT_SIZE</code>.
 *
 *	@param out_digest_size
 *		The output digest size that will be set to the size of the generated digest, if no error occurs.
 *
 *	@result
 *		DNSSEC_ERROR_NO_ERROR if no error occurs, otherwise, the error code that indicates what went wrong while computing the digest.
 */
dns_obj_error_t
dns_obj_rr_dnskey_compute_digest(dns_obj_rr_dnskey_t NONNULL dnskey, uint8_t ds_digest_type,
	uint8_t out_digest[static MAX_DIGEST_OUTPUT_SIZE], size_t * NONNULL out_digest_size);

/*!
 *	@brief
 *		Get the priority of the algorithm used by the DNSKEY algorithm.
 *
 *	@param algorithm
 *		The algorithm of the DNSKEY resource record.
 *
 *	@result
 *		The priority of the DNSKEY algorithm, the higher the better. Priority 0 means that the corresponding algorithm is not supported and should not be used.
 */
uint16_t
dns_obj_rr_dnskey_algorithm_get_priority(uint8_t algorithm);

#endif // DNS_OBJ_RR_DNSKEY_H
