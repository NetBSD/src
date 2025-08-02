/*
 * Copyright (c) 2020-2023 Apple Inc. All rights reserved.
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
 *
 * This file contains the function declarations for discover_resolver.c,
 * which will be called by mDNSCore code to set up the automatic browsing
 * domain resolver.
 */

#ifndef __RESOLVER_DISCOVER_H__
#define __RESOLVER_DISCOVER_H__

#include "mDNSFeatures.h" // for MDNSRESPONDER_SUPPORTS(COMMON, LOCAL_DNS_RESOLVER_DISCOVERY)

#if MDNSRESPONDER_SUPPORTS(COMMON, LOCAL_DNS_RESOLVER_DISCOVERY)

//======================================================================================================================
// MARK: - Headers

#include "mDNSEmbeddedAPI.h"
#include "general.h"

#include <stdbool.h>

#include "nullability.h"

MDNS_ASSUME_NONNULL_BEGIN

MDNS_C_DECLARATIONS_BEGIN

//======================================================================================================================
// MARK: - Functions

/*!
 *	@brief
 *		Start the local DNS resolver discovery for the domain specified, by using reference counting to add/retain the discovery process.
 *
 *	@param domain_to_discover
 *		The domain where the local DNS resolver should be discovered.
 *
 *	@param grab_mdns_lock
 *		The boolean value of whether to grab the mDNS_Lock when doing query-related operation.
 *
 *	@result
 *		True if the function succeeds, otherwise, false.
 *
 *	@discussion
 *		When <code>resolver_discovery_add</code> is called, the discovery is not necessarily started on behalf of the caller. The discovery may have been
 *		started earlier by other caller. In which case, the same discovery activity will be retained and reused for the current caller.
 */

bool
resolver_discovery_add(const domainname *domain_to_discover, bool grab_mdns_lock);

/*!
 *	@brief
 *		Stop the local DNS resolver discovery for the domain specified, by using reference counting to remove/release the discovery process.
 *
 *	@param domain_to_discover
 *		The domain where the local DNS resolver should be discovered.
 *
 *	@param grab_mdns_lock
 *		The boolean value of whether to grab the mDNS_Lock when doing query-related operation.
 *
 *	@result
 *		True if the function succeeds, otherwise, false.
 *
 *	@discussion
 *		When <code>resolver_discovery_remove</code> is called, the discovery is not necessarily stopped on behalf of the caller. The discovery may be still used
 *		by other callers. In which case, the discovery will not be stopped, instead, it will be removed/released with reference counting. The resolver discovery will
 *		not be stopped until the last caller of <code>resolver_discovery_add</code> calls <code>resolver_discovery_remove</code>.
 */

bool
resolver_discovery_remove(const domainname *domain_to_discover, bool grab_mdns_lock);

/*!
 *	@brief
 *		Get the next time when mDNSCore should start processing the previously scheduled task for the resolver discovery.
 *
 *	@result
 *		The next time to perform resolver discovery related tasks.
 */
mDNSs32
resolver_discovery_get_next_scheduled_event(void);
#define ResolverDiscovery_GetNextScheduledEvent(...)	resolver_discovery_get_next_scheduled_event(__VA_ARGS__)

/*!
 *	@brief
 *		Perform resolver discovery related tasks.
 */
void
resolver_discovery_perform_periodic_tasks(void);
#define ResolverDiscovery_PerformPeriodicTasks(...)		resolver_discovery_perform_periodic_tasks(__VA_ARGS__)

/*!
 *	@brief
 *		Check if the current DNS question is allowed to do resolve discovery, if so, return the domain that can do resolver discovery.
 *
 *	@param q
 *		The DNS question.
 *
 *	@param out_domain
 *		The pointer of the domain name to do resolver discovery when the DNS question is allowed to discover it.
 *
 *	@result
 *		Returns true if the question is capable of doing resolver discovery and `out_domain` contains the domain, otherwise, false.
 */
bool
dns_question_requires_resolver_discovery(const DNSQuestion *q, const domainname * NULLABLE * NONNULL out_domain);

MDNS_C_DECLARATIONS_END

MDNS_ASSUME_NONNULL_END

#endif // MDNSRESPONDER_SUPPORTS(COMMON, LOCAL_DNS_RESOLVER_DISCOVERY)

#endif /* __RESOLVER_DISCOVER_H__ */
