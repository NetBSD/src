/*
 * Copyright (c) 2021-2024 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1.  Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of its
 *     contributors may be used to endorse or promote products derived from this
 *     software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "dns_sd.h"
#include "dnssd_ipc.h"

DNSServiceErrorType
DNSServiceBrowseInternal(DNSServiceRef *sdRef, DNSServiceFlags flags, uint32_t interfaceIndex, const char *regtype,
    const char *domain, const DNSServiceAttribute *attr, DNSServiceBrowseReply callBack, void *context);

DNSServiceErrorType
DNSServiceResolveInternal(DNSServiceRef *sdRef, DNSServiceFlags flags, uint32_t interfaceIndex, const char *name,
    const char *regtype, const char *domain, const DNSServiceAttribute *attr, DNSServiceResolveReply callBack,
    void *context);

DNSServiceErrorType
DNSServiceGetAddrInfoInternal(DNSServiceRef *sdRef, DNSServiceFlags flags, uint32_t interfaceIndex,
    DNSServiceProtocol protocol, const char *hostname, const DNSServiceAttribute *attr, DNSServiceGetAddrInfoReply callBack,
    void *context);

DNSServiceErrorType
DNSServiceQueryRecordInternal(DNSServiceRef *sdRef, DNSServiceFlags flags, uint32_t interfaceIndex, const char *name,
    uint16_t rrtype, uint16_t rrclass, const DNSServiceAttribute *attr, const DNSServiceQueryRecordReply callback,
    void *context);

DNSServiceErrorType
DNSServiceRegisterInternal(DNSServiceRef *sdRef, DNSServiceFlags flags, uint32_t interfaceIndex, const char *name,
    const char *regtype, const char *domain, const char *host, uint16_t portInNetworkByteOrder, uint16_t txtLen,
    const void *txtRecord, const DNSServiceAttribute *attr, DNSServiceRegisterReply callBack, void *context);

DNSServiceErrorType
DNSServiceRegisterRecordInternal(DNSServiceRef sdRef, DNSRecordRef *recordRef, DNSServiceFlags flags,
    uint32_t interfaceIndex, const char *fullname, uint16_t rrtype, uint16_t rrclass, uint16_t rdlen,
    const void *rdata, uint32_t ttl, const DNSServiceAttribute *attr, DNSServiceRegisterRecordReply callBack,
    void *context);

DNSServiceErrorType
DNSServiceSendQueuedRequestsInternal(DNSServiceRef sdr);
