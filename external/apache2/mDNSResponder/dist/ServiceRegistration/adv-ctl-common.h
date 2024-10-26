/* adv-ctl-common.h
 *
 * Copyright (c) 2019-2023 Apple Inc. All rights reserved.
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
 * This file contains Macros shared by service registration code.
 */


#ifndef XPC_CLIENT_ADVERTISING_PROXY_H
#define XPC_CLIENT_ADVERTISING_PROXY_H

#define kDNSSDAdvertisingProxyResponse                     0
#define kDNSSDAdvertisingProxyEnable                       1
#define kDNSSDAdvertisingProxyListServiceTypes             2
#define kDNSSDAdvertisingProxyListServices                 3
#define kDNSSDAdvertisingProxyListHosts                    4
#define kDNSSDAdvertisingProxyGetHost                      5
#define kDNSSDAdvertisingProxyFlushEntries                 6
#define kDNSSDAdvertisingProxyBlockService                 7
#define kDNSSDAdvertisingProxyUnblockService               8
#define kDNSSDAdvertisingProxyRegenerateULA                9
#define kDNSSDAdvertisingProxyAdvertisePrefix              10
#define kDNSSDAdvertisingProxyStop                         11
#define kDNSSDAdvertisingProxyGetULA                       12
#define kDNSSDAdvertisingProxyDisableReplication           13
#define kDNSSDAdvertisingProxyDropSrplConnection           14
#define kDNSSDAdvertisingProxyUndropSrplConnection         15
#define kDNSSDAdvertisingProxyDropSrplAdvertisement        16
#define kDNSSDAdvertisingProxyUndropSrplAdvertisement      17
#define kDNSSDAdvertisingProxyAddPrefix                    18
#define kDNSSDAdvertisingProxyRemovePrefix                 19
#define kDNSSDAdvertisingProxyStartDroppingPushConnections 20
#define kDNSSDAdvertisingProxyAddNAT64Prefix               21
#define kDNSSDAdvertisingProxyRemoveNAT64Prefix            22
#define kDNSSDAdvertisingProxyStartBreakingTimeValidation  23
#define kDNSSDAdvertisingProxySetVariable                  24
#define kDNSSDAdvertisingProxyBlockAnycastService          25
#define kDNSSDAdvertisingProxyUnblockAnycastService        26

#endif /* XPC_CLIENT_ADVERTISING_PROXY_H */

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 108
// indent-tabs-mode: nil
// End:
