/*
 * Copyright (c) 2015-2024 Apple Inc. All rights reserved.
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

#ifndef _DNS_SD_PRIVATE_H
#define _DNS_SD_PRIVATE_H

#include <dns_sd.h>


    #define DNS_SD_ENUM_SPI_AVAILABLE(...)

#define DNS_SD_ENUM_SPI_AVAILABLE_FALL_2021 DNS_SD_ENUM_SPI_AVAILABLE(macos(12.0), ios(15.0), tvos(15.0), watchos(8.0))
#define DNS_SD_ENUM_SPI_AVAILABLE_FALL_2022 DNS_SD_ENUM_SPI_AVAILABLE(macos(13.0), ios(16.0), tvos(16.0), watchos(9.0))

__BEGIN_DECLS

// Private flags (kDNSServiceFlagsPrivateOne, kDNSServiceFlagsPrivateTwo, kDNSServiceFlagsPrivateThree, kDNSServiceFlagsPrivateFour, kDNSServiceFlagsPrivateFive) from dns_sd.h
enum
{
    kDNSServiceFlagsDenyConstrained        = 0x2000,
    /*
     * This flag is meaningful only for Unicast DNS queries. When set, the daemon will restrict
     * DNS resolutions on interfaces defined as constrained for that request.
     */

    kDNSServiceFlagsDenyCellular           = 0x8000000,
    /*
     * This flag is meaningful only for Unicast DNS queries. When set, the daemon will restrict
     * DNS resolutions on the cellular interface for that request.
     */
    kDNSServiceFlagsServiceIndex           = 0x10000000,
    /*
     * This flag is meaningful only for DNSServiceGetAddrInfo() for Unicast DNS queries.
     * When set, DNSServiceGetAddrInfo() will interpret the "interfaceIndex" argument of the call
     * as the "serviceIndex".
     */

    kDNSServiceFlagsDenyExpensive          = 0x20000000,
    /*
     * This flag is meaningful only for Unicast DNS queries. When set, the daemon will restrict
     * DNS resolutions on interfaces defined as expensive for that request.
     */

    kDNSServiceFlagsPathEvaluationDone     = 0x40000000
    /*
     * This flag is meaningful for only Unicast DNS queries.
     * When set, it indicates that Network PathEvaluation has already been performed.
     */
};

typedef enum
{
    kDNSServiceFailoverPolicyNone  DNS_SD_ENUM_SPI_AVAILABLE_FALL_2021 = 0,
    kDNSServiceFailoverPolicyAllow DNS_SD_ENUM_SPI_AVAILABLE_FALL_2021 = 1
} DNSServiceFailoverPolicy;

typedef enum
{
    kDNSServiceValidationPolicyNone     DNS_SD_ENUM_SPI_AVAILABLE_FALL_2022 = 0,
    kDNSServiceValidationPolicyRequired DNS_SD_ENUM_SPI_AVAILABLE_FALL_2022 = 1
} DNSServiceValidationPolicy;


#define kDNSServiceCompPrivateDNS   "PrivateDNS"
#define kDNSServiceCompMulticastDNS "MulticastDNS"

__END_DECLS

#endif  // _DNS_SD_PRIVATE_H
