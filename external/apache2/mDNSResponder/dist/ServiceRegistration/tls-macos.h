/* tls-macos.h
 *
 * Copyright (c) 2020-2021 Apple Inc. All rights reserved.
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
 * Definitions for getting TLS certificate and setting TLS.
 */

#ifndef __DNSSD_PROXY_TLS_APPLE_H__
#define __DNSSD_PROXY_TLS_APPLE_H__

#include <stdio.h>
#include <Security/SecKey.h>
#include <Network/Network.h>
#include <CoreUtils/CoreUtils.h> // For OSStatus.

//======================================================================================================================
// MARK: - Structures

typedef struct tls_config_context tls_config_context_t;
struct tls_config_context {
    nw_protocol_options_t tls_options;
    dispatch_queue_t queue;
};

//======================================================================================================================
// MARK: - Function Declarations

/*!
 *  @brief
 *      Get identity (the combination of private key and certificate) from keychain, or generate a new one if there is no existing identity in keychain.
 *
 *  @result
 *      Returns true if identity is fetched successfully.
 *
 *  @discussion
 *      The certificate being generated is a self-signed one, which means there will be no CA to veriry this trustworthiness of certificate.
 */
bool
srp_tls_init(void);

#endif /* #ifndef __DNSSD_PROXY_TLS_APPLE_H__ */
