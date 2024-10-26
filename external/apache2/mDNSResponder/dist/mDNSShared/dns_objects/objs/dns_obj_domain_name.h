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

#ifndef DNS_OBJ_DOMAIN_NAME_H
#define DNS_OBJ_DOMAIN_NAME_H

//======================================================================================================================
// MARK: - Headers

#include "dns_obj.h"
#include "dns_common.h"
#include <stdint.h>
#include <stdbool.h>

#include "nullability.h"

//======================================================================================================================
// MARK: - Object Reference Definition

DNS_OBJECT_TYPEDEF_OPAQUE_POINTER(domain_name);

#include "dns_obj_rr_nsec3.h"

//======================================================================================================================
// MARK: - Object Methods

/*!
 *	@brief
 *		Create a domain name object from the domain name labels.
 *
 *	@param labels
 *		The domain name labels.
 *
 *	@param allocate_memory
 *		The boolean value to indicate whether to copy the domain name labels to a newly allocated memory. If the it is false, the caller is responsible to ensure
 *		that the domain name labels is always valid during the life time of this domain name object.
 *
 *	@param out_error
 *		The error value indicates the success of the function call or the error encountered.
 *
 *	@result
 *		The domain name object created, or NULL if error happens during creation. <code>out_error</code> will be set to the error encountered if it is not NULL.
 */
dns_obj_domain_name_t NULLABLE
dns_obj_domain_name_create_with_labels(const uint8_t * NONNULL labels, const bool allocate_memory,
	dns_obj_error_t * NULLABLE out_error);

/*!
 *	@brief
 *		Concatenate two domain name object together to form a new domain name object, where <code>front_domain</code> is the left part, and
 *		<code>end_domain</code> is the right part.
 *
 *	@param front_domain
 *		The left part of the concatenation result, or the subdomain part.
 *
 *	@param end_domain
 *		The right part of the concatenation result, or the parent domain part.
 *
 *	@param out_error
 *		The pointer to the error value indicates the success of the function call or the error encountered.
 *
 *	@result
 *		The output domain object after the concatenation, or NULL if error happens.
 */
dns_obj_domain_name_t NULLABLE
dns_obj_domain_name_create_concatenation(dns_obj_domain_name_t NONNULL front_domain,
	dns_obj_domain_name_t NONNULL end_domain, dns_obj_error_t * NULLABLE out_error);

/*!
 *	@brief
 *		Append the subdomain name label(s) to a domain name object to form a new one.
 *
 *	@param subdomain_in_labels
 *		The left part of the concatenation result, or the subdomain part.
 *
 *	@param end_domain
 *		The right part of the concatenation result, or the parent domain part.
 *
 *	@param out_error
 *		The pointer to the error value indicates the success of the function call or the error encountered.
 *
 *	@result
 *		The output domain object after the concatenation, or NULL if error happens.
 */
dns_obj_domain_name_t NULLABLE
dns_obj_domain_name_create_concatenation_with_subdomain(const uint8_t * NONNULL subdomain_in_labels,
	dns_obj_domain_name_t NONNULL end_domain, dns_obj_error_t * NULLABLE out_error);

/*!
 *	@brief
 *		Create the domain name in canonical form based on the labels value of RRSIG record.
 *
 *	@param name
 *		The domain name object.
 *
 *	@param rrsig_labels
 *		The "labels" field of RRSIG rdata.
 *
 *	@param out_error
 *		The pointer to the error value indicates the success of the function call or the error encountered.
 *
 *	@result
 *		The domain name in canonical form. See <https://datatracker.ietf.org/doc/html/rfc4034#section-6.1> for Canonical DNS Name Order.
 */
dns_obj_domain_name_t NULLABLE
dns_obj_domain_name_create_canonical(dns_obj_domain_name_t NONNULL name, uint8_t rrsig_labels,
	dns_obj_error_t * NULLABLE out_error);

/*!
 *	@brief
 *		Create the domain name object from NULL-terminated C string.
 *
 *	@param name_cstring
 *		The domain name in C string format.
 *
 *	@param our_error
 *		The pointer to the error value indicates the success of the function call or the error encountered.
 *
 *	@result
 *		The domain name object created, or NULL if error happens (such as malformed C string domain name) during creation. <code>out_error</code> will be
 *		set to the error encountered if it is not NULL.
 *
 */
dns_obj_domain_name_t NULLABLE
dns_obj_domain_name_create_with_cstring(const char * NONNULL name_cstring, dns_obj_error_t * NULLABLE our_error);

/*!
 *	@brief
 *		Create a new domain name object from the existing one with newly allocated memory.
 *
 *	@param name
 *		The domain name object to be copied.
 *
 *	@param out_error
 *		The pointer to the error value indicates the success of the function call or the error encountered.
 *
 *	@result
 *		The domain name object copy, or NULL if error happens. <code>out_error</code> will be set to the error encountered if it is not NULL.
 */
