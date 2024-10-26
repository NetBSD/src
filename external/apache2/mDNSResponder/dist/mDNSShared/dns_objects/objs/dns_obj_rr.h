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

#ifndef DNS_OBJ_RR_H
#define DNS_OBJ_RR_H

//======================================================================================================================
// MARK: - Headers

#include "dns_obj.h"
#include "dns_obj_domain_name.h"
#include "dns_common.h"
#include <stdint.h>
#include <stdbool.h>

#include "nullability.h"

//======================================================================================================================
// MARK: - Object Reference Definition

DNS_OBJECT_TYPEDEF_OPAQUE_POINTER(rr);

//======================================================================================================================
// MARK: - The Resource Record and Its Subkind.

// All the subkind of resource record object needs to add `DNS_OBJECT_DECLARE_SUPPORTED_OBJECT_SUBKIND(rr, <subkind object type name>)`
// to let the subkind be able to use the resource record method declared here.
typedef union {
	DNS_OBJECT_DECLARE_SUPPORTED_OBJECT(rr);
	DNS_OBJECT_DECLARE_SUPPORTED_OBJECT_SUBKIND(rr, cname);
	DNS_OBJECT_DECLARE_SUPPORTED_OBJECT_SUBKIND(rr, soa);
	DNS_OBJECT_DECLARE_SUPPORTED_OBJECT_SUBKIND(rr, srv);
	DNS_OBJECT_DECLARE_SUPPORTED_OBJECT_SUBKIND(rr, ds);
	DNS_OBJECT_DECLARE_SUPPORTED_OBJECT_SUBKIND(rr, nsec);
	DNS_OBJECT_DECLARE_SUPPORTED_OBJECT_SUBKIND(rr, rrsig);
	DNS_OBJECT_DECLARE_SUPPORTED_OBJECT_SUBKIND(rr, dnskey);
	DNS_OBJECT_DECLARE_SUPPORTED_OBJECT_SUBKIND(rr, nsec3);
} dns_obj_rr_any_t __attribute__((__transparent_union__));

typedef union {
	DNS_OBJECT_ARRAY_DECLARE_SUPPORTED_OBJECT(rr);
	DNS_OBJECT_ARRAY_DECLARE_SUPPORTED_OBJECT_SUBKIND(rr, nsec);
	DNS_OBJECT_ARRAY_DECLARE_SUPPORTED_OBJECT_SUBKIND(rr, dnskey);
	DNS_OBJECT_ARRAY_DECLARE_SUPPORTED_OBJECT_SUBKIND(rr, nsec3);
} dns_objs_rr_any_t __attribute__((__transparent_union__));

//======================================================================================================================
// MARK: - Object Methods

/*!
 *	@brief
 *		Create a resource record object.
 *
 *	@param name
 *		The name of the resource record in domain name labels.
 *
 *	@param type
 *		The type of the record.
 *
 *	@param class
 *		The class of the record.
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
 *		of this resource record object.
 *
 *	@param out_error
 *		The pointer to the error value indicates the success of the function call or the error encountered.
 *
 *	@result
 *		The resource record object created, or NULL if error happens during creation. <code>out_error</code> will be set to the error encountered if it is not NULL.
 */
dns_obj_rr_t NULLABLE
dns_obj_rr_create(const uint8_t * NONNULL name, uint16_t type, uint16_t class, const uint8_t * NULLABLE rdata,
	uint16_t rdata_len, bool allocate_memory, dns_obj_error_t * NULLABLE out_error);

/*!
 *	@brief
 *		Get the name of the resource record in domain name object format.
 *
 *	@param record
 *		The resource record object.
 *
 *	@result
 *		The domain name object. It is ensured to be valid as long as the resource record object is valid.
 */
dns_obj_domain_name_t NONNULL
dns_obj_rr_get_name(dns_obj_rr_any_t record);

/*!
 *	@brief
 *		Get the name of the resource record in domain name labels format.
 *
 *	@param record
 *		The resource record object.
 *
 *	@result
 *		The domain name labels. If <code>allocate_memory</code> is set to false, the pointer returned will be the same as the one when creating this resource
 *		record.
 */
const uint8_t * NONNULL
dns_obj_rr_get_name_in_labels(dns_obj_rr_any_t record);

/*!
 *	@brief
 *		Get the type of the resource record.
 *
 *	@param record
 *		The resource record object.
 *
 *	@result
 *		The DNS record type.
 */
uint16_t
dns_obj_rr_get_type(dns_obj_rr_any_t record);

/*!
 *	@brief
 *		Get the class of the resource record.
 *
 *	@param record
 *		The resource record object.
 *
 *	@result
 *		The DNS record class.
 */
uint16_t
dns_obj_rr_get_class(dns_obj_rr_any_t record);

/*!
 *	@brief
 *		Get the rdata length of the resource record.
 *
 *	@param record
 *		The resource record object.
 *
 *	@result
 *		The rdata length.
 */
uint16_t
dns_obj_rr_get_rdata_len(dns_obj_rr_any_t record);

/*!
 *	@brief
 *		Get the rdata pointer associated with this resource record object.
 *
 *	@param record
 *		The resource record object.
 *
 *	@result
 *		The pointer to the rdata. If <code>allocate_memory</code> is set to false, the pointer returned will be the same as the one when creating this resource
 *		record.
 */
const uint8_t * NULLABLE
dns_obj_rr_get_rdata(dns_obj_rr_any_t record);

