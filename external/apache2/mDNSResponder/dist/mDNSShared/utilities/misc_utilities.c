/*
 * Copyright (c) 2021-2024 Apple Inc. All rights reserved.
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

#include "misc_utilities.h"

#include <string.h>						// For memset().
#include "DebugServices.h"				// For check_compile_time_code().
#include "mdns_strict.h"

//======================================================================================================================
// MARK: - Functions

mDNSAddr
mDNSAddr_from_in_addr(const struct in_addr * const NONNULL v4)
{
	mDNSAddr mdns_addr; // NOLINT(misc-uninitialized-record-variable): No need to initialize mdns_addr here.
	mdns_compile_time_check_local(sizeof(mdns_addr.ip.v4) == sizeof(v4->s_addr));

	mdns_addr.type = mDNSAddrType_IPv4;
	mDNSPlatformMemCopy(&mdns_addr.ip.v4, &v4->s_addr, sizeof(v4->s_addr));

	return mdns_addr;
}

//======================================================================================================================

mDNSAddr
mDNSAddr_from_in6_addr(const struct in6_addr * const NONNULL v6)
{
	mDNSAddr mdns_addr; // NOLINT(misc-uninitialized-record-variable): No need to initialize mdns_addr here.
	mdns_compile_time_check_local(sizeof(mdns_addr.ip.v6) == sizeof(v6->s6_addr));

	mdns_addr.type = mDNSAddrType_IPv6;
	mDNSPlatformMemCopy(&mdns_addr.ip.v6, v6->s6_addr, sizeof(v6->s6_addr));

	return mdns_addr;
}

//======================================================================================================================

mDNSAddr
mDNSAddr_from_sockaddr(const struct sockaddr * const NONNULL sa)
{
	mDNSAddr mdns_addr; // NOLINT(misc-uninitialized-record-variable): No need to initialize mdns_addr here.

	if (sa->sa_family == AF_INET) {
		const struct in_addr *const v4_addr = &((const struct sockaddr_in *)sa)->sin_addr;
		mdns_addr = mDNSAddr_from_in_addr(v4_addr);

	} else if (sa->sa_family == AF_INET6) {
		const struct in6_addr *const v6_addr = &((const struct sockaddr_in6 *)sa)->sin6_addr;
		mdns_addr = mDNSAddr_from_in6_addr(v6_addr);

	} else {
		memset(&mdns_addr, 0, sizeof(mdns_addr));
	}

	return mdns_addr;
}

//======================================================================================================================

const char *
get_address_string_from_mDNSAddr(const mDNSAddr * const NONNULL addr,
								 char out_string_buf[static NONNULL INET6_ADDRSTRLEN + 1])
{
	if (addr->type == mDNSAddrType_IPv4) {
		inet_ntop(AF_INET, addr->ip.v4.b, out_string_buf, INET6_ADDRSTRLEN + 1);
	} else if (addr->type == mDNSAddrType_IPv6) {
		inet_ntop(AF_INET6, addr->ip.v6.b, out_string_buf, INET6_ADDRSTRLEN + 1);
	} else {
		out_string_buf[0] = '\0';
	}

	return out_string_buf;
}
