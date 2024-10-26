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
#include "dns_obj_rr_soa.h"
#include "dns_obj_rr_private.h"
#include "dns_obj.h"
#include "dns_common.h"
#include "rdata_parser.h"

#include "dns_assert_macros.h"
#include "mdns_strict.h"

//======================================================================================================================
// MARK: - DNSSEC SOA Resource Record Kind Definition

struct dns_obj_rr_soa_s {
	struct dns_obj_rr_s	base; // The reference count and kind support base.
};

// dns_obj_rr_soa_t is a subkind of dns_obj_rr_t, and it always have DNS type: kDNSRecordType_SOA.
DNS_OBJECT_SUBKIND_DEFINE_ABSTRUCT(rr, soa,
	.rr_type = kDNSRecordType_SOA,
	.copy_rdata_rfc_description_method = NULL
);

//======================================================================================================================
// MARK: - DNSSEC SOA Resource Record Public Methods

dns_obj_rr_soa_t
dns_obj_rr_soa_create(const uint8_t * const name, const uint8_t * const rdata, const uint16_t rdata_len,
	const bool allocate, dns_obj_error_t * const out_error)
{
	dns_obj_error_t err;
	dns_obj_rr_soa_t soa = NULL;
	dns_obj_rr_soa_t obj = NULL;

	const bool valid = rdata_parser_soa_check_validity(rdata, rdata_len);
	require_action(valid, exit, err = DNS_OBJ_ERROR_PARAM_ERR);

	obj = _dns_obj_rr_soa_new();
	require_action(obj != NULL, exit, err = DNS_OBJ_ERROR_NO_MEMORY);

	_dns_obj_rr_soa_kind.dns_obj_rr_init_fields(&obj->base, name, _dns_obj_rr_soa_kind.rr_type,
		kDNSClassType_IN, rdata, rdata_len, allocate, _dns_obj_rr_soa_kind.copy_rdata_rfc_description_method, &err);
	require_noerr(err, exit);

	soa = obj;
	obj = NULL;
	err = DNS_OBJ_ERROR_NO_ERROR;

exit:
	if (out_error != NULL) {
		*out_error = err;
	}
	MDNS_DISPOSE_DNS_OBJ(obj);
	return soa;
}

//======================================================================================================================

uint32_t
dns_obj_rr_soa_get_minimum_ttl(const dns_obj_rr_soa_t me)
{
	const uint8_t * const rdata = dns_obj_rr_get_rdata(me);
	return rdata_parser_soa_get_minimum_ttl(rdata);
}
