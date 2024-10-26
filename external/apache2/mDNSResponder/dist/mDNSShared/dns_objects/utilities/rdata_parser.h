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

#ifndef RDATA_PARSER_H
#define RDATA_PARSER_H

//======================================================================================================================
// MARK: - Headers

#include <stdint.h>
#include <stdbool.h>

#include "nullability.h"

//======================================================================================================================
// MARK: - CNAME Parser

/*!
 *	@brief
 *		Get the canonical name of the CNAME record owner specified in the rdata.
 *
 *	@param rdata
 *		The pointer to the rdata of the CNAME record.
 *
 *	@result
 *		The canonical name in domain name labels.
 */
const uint8_t * NONNULL
rdata_parser_cname_get_canonical_name(const uint8_t * NONNULL rdata);

/*!
 *	@brief
 *		Check if the rdata format of the CNAME is valid or not, by checking the minimal length of the CNAME member.
 *
 *	@param rdata
 *		The pointer to the rdata of the CNAME record.
 *
 *	@param rdata_len
 *		The length of the rdata.
 *
 *	@result
 *		True if the rdata passes the check, otherwise, false.
 *
 *	@discussion
 *		Note that the rdata must not have the compression pointer, or the validation would fail, because we need the expanded uncompressed RR data to to
 *		DNSSEC validation. The caller of the function should already expand all compression pointers in the record before calling it.
 */
bool
rdata_parser_cname_check_validity(const uint8_t * NONNULL rdata, uint16_t rdata_len);

//======================================================================================================================
// MARK: - SOA Parser

/*!
 *	@brief
 *		Get the minimum TTL in the SOA record.
 *
 *	@param rdata
 *		The pointer to the rdata of the SOA record.
 *
 *	@result
 *		The minimum TTL specified by the SOA record in seconds.
 */
uint32_t
rdata_parser_soa_get_minimum_ttl(const uint8_t * NONNULL rdata);

/*!
 *	@brief
 *		Check if the rdata format of the SOA is valid or not, by checking the minimal length of the SOA members.
 *
 *	@param rdata
 *		The pointer to the rdata of the SOA record.
 *
 *	@param rdata_len
 *		The length of the rdata.
 *
 *	@result
 *		True if the rdata passes the check, otherwise, false.
 *
 *	@discussion
 *		Note that the rdata must not have the compression pointer, or the validation would fail, because we need the expanded uncompressed RR data to to
 *		DNSSEC validation. The caller of the function should already expand all compression pointers in the record before calling it.
 */
bool
rdata_parser_soa_check_validity(const uint8_t * NONNULL rdata, uint16_t rdata_len);

//======================================================================================================================
// MARK: - SRV Parser

uint16_t
rdata_parser_srv_get_priority(const uint8_t * NONNULL rdata);

uint16_t
rdata_parser_srv_get_weight(const uint8_t * NONNULL rdata);

uint16_t
rdata_parser_srv_get_port(const uint8_t * NONNULL rdata);

const uint8_t * NONNULL
rdata_parser_srv_get_target(const uint8_t * NONNULL rdata);

bool
rdata_parser_srv_check_validity(const uint8_t * NONNULL rdata, uint16_t rdata_len);

//======================================================================================================================
// MARK: - NSEC Parser

/*!
 *	@brief
 *		Get the next domain name of the NSEC record.
 *
 *	@param rdata
 *		The pointer to the rdata of the NSEC record.
 *
 *	@result
 *		The next domain name of the NSEC record in domain name labels format.
 */
const uint8_t * NONNULL
rdata_parser_nsec_get_next_domain_name(const uint8_t * NONNULL rdata);

/*!
 *	@brief
 *		Get the type bit maps of the NSEC record.
 *
 *	@param rdata
 *		The pointer to the rdata of the NSEC record.
 *
 *	@param out_type_bit_maps_len
 *		The pointer to the type bit maps length value being returned for the returned type bit maps.
 *
 *	@result
 *		The type bit maps that shows what types of DNS record is covered by the current NSEC.
 */
const uint8_t * NULLABLE
rdata_parser_nsec_get_type_bit_maps(const uint8_t * NONNULL rdata, uint16_t rdata_len,
	uint16_t * NONNULL out_type_bit_maps_len);

/*!
 *	@brief
 *		Check if the rdata format of the NSEC is valid or not, by checking the minimal length of the NSEC members.
 *
 *	@param rdata
 *		The pointer to the rdata of the NSEC record.
 *
 *	@param rdata_len
 *		The length of the rdata.
 *
 *	@result
 *		True if the rdata passes the check, otherwise, false.
 */
bool
rdata_parser_nsec_check_validity(const uint8_t * NONNULL rdata, uint16_t rdata_len);

/*!
 *	@brief
 *		Check if the specified DNS type is covered by the type bit maps.
 *
 *	@param maps
 *		The pointer to the type bit maps of NSEC or NSEC3 record.
 *
 *	@param maps_len
 *		The length of the  type bit maps.
 *
 *	@param type
 *		The DNS type to be checked.
 *
 *	@result
 *		The boolean value indicates that whether the  type bit maps covers the specified DNS type.
 */
bool
rdata_parser_type_bit_maps_cover_dns_type(const uint8_t * NONNULL maps, uint16_t maps_len, uint16_t type);

//======================================================================================================================
// MARK: - DS Parser

/*!
 *	@brief
 *		Get the key tag from the DS rdata.
 *
 *	@param rdata
 *		The pointer to the DS rdata bytes.
 *
 *	@result
 *		The key tag that can be used to match a DNSKEY.
 */
uint16_t
rdata_parser_ds_get_key_tag(const uint8_t * NONNULL rdata);

/*!
 *	@brief
 *		Get the algorithm of the DNSKEY from the DS rdata.
 *
 *	@param rdata
 *		The pointer to the DS rdata bytes.
 *
 *	@result
 *		The algorithm of the corresponding DNSKEY matched by this DS record.
 */
uint8_t
rdata_parser_ds_get_algorithm(const uint8_t * NONNULL rdata);

/*!
 *	@brief
 *		Get the digest type from the DS rdata.
 *
 *	@param rdata
 *		The pointer to the DS rdata bytes.
 *
 *	@result
 *		The digest type that DS record uses to generate the digest.
 */
uint8_t
rdata_parser_ds_get_digest_type(const uint8_t * NONNULL rdata);

/*!
 *	@brief
 *		Get the digest in bytes from the DS rdata
 *
 *	@param rdata
 *		The pointer to the DS rdata bytes.
 *
 *	@result
 *		The digest of the corresponding DNSKEY record for this DS record.
 */
const uint8_t * NONNULL
rdata_parser_ds_get_digest(const uint8_t * NONNULL rdata);

/*!
 *	@brief
 *		Get the length of the digest contained in the DS record.
 *
 *	@param rdata_len
 *		The length of the DS rdata, in bytes.
 *
 *	@result
 *		The length of the digest, in bytes.
 */
uint16_t
rdata_parser_ds_get_digest_length(uint16_t rdata_len);

/*!
 *	@brief
 *		Check if the rdata with the length is a valid rdata for DS record.
 *
 *	@param rdata
 *		The pointer to the DS rdata bytes.
 *
 *	@param rdata_len
 *		The length of the DS rdata, in bytes.
 *
 *	@result
 *		True if the rdata is a valid DS rdata format, otherwise, false.
 */
bool
rdata_parser_ds_check_validity(const uint8_t * NONNULL rdata, uint16_t rdata_len);

//======================================================================================================================
// MARK: - RRSIG Parser

/*!
 *	@brief
 *		Get the DNS type covered by the RRSIG record.
 *
 *	@param rdata
 *		The pointer to the rdata of the RRSIG record.
 *
 *	@result
 *		The DNS type covered.
 */
uint16_t
rdata_parser_rrsig_get_type_covered(const uint8_t * NONNULL rdata);

/*!
 *	@brief
 *		Get the algorithm used by the DNSKEY to generate the current RRSIG record.
 *
 *	@param rdata
 *		The pointer to the rdata of the RRSIG record.
 *
 *	@result
 *		The algorithm of the DNSKEY.
 */
uint8_t
rdata_parser_rrsig_get_algorithm(const uint8_t * NONNULL rdata);

/*!
 *	@brief
 *		Get the number of uncompressed domain name labels in the RRSIG record (excluding the "*" wildcard matching).
 *
 *	@param rdata
 *		The pointer to the rdata of the RRSIG record record.
 *
 *	@result
 *		The number of uncompressed domain name labels.
 */
uint8_t
rdata_parser_rrsig_get_labels(const uint8_t * NONNULL rdata);

/*!
 *	@brief
 *		Get the original TTL of the records that are signed to  generate the current RRSIG record.
 *
 *	@param rdata
 *		The pointer to the rdata of the RRSIG record.
 *
 *	@result
 *		The original TTL of the records that are signed.
 */
