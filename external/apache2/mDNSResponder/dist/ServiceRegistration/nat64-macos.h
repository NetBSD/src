/* nat64-macos.h
 *
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

#ifndef __NAT64_MACOS_H__
#define __NAT64_MACOS_H__

#include <CoreUtils/CoreUtils.h>
#include <dispatch/dispatch.h>

CU_ASSUME_NONNULL_BEGIN

void
nat64_start_translation(dispatch_queue_t queue);

void
nat64_stop_translation(void);

void
nat64_set_ula_prefix(const struct in6_addr *prefix);

const struct in6_addr *
nat64_get_ipv6_prefix(void);

bool
nat64_is_active(void);

void
nat64_pass_all_pf_rule_delete(void);

void
nat64_pass_all_pf_rule_set(const char *interface);

CU_ASSUME_NONNULL_END

#endif // __NAT64_MACOS_H__
