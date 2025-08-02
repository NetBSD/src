/* -*- Mode: C; tab-width: 4; c-file-style: "bsd"; c-basic-offset: 4; fill-column: 108; indent-tabs-mode: nil; -*-
 *
 * Copyright (c) 2004 Apple Computer, Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __PLATFORM_COMMON_H
#define __PLATFORM_COMMON_H

#ifndef NSEC_PER_SEC
    #define NSEC_PER_SEC 1000000000ull
#endif
#ifndef USEC_PER_SEC
    #define USEC_PER_SEC 1000000ull
#endif
#ifndef MSEC_PER_SEC
    #define MSEC_PER_SEC 1000ull
#endif
#ifndef NSEC_PER_USEC
    #define NSEC_PER_USEC 1000ull
#endif
#ifndef NSEC_PER_MSEC
    #define NSEC_PER_MSEC 1000000ull
#endif
#ifndef USEC_PER_MSEC
    #define USEC_PER_MSEC (NSEC_PER_MSEC / NSEC_PER_USEC)
#endif

extern void ReadDDNSSettingsFromConfFile(mDNS *const m, const char *const filename,
										 domainname *const hostname, domainname *const domain,
										 mDNSBool *DomainDiscoveryDisabled);
extern mDNSBool mDNSPosixTCPSocketSetup(int *fd, mDNSAddr_Type addrType, mDNSIPPort *port, mDNSIPPort *outTcpPort);
extern TCPSocket *mDNSPosixDoTCPListenCallback(int fd, mDNSAddr_Type addressType, TCPSocketFlags socketFlags,
                     TCPAcceptedCallback callback, void *context);
extern mDNSBool mDNSPosixTCPListen(int *fd, mDNSAddr_Type addrtype, mDNSIPPort *port, mDNSAddr *addr,
                   mDNSBool reuseAddr, int queueLength);
extern long mDNSPosixReadTCP(int fd, void *buf, unsigned long buflen, mDNSBool *closed);
extern long mDNSPosixWriteTCP(int fd, const char *msg, unsigned long len);
#endif
