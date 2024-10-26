/*
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

#ifndef MDNS_ADDR_TAIL_QUEUE_H
#define MDNS_ADDR_TAIL_QUEUE_H

//======================================================================================================================
// MARK: - Headers

#include "nullability.h"		// For NULLABLE and NONNULL.
#include "bsd_queue.h"			// For STAILQ.
#include <stdlib.h>
#include <stdbool.h>			// For bool.
#include "mDNSEmbeddedAPI.h"	// For mDNSAddr, mDNSIPPort and mStatus.
#if defined(POSIX_BUILD)
#include "DebugServices.h"
#endif // defined(POSIX_BUILD)

//======================================================================================================================
// MARK: - Structures

typedef struct mdns_addr_with_port mdns_addr_with_port_t;
struct mdns_addr_with_port {
	STAILQ_ENTRY(mdns_addr_with_port) __entries;	// The entry used by STAILQ to form the linked list.
	mDNSAddr address;								// IPv4 or IPv6 address.
	mDNSIPPort port;								// The port number that is associated with the address above.
};

typedef struct mdns_addr_tailq mdns_addr_tailq_t;
// The tail queue struct mdns_addr_tailq that contains struct mdns_addr_with_port.
STAILQ_HEAD(mdns_addr_tailq, mdns_addr_with_port);

//======================================================================================================================
// MARK: - Function Declarations

/*!
 *	@brief
 *		Creates a tail queue that contains IP address and port number.
 *
 *	@result
 *		A pointer to the tail queue or NULL if the system was out of memory.
 */
mdns_addr_tailq_t * NULLABLE
mdns_addr_tailq_create(void);

/*!
 *	@brief
 * 		Disposes a tail queue that is created by <code>mdns_addr_tailq_create()</code>.
 *
 *	@param me
 *		The tail queue that will be disposed.
 *
 *	@discussion
 *		Use <code>MDNS_DISPOSE_MDNS_ADDR_TAILQ()</code> to dispose a tail queue safely.
 */
void
mdns_addr_tailq_dispose(mdns_addr_tailq_t * NONNULL me);
#define MDNS_DISPOSE_MDNS_ADDR_TAILQ(obj) _MDNS_STRICT_DISPOSE_TEMPLATE(obj, mdns_addr_tailq_dispose)

/*!
 *	@brief
 *		Checks if the current tail queue is empty.
 *
 *	@param me
 *		The tail queue to be checked.
 *
 *	@result
 *		True if the tail queue does not contain any element. Otherwise, false.
 */
bool
mdns_addr_tailq_empty(const mdns_addr_tailq_t * NONNULL me);

/*!
 *	@brief
 *		Adds an IP address with its corresponding port number into the front of the tail queue.
 *
 *	@param me
 *		The tail queue where the address and port number to be added into.
 *
 *	@param address
 *		The IP address to be added.
 *
 *	@param port
 *		The corresponding port number of the added address to be added.
 *
 *	@result
 *		The added address with its port number in the front of the tail queue, or NULL if the system was out of memory.
 */
const mdns_addr_with_port_t * NULLABLE
mdns_addr_tailq_add_front(mdns_addr_tailq_t * NONNULL me, const mDNSAddr * NONNULL address, mDNSIPPort port);

/*!
 *	@brief
 *		Adds an IP address with its corresponding port number into the back of the tail queue.
 *
 *	@param me
 *		The tail queue where the address and port number to be added into.
 *
 *	@param address
 *		The IP address to be added.
 *
 *	@param port
 *		The corresponding port number of the added address to be added.
 *
 *	@result
 *		The added address with its port number in the back of the tail queue, or NULL if the system was out of memory.
 */
const mdns_addr_with_port_t * NULLABLE
mdns_addr_tailq_add_back(mdns_addr_tailq_t * NONNULL me, const mDNSAddr * NONNULL address, mDNSIPPort port);

/*!
 *	@brief
 *		Gets the pointer to mdns_addr_with_port_t (which contains the IP address and port number added before) in the front of the tail queue.
 *
 *	@param me
 *		The tail queue where the mdns_addr_with_port_t to be get from.
 *
 *	@result
 *		The pointer to the mdns_addr_with_port_t in the front of the tail queue, or NULL if the tail queue is empty.
 */
const mdns_addr_with_port_t * NULLABLE
mdns_addr_tailq_get_front(const mdns_addr_tailq_t * NONNULL me);

/*!
 *	@brief
 *		Removes the first IP address and its port number in the front of the tail queue.
 *
 *	@param me
 *		The tail queue where the IP address and its port number to be removed.
 */
void
mdns_addr_tailq_remove_front(mdns_addr_tailq_t * NONNULL me);

/*!
 *	@brief
 *		Removes mdns_addr_with_port_t object in the tail queue with the IP address and port number specified in the parameters.
 *
 *	@param me
 *		The tail queue where the IP address and its port number to be removed.
 *
 *	@param address
 *		The IP address to be matched.
 *
 *	@param port
 *		The corresponding port number of the IP address to be matched.
 *
 *	@result
 *		True if the mdns_addr_with_port_t object is found and removed from the tail queue, for the IP address and port number specified in the parameters.
 *		False if such mdns_addr_with_port_t object does not exist in the tail queue.
 */
bool
mdns_addr_tailq_remove(mdns_addr_tailq_t * NONNULL me, const mDNSAddr * NONNULL address, mDNSIPPort port);

/*!
 *	@brief
 *		Checks if two mdns_addr_with_port_t object are equal in value.
 *
 *	@param addr_1
 *		mdns_addr_with_port_t object to be compared.
 *
 *	@param addr_2
 *		mdns_addr_with_port_t object to be compared.
 *
 *	@result
 *		True if two mdns_addr_with_port_t are equal in value, otherwise, false.
 */
bool
mdns_addr_with_port_equal(const mdns_addr_with_port_t * NONNULL addr_1, const mdns_addr_with_port_t * NONNULL addr_2);

#endif // MDNS_ADDR_TAIL_QUEUE_H
