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

#ifndef DNS_OBJ_OBJ_RR_DS_H
#define DNS_OBJ_OBJ_RR_DS_H

//======================================================================================================================
// MARK: - Headers

#include "dns_obj_rr_dnskey.h"
#include "dns_obj.h"
#include "dns_obj_crypto.h"
#include "dns_common.h"
#include <stdint.h>
#include <stdbool.h>

#include "nullability.h"

//======================================================================================================================
// MARK: - Object Reference Definition

DNS_OBJECT_SUBKIND_TYPEDEF_OPAQUE_POINTER(rr, ds);

//======================================================================================================================
// MARK: - Object Constants

// DS digest types
// Taken from <https://www.iana.org/assignments/ds-rr-types/ds-rr-types.xhtml>
typedef enum ds_digest_type {
	// Reserved 0
	DS_DIGEST_SHA_1				= 1,
	DS_DIGEST_SHA_256			= 2,
	DS_DIGEST_GOST_R_34_11_94	= 3,
	DS_DIGEST_SHA_384			= 4
	// Reserved 5 - 255
} ds_digest_type_t;

//======================================================================================================================
// MARK: - Object Methods

/*!
 *	@brief
 *		Create an DS resource record object.
 *
 *	@param name
 *		The name of the DS resource record in domain name labels.
 *
 *	@param class
 *		The class of the resource record .
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
 *		of this DS resource record object.
 *
 *	@param out_error
 *		The pointer to the error value indicates the success of the function call or the error encountered.
 *
 *	@result
 *		The DS resource record object created, or NULL if error happens during creation. <code>out_error</code> will be set to the error encountered if it is not NULL.
 */
dns_obj_rr_ds_t NULLABLE
dns_obj_rr_ds_create(const uint8_t * NONNULL name, uint16_t class, const uint8_t * NONNULL rdata,
	uint16_t rdata_len, bool allocate, dns_obj_error_t * NULLABLE out_error);

/*!
 *	@brief
 *		Get the key tag of the DNSKEY that this DS resource record object validates.
 *
 *	@param ds
 *		The DS resource record object.
 *
 *	@result
 *		The key tag of the corresponding DNSKEY.
 */
uint16_t
dns_obj_rr_ds_get_key_tag(dns_obj_rr_ds_t NONNULL ds);

/*!
 *	@brief
 *		Get the algorithm of the DNSKEY that this DS resource record object validates.
 *
 *	@param ds
 *		The DS resource record object.
 *
 *	@result
 *		The algorithm of the corresponding DNSKEY.
 */
uint8_t
dns_obj_rr_ds_get_algorithm(dns_obj_rr_ds_t NONNULL ds);

/*!
 *	@brief
 *		Get the digest type of the DS resource record object uses to compute the digest of the DNSKEY.
 *
 *	@param ds
 *		The DS resource record object.
 *
 *	@result
 *		The digest type used to compute the digest. Possible values are:
 *		DS_DIGEST_SHA_1
 *		DS_DIGEST_SHA_256
 *		DS_DIGEST_GOST_R_34_11_94
 *		DS_DIGEST_SHA_384
 *		0, or 5 - 255 which are currently not defined yet.
 */
uint8_t
dns_obj_rr_ds_get_digest_type(dns_obj_rr_ds_t NONNULL ds);

/*!
 *	@brief
 *		Get the digest of the DNSKEY that this DS resource record object validates.
 *
 *	@param ds
 *		The DS resource record object.
 *
 *	@result
 *		The digest of the corresponding DNSKEY.
 */
const uint8_t * NONNULL
dns_obj_rr_ds_get_digest(dns_obj_rr_ds_t NONNULL ds);

/*!
 *	@brief
 *		Get the digest length of the DNSKEY that this DS resource record object validates.
 *
 *	@param ds
 *		The DS resource record object.
 *
 *	@result
 *		The digest length of the corresponding DNSKEY.
 */
uint16_t
dns_obj_rr_ds_get_digest_length(dns_obj_rr_ds_t NONNULL ds);

/*!
 *	@brief
 *		Check if the DS resource record object refers to a DNSKEY RR whose algorithm number is what the system supports.
 *
 *	@param ds
 *		The DS resource record object.
 *
 *	@result
 *		True if the algorithm is supported, otherwise, false.
 */
bool
dns_obj_rr_ds_refers_to_supported_key_algorithm(dns_obj_rr_ds_t NONNULL ds);

/*!
 *	@brief
 *		Check if the specified DS resource record object is valid to be used for DNSSEC validation.
 *
 *	@param ds
 *		The DS resource record object.
 *
 *	@param out_error
 *		The pointer to the error value indicates the success of the function call or the error that shows why the DS object is not valid for DNSSEC.
 *
 *	@result
 *		A boolean value to indicate whether the DS object can be used for DNSSEC.
 */
bool
dns_obj_rr_ds_is_valid_for_dnssec(dns_obj_rr_ds_t NONNULL ds, dns_obj_error_t * NULLABLE out_error);

/*!
 *	@brief
 *		Check if the current DS resource record object validates the specified DNSKEY resource record object in the DNSSEC validation chain.
 *
 *	@param ds
 *		The DS resource record object.
 *
 *	@param dnskey
 *		The DNSKEY resource record object.
 *
 *	@param out_error
 *		The pointer to the error value indicates the success of the function call or the error that shows why the DS object does not validate the DNSKEY object.
 *
 *	@result
 *		A boolean value to indicate whether the DS object validates the DNSKEY object.
 */
bool
dns_obj_rr_ds_validates_dnskey(dns_obj_rr_ds_t NONNULL ds, dns_obj_rr_dnskey_t NONNULL dnskey,
	dns_obj_error_t * NULLABLE out_error);

/*!
 *	@brief
 *		Convert the DS's digest type value defined by RFC 4035 <https://www.rfc-editor.org/rfc/rfc4035.html> to the internal digest type
 *		representation.
 *
 *	@param ds_digest_type
 *		The DS digest type value specified in the packet.
 *
 *	@result
 *		The internal <code>digest_type_t</code> value that can be used to compute the digest, if the digest type is supported. Otherwise,
 *		DIGEST_UNSUPPORTED.
 */
digest_type_t
dns_obj_rr_ds_digest_type_to_digest_type_enum(uint16_t ds_digest_type);

#endif // DNS_OBJ_OBJ_RR_DS_H
