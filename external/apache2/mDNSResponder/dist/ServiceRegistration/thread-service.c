/* thread-service.c
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
 * Manage Thread service objects
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <pwd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <time.h>
#include <dns_sd.h>
#include <net/if.h>
#include <inttypes.h>
#include <sys/resource.h>
#include <netinet/icmp6.h>
#include "srp.h"
#include "dns-msg.h"
#include "srp-crypto.h"
#include "ioloop.h"
#include "srp-mdns-proxy.h"
#include "dnssd-proxy.h"
#include "srp-gw.h"
#include "srp-proxy.h"
#include "config-parse.h"
#include "cti-services.h"
#include "thread-device.h"
#include "state-machine.h"
#include "thread-service.h"

const char *
thread_service_publication_state_name_get(thread_service_publication_state_t publication_state)
{
    switch (publication_state) {
    default:
        return "<unknown>";
    case add_complete:
        return "add_complete";
    case delete_complete:
        return "delete_complete";
    case add_failed:
        return "add_failed";
    case delete_failed:
        return "delete_failed";
    case add_pending:
        return "add_pending";
    case delete_pending:
        return "delete_pending";
    case want_add:
        return "want_add";
    case want_delete:
        return "want_delete";
    }
}

static void
thread_service_finalize(thread_service_t *service)
{
    free(service);
}

RELEASE_RETAIN_FUNCS(thread_service);

void
thread_service_list_release(thread_service_t **list_pointer)
{
    while (*list_pointer != NULL) {
        thread_service_t *service = *list_pointer;
        *list_pointer = service->next;
        RELEASE_HERE(service, thread_service);
    }
}

thread_service_t *
thread_service_unicast_create_(uint16_t rloc16, uint8_t *address, uint8_t *port, uint8_t service_id, const char *file, int line)
{
    thread_service_t *service;

    service = calloc(1, sizeof(*service));
    if (service != NULL) {
        in6prefix_copy_from_data(&service->u.unicast.address, address, sizeof(service->u.unicast.address));
        memcpy(&service->u.unicast.port, port, 2);
        service->rloc16 = rloc16;
        service->service_type = unicast_service;
        service->service_id = service_id;
        RETAIN(service, thread_service);
    }
    return service;
}

thread_service_t *
thread_service_anycast_create_(uint16_t rloc16, uint8_t sequence_number, uint8_t service_id, const char *file, int line)
{
    thread_service_t *service;

    service = calloc(1, sizeof(*service));
    if (service != NULL) {
        service->rloc16 = rloc16;
        service->u.anycast.sequence_number = sequence_number;
        service->service_type = anycast_service;
        service->service_id = service_id;
        RETAIN(service, thread_service);
    }
    return service;
}

thread_service_t *
thread_service_pref_id_create_(uint16_t rloc16, uint8_t *partition_id, uint8_t *prefix, uint8_t service_id, const char *file, int line)
{
    thread_service_t *service;

    service = calloc(1, sizeof(*service));
    if (service != NULL) {
        service->rloc16 = rloc16;
        memcpy(&service->u.pref_id.partition_id, partition_id, 4);
        memcpy(&service->u.pref_id.prefix, prefix, 5);
        service->service_type = pref_id;
        service->service_id = service_id;
        RETAIN(service, thread_service);
    }
    return service;
}

void
thread_service_note(const char *owner_id, thread_service_t *tservice, const char *event_description)
{
    switch(tservice->service_type) {
    default:
        INFO("invalid service type %d on service %p", tservice->service_type, tservice);
        break;
    case unicast_service:
        {
            struct thread_unicast_service *service = &tservice->u.unicast;
            uint16_t port;

            port = (uint16_t)(service->port[0] << 8) | service->port[1];
            SEGMENTED_IPv6_ADDR_GEN_SRP(&service->address, service_add_buf);
            INFO(PUB_S_SRP " SRP service " PRI_SEGMENTED_IPv6_ADDR_SRP "%%%d, rloc16 %x " PUB_S_SRP, owner_id,
                 SEGMENTED_IPv6_ADDR_PARAM_SRP(&service->address, service_add_buf),
                 port, tservice->rloc16, event_description);
        }
        break;
    case anycast_service:
        {
            struct thread_anycast_service *service = &tservice->u.anycast;
            INFO(PUB_S_SRP " SRP service on RLOC16 %x with sequence number %02x "  PUB_S_SRP, owner_id,
                 tservice->rloc16, service->sequence_number, event_description);
        }
        break;
    case pref_id:
        {
            struct thread_pref_id *pref_id = &tservice->u.pref_id;
            struct in6_addr addr;

            addr.s6_addr[0] = 0xfd;
            memcpy(&addr.s6_addr[1], pref_id->prefix, 5);
            memset(&addr.s6_addr[6], 0, 10);
            SEGMENTED_IPv6_ADDR_GEN_SRP(addr.s6_addr, addr_buf);
            INFO(PUB_S_SRP " pref:id " PRI_SEGMENTED_IPv6_ADDR_SRP ":%02x%02x%02x%02x rloc %x " PUB_S_SRP, owner_id,
                 SEGMENTED_IPv6_ADDR_PARAM_SRP(addr.s6_addr, addr_buf),
                 pref_id->partition_id[0], pref_id->partition_id[1], pref_id->partition_id[2], pref_id->partition_id[3],
                 tservice->rloc16, event_description);
        }
        break;
    }
}

bool
thread_service_equal(thread_service_t *a, thread_service_t *b)
{
    if (a == NULL || b == NULL) {
        return false;
    }
    if (a->service_type != b->service_type) {
        return false;
    }
    switch(a->service_type) {
    default:
        return false;
    case unicast_service:
        {
            return (!in6addr_compare(&a->u.unicast.address, &b->u.unicast.address) &&
                    !memcmp(a->u.unicast.port, b->u.unicast.port, 2));
        }
        break;
    case anycast_service:
        {
            return a->u.anycast.sequence_number == b->u.anycast.sequence_number;
        }
        break;
    case pref_id:
        {
            return false;
        }
        break;
    }
    return false;
}

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 120
// indent-tabs-mode: nil
// End:
