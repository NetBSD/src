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

#ifndef DNS_OBJ_RR_NSEC3_H
#define DNS_OBJ_RR_NSEC3_H

//======================================================================================================================
// MARK: - Headers

#include "dns_obj.h"
#include "dns_common.h"
#include <stdint.h>
#include <stdbool.h>

#include "nullability.h"

//======================================================================================================================
// MARK: - Object Reference Definition

DNS_OBJECT_SUBKIND_TYPEDEF_OPAQUE_POINTER(rr, nsec3);

// Has to be after the definition of dns_obj_rr_nsec3_t because dns_obj_domain_name_t uses dns_obj_rr_nsec3_t.
#include "dns_obj_domain_name.h"

//======================================================================================================================
// MARK: - Object Methods

/*!
 *	@brief
 *		Create a NSEC3 resource record object.
 *
 *	@param name
 *		The name of the NSEC3 resource record in domain name labels.
 *
 *	@param rdata
 *		The pointer to the rdata of the record, when it is NULL, it is negative response.
 *
 *	@param rdata_len
 *		The length of the rdata, when <code>rdata</code> is NULL, it should be zero.
 *
 *	@param allocate_memory
 *		The boolean value to indicate whether to allocate new memory and copy all rdata from the memory region pointed by <code>name</code>,
 *		<code>rdata</code>. If it is false, the caller is required to ensure that <code>name</code> and <code>rdata</code> are always valid during the life time
 *		of this NSEC resource record object.
 *
 *	@param out_error
 *		The pointer to the error value indicates the success of the function call or the error encountered.
 *
 *	@result
 *		The NSEC3 resource record object created, or NULL if error happens during creation. <code>out_error</code> will be set to the error encountered if it is not NULL.
 */
dns_obj_rr_nsec3_t NULLABLE
dns_obj_rr_nsec3_create(const uint8_t * NONNULL name, const uint8_t * NONNULL rdata, uint16_t rdata_len,
	bool allocate_memory, dns_obj_error_t * NULLABLE out_error);

/*!
 *	@brief
 *		Get the current owner name of the NSEC3 resource record object.
 *
 *	@param nsec3
 *		The NSEC3 resource record object.
 *
 *	@result
 *		The domain name object of the current owner name.
 */
dns_obj_domain_name_t NONNULL
dns_obj_rr_nsec3_get_current_owner_name(dns_obj_rr_nsec3_t NONNULL nsec3);

/*!
 *	@brief
 *		Get the hash algorithm used to generate the next hashed owner name of the NSEC3 resource record object.
 *
 *	@param nsec3
 *		The NSEC3 resource record object.
 *
 *	@result
 *		The hash algorithm type.
 */
uint8_t
dns_obj_rr_nsec3_get_hash_algorithm(dns_obj_rr_nsec3_t NONNULL nsec3);

/*!
 *	@brief
 *		Get the flags of the NSEC3 resource record object.
 *
 *	@param nsec3
 *		The NSEC3 resource record object.
 *
 *	@result
 *		The flags.
 */
uint8_t
dns_obj_rr_nsec3_get_flags(dns_obj_rr_nsec3_t NONNULL nsec3);

/*!
 *	@brief
 *		Get the number of extra hash iteration performed to generate the next hashed owner name of the NSEC3 resource record object.
 *
 *	@param nsec3
 *		The NSEC3 resource record object.
 *
 *	@result
 *		The number of extra hash iteration.
 *
 *	@discussion
 *		Note that the returned iteration is the extra iteration times performed, which means the original data is already hashed once.
 */
uint16_t
dns_obj_rr_nsec3_get_iterations(dns_obj_rr_nsec3_t NONNULL nsec3);

/*!
 *	@brief
 *		Get the length of the salt that will be used to calculate the NSEC3 hash.
 *
 *	@param nsec3
 *		The NSEC3 resource record object.
 *
 *	@result
 *		The length of the salt.
 */
uint8_t
dns_obj_rr_nsec3_get_salt_length(dns_obj_rr_nsec3_t NONNULL nsec3);

/*!
 *	@brief
 *		Get the salt added when doing hash for the he next hashed owner name of the NSEC3 resource record object.
 *
 *	@param nsec3
 *		The NSEC3 resource record object.
 *
 *	@param out_salt_length
 *		The pointer to the salt length value being returned for the returned salt pointer.
 *
 *	@result
 *		The salt.
 */
const uint8_t * NONNULL
dns_obj_rr_nsec3_get_salt(dns_obj_rr_nsec3_t NONNULL nsec3, uint8_t * NONNULL out_salt_length);

/*!
 *	@brief
 *		Get the next hashed owner name of the NSEC3 resource record object in raw, unprocessed binary format.
 *
 *	@param nsec3
 *		The NSEC3 resource record object.
 *
 *	@result
 *		The next hashed owner name in binary.
 */
const uint8_t * NONNULL
dns_obj_rr_nsec3_get_next_hashed_owner_name_in_binary(dns_obj_rr_nsec3_t NONNULL nsec3,
	uint8_t * NONNULL out_hash_length);

/*!
 *	@brief
 *		Get the next hashed owner name of the NSEC3 resource record object in domain name object format, with base 32 hex encoding.
 *
 *	@param nsec3
 *		The NSEC3 resource record object.
 *
 *	@result
 *		The next hashed owner name in domain name object format, , with base 32 hex encoding.
 */
dns_obj_domain_name_t NONNULL
dns_obj_rr_nsec3_get_next_hashed_owner_name(dns_obj_rr_nsec3_t NONNULL nsec3);

/*!
 *	@brief
 *		Check whether the NSEC3 resource record object has enabled Opt-out.
 *
 *	@param nsec3
 *		The NSEC3 resource record object.
 *
 *	@result
 *		True if the NSEC3 record has Opt-out bit set, otherwise, false.
 */
bool
dns_obj_rr_nsec3_get_opt_out_enabled(dns_obj_rr_nsec3_t NONNULL nsec3);

/*!
 *	@brief
 *		Check whether the DNS type is covered by the NSEC3 resource record object.
 *
 *	@param nsec3
 *		The NSEC3 resource record object.
 *
 *	@param type
 *		The DNS type to check if it is covered by the NSEC3 resource record object.
 *
 *	@result
 *		A boolean value to indicate whether the specified DNS type is covered by the NSEC3 resource record object.
 */
bool
dns_obj_rr_nsec3_covers_dns_type(dns_obj_rr_nsec3_t NONNULL nsec3, uint16_t type);

/*!
 *	@brief
 *		Check if two nsec3 resource record objects have the same closest  parent domain, in other word, to check if they come from the same zone.
 *
 *	@param nsec3_1
 *		One nsec3 resource record object to check.
 *
 *	@param nsec3_2
 *		Another nsec3 resource record object to check.
 *
 *	@result
 *		A boolean value to indicate f two nsec3 resource record objects have the same closest  parent domain.
 */
bool
dns_obj_rr_nsec3_have_same_closest_parent(dns_obj_rr_nsec3_t NONNULL nsec3_1, dns_obj_rr_nsec3_t NONNULL nsec3_2);

/*!
 *	@brief
 *		Check if two NSEC3 resource record objects have the same NSEC3 parameters, including algorithm, iterations and salt.
 *
 *	@param nsec3
 *		The NSEC3 resource record object.
 *
 *	@param other
 *		The other NSEC3 resource record object.
 *
 *	@result
 *		True if two NSEC3 have the same NSEC3 parameters, otherwise, false.
 */
bool
dns_obj_rr_nsec3_has_same_nsec3_parameters(dns_obj_rr_nsec3_t NONNULL nsec3, dns_obj_rr_nsec3_t NONNULL other);

/*!
 *	@brief
 *		Check if NSEC3 resource record has a reasonable low iteration value that would not introduce too much computational cost.
 *
 *	@param nsec3
 *		The NSEC3 resource record object.
 *
 *	@result
 *		True if iteration value is a reasonable value, otherwise, false.
 */
bool
dns_obj_rr_nsec3_has_reasonable_iterations(dns_obj_rr_nsec3_t NONNULL nsec3);

/*!
 *	@brief
 *		Check if the NSEC3 resource record has valid settings by checking the rdata, if not, it should be ignored when processing the denial of existence record.
 *
 *	@param nsec3
 *		The NSEC3 resource record object.
 *
 *	@result
 *		True if it has invalid NSEC3 parameter settings and should be ignored, otherwise, false.
 */
bool
dns_obj_rr_nsec3_should_be_ignored(dns_obj_rr_nsec3_t NONNULL nsec3);

/*!
 *	@brief
 *		Check if the NSEC3 resource record asserts that the domain name exists by proving the name is covered by it.
 *
 *	@param nsec3
 *		The NSEC3 resource record object.
 *
 *	@param qname
 *		The domain name object.
 *
 *	@param qclass
 *		The class of the question.
 *
 *	@result
 *		True, if it proves that name exists, otherwise, false.
 */
bool
dns_obj_rr_nsec3_asserts_name_exists(dns_obj_rr_nsec3_t NONNULL nsec3, dns_obj_domain_name_t NONNULL qname,
	uint16_t qclass);

/*!
 *	@brief
 *		Check if the NSEC3 resource record asserts that the domain name exists but the requested DNS data does not exist.
 *
 *	@param nsec3
 *		The NSEC3 resource record object.
 *
 *	@param qname
 *		The domain name object.
 *
 *	@param qclass
 *		The class of the question.
 *
 *	@param qtype
 *		The type of the DNS data.
 *
 *	@result
 *		True, if it proves that name exists and the data does not exist, otherwise, false.
 */
bool
dns_obj_rr_nsec3_asserts_name_exists_data_does_not_exist(dns_obj_rr_nsec3_t NONNULL nsec3,
	dns_obj_domain_name_t NONNULL qname, uint16_t qclass, uint16_t qtype);

/*!
 *	@brief
 *		Check if the NSEC3 resource record asserts that the domain name exists by proving the name is covered by it.
 *
 *	@param nsec3
 *		The NSEC3 resource record object.
 *
 *	@param qname
 *		The domain name object.
 *
 *	@param qclass
 *		The class of the question.
 *
 *	@result
 *		True, if it proves that name exists, otherwise, false.
 */
bool
dns_obj_rr_nsec3_asserts_name_does_not_exist(dns_obj_rr_nsec3_t NONNULL nsec3, dns_obj_domain_name_t NONNULL qname,
	uint16_t qclass);

/*!
 *	@brief
 *		Check if the current NSEC3 record can be used to prove that the delegation is insecure.
 *
 *	@param nsec3
 *		The NSEC3 resource record object.
 *
 *	@result
 *		True if it can be used to prove that the current delegation is insecure, otherwise, false.
 */
bool
dns_obj_rr_nsec3_is_usable_for_insecure_validation(dns_obj_rr_nsec3_t NONNULL nsec3);

#endif // DNS_OBJ_RR_NSEC3_H
