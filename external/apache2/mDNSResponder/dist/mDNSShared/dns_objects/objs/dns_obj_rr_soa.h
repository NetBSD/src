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

#ifndef DNS_OBJ_RR_SOA_H
#define DNS_OBJ_RR_SOA_H

//======================================================================================================================
// MARK: - Headers

#include "dns_obj_domain_name.h"
#include "dns_obj.h"
#include "dns_common.h"
#include <stdint.h>
#include <stdbool.h>

#include "nullability.h"

//======================================================================================================================
// MARK: - Object Reference Definition

DNS_OBJECT_SUBKIND_TYPEDEF_OPAQUE_POINTER(rr, soa);

//======================================================================================================================
// MARK: - Object Methods

/*!
 *	@brief
 *		Create an SOA resource record object.
 *
 *	@param name
 *		The name of the SOA resource record in domain name labels.
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
 *		of this SOA resource record object.
 *
 *	@param out_error
 *		The pointer to the error value indicates the success of the function call or the error encountered.
 *
 *	@result
 *		The SOA resource record object created, or NULL if error happens during creation. <code>out_error</code> will be set to the error encountered if it is not NULL.
 */
dns_obj_rr_soa_t NULLABLE
dns_obj_rr_soa_create(const uint8_t * NONNULL name, const uint8_t * NONNULL rdata, uint16_t rdata_len,
	bool allocate_memory, dns_obj_error_t * NULLABLE out_error);

//======================================================================================================================

/*!
 *	@brief
 *		Get the minimum TTL specified by SOA resource record.
 *
 *	@param soa
 *		The SOA resource record object.
 *
 *	@result
 *		The minimum TTL in seconds.
 */
uint32_t
dns_obj_rr_soa_get_minimum_ttl(dns_obj_rr_soa_t NONNULL soa);

#endif // DNS_OBJ_RR_SOA_H
