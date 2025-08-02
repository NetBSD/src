/* -*- Mode: C; tab-width: 4 -*-
 *
 * Copyright (c) 2020-2022 Apple Inc. All rights reserved.
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

#ifndef __MDNS_STRICT_H__
#define __MDNS_STRICT_H__

#ifndef MDNS_NO_STRICT
	#if !defined(__APPLE__)
		#define MDNS_NO_STRICT				1
	#else // !defined(__APPLE__)
		#define MDNS_NO_STRICT				0
	#endif // !defined(__APPLE__)
#endif // MDNS_NO_STRICT

	#define APPLE_OSX_mDNSResponder			0

#ifndef DEBUG
	#define DEBUG 							0
#endif

#ifndef _MDNS_STRICT_DISPOSE_TEMPLATE
	#if MDNS_NO_STRICT
		#define _MDNS_STRICT_DISPOSE_TEMPLATE(ptr, function) \
			do {                                        \
				if ((ptr) != NULL) {                    \
					function(ptr);                      \
					(ptr) = NULL;                       \
				}                                       \
			} while(0)
	#else // MDNS_NO_STRICT
		#define _MDNS_STRICT_DISPOSE_TEMPLATE _STRICT_DISPOSE_TEMPLATE
	#endif // MDNS_NO_STRICT
#endif // _MDNS_STRICT_DISPOSE_TEMPLATE

#if !MDNS_NO_STRICT
#include <CoreFoundation/CoreFoundation.h>
#include <os/log.h>

#include "../mDNSMacOSX/secure_coding/strict.h"

#pragma mark -- Alloc --

#define mdns_malloc strict_malloc
#define MDNS_MALLOC_TYPE STRICT_MALLOC_TYPE
#define mdns_calloc strict_calloc
#define MDNS_CALLOC_TYPE STRICT_CALLOC_TYPE
#define mdns_reallocf strict_reallocf
#define MDNS_REALLOCF_TYPE STRICT_REALLOCF_TYPE
#define mdns_memalign strict_memalign
#define MDNS_ALLOC_ALIGN_TYPE STRICT_ALLOC_ALIGN_TYPE
#define mdns_strdup strict_strdup
#define mdns_strlcpy strict_strlcpy

#pragma mark -- Dispose --

#define MDNS_DISPOSE_XPC STRICT_DISPOSE_XPC
#define MDNS_DISPOSE_XPC_PROPERTY(obj, prop) MDNS_DISPOSE_XPC(obj->prop)

#define MDNS_DISPOSE_ALLOCATED STRICT_DISPOSE_ALLOCATED
#define MDNS_DISPOSE_ALLOCATED_PROPERTY(obj, prop) MDNS_DISPOSE_ALLOCATED(obj->prop)
#define mdns_free(ptr) MDNS_DISPOSE_ALLOCATED(ptr)

#define MDNS_DISPOSE_DISPATCH STRICT_DISPOSE_DISPATCH
#define MDNS_DISPOSE_DISPATCH_PROPERTY(obj, prop) MDNS_DISPOSE_DISPATCH(obj->prop)

#define MDNS_RESET_BLOCK STRICT_RESET_BLOCK
#define MDNS_RESET_BLOCK_PROPERTY(obj, prop, new_block) MDNS_RESET_BLOCK(obj->prop, new_block)

#define MDNS_DISPOSE_BLOCK STRICT_DISPOSE_BLOCK
#define MDNS_DISPOSE_BLOCK_PROPERTY(obj, prop) MDNS_DISPOSE_BLOCK(obj->prop)

#define MDNS_DISPOSE_CF_OBJECT STRICT_DISPOSE_CF_OBJECT
#define MDNS_DISPOSE_CF_PROPERTY(obj, prop) MDNS_DISPOSE_CF_OBJECT(obj->prop)

#define MDNS_DISPOSE_ADDRINFO STRICT_DISPOSE_ADDRINFO

#define MDNS_DISPOSE_NW(obj) _MDNS_STRICT_DISPOSE_TEMPLATE(obj, nw_release)

#define MDNS_DISPOSE_SEC(obj) _MDNS_STRICT_DISPOSE_TEMPLATE(obj, sec_release)

#define MDNS_DISPOSE_DNS_SERVICE_REF(obj) _MDNS_STRICT_DISPOSE_TEMPLATE(obj, DNSServiceRefDeallocate)

#ifdef BlockForget
// Redfine BlockForget to bypass poisoned Block_release
#undef BlockForget
#if( COMPILER_ARC )
	#define	BlockForget( X )			do { *(X) = nil; } while( 0 )
#else
	#define	BlockForget( X )			ForgetCustom( X, _Block_release )
#endif
#endif

#else  // !MDNS_NO_STRICT

#include <stddef.h>
#include <stdlib.h>

#define mdns_malloc 			malloc
#define mdns_calloc 			calloc
#define mdns_strdup 			strdup
#define mdns_free(obj) \
	_MDNS_STRICT_DISPOSE_TEMPLATE(obj, free)

static
#if defined(_WIN32)
__forceinline
#else
inline __attribute__((always_inline))
#endif
void _mdns_strict_strlcpy(char * const restrict dst, const char * const restrict src, const size_t dst_len)
{
	if (dst_len == 0) {
		return;
	}

	char *d = dst;
	const char *s = src;
	for (size_t n = dst_len - 1; n > 0; n--) {
		if ((*d++ = *s++) == '\0') {
			return;
		}
	}
	*d = '\0';
}
#define mdns_strlcpy			_mdns_strict_strlcpy


#define MDNS_DISPOSE_DNS_SERVICE_REF(obj) _MDNS_STRICT_DISPOSE_TEMPLATE(obj, DNSServiceRefDeallocate)



#endif // !MDNS_NO_STRICT

#endif // __MDNS_STRICT_H__
