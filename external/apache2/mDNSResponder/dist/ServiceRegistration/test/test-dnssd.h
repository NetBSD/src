/* test-dnssd.h
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

typedef struct dns_service_event dns_service_event_t;
typedef enum {
    dns_service_event_type_register,
    dns_service_event_type_register_record,
    dns_service_event_type_remove_record,
    dns_service_event_type_update_record,
    dns_service_event_type_ref_deallocate,
    dns_service_event_type_register_callback,
    dns_service_event_type_register_record_callback,
} dns_service_event_type_t;

struct dns_service_event {
    dns_service_event_t *NULLABLE next;
    srp_server_t *NULLABLE server_state;
    dns_service_event_type_t event_type;
    intptr_t sdref;
    intptr_t parent_sdref;
    intptr_t rref;
    DNSServiceFlags flags;
    uint32_t interface_index;
    const char *NULLABLE name;
    const char *NULLABLE regtype;
    const char *NULLABLE domain;
    const char *NULLABLE host;
    uint16_t port;
    uint16_t rrclass;
    uint16_t rrtype;
    uint16_t rdlen;
    uint32_t ttl;
    void *NULLABLE rdata;
    intptr_t attr;
    intptr_t callBack;
    intptr_t context;
    int status;
    bool consumed;
};

typedef struct _DNSServiceRef_t dns_service_ref_t;
struct _DNSServiceRef_t {
    srp_server_t *NULLABLE server_state;
    DNSServiceRef NONNULL sdref;
    void *NULLABLE context;
    union {
        DNSServiceRegisterReply NONNULL register_reply;
        DNSServiceQueryRecordReply NONNULL query_record_reply;
    } callback;
};

typedef struct _DNSRecordRef_t dns_record_ref_t;
struct _DNSRecordRef_t {
    srp_server_t *NULLABLE server_state;
    DNSRecordRef NONNULL rref;
    void *NULLABLE context;
    DNSServiceRegisterRecordReply NULLABLE callback;
};

bool dns_service_dump_unexpected_events(test_state_t *NONNULL test_state, srp_server_t *NONNULL server_state);
dns_service_event_t *NULLABLE dns_service_find_first_register_event_by_name_and_type(srp_server_t *NONNULL state,
                                                                                     const char *NONNULL name,
                                                                                     const char *NONNULL regtype);
dns_service_event_t *NULLABLE dns_service_find_first_register_record_event_by_name(srp_server_t *NONNULL state,
                                                                                   const char *NONNULL name);
dns_service_event_t *NULLABLE dns_service_find_callback_for_registration(srp_server_t *NONNULL state,
                                                                         dns_service_event_t *NONNULL register_event);
dns_service_event_t *NULLABLE dns_service_find_ref_deallocate_event(srp_server_t *NONNULL state);
dns_service_event_t *NULLABLE dns_service_find_update_for_register_event(srp_server_t *NONNULL state,
                                                                         dns_service_event_t *NONNULL register_event,
                                                                         dns_service_event_t *NULLABLE after_event);
dns_service_event_t *NULLABLE dns_service_find_remove_for_register_event(srp_server_t *NONNULL state,
                                                                         dns_service_event_t *NONNULL register_event,
                                                                         dns_service_event_t *NULLABLE after_event);
// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 108
// indent-tabs-mode: nil
// End:
