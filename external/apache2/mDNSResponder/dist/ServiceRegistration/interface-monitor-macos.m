/* interface-monitor-macos.m
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

#include "interface-monitor-macos.h"

#include <CoreUtils/CoreUtils.h>

static CUNetInterfaceMonitor *
_ifmon_to_objc(ifmon_t monitor);

ifmon_t
ifmon_create(const dispatch_queue_t queue)
{
	@autoreleasepool {
		CUNetInterfaceMonitor *monitor = [[CUNetInterfaceMonitor alloc] init];
		monitor.dispatchQueue = queue;
		const ifmon_t ifmon = (ifmon_t)CFBridgingRetain(monitor);
		ForgetObjectiveCObject(&monitor);
		return ifmon;
	}
}

void
ifmon_set_primary_ip_changed_handler(const ifmon_t me, const dispatch_block_t handler)
{
	@autoreleasepool {
		CUNetInterfaceMonitor *monitor = _ifmon_to_objc(me);
		monitor.primaryIPChangedHandler = handler;
	}
}

void
ifmon_activate(const ifmon_t me, const dispatch_block_t completion_handler)
{
	@autoreleasepool {
		[_ifmon_to_objc(me) activateWithCompletion:completion_handler];
	}
}

sockaddr_ip
ifmon_get_primary_ipv4_address(const ifmon_t me)
{
	@autoreleasepool {
		CUNetInterfaceMonitor *monitor = _ifmon_to_objc(me);
		return monitor.primaryIPv4Addr;
	}
}

sockaddr_ip
ifmon_get_primary_ipv6_address(const ifmon_t me)
{
	@autoreleasepool {
		CUNetInterfaceMonitor *monitor = _ifmon_to_objc(me);
		return monitor.primaryIPv6Addr;
	}
}

void
ifmon_invalidate(const ifmon_t me)
{
	@autoreleasepool {
		[_ifmon_to_objc(me) invalidate];
	}
}

void
ifmon_release(const ifmon_t me)
{
	@autoreleasepool {
		CUNetInterfaceMonitor *monitor = (CUNetInterfaceMonitor *)CFBridgingTransfer(me);
		ForgetObjectiveCObject(&monitor);
	}
}

static CUNetInterfaceMonitor *
_ifmon_to_objc(const ifmon_t me)
{
	return (__bridge CUNetInterfaceMonitor *)me;
}
