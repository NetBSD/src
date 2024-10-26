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

#ifndef DNS_OBJ_RR_SRV_H
#define DNS_OBJ_RR_SRV_H

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

DNS_OBJECT_SUBKIND_TYPEDEF_OPAQUE_POINTER(rr, srv);

//======================================================================================================================
// MARK: - Object Methods

dns_obj_rr_srv_t NULLABLE
dns_obj_rr_srv_create(const uint8_t * NONNULL name, const uint8_t * NONNULL rdata, uint16_t rdata_len, bool allocate,
	dns_obj_error_t * NULLABLE out_error);

uint16_t
dns_obj_rr_srv_get_priority(dns_obj_rr_srv_t NONNULL srv);

uint16_t
dns_obj_rr_srv_get_weight(dns_obj_rr_srv_t NONNULL srv);

uint16_t
dns_obj_rr_srv_get_port(dns_obj_rr_srv_t NONNULL srv);

dns_obj_domain_name_t NONNULL
dns_obj_rr_srv_get_target(dns_obj_rr_srv_t NONNULL srv);

#endif // DNS_OBJ_RR_SRV_H