dns_obj_domain_name_t NULLABLE
dns_obj_domain_name_create_copy(dns_obj_domain_name_t NONNULL name, dns_obj_error_t * NULLABLE out_error);

/*!
 *	@brief
 *		Get the domain name labels from the domain name object.
 *
 *	@param domain_name
 *		The domain name object.
 *
 *	@result
 *		The domain name labels passed in when the domain name object is created. If <code>allocate_memory</code> is false, the exact same pointer passed in
 *		when calling <code>dns_obj_domain_name_create_with_labels()</code> will be returned.
 */
const uint8_t * NONNULL
dns_obj_domain_name_get_labels(dns_obj_domain_name_t NONNULL domain_name);

/*!
 *	@brief
 *		Get the number of labels in the domain name from the domain name object.
 *
 *	@param domain_name
 *		The domain name object.
 *
 *	@result
 *		The number of labels.
 *
 *	@discussion
 *		The root label is not counted as a label count. Therefore, this function will return <code>0</code> for the root label.
 */
size_t
dns_obj_domain_name_get_label_count(dns_obj_domain_name_t NONNULL domain_name);

/*!
 *	@brief
 *		Get the length of the domain name labels for the domain name object.
 *
 *	@param domain_name
 *		The domain name object.
 *
 *	@result
 *		The length of the domain name labels of the domain name object.
 */
size_t
dns_obj_domain_name_get_length(dns_obj_domain_name_t NONNULL domain_name);

/*!
 *	@brief
 *		Convert the domain name object to C string name.
 *
 *	@param domain_name
 *		The domain name object.
 *
 *	@param out_name_cstring
 *		The output buffer that will be used to store the converted domain name C string. It has to be no less than <code>MAX_ESCAPED_DOMAIN_NAME</code>,
 *		or the result will be undefined.
 *
 *	@result
 *		The error value indicates the success of the function call or the error encountered. If the value is <code>DNSSEC_ERROR_NO_ERROR</code>,
 *		<code>out_name_cstring</code> will be filled with the converted domain name C string.
 *
 */
dns_obj_error_t
dns_obj_domain_name_to_cstring(dns_obj_domain_name_t NONNULL domain_name, char out_name_cstring[static MAX_ESCAPED_DOMAIN_NAME]);

/*!
 *	@brief
 *		Check if the domain name object is a root domain.
 *
 *	@param domain_name
 *		The domain name object to check.
 *
 *	@result
 *		True, if it is root domain, otherwise, false.
 */
bool
dns_obj_domain_name_is_root(dns_obj_domain_name_t NONNULL domain_name);

bool
dns_obj_domain_name_is_single_label(dns_obj_domain_name_t NONNULL domain_name);

/*!
 *	@brief
 *		Check if <code>domain_name</code> is a subdomain of <code>parent</code>.
 *
 *	@param domain_name
 *		The subdomain domain name object to be checked.
 *
 *	@param parent
 *		The parent domain name object to be checked.
 *
 *	@result
 *		True if <code>domain_name</code> is a subdomain of <code>parent</code>, false otherwise.
 *
 *	@discussion
 *		Note that if two domain name object are the same, this function will return <code>false</code>. Since www.apple.com .is not a real subdomain of
 *		www.apple.com.
 */
bool
dns_obj_domain_name_is_sub_domain_of(dns_obj_domain_name_t NONNULL domain_name, dns_obj_domain_name_t NONNULL parent);

/*!
 *	@brief
 *		Copy the parent domain of the domain name object, the level of the parent is specified by <code>index</code>.
 *
 *	@param domain_name
 *		The domain name object.
 *
 *	@param index
 *		The level of the parent domain, for example, for domain name labels: www.apple.com.
 *		index 0 means www.apple.com.
 *		index 1 means apple.com.
 *		index 2 means com.
 *		index 3 means <root>.
 *		index 4 or more means nothing.
 *
 *	@param out_error
 *		The pointer to the error value indicates the success of the function call or the error encountered.
 *
 *	@result
 *		The parent domain name object if index is less than or equal to <code>dns_obj_domain_name_get_label_count(labels)</code>, otherwise, NULL.
 */
dns_obj_domain_name_t NULLABLE
dns_obj_domain_name_copy_parent_domain(dns_obj_domain_name_t NONNULL domain_name, size_t index, dns_obj_error_t * NULLABLE out_error);

/*!
 *	@brief
 *		Compute and create a new domain name object that is the closest common ancestor of <code>domain_name</code> and <code>other</code>.
 *
 *	@param domain_name
 *		The domain name object.
 *
 *	@param other
 *		The domain name object.
 *
 *	@param out_error
 *		The pointer to the error value indicates the success of the function call or the error encountered.
 *
 *	@result
 *		A new domain name object that is the closest common ancestor of the two domain names passed in.
 *
 *	@discussion
 *		The closest common ancestor of two domain names is the longest name matching of them starting from the root.
 */
dns_obj_domain_name_t NULLABLE
dns_obj_domain_name_copy_closest_common_ancestor(dns_obj_domain_name_t NONNULL domain_name,
	dns_obj_domain_name_t NONNULL other, dns_obj_error_t * NONNULL out_error);

/*!
 *	@brief
 *		Check if the domain name is a "wildcard domain name".
 *
 *	@param domain_name
 *		The domain name object to check.
 *
 *	@result
 *		True, if the domain name is a wildcard domain name, otherwise, false.
 *
 *	@discussion
 *		For the definition of the "wildcard domain name", see [RFC 4592 2.1.1. Wildcard Domain Name and Asterisk Label](https://datatracker.ietf.org/doc/html/rfc4592#section-2.1.1)
 */
bool
dns_obj_domain_name_is_wildcard_domain_name(dns_obj_domain_name_t NONNULL domain_name);

/*!
 *	@brief
 *		Check if the <code>domain_name</code> is a wildcard expansion of wildcard domain name <code>wildcard</code>.
 *
 *	@param domain_name
 *		The domain name object to be checked.
 *
 *	@param wildcard
 *		The wildcard domain name to match.
 *
 *	@result
 *		True, if the <code>domain_name</code> is a wildcard expansion of wildcard domain name <code>wildcard</code>, otherwise, false.
 */
bool
dns_obj_domain_name_is_a_wildcard_expansion(dns_obj_domain_name_t NONNULL domain_name,
	dns_obj_domain_name_t NONNULL wildcard);

/*!
 * @brief
 * 		Calculate and set the base32 hex encoded hashed domain name, as specified by the parameters provided.
 *
 * 	@param domain_name
 * 		The domain name object to calculate the hash.
 *
 * 	@param algorithm
 * 		The algorithm that will be used to calculate the hash value.
 *
 * 	@param iterations
 * 		The number of additional hash iterations performed when generating the hash.
 *
 * 	@param salt
 * 		The salt added to the data to be hashed to make hash output more difficult to guess.
 *
 * 	@param salt_length
 * 		The length of the salt.
 *
 * 	@param zone_domain
 * 		The zone that generates the hash value. This is required because all NSEC3 record name has the zone domain appended. To be able to compare the
 * 		hashed name with the NSEC3 name, the zone domain has to be specified.
 *
 * 	@result
 *		The error value indicates the success of the function call or the error encountered.
 *
 *	@discussion
 *		After calling <code>dns_obj_domain_name_set_nsec3_hashed_name_with_params()</code>, if no error occurs, the hashed name can be get by calling
 *		<code>dns_obj_domain_name_get_nsec3_hashed_name()</code>.
 */
dns_obj_error_t
dns_obj_domain_name_set_nsec3_hashed_name_with_params(dns_obj_domain_name_t NONNULL domain_name, uint8_t algorithm,
	uint16_t iterations, const uint8_t * NULLABLE salt, uint8_t salt_length, const uint8_t * NONNULL zone_domain);

/*!
 *	@brief
 *		Get the base32 hex encoded hashed domain name object.
 *
 *	@param domain_name
 *		The domain name object.
 *
 * 	@param nsec3
 * 		The NSEC3 resource record that provides the parameters required to calculate the hash.
 *
 *	@result
 *		The base32 hex encoded hashed domain name object if NONNULL <code>nsec3</code> has been provided when calling this function previously,
 *		or NULL if <code>dns_obj_domain_name_get_nsec3_hashed_name()</code> has never been called with NONNULL <code>nsec3</code>.
 *
 *	@discussion
 *		If <code>dns_obj_domain_name_get_nsec3_hashed_name()</code> is called with NONNULL <code>nsec3</code> for the first time, the hash value
 *		will be computed and the result base32 hex encoded hashed domain name object will be returned.
 *		Note that the final hashed name is encoded as base32 hex (without padding) to be abled to be compared with other domain name object.
 */
dns_obj_domain_name_t NULLABLE
dns_obj_domain_name_get_nsec3_hashed_name(dns_obj_domain_name_t NONNULL domain_name, dns_obj_rr_nsec3_t NULLABLE nsec3);

/*!
 *	@brief
 *		Clear previously calculated hashed domain name that is specified by the NSEC3 resource record object or the parameters, so a new hash value can be
 *		calculated based on the new NSEC3 record.
 *
 *	@param domain_name
 *		The domain name object.
 */
void
dns_obj_domain_name_clear_nsec3_hashed_name(dns_obj_domain_name_t NONNULL domain_name);

#endif // DNS_OBJ_DOMAIN_NAME_H
