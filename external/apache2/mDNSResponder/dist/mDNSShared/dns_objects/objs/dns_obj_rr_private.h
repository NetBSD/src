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

#ifndef DNS_OBJ_RR_PRIVATE_H
#define DNS_OBJ_RR_PRIVATE_H

//======================================================================================================================
// MARK: - Headers

#include "dns_obj_rr.h"
#include "dns_obj.h"
#include "dns_common.h"
#include <stdint.h>
#include <stdbool.h>

#include "nullability.h"

//======================================================================================================================
// MARK: - DNS Resource Record Kind Definition

typedef char * NULLABLE
(*dns_obj_rr_copy_rdata_rfc_description_f)(dns_obj_rr_t NONNULL record, dns_obj_error_t * NULLABLE out_error);

struct dns_obj_rr_s {
	struct ref_count_obj_s	base;							// The reference count and kind support base.
	dns_obj_domain_name_t NULLABLE	name;					// The name of the resource record.
	union {
		const uint8_t * NULLABLE		const_rdata;		// The pointer to the rdata that is valid for the life time of this resource record object.
		uint8_t * NULLABLE				allocated_rdata;	// The pointer to the rdata that is allocated when initializing this resource record object.
	} rdata_u;
	uint16_t				type;							// The DNS data type of the record.
	uint16_t				class;							// The DNS class of the record.
	uint16_t				rdata_len;						// The length of the rdata.
	bool					allocated_memory;				// Indicate whether the domain name labels in the name of
															// the resource record and the rdata point to an external
															// memory or is allocated.
	uint32_t				ttl;							// The real TTL of the resource record.
	uint32_t				original_ttl;					// The original TTL value specified by RRSIG.
	uint8_t					rrsig_labels;					// The labels value specified by RRSIG
	uint8_t * NULLABLE		signed_data;					// The signed data of the resource record that is used for DNSSEC validation.
	size_t					signed_data_len;				// The length of the signed data above.

	dns_obj_rr_copy_rdata_rfc_description_f NULLABLE	copy_rdata_rfc_description_method;	// The method that can be customized for each RR type to generate the rdata description.
};

// The member initialization function to be called by the subkind of the resource record object.
typedef void
(*dns_obj_rr_init_fields_f)(dns_obj_rr_t NONNULL uninitialized_record, const uint8_t * NONNULL name, uint16_t type,
	uint16_t class, const uint8_t * NULLABLE rdata, uint16_t rdata_len, bool allocate,
	dns_obj_rr_copy_rdata_rfc_description_f NULLABLE copy_rdata_rfc_description_method, dns_obj_error_t * NULLABLE out_error);

void
dns_obj_rr_init_fields(dns_obj_rr_t NONNULL uninitialized_record, const uint8_t * NONNULL name, uint16_t type,
	uint16_t class, const uint8_t * NULLABLE rdata, uint16_t rdata_len, bool allocate,
	dns_obj_rr_copy_rdata_rfc_description_f NULLABLE copy_rdata_rfc_description_method, dns_obj_error_t * NULLABLE out_error);

// The resource record kind type for a subkind of the resource record object.
DNS_OBJECT_DEFINE_KIND_TYPE_FOR_SUBKIND(rr,
	uint16_t rr_type;	// Any subkind of the resource record should have a specific DNS type.
	dns_obj_rr_copy_rdata_rfc_description_f NONNULL copy_rdata_rfc_description_method;
);

#endif // DNS_OBJ_RR_PRIVATE_H
