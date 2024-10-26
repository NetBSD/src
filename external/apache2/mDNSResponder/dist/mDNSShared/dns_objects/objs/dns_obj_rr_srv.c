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

//======================================================================================================================
// MARK: - Headers

#include "dns_obj_log.h"
#include "dns_obj_rr_srv.h"
#include "dns_obj_rr_private.h"
#include "dns_obj.h"
#include "dns_common.h"
#include "rdata_parser.h"

#include "dns_assert_macros.h"
#include "mdns_strict.h"

//======================================================================================================================
// MARK: - DNS SRV Resource Record Kind Definition

struct dns_obj_rr_srv_s {
	struct dns_obj_rr_s		base; // The reference count and kind support base.
	dns_obj_domain_name_t	target;
};

// dns_obj_rr_srv_t is a subkind of dns_obj_rr_t, and it always have DNS type: kDNSRecordType_SRV.
DNS_OBJECT_SUBKIND_DEFINE_FULL(rr, srv,
	.rr_type = kDNSRecordType_SRV,
	.copy_rdata_rfc_description_method = NULL
);

//======================================================================================================================
// MARK: - DNS SRV Resource Record Public Methods

dns_obj_rr_srv_t
dns_obj_rr_srv_create(const uint8_t * const name, const uint8_t * const rdata, const uint16_t rdata_len,
	const bool allocate, dns_obj_error_t * const out_error)
{
	dns_obj_error_t err;
	dns_obj_rr_srv_t srv = NULL;
	dns_obj_rr_srv_t obj = NULL;

	const bool valid = rdata_parser_srv_check_validity(rdata, rdata_len);
	require_action(valid, exit, err = DNS_OBJ_ERROR_MALFORMED_ERR);

	obj = _dns_obj_rr_srv_new();
	require_action(obj != NULL, exit, err = DNS_OBJ_ERROR_NO_MEMORY);

	_dns_obj_rr_srv_kind.dns_obj_rr_init_fields(&obj->base, name, _dns_obj_rr_srv_kind.rr_type,
		kDNSClassType_IN, rdata, rdata_len, allocate, _dns_obj_rr_srv_kind.copy_rdata_rfc_description_method, &err);
	require_noerr(err, exit);

	const uint8_t * const target_in_labels = rdata_parser_srv_get_target(rdata);
	obj->target = dns_obj_domain_name_create_with_labels(target_in_labels, true, &err);
	require_noerr(err, exit);

	dns_obj_replace(&srv, obj);
	err = DNS_OBJ_ERROR_NO_ERROR;

exit:
	if (out_error != NULL) {
		*out_error = err;
	}
	MDNS_DISPOSE_DNS_OBJ(obj);
	return srv;
}

//======================================================================================================================

uint16_t
dns_obj_rr_srv_get_priority(const dns_obj_rr_srv_t me)
{
	const uint8_t * const rdata = dns_obj_rr_get_rdata(me);
	return rdata_parser_srv_get_priority(rdata);
}

//======================================================================================================================

uint16_t
dns_obj_rr_srv_get_weight(const dns_obj_rr_srv_t me)
{
	const uint8_t * const rdata = dns_obj_rr_get_rdata(me);
	return rdata_parser_srv_get_weight(rdata);
}

//======================================================================================================================

uint16_t
dns_obj_rr_srv_get_port(const dns_obj_rr_srv_t me)
{
	const uint8_t * const rdata = dns_obj_rr_get_rdata(me);
	return rdata_parser_srv_get_port(rdata);
}

//======================================================================================================================

dns_obj_domain_name_t
dns_obj_rr_srv_get_target(const dns_obj_rr_srv_t me)
{
	return me->target;
}

//======================================================================================================================
// MARK: - DNS Resource Record Private Methods

static compare_result_t
_dns_obj_rr_srv_compare(const dns_obj_rr_srv_t me, const dns_obj_rr_srv_t other,
	const bool check_equality_only)
{
	if (check_equality_only) {
		// Let the comparator of the super kind to do comparison if there is one.
		return compare_result_unknown;
	}

	const dns_obj_domain_name_t my_name = dns_obj_rr_get_name(me);
	const dns_obj_domain_name_t others_name = dns_obj_rr_get_name(other);
	if (!dns_obj_equal(my_name, others_name)) {
		return compare_result_notequal;
	}

	const uint16_t my_priority = dns_obj_rr_srv_get_priority(me);
	const uint16_t others_priority = dns_obj_rr_srv_get_priority(other);
	if (my_priority < others_priority) {
		return compare_result_greater;
	} else if (my_priority > others_priority) {
		return compare_result_less;
	} else {
		return compare_result_equal;
	}
}

//======================================================================================================================

static void
_dns_obj_rr_srv_finalize(const dns_obj_rr_srv_t me)
{
	MDNS_DISPOSE_DNS_OBJ(me->target);
}
