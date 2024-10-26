/* srp-features.h
 *
 * Copyright (c) 2020-2024 Apple Inc. All rights reserved.
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
 * This file contains compile time feature flag which enables or disables
 * different behavior of srp-mdns-proxy.
 */

#ifndef __SRP_FEATURES_H__
#define __SRP_FEATURES_H__

// SRP_FEATURE_COMBINED_SRP_DNSSD_PROXY: controls whether to initialize dnssd-proxy in srp-mdns-proxy.
#if defined(BUILD_SRP_MDNS_PROXY) && (BUILD_SRP_MDNS_PROXY == 1)

// SRP_TEST_SERVER always builds a full (simulated) Thread border router
#  if defined(SRP_TEST_SERVER)
#    define SRP_TEST_SERVER_OVERRIDE 1
#  else
#    define SRP_TEST_SERVER_OVERRIDE 0
#  endif

// We can only have combined srp-dnssd-proxy if we are building srp-mdns-proxy
#  define SRP_FEATURE_PUBLISH_SPECIFIC_ROUTES 0
#  define SRP_FEATURE_DNSSD_PROXY_SHARED_CONNECTION 0
#    define SRP_FEATURE_DYNAMIC_CONFIGURATION 1
#else
#  ifndef SRP_FEATURE_REPLICATION
#    define SRP_FEATURE_REPLICATION 1
#  endif
#  ifndef THREAD_BORDER_ROUTER
#    define THREAD_BORDER_ROUTER 1
#  endif
#  define STUB_ROUTER 1
#  ifndef SRP_FEATURE_COMBINED_SRP_DNSSD_PROXY
#    define SRP_FEATURE_COMBINED_SRP_DNSSD_PROXY 1
#  endif
#  ifndef SRP_FEATURE_DYNAMIC_CONFIGURATION
#    define SRP_FEATURE_DYNAMIC_CONFIGURATION 0
#endif
#endif

// SRP_FEATURE_CAN_GENERATE_TLS_CERT: controls whether to let srp-mdns-proxy generate the TLS certificate internally.
#if defined(__APPLE__)
    // All the Apple platforms support security framework, so it can generate TLS certifcate internally.
    #define SRP_FEATURE_CAN_GENERATE_TLS_CERT 1
#else
    #define SRP_FEATURE_CAN_GENERATE_TLS_CERT 0
#endif

#if !defined(SRP_FEATURE_NAT64)
    #define SRP_FEATURE_NAT64 0
#endif

    #define SRP_ANALYTICS 0


// At present we never want this, but we're keeping the code around.
#define SRP_ALLOWS_MDNS_CONFLICTS 0

#endif // __SRP_FEATURES_H__
