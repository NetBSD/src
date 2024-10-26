/* interface-monitor-macos.h
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

#ifndef __INTERFACE_MONITOR_MACOS_H__
#define __INTERFACE_MONITOR_MACOS_H__

#include <CoreUtils/CommonServices.h>
#include <dispatch/dispatch.h>

typedef struct ifmon_s *ifmon_t;

CU_ASSUME_NONNULL_BEGIN

__BEGIN_DECLS

ifmon_t
ifmon_create(dispatch_queue_t queue);

void
ifmon_set_primary_ip_changed_handler(ifmon_t monitor, dispatch_block_t handler);

void
ifmon_activate(ifmon_t monitor, dispatch_block_t completion_handler);

sockaddr_ip
ifmon_get_primary_ipv4_address(ifmon_t monitor);

sockaddr_ip
ifmon_get_primary_ipv6_address(ifmon_t monitor);

void
ifmon_invalidate(ifmon_t monitor);

void
ifmon_release(ifmon_t monitor);

__END_DECLS

CU_ASSUME_NONNULL_END

#endif	// __INTERFACE_MONITOR_MACOS_H__