uint32_t
rdata_parser_rrsig_get_original_ttl(const uint8_t * NONNULL rdata);

/*!
 *	@brief
 *		Get the expiration date of the RRSIG record.
 *
 *	@param rdata
 *		The pointer to the rdata of the RRSIG record.
 *
 *	@result
 *		The expiration date of the RRSIG in epoch time.
 */
uint32_t
rdata_parser_rrsig_get_signature_expiration(const uint8_t * NONNULL rdata);

/*!
 *	@brief
 *		Get the inception date of the RRSIG record.
 *
 *	@param rdata
 *		The pointer to the rdata of the RRSIG record.
 *
 *	@result
 *		The inception date of the RRSIG in epoch time.
 */
uint32_t
rdata_parser_rrsig_get_signature_inception(const uint8_t * NONNULL rdata);

/*!
 *	@brief
 *		Get the key tag of the DNSKEY used to generate the current RRSIG record.
 *
 *	@param rdata
 *		The pointer to the rdata of the RRSIG record.
 *
 *	@result
 *		The key tag of the DNSKEY.
 */
uint16_t
rdata_parser_rrsig_get_key_tag(const uint8_t * NONNULL rdata);

/*!
 *	@brief
 *		Get the signer name of the RRSIG record.
 *
 *	@param rdata
 *		The pointer to the rdata of the RRSIG record.
 *
 *	@result
 *		The signer name of the RRSIG in domain name labels format.
 */
const uint8_t * NONNULL
rdata_parser_rrsig_get_signer_name(const uint8_t * NONNULL rdata);

/*!
 *	@brief
 *		Get the signature contained in the RRSIG record.
 *
 *	@param rdata
 *		The pointer to the rdata of the RRSIG record.
 *
 *	@param rdata_len
 *		The length of the rdata.
 *
 *	@param out_signature_len
 *		The pointer to the signature length value being returned for the returned signature pointer.
 *
 *	@result
 *		The signature data with the signature length returned in <code>*out_signature_len</code>.
 */
const uint8_t * NONNULL
rdata_parser_rrsig_get_signature(const uint8_t * NONNULL rdata, uint16_t rdata_len,
	uint16_t * NONNULL out_signature_len);

/*!
 *	@brief
 *		Check if the rdata format of the RRSIG is valid or not by checking the minimal length of the RRSIG members.
 *
 *	@param rdata
 *		The pointer to the rdata of the RRSIG record.
 *
 *	@param rdata_len
 *		The length of the rdata.
 *
 *	@result
 *		True if the rdata passes the check, otherwise, false.
 */
bool
rdata_parser_rrsig_check_validity(const uint8_t * NONNULL rdata, uint16_t rdata_len);

//======================================================================================================================
// MARK: - DNSKEY Parser

/*!
 *	@brief
 *		Get the flags value from the DNSKEY rdata.
 *
 *	@param rdata
 *		The pointer to the rdata of the DNSKEY record.
 *
 *	@result
 *		The flags of DNSKEY record.
 */
uint16_t
rdata_parser_dnskey_get_flags(const uint8_t * NONNULL rdata);

/*!
 *	@brief
 *		Get the protocol of the DNSKEY.
 *
 *	@param rdata
 *		The pointer to the rdata of the DNSKEY record.
 *
 *	@result
 *		The protocol of the DNSKEY record.
 */
uint8_t
rdata_parser_dnskey_get_protocol(const uint8_t * NONNULL rdata);

/*!
 *	@brief
 *		Get the algorithm used by the DNSKEY record.
 *
 *	@param rdata
 *		The pointer to the rdata of the DNSKEY record.
 *
 *	@result
 *		The algorithm of the DNSKEY record.
 */
uint8_t
rdata_parser_dnskey_get_algorithm(const uint8_t * NONNULL rdata);

/*!
 *	@brief
 *		Get the public key bytes of the DNSKEY record.
 *
 *	@param rdata
 *		The pointer to the rdata of the DNSKEY record.
 *
 *	@result
 *		The public key in bytes.
 */
const uint8_t * NONNULL
rdata_parser_dnskey_get_public_key(const uint8_t * NONNULL rdata);

/*!
 *	@brief
 *		Get the size of the public key for the DNSKEY record.
 *
 *	@param rdata_len
 *		The length of the entire DNSKEY rdata in bytes.
 *
 *	@result
 *		The size of the public key bytes part of the DNSKEY rdata.
 */
uint16_t
rdata_parser_dnskey_get_public_key_size(uint16_t rdata_len);

/*!
 *	@brief
 *		Check if the rdata with the length is a valid rdata for DNSKEY record.
 *
 *	@param rdata
 *		The pointer to the DNSKEY rdata bytes.
 *
 *	@param rdata_len
 *		The length of the DNSKEY rdata, in bytes.
 *
 *	@result
 *		True if the rdata is a valid DNSKEY rdata format, otherwise, false.
 */
bool
rdata_parser_dnskey_check_validity(const uint8_t * NONNULL rdata, uint16_t rdata_len);

//======================================================================================================================
// MARK: - NSEC3 Parser

/*!
 *	@brief
 *		Get the hash algorithm used to generate the next hashed owner name of the NSEC3 record.
 *
 *	@param rdata
 *		The pointer to the rdata of the NSEC3 record.
 *
 *	@result
 *		The hash algorithm.
 */
uint8_t
rdata_parser_nsec3_get_hash_algorithm(const uint8_t * NONNULL rdata);

/*!
 *	@brief
 *		Get the flags of the NSEC3 record.
 *
 *	@param rdata
 *		The pointer to the rdata of the NSEC3 record.
 *
 *	@result
 *		The flags.
 */
uint8_t
rdata_parser_nsec3_get_flags(const uint8_t * NONNULL rdata);

/*!
 *	@brief
 *		Get the extra number of hash iteration applied to generate the next hashed owner name of the NSEC3 record.
 *
 *	@param rdata
 *		The pointer to the rdata of the NSEC3 record.
 *
 *	@result
 *		The extra number of hash iteration.
 */
uint16_t
rdata_parser_nsec3_get_iterations(const uint8_t * NONNULL rdata);

/*!
 *	@brief
 *		Get the length of the salt added to the hash value when generating the next hashed owner name of the NSEC3 record.
 *
 *	@param rdata
 *		The pointer to the rdata of the NSEC3 record.
 *
 *	@result
 *		The length of the salt.
 */
uint8_t
rdata_parser_nsec3_get_salt_length(const uint8_t * NONNULL rdata);

/*!
 *	@brief
 *		Get the salt added to the hash value when generating the next hashed owner name of the NSEC3 record.
 *
 *	@param rdata
 *		The pointer to the rdata of the NSEC3 record.
 *
 *	@result
 *		The salt.
 */
const uint8_t * NONNULL
rdata_parser_nsec3_get_salt(const uint8_t * NONNULL rdata);

/*!
 *	@brief
 *		Get the bytes length of the next hashed owner name of the NSEC3 record.
 *
 *	@param rdata
 *		The pointer to the rdata of the NSEC3 record.
 *
 *	@result
 *		The length of the next hashed owner name that is in binary format.
 */
uint8_t
rdata_parser_nsec3_get_hash_length(const uint8_t * NONNULL rdata);

/*!
 *	@brief
 *		Get the next hashed owner name of the NSEC3 record, in binary format.
 *
 *	@param rdata
 *		The pointer to the rdata of the NSEC3 record.
 *
 *	@result
 *		The next hashed owner name that is in binary format.
 */
const uint8_t * NONNULL
rdata_parser_nsec3_get_next_hashed_owner_name(const uint8_t * NONNULL rdata);

/*!
 *	@brief
 *		Get the type bit maps of the NSEC3 record.
 *
 *	@param rdata
 *		The pointer to the rdata of the NSEC3 record.
 *
 *	@param out_type_bit_maps_len
 *		The pointer to the type bit maps length value being returned for the returned type bit maps.
 *
 *	@result
 *		The type bit maps that shows what types of DNS record is covered by the current NSEC3.
 */
const uint8_t * NONNULL
rdata_parser_nsec3_get_type_bit_maps(const uint8_t * NONNULL rdata, uint16_t rdata_len,
	uint16_t * NONNULL out_type_bit_maps_len);

/*!
 *	@brief
 *		Check if the rdata format of the NSEC3 is valid or not, by checking the minimal length of the NSEC3 members.
 *
 *	@param rdata
 *		The pointer to the rdata of the NSEC3 record.
 *
 *	@param rdata_len
 *		The length of the rdata.
 *
 *	@result
 *		True if the rdata passes the check, otherwise, false.
 */
bool
rdata_parser_nsec3_check_validity(const uint8_t * NONNULL rdata, uint16_t rdata_len);

#endif // RDATA_PARSER_H
