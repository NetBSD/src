/* srp-dnssd.h
 *
 * Copyright (c) 2023 Apple Inc. All rights reserved.
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
 * DNSSD intercept API for testing srp-mdns-proxy
 */

#ifdef SRP_TEST_SERVER
typedef struct srp_server_state srp_server_t;
DNSServiceErrorType dns_service_register(srp_server_t *NULLABLE srp_server, DNSServiceRef NONNULL *NULLABLE sdRef,
                                         DNSServiceFlags flags, uint32_t interfaceIndex, const char *NULLABLE name,
                                         const char *NONNULL regtype, const char *NULLABLE domain,
                                         const char *NULLABLE host, uint16_t port, uint16_t txtLen,
                                         const void *NULLABLE txtRecord, DNSServiceRegisterReply NULLABLE callBack,
                                         void *NULLABLE context);
DNSServiceErrorType dns_service_register_wa(srp_server_t *NULLABLE srp_server, DNSServiceRef NONNULL *NULLABLE sdRef,
                                            DNSServiceFlags flags, uint32_t interfaceIndex, const char *NULLABLE name,
                                            const char *NONNULL regtype, const char *NULLABLE domain,
                                            const char *NULLABLE host, uint16_t port, uint16_t txtLen,
                                            const void *NULLABLE txtRecord, DNSServiceAttributeRef NULLABLE attr,
                                            DNSServiceRegisterReply NULLABLE callBack, void *NULLABLE context);
DNSServiceErrorType dns_service_register_record(srp_server_t *NULLABLE srp_server, DNSServiceRef NONNULL sdRef,
                                                DNSRecordRef NONNULL *NULLABLE RecordRef, DNSServiceFlags flags,
                                                uint32_t interfaceIndex, const char *NONNULL fullname, uint16_t rrtype,
                                                uint16_t rrclass, uint16_t rdlen, const void *NONNULL rdata,
                                                uint32_t ttl, DNSServiceRegisterRecordReply NULLABLE callBack,
                                                void *NULLABLE context);
DNSServiceErrorType dns_service_register_record_wa(srp_server_t *NULLABLE srp_server, DNSServiceRef NONNULL sdRef,
                                                   DNSRecordRef NONNULL *NULLABLE RecordRef, DNSServiceFlags flags,
                                                   uint32_t interfaceIndex, const char *NONNULL fullname,
                                                   uint16_t rrtype, uint16_t rrclass, uint16_t rdlen,
                                                   const void *NONNULL rdata, uint32_t ttl,
                                                   DNSServiceAttributeRef NULLABLE attr,
                                                   DNSServiceRegisterRecordReply NULLABLE callBack,
                                                   void *NULLABLE context);
DNSServiceErrorType dns_service_remove_record(srp_server_t *NULLABLE srp_server, DNSServiceRef NONNULL sdRef,
                                              DNSRecordRef NONNULL RecordRef, DNSServiceFlags flags);
DNSServiceErrorType dns_service_update_record(srp_server_t *NULLABLE srp_server, DNSServiceRef NONNULL sdRef,
                                              DNSRecordRef NULLABLE recordRef, DNSServiceFlags flags, uint16_t rdlen,
                                              const void *NONNULL rdata, uint32_t ttl);
DNSServiceErrorType dns_service_update_record_wa(srp_server_t *NULLABLE srp_server, DNSServiceRef NONNULL sdRef,
                                                 DNSRecordRef NULLABLE recordRef, DNSServiceFlags flags, uint16_t rdlen,
                                                 const void *NULLABLE rdata, uint32_t ttl,
                                                 DNSServiceAttributeRef NULLABLE attr);
void dns_service_ref_deallocate(srp_server_t *NULLABLE srp_server, DNSServiceRef NONNULL sdRef);
void ioloop_dnssd_txn_cancel_srp(void *NULLABLE srp_server, dnssd_txn_t *NONNULL txn);
DNSServiceErrorType
dns_service_query_record(srp_server_t *NULLABLE srp_server, DNSServiceRef NONNULL *NULLABLE sdRef,
                         DNSServiceFlags flags, uint32_t interfaceIndex, const char *NONNULL fullname,
                         uint16_t rrtype, uint16_t rrclass, DNSServiceQueryRecordReply NONNULL callBack,
                         void *NULLABLE context);
DNSServiceErrorType
dns_service_query_record_wa(srp_server_t *NULLABLE srp_server, DNSServiceRef NONNULL *NULLABLE sdRef,
                            DNSServiceFlags flags, uint32_t interfaceIndex, const char *NONNULL fullname,
                            uint16_t rrtype, uint16_t rrclass, DNSServiceAttribute const *NULLABLE attr,
                            DNSServiceQueryRecordReply NONNULL callBack, void *NULLABLE context);
dnssd_txn_t *NULLABLE
dns_service_ioloop_txn_add(srp_server_t *NULLABLE srp_server, DNSServiceRef NONNULL sdref, void *NULLABLE context,
                           dnssd_txn_finalize_callback_t NULLABLE finalize_callback,
                           dnssd_txn_failure_callback_t NULLABLE failure_callback);
#else
#define dns_service_ref_deallocate(srp_server, ...)     DNSServiceRefDeallocate(__VA_ARGS__)
#define dns_service_register_record(srp_server, ...)    DNSServiceRegisterRecord(__VA_ARGS__)
#define dns_service_register_record_wa(srp_server, ...) DNSServiceRegisterRecordWithAttribute(__VA_ARGS__)
#define dns_service_register(srp_server, ...)           DNSServiceRegister(__VA_ARGS__)
#define dns_service_register_wa(srp_server, ...)        DNSServiceRegisterWithAttribute(__VA_ARGS__)
#define dns_service_remove_record(srp_server, ...)      DNSServiceRemoveRecord(__VA_ARGS__)
#define dns_service_update_record(srp_server, ...)      DNSServiceUpdateRecord(__VA_ARGS__)
#define dns_service_update_record_wa(srp_server, ...)   DNSServiceUpdateRecordWithAttribute(__VA_ARGS__)
#define ioloop_dnssd_txn_cancel_srp(srp_server, ...)    ioloop_dnssd_txn_cancel(__VA_ARGS__)
#define dns_service_query_record(srp_server, ...)       DNSServiceQueryRecord(__VA_ARGS__)
#define dns_service_query_record_wa(srp_server, ...)    DNSServiceQueryRecordWithAttribute(__VA_ARGS__)
#define dns_service_ioloop_txn_add(srp_server, ...)     ioloop_dnssd_txn_add(__VA_ARGS__)
#endif

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 108
// indent-tabs-mode: nil
// End:
