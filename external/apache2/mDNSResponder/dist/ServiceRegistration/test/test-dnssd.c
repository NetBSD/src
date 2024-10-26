/* test-dnssd.c
 *
 * Copyright (c) 2023-2024 Apple Inc. All rights reserved.
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

#include <dns_sd.h>
#include "srp.h"
#include "srp-test-runner.h"
#include "srp-api.h"
#include "dns-msg.h"
#include "ioloop.h"
#include "srp-proxy.h"
#include "srp-dnssd.h"
#include "srp-mdns-proxy.h"
#include "test-api.h"
#include "test-dnssd.h"

static char *
dns_service_rdata_to_text(test_state_t *state, int rrtype, const uint8_t *rdata, uint16_t rdlen, char *outbuf, size_t buflen)
{
    dns_rr_t rr;
    unsigned offp = 0;

    rr.type = rrtype;
    TEST_FAIL_CHECK(state, dns_rdata_parse_data(&rr, rdata, &offp, rdlen, rdlen, 0), "rr parse failed");
    dns_rdata_dump_to_buf(&rr, outbuf, buflen);
    return outbuf;
}

static void
dns_service_dump_event(test_state_t *state, dns_service_event_t *event, dns_service_event_t *events)
{
    char rrbuf[1024];
    char *rr_printed;
    int rrtype = 0;
    uint16_t rdlen;
    uint8_t *rdata;
#define STR_OR_NULL(str) ((str) != NULL ? str : "<null>")
    switch(event->event_type) {
    case dns_service_event_type_register:
        rr_printed = dns_service_rdata_to_text(state, dns_rrtype_txt, event->rdata, event->rdlen, rrbuf, sizeof(rrbuf));
        INFO("         register: %s.%s.%s port %d IN %s (%p %p) -> %d", STR_OR_NULL(event->name),
             STR_OR_NULL(event->regtype), STR_OR_NULL(event->domain), event->port, STR_OR_NULL(rr_printed),
             (char *)event->sdref, (char *)event->parent_sdref, event->status);
        break;
    case dns_service_event_type_register_record:
        rr_printed = dns_service_rdata_to_text(state, event->rrtype, event->rdata, event->rdlen, rrbuf, sizeof(rrbuf));
        INFO("  register record: %s IN %s (%p %p) -> %d", STR_OR_NULL(event->name),
             STR_OR_NULL(rr_printed), (char *)event->rref, (char *)event->sdref, event->status);
        break;
    case dns_service_event_type_remove_record:
        INFO("    remove record: (%p %p) -> %d", (char *)event->rref, (char *)event->sdref, event->status);
        break;
    case dns_service_event_type_update_record:
        // Get the rrtype from the previous event.
        rdata = event->rdata;
        rdlen = event->rdlen;
        for (dns_service_event_t *ep = events; ep != NULL; ep = ep->next) {
            if (ep->event_type == dns_service_event_type_register_record && ep->rref == event->rref) {
                rrtype = ep->rrtype;
                // When updating the TSR record, we send rdlen=0 and no rdata, which means just update the
                // TSR and don't change the RR. But that would cause a parse failure, so we need the data
                // as well as the rrtype from the RegisterRecord event.
                if (rdlen == 0 && ep->rdlen != 0) {
                    rdata = ep->rdata;
                    rdlen = ep->rdlen;
                }
                break;
            }
        }
        rr_printed = dns_service_rdata_to_text(state, rrtype, rdata, rdlen, rrbuf, sizeof(rrbuf));
        INFO("    update record: %s (%p %p) -> %d", STR_OR_NULL(rr_printed), (char *)event->rref, (char *)event->sdref,
             event->status);
        break;
    case dns_service_event_type_ref_deallocate:
        INFO("   ref deallocate: (%p %p) -> %d", (char *)event->sdref, (char *)event->parent_sdref, event->status);
        break;
    case dns_service_event_type_register_callback:
        INFO("     reg callback: (%p %p) -> %d", (char *)event->sdref, (char *)event->parent_sdref, event->status);
        break;
    case dns_service_event_type_register_record_callback:
        INFO("  regrec callback: (%p %p) -> %d", (char *)event->rref, (char *)event->sdref, event->status);
        break;
    }
}

bool
dns_service_dump_unexpected_events(test_state_t *test_state, srp_server_t *server_state)
{
    bool ret = true;
    for (dns_service_event_t *event = server_state->dns_service_events; event; event = event->next) {
        if (!event->consumed) {
            dns_service_dump_event(test_state, event, server_state->dns_service_events);
            ret = false;
        }
    }
    return ret;
}

dns_service_event_t *
dns_service_find_first_register_event_by_name_and_type(srp_server_t *state, const char *name,
                                                       const char *regtype)
{
    for (dns_service_event_t *event = state->dns_service_events; event; event = event->next) {
        if (event->event_type == dns_service_event_type_register &&
            event->name != NULL && event->regtype != NULL && !event->consumed)
        {
            INFO("event->name %s name %s  event->regtype %s regtype %s",
                 event->name, name, event->regtype, regtype);
            if (!strcmp(event->name, name) && !strcmp(event->regtype, regtype)) {
                return event;
            }
        }
    }
    return NULL;
}

dns_service_event_t *
dns_service_find_first_register_record_event_by_name(srp_server_t *state, const char *name)
{
    for (dns_service_event_t *event = state->dns_service_events; event; event = event->next) {
        if (event->event_type == dns_service_event_type_register_record &&
            event->name != NULL && !event->consumed && !strcmp(name, event->name))
        {
            return event;
        }
    }
    return NULL;
}

dns_service_event_t *
dns_service_find_callback_for_registration(srp_server_t *state, dns_service_event_t *register_event)
{
    for (dns_service_event_t *event = state->dns_service_events; event; event = event->next) {
        if (event->event_type == dns_service_event_type_register_callback &&
            register_event->event_type == dns_service_event_type_register &&
            register_event->sdref == event->sdref && !event->consumed)
        {
            return event;
        }
        if (event->event_type == dns_service_event_type_register_record_callback &&
            register_event->event_type == dns_service_event_type_register_record &&
            register_event->rref == event->rref && !event->consumed)
        {
            return event;
        }
    }
    return NULL;
}

dns_service_event_t *
dns_service_find_ref_deallocate_event(srp_server_t *state)
{
    for (dns_service_event_t *event = state->dns_service_events; event; event = event->next) {
        if (event->event_type == dns_service_event_type_ref_deallocate) {
            return event;
        }
    }
    return NULL;
}

// Find a DNSServiceUpdateRecord that corresponds to a DNSServiceRegister or DNSServiceRegisterRecord event.
// For a DNSServiceRegister update, event->rref will be NULL.
dns_service_event_t *
dns_service_find_update_for_register_event(srp_server_t *state, dns_service_event_t *register_event,
                                           dns_service_event_t *after_event)
{
    bool after_event_matched = after_event == NULL ? true : false;
    for (dns_service_event_t *event = state->dns_service_events; event != NULL; event = event->next) {
        // If we are past any after_event and this is an update_record event, see if it's for the same
        // registration.
        if (after_event_matched && event->event_type == dns_service_event_type_update_record &&
            ((register_event->rref == 0 && event->sdref == register_event->sdref) ||
             (register_event->rref != 0 && event->rref == register_event->rref)))
        {
            return event;
        }
        // Don't match events that are prior to after_event (so that we can skip an event if it's not the right one
        if (event == after_event) {
            after_event_matched = true;
        }
    }
    return NULL;
}

// Find a DNSServiceRemoveRecord
dns_service_event_t *
dns_service_find_remove_for_register_event(srp_server_t *state, dns_service_event_t *register_event,
                                           dns_service_event_t *after_event)
{
    bool after_event_matched = after_event == NULL ? true : false;
    for (dns_service_event_t *event = state->dns_service_events; event != NULL; event = event->next) {
        // If we are past any after_event and this is an update_record event, see if it's for the same
        // registration.
        if (after_event_matched && event->event_type == dns_service_event_type_remove_record &&
            ((register_event->rref == 0 && event->sdref == register_event->sdref) ||
             (register_event->rref != 0 && event->rref == register_event->rref)))
        {
            return event;
        }
        // Don't match events that are prior to after_event (so that we can skip an event if it's not the right one)
        if (event == after_event) {
            after_event_matched = true;
        }
    }
    return NULL;
}

static dns_service_ref_t *
dns_service_ref_create(srp_server_t *server_state,
                       DNSServiceRef *target, int flags, void *context)
{
    dns_service_ref_t *ret = calloc(1, sizeof(*ret));
    TEST_FAIL_CHECK(server_state->test_state, ret != NULL, "no memory for dns_service_ref_t");
    if (flags & kDNSServiceFlagsShareConnection) {
        ret->sdref = *target;
    }
    ret->server_state = server_state;
    ret->context = context;
    return ret;
}

static dns_service_ref_t *
dns_service_ref_create_for_register(srp_server_t *server_state,
                                    DNSServiceRef *target, int flags, void *context, DNSServiceRegisterReply callback)
{
    dns_service_ref_t *ret = dns_service_ref_create(server_state, target, flags, context);
    if (ret != NULL) {
        ret->callback.register_reply = callback;
    }
    return ret;
}

static dns_service_ref_t *
dns_service_ref_create_for_query_record(srp_server_t *server_state,
                                        DNSServiceRef *target, int flags, void *context, DNSServiceQueryRecordReply callback)
{
    dns_service_ref_t *ret = dns_service_ref_create(server_state, target, flags, context);
    if (ret != NULL) {
        ret->callback.query_record_reply = callback;
    }
    return ret;
}

static dns_record_ref_t *
dns_record_ref_create(srp_server_t *server_state, void *context, DNSServiceRegisterRecordReply callback)
{
    dns_record_ref_t *ret = calloc(1, sizeof(*ret));
    TEST_FAIL_CHECK(server_state->test_state, ret != NULL, "no memory for dns_record_ref_t");
    ret->server_state = server_state;
    ret->context = context;
    ret->callback = callback;
    return ret;
}

static char *
dns_service_string_dup(const char *src)
{
    if (src == NULL) {
        return NULL;
    }
    return strdup(src);
}

static dns_service_event_t *
dns_service_event_append(srp_server_t *state, dns_service_event_type_t event_type, DNSServiceRef sdref,
                         DNSServiceRef in_sdref, DNSRecordRef rref, DNSServiceFlags flags, uint32_t interfaceIndex,
                         const char *name, const char *regtype, const char *domain, const char *host, uint16_t port,
                         uint16_t rdlen, const void *rdata, uint16_t rrtype, uint16_t rrclass, uint32_t ttl, void *attr,
                         void *callBack, void *context, DNSServiceErrorType status)
{
    TEST_FAIL_CHECK(NULL, state != NULL, "invalid server state");
    dns_service_event_t **ep = &state->dns_service_events;
    while (*ep) {
        ep = &(*ep)->next;
    }
    dns_service_event_t *event = calloc(1, sizeof(*event));
    TEST_FAIL_CHECK(state->test_state, event != NULL, "no memory for dns service event");
    *ep = event;
    event->server_state = state;
    event->event_type = event_type;
    event->sdref = (intptr_t)sdref;
    if (in_sdref != NULL && (flags & kDNSServiceFlagsShareConnection)) {
        event->parent_sdref = (intptr_t)in_sdref;
    }
    event->rref = (intptr_t)rref;
    event->flags = flags;
    event->interface_index = interfaceIndex;
    event->name = dns_service_string_dup(name);
    event->regtype = dns_service_string_dup(regtype);
    event->domain = dns_service_string_dup(domain);
    event->host = dns_service_string_dup(host);
    event->port = port;
    event->rdlen = rdlen;
    if (rdata != NULL) {
        event->rdata = malloc(rdlen);
        TEST_FAIL_CHECK(state->test_state, event->rdata != NULL, "no memory to save rdata");
        memcpy(event->rdata, rdata, rdlen);
    }
    event->rrclass = rrclass;
    event->rrtype = rrtype;
    event->ttl = ttl;
    event->attr = (intptr_t)attr;
    event->callBack = (intptr_t)callBack;
    event->context = (intptr_t)context;
    event->status = status;
    return event;
}

static void
dns_service_register_callback(DNSServiceRef sdRef, DNSServiceFlags flags, DNSServiceErrorType errorCode,
                              const char *name, const char *regtype, const char *domain, void *context)
{
    dns_service_ref_t *ref = context;
    dns_service_event_append(ref->server_state, dns_service_event_type_register_callback, sdRef, NULL, NULL, flags, 0,
                             name, regtype, domain, NULL, 0, 0, NULL, 0, 0, 0, NULL, NULL, context, errorCode);
    DNSServiceRegisterReply callback = ref->callback.register_reply;
    if (callback != NULL) {
        callback(ref, flags, errorCode, name, regtype, domain, ref->context);
    }
}

DNSServiceErrorType
dns_service_register(srp_server_t *state, DNSServiceRef *sdRef, DNSServiceFlags flags, uint32_t interfaceIndex,
                     const char *name, const char *regtype, const char *domain, const char *host, uint16_t port,
                     uint16_t txtLen, const void *txtRecord, DNSServiceRegisterReply callBack, void *context)
{
    return dns_service_register_wa(state, sdRef, flags, interfaceIndex, name, regtype, domain, host, port, txtLen,
                                   txtRecord, NULL, callBack, context);
}

DNSServiceErrorType
dns_service_register_wa(srp_server_t *state, DNSServiceRef *sdRef, DNSServiceFlags flags, uint32_t interfaceIndex,
                        const char *name, const char *regtype, const char *domain, const char *host, uint16_t port,
                        uint16_t txtLen, const void *txtRecord, DNSServiceAttributeRef attr,
                        DNSServiceRegisterReply callBack, void *context)
{
#undef DNSServiceRegisterWithAttribute
    dns_service_ref_t *ret = dns_service_ref_create_for_register(state, sdRef, flags, context, callBack);
    char updated_name[DNS_MAX_NAME_SIZE + 1];
    snprintf(updated_name, sizeof(updated_name), "%d-%s", state->server_id, name);
    int status = DNSServiceRegisterWithAttribute(&ret->sdref, flags, interfaceIndex, updated_name, regtype, domain, host, port,
                                                 txtLen, txtRecord, attr, dns_service_register_callback, ret);
    dns_service_event_append(state, dns_service_event_type_register, ret->sdref, *sdRef, NULL, flags,
                             interfaceIndex, name, regtype, domain, host, port, txtLen, txtRecord,
                             0, 0, 0, attr, callBack, context, status);
    if (status != kDNSServiceErr_NoError) {
        free(ret);
    }
    *sdRef = ret;
    return status;
}

static void
dns_service_register_record_callback(DNSServiceRef sdRef, DNSRecordRef RecordRef, DNSServiceFlags flags,
                                     DNSServiceErrorType errorCode, void *context)
{
    dns_record_ref_t *ref = context;
    dns_service_event_append(ref->server_state, dns_service_event_type_register_record_callback, sdRef, NULL,
                             RecordRef, flags, 0, NULL, NULL, NULL, NULL, 0, 0, NULL, 0, 0, 0, NULL, NULL,
                             context, errorCode);
    if (ref->callback != NULL) {
        ref->callback(sdRef, ref, flags, errorCode, ref->context);
    }
}

DNSServiceErrorType
dns_service_register_record(srp_server_t *state, DNSServiceRef sdRef, DNSRecordRef *RecordRef, DNSServiceFlags flags,
                            uint32_t interfaceIndex, const char *fullname, uint16_t rrtype, uint16_t rrclass,
                            uint16_t rdlen, const void *rdata, uint32_t ttl, DNSServiceRegisterRecordReply callBack,
                            void *context)
{
    return dns_service_register_record_wa(state, sdRef, RecordRef, flags, interfaceIndex, fullname, rrtype, rrclass, rdlen,
                                          rdata, ttl, NULL, callBack, context);
}

DNSServiceErrorType
dns_service_register_record_wa(srp_server_t *state, DNSServiceRef sdRef, DNSRecordRef *RecordRef, DNSServiceFlags flags,
                               uint32_t interfaceIndex, const char *fullname, uint16_t rrtype, uint16_t rrclass,
                               uint16_t rdlen, const void *rdata, uint32_t ttl, DNSServiceAttributeRef attr,
                               DNSServiceRegisterRecordReply callBack,
                               void *context)
{
#undef DNSServiceRegisterRecordWithAttribute
    dns_record_ref_t *ret = dns_record_ref_create(state, context, callBack);

    // Since in testing multiple srp servers share the same mDNSResponder,
    // we intentionally rename the record by prepending the server_id
    // to the record name when registering with mDNSResponder, so that
    // it will not generate conflict for replicated records.
    char updated_name[DNS_MAX_NAME_SIZE + 1];
    snprintf(updated_name, sizeof(updated_name), "%d-%s", state->server_id, fullname);
    int status = DNSServiceRegisterRecordWithAttribute(sdRef, &ret->rref, flags, interfaceIndex, updated_name, rrtype,
                                                   rrclass, rdlen, rdata, ttl, attr,
                                                   dns_service_register_record_callback, ret);
    dns_service_event_append(state, dns_service_event_type_register_record, sdRef, NULL, ret->rref, flags,
                             interfaceIndex, fullname, NULL, NULL, NULL, 0, rdlen, rdata, rrtype, rrclass, ttl, attr,
                             callBack, context, status);
    if (status != kDNSServiceErr_NoError) {
        free(ret);
    }
    *RecordRef = ret;
    return status;
}

// Note that this will not work with a recordref returned by DNSServiceAddRecord(), which we aren't currently intercepting
// because we don't use it.
DNSServiceErrorType
dns_service_remove_record(srp_server_t *state, DNSServiceRef sdRef, DNSRecordRef RecordRef, DNSServiceFlags flags)
{
#undef DNSServiceRemoveRecord
    int ret = DNSServiceRemoveRecord(sdRef, RecordRef->rref, flags);
    dns_service_event_append(state, dns_service_event_type_remove_record, sdRef->sdref, NULL, RecordRef->rref,
                             flags, 0, NULL, NULL, NULL, NULL, 0, 0, NULL, 0, 0, 0, NULL, NULL, NULL, ret);
    return ret;
}

DNSServiceErrorType
dns_service_update_record(srp_server_t *state, DNSServiceRef sdRef, DNSRecordRef RecordRef, DNSServiceFlags flags,
                          uint16_t rdlen, const void *rdata, uint32_t ttl)
{
    return dns_service_update_record_wa(state, sdRef, RecordRef, flags, rdlen, rdata, ttl, NULL);
}

DNSServiceErrorType
dns_service_update_record_wa(srp_server_t *state, DNSServiceRef sdRef, DNSRecordRef RecordRef, DNSServiceFlags flags,
                             uint16_t rdlen, const void *rdata, uint32_t ttl, DNSServiceAttributeRef attr)
{
#undef DNSServiceUpdateRecordWithAttribute
    int ret;
    if (RecordRef != NULL) {
        ret = DNSServiceUpdateRecordWithAttribute(sdRef, RecordRef->rref, flags, rdlen, rdata, ttl, attr);
        dns_service_event_append(state, dns_service_event_type_update_record, sdRef, NULL, RecordRef->rref, flags, 0,
                                 NULL, NULL, NULL, NULL, 0, rdlen, rdata, 0, 0, ttl, NULL, NULL, NULL, ret);
    } else {
        ret = DNSServiceUpdateRecordWithAttribute(sdRef->sdref, NULL, flags, rdlen, rdata, ttl, attr);
        dns_service_event_append(state, dns_service_event_type_update_record, sdRef->sdref, NULL, NULL, flags, 0,
                                 NULL, NULL, NULL, NULL, 0, rdlen, rdata, 0, 0, ttl, NULL, NULL, NULL, ret);
    }
    return ret;
}

static void
dns_service_query_record_callback(DNSServiceRef UNUSED sdRef, DNSServiceFlags flags, uint32_t interfaceIndex,
                                  DNSServiceErrorType errorCode, const char *fullname, uint16_t rrtype,
                                  uint16_t rrclass, uint16_t rdlen, const void *rdata, uint32_t ttl, void *context)
{
    dns_service_ref_t *ref = context;
    srp_server_t *server_state = ref->server_state;
    test_state_t *state = server_state->test_state;
    bool call_callback = true;
    dns_service_query_record_callback_intercept_t intercept = state->dns_service_query_callback_intercept;
    if (intercept != NULL) {
        call_callback = intercept(ref, flags, interfaceIndex, errorCode, fullname, rrtype, rrclass, rdlen, rdata, ttl, ref->context);
    }
    if (call_callback) {
        DNSServiceQueryRecordReply callback = ref->callback.query_record_reply;
        callback(ref, flags, interfaceIndex, errorCode, fullname, rrtype, rrclass, rdlen, rdata, ttl, ref->context);
    }
}

DNSServiceErrorType
dns_service_query_record(srp_server_t *NULLABLE srp_server, DNSServiceRef NONNULL *NULLABLE sdRef,
                         DNSServiceFlags flags, uint32_t interfaceIndex, const char *NONNULL fullname,
                         uint16_t rrtype, uint16_t rrclass, DNSServiceQueryRecordReply NONNULL callBack,
                         void *NULLABLE context)
{
    return dns_service_query_record_wa(srp_server, sdRef, flags, interfaceIndex, fullname, rrtype, rrclass,
                                       NULL, callBack, context);
}

DNSServiceErrorType
dns_service_query_record_wa(srp_server_t *NULLABLE srp_server, DNSServiceRef NONNULL *NULLABLE sdRef,
                            DNSServiceFlags flags, uint32_t interfaceIndex, const char *NONNULL fullname,
                            uint16_t rrtype, uint16_t rrclass, DNSServiceAttribute const *NULLABLE attr,
                            DNSServiceQueryRecordReply NONNULL callBack, void *NULLABLE context)
{
    dns_service_ref_t *ret = dns_service_ref_create_for_query_record(srp_server, sdRef, flags, context, callBack);
    test_state_t *state = srp_server->test_state;
    int status;
    if (state->query_record_intercept != NULL) {
        status = state->query_record_intercept(state, &ret->sdref, flags, interfaceIndex, fullname, rrtype, rrclass,
                                               attr, dns_service_query_record_callback, ret);
    } else {
        status = DNSServiceQueryRecordWithAttribute(&ret->sdref, flags, interfaceIndex, fullname, rrtype, rrclass,
                                                    attr, dns_service_query_record_callback, ret);
    }
    if (status != kDNSServiceErr_NoError) {
        free(ret);
    }
    *sdRef = ret;
    return status;
}

dnssd_txn_t *NULLABLE
dns_service_ioloop_txn_add(srp_server_t UNUSED *NULLABLE srp_server, DNSServiceRef NONNULL sdref,
                           void *NULLABLE context, dnssd_txn_finalize_callback_t NULLABLE finalize_callback,
                           dnssd_txn_failure_callback_t NULLABLE failure_callback)
{
    return ioloop_dnssd_txn_add(sdref->sdref, context, finalize_callback, failure_callback);
}

void
dns_service_ref_deallocate(srp_server_t *state, DNSServiceRef sdRef)
{
#undef DNSServiceRefDeallocate
    dns_service_event_append(state, dns_service_event_type_ref_deallocate, sdRef, NULL, NULL,
                             0, 0, NULL, NULL, NULL, NULL, 0, 0, NULL, 0, 0, 0, NULL, NULL, NULL, 0);
    DNSServiceRefDeallocate(sdRef->sdref);
    free(sdRef);
}


// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 108
// indent-tabs-mode: nil
// End:
