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

#include "ref_count.h"
#include "dns_obj.h"
#include "dns_common.h"
#include <stdint.h>
#include <stdbool.h>

#include "mdns_strict.h"

//======================================================================================================================
// MARK: - Object Public Methods

dns_obj_t
dns_obj_retain(const dns_obj_t me)
{
	ref_count_obj_retain(me);
	return me;
}

//======================================================================================================================

void
dns_obj_release(const dns_obj_t me)
{
	ref_count_obj_release(me);
}

//======================================================================================================================

bool
dns_obj_equal(const dns_obj_t me, const dns_obj_t other)
{
	return ref_count_obj_compare(me, other, true) == compare_result_equal;
}

//======================================================================================================================

compare_result_t
dns_obj_compare(const dns_obj_t me, const dns_obj_t other)
{
	return ref_count_obj_compare(me, other, false);
}

//======================================================================================================================

void
dns_objs_sort(dns_obj_t * const us, const size_t count, const sort_order_t order)
{
	ref_count_objs_sort(us, count, order);
}