/*!
 *	@brief
 *		Get the TTL of the resource record.
 *
 *	@param record
 *		The resource record object.
 *
 *	@result
 *		The TTL in seconds.
 */
uint32_t
dns_obj_rr_get_ttl(dns_obj_rr_any_t record);

/*!
 *	@brief
 *		Get the signed data of the resource record object that can be used to sort the object in canonical order or reconstruct the signed data for DNSSEC
 *		validation.
 *
 *	@param record
 *		The resource record object.
 *
 *	@result
 *		The pointer to the signed data. This allocated memory region can be released by calling dns_obj_rr_clear_comparison_attributes().
 *
 *	@discussion
 *		Note that <code>dns_obj_rr_set_comparison_attributes()</code> has to be called before calling this function since the signed data needs the RRSIG
 *		values to be calculated correctly.
 */
const uint8_t * NULLABLE
dns_obj_rr_get_signed_data(dns_obj_rr_any_t record);

/*!
 *	@brief
 *		Get the length of the signed data of the resource record object.
 *
 *	@param record
 *		The resource record object.
 *
 *	@result
 *		The length of the signed data.
 */
size_t
dns_obj_rr_get_signed_data_len(dns_obj_rr_any_t record);

/*!
 *	@brief
 *		Get an allocated string description of the corresponding resource record object's rdata.  The format follows what is described in:
 *		https://datatracker.ietf.org/doc/html/rfc8499#section-5
 *
 *	@param record
 *		The resource record object.
 *
 *	@param out_error
 *		The pointer to the error value indicates the success of the function call or the error encountered.
 *
 *	@result
 *		The string description of the rdata if no error occurs. Otherwise, <code>NULL</code> will be returned, if <code>out_error</code> is non-null, an error code
 *		will be set to <code>*out_error</code>.
 *
 *	@discussion
 *		This function returns an allocated C string, the caller is responsible for releasing it.
 */
char * NULLABLE
dns_obj_rr_copy_rdata_rfc_description(dns_obj_rr_any_t record, dns_obj_error_t * NULLABLE out_error);

/*!
 *	@brief
 *		Set the Original TTL and RRSIG labels value for the resource record object so that it can be comparable. <code>dns_obj_rr_get_signed_data</code>
 *		also requires this function call because the signed data contains these two values.
 *
 *	@param record
 *		The resource record object
 *
 *	@param original_ttl
 *		The original TTL value of the RRSIG that covers this resource record.
 *
 *	@param rrsig_labels
 *		The labels value of the RRSIG that covers this resource record.
 */
void
dns_obj_rr_set_comparison_attributes(dns_obj_rr_any_t record, uint32_t original_ttl, uint8_t rrsig_labels);

/*!
 *	@brief
 *		Clear the comparison attributes set by <code>dns_obj_rr_set_comparison_attributes()</code>, also free the memory allocated for the signed data returned
 *		by <code>dns_obj_rr_get_signed_data()</code>.
 *
 *	@param record
 *		The resource record object.
 */
void
dns_obj_rr_clear_comparison_attributes(dns_obj_rr_any_t record);

/*!
 *	@brief
 *		Set the Original TTL and RRSIG labels value for the resource record objects so that they can be comparable. <code>dns_obj_rr_get_signed_data</code>
 *		also requires this function call because the signed data of the resource record objects contains these two values.
 *
 *	@param records
 *		The resource record objects array.
 *
 *	@param original_ttl
 *		The original TTL value of the RRSIG that covers this resource record.
 *
 *	@param rrsig_labels
 *		The labels value of the RRSIG that covers this resource record.
 */
void
dns_obj_rrs_set_comparison_attributes(dns_objs_rr_any_t records, size_t rr_count, uint32_t original_ttl, uint8_t rrsig_labels);

/*!
 *	@brief
 *		Clear the comparison attributes set by <code>dns_obj_rrs_set_comparison_attributes()</code>, also free the memory allocated for the signed data returned
 *		by <code>dns_obj_rr_get_signed_data()</code>.
 *
 *	@param records
 *		The resource record objects array.
 */
void
dns_obj_rrs_clear_comparison_attributes(dns_objs_rr_any_t records, size_t rr_count);

/*!
 *	@brief
 *		Check if the resource record object is equal to a raw record with name, type and rdata.
 *
 *	@param record
 *		The resource record object to be checked.
 *
 *	@param name
 *		The name of the record.
 *
 *	@param type
 *		The type of the record.
 *
 *	@param class
 *		The class of the record.
 *
 *	@param rdata
 *		The pointer to the rdata.
 *
 *	@param rdata_len
 *		The length of the rdata.
 *
 *	@result
 *		True, if they are equal, otherwise, false.
 */
bool
dns_obj_rr_equal_to_raw_data(dns_obj_rr_any_t record,
	const uint8_t * NONNULL name, uint16_t type, uint16_t class, const uint8_t * NULLABLE rdata, uint16_t rdata_len);

/*!
 *	@brief
 *		Check if the resource records in the array come from the same RRSET.
 *
 *	@param records
 *		The resource record objects array.
 *
 *	@param count
 *		The number of records in the array.
 *
 *	@result
 *		True if they come from the same RRSET, otherwise, false.
 */
bool
dns_obj_rrs_belong_to_one_rrset(dns_objs_rr_any_t records, size_t count);

#endif // DNS_OBJ_RR_H
