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

#include "dns_sd_private.h"
#include "mdns_strict.h"

const char * DNSSD_API
DNSServiceErrorCodeToString(DNSServiceErrorType error_code)
{
#define CASE_TO_STR(s) case kDNSServiceErr_ ## s: return (#s);
	switch (error_code) {
		CASE_TO_STR(NoError)
		CASE_TO_STR(Unknown)
		CASE_TO_STR(NoSuchName)
		CASE_TO_STR(NoMemory)
		CASE_TO_STR(BadParam)
		CASE_TO_STR(BadReference)
		CASE_TO_STR(BadState)
		CASE_TO_STR(BadFlags)
		CASE_TO_STR(Unsupported)
		CASE_TO_STR(NotInitialized)
		CASE_TO_STR(AlreadyRegistered)
		CASE_TO_STR(NameConflict)
		CASE_TO_STR(Invalid)
		CASE_TO_STR(Firewall)
		CASE_TO_STR(Incompatible)
		CASE_TO_STR(BadInterfaceIndex)
		CASE_TO_STR(Refused)
		CASE_TO_STR(NoSuchRecord)
		CASE_TO_STR(NoAuth)
		CASE_TO_STR(NoSuchKey)
		CASE_TO_STR(NATTraversal)
		CASE_TO_STR(DoubleNAT)
		CASE_TO_STR(BadTime)
		CASE_TO_STR(BadSig)
		CASE_TO_STR(BadKey)
		CASE_TO_STR(Transient)
		CASE_TO_STR(ServiceNotRunning)
		CASE_TO_STR(NATPortMappingUnsupported)
		CASE_TO_STR(NATPortMappingDisabled)
		CASE_TO_STR(NoRouter)
		CASE_TO_STR(PollingMode)
		CASE_TO_STR(Timeout)
		CASE_TO_STR(DefunctConnection)
		CASE_TO_STR(PolicyDenied)
		CASE_TO_STR(NotPermitted)
		default:
			return "<INVALID ERROR CODE>";
	}
#undef CASE_TO_STR
}

