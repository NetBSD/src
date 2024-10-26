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

#ifndef MISC_UTILITIES_H
#define MISC_UTILITIES_H

//======================================================================================================================
// MARK: - Headers

#include <netinet/in.h>			// For sockaddr.
#include "nullability.h"		// For NULLABLE and NONNULL.
#include "mDNSEmbeddedAPI.h"	// For mDNSAddr.

//======================================================================================================================
// MARK: - Macros

#ifndef __APPLE__

#ifndef require
	#define require(assertion, exception_label)			\
		do {											\
			if (__builtin_expect(!(assertion), 0)) {	\
				goto exception_label;					\
			}											\
		} while (false)
#endif // require

#ifndef require_action
	#define require_action(assertion, exception_label, action)	\
		do {													\
			if (__builtin_expect(!(assertion), 0)) {			\
				{												\
					action;										\
				}												\
				goto exception_label;							\
			}													\
		} while (false)
#endif // require_action

#endif // __APPLE__

//======================================================================================================================
// MARK: - Function Declarations

/*!
 *	@brief
 *		Convert struct in_addr to mDNSAddr.
 *
 *	@param v4
 *		The IPv4 struct in_addr to be converted.
 *
 *	@result
 *		The converted mDNSAddr.
 */
mDNSAddr
mDNSAddr_from_in_addr(const struct in_addr * NONNULL v4);

/*!
 *	@brief
 *		Convert struct in6_addr to mDNSAddr.
 *
 *	@param v6
 *		The IPv6 struct in6_addr to be converted.
 *
 *	@result
 *		The converted mDNSAddr.
 */
mDNSAddr
mDNSAddr_from_in6_addr(const struct in6_addr * NONNULL v6);

/*!
 *	@brief
 *		Convert struct sockaddr to mDNSAddr.
 *
 *	@param sa
 *		The struct sockaddr to be converted.
 *
 *	@result
 *		The converted mDNSAddr.
 */
mDNSAddr
mDNSAddr_from_sockaddr(const struct sockaddr * NONNULL sa);


const char * NONNULL
get_address_string_from_mDNSAddr(const mDNSAddr * const NONNULL addr,
								 char out_string_buf[static NONNULL INET6_ADDRSTRLEN + 1]);

#endif // MISC_UTILITIES_H
