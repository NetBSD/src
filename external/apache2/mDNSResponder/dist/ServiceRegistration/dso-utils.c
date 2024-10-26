/* dso-utils.c
 *
 * Copyright (c) 2018-2021 Apple Inc. All rights reserved.
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

#include <netinet/in.h>
#include "dns-msg.h"
#include "ioloop.h"
#include "dso-utils.h"
#include "dso.h"

void
dso_simple_response(comm_t *comm, message_t *message, const dns_wire_t *wire, int rcode)
{
    struct iovec iov;
    dns_wire_t response;
    memset(&response, 0, DNS_HEADER_SIZE);

    // We take the ID and the opcode from the incoming message, because if the
    // header has been mangled, we (a) wouldn't have gotten here and (b) don't
    // have any better choice anyway.
    response.id = wire->id;
    dns_qr_set(&response, dns_qr_response);
    dns_opcode_set(&response, dns_opcode_get(wire));
    dns_rcode_set(&response, rcode);

    size_t length = DNS_HEADER_SIZE;
    uint16_t wire_length = message != NULL ? message->length : (uint16_t)sizeof(*wire);
    dns_rr_t question;
    memset(&question, 0, sizeof(question));
    unsigned offp = 0;
    if (ntohs(wire->qdcount) == 1 &&
        dns_rr_parse(&question, wire->data, wire_length - DNS_HEADER_SIZE, &offp, false, false))
    {
        dns_towire_state_t towire;
        memset(&towire, 0, sizeof(towire));
        towire.p = &response.data[0];
        towire.lim = ((uint8_t *)&response) + sizeof(response);
        towire.message = &response;

        size_t namelen = dns_name_to_wire_canonical(towire.p, towire.lim - towire.p, question.name);
        if (namelen != 0) {
            towire.p += namelen;
            dns_u16_to_wire(&towire, question.type);
            dns_u16_to_wire(&towire, question.qclass);
            if (!towire.truncated && !towire.error) {
                response.qdcount = htons(1);
                length += towire.p - (uint8_t *)&response.data[0];
            }
        }
        dns_name_free(question.name);
    }
    iov.iov_base = &response;
    iov.iov_len = length; // No RRs
    ioloop_send_message(comm, message, &iov, 1);
}

bool
dso_send_simple_response(dso_state_t *dso, int rcode, const dns_wire_t *header, const char *UNUSED rcode_name)
{
    dso_simple_response((comm_t *)dso->transport, NULL, header, rcode);
    return true;
}

bool
dso_send_formerr(dso_state_t *dso, const dns_wire_t *header)
{
    comm_t *transport = dso->transport;
    (void)header;
    dso_simple_response(transport, NULL, header, dns_rcode_formerr);
    return true;
}

void
dso_retry_delay_response(comm_t *comm, message_t *message, const dns_wire_t *wire, int rcode, uint32_t milliseconds)
{
    dns_wire_t response;
    dns_towire_state_t towire;
    struct iovec iov;
    memset(&response, 0, DNS_HEADER_SIZE);
    memset(&towire, 0, sizeof(towire));

    towire.p = &response.data[0];  // We start storing RR data here.
    towire.lim = ((uint8_t *)&response) + sizeof(response);
    towire.message = &response;
    towire.p_rdlength = NULL;
    towire.p_opt = NULL;

    response.id = wire->id;
    dns_qr_set(&response, dns_qr_response);
    dns_opcode_set(&response, dns_opcode_get(wire));
    dns_rcode_set(&response, rcode);

    dns_u16_to_wire(&towire, kDSOType_RetryDelay);
    // This shouldn't be possible.
    if (towire.p + 2 > towire.lim) {
        FAULT("No room for dso length in Retry Delay message.");
        return;
    }
    uint8_t *p_dso_length = towire.p;
    towire.p += 2;

    dns_u32_to_wire(&towire, milliseconds);

    int16_t dso_length = towire.p - p_dso_length - 2;
    iov.iov_len = (towire.p - (uint8_t *)&response);
    iov.iov_base = &response;

    towire.p = p_dso_length;
    dns_u16_to_wire(&towire, dso_length);
    ioloop_send_message(comm, message, &iov, 1);
}

int32_t
dso_transport_idle(void * UNUSED context, int32_t UNUSED now, int32_t next_event)
{
    return next_event;
}

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 108
// indent-tabs-mode: nil
// End:
