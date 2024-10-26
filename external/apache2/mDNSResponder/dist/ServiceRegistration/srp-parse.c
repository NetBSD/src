/* srp-parse.c
 *
 * Copyright (c) 2018-2022 Apple Inc. All rights reserved.
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
 * This file contains support routines for the DNSSD SRP update and mDNS proxies.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/time.h>
#include <dns_sd.h>
#include <inttypes.h>

#include "srp.h"
#include "dns-msg.h"
#include "srp-crypto.h"
#include "ioloop.h"
#include "srp-gw.h"
#include "config-parse.h"
#include "srp-proxy.h"
#include "cti-services.h"
#include "srp-mdns-proxy.h"
#include "dnssd-proxy.h"
#include "srp-replication.h"


static dns_name_t *service_update_zone; // The zone to update when we receive an update for default.service.arpa.

// Free the data structures into which the SRP update was parsed.   The pointers to the various DNS objects that these
// structures point to are owned by the parsed DNS message, and so these do not need to be freed here.
void
srp_parse_client_updates_free_(client_update_t *messages, const char *file, int line)
{
    client_update_t *message = messages;
    while (message) {
        INFO("%p at " PUB_S_SRP ":%d", message, file, line);
        client_update_t *next_message = message->next;

        for (service_instance_t *sip = message->instances; sip; ) {
            service_instance_t *next = sip->next;
            free(sip);
            sip = next;
        }
        for (service_t *sp = message->services; sp; ) {
            service_t *next = sp->next;
            free(sp);
            sp = next;
        }
        for (delete_t *dp = message->removes; dp != NULL; ) {
            delete_t *next = dp->next;
            free(dp);
            dp = next;
        }
        if (message->host != NULL) {
            host_addr_t *host_addr, *next;
            for (host_addr = message->host->addrs; host_addr; host_addr = next) {
                next = host_addr->next;
                free(host_addr);
            }
            free(message->host);
        }
        if (message->parsed_message != NULL) {
            dns_message_free(message->parsed_message);
        }
        if (message->message != NULL) {
            ioloop_message_release(message->message);
        }
#if SRP_FEATURE_REPLICATION
        if (message->srpl_connection != NULL) {
            srpl_connection_release(message->srpl_connection);
            message->srpl_connection = NULL;
        }
#endif
        if (message->connection != NULL) {
            ioloop_comm_release(message->connection);
            message->connection = NULL;
        }
        free(message);
        message = next_message;
    }
}

static bool
add_host_addr(host_addr_t **dest, dns_rr_t *rr)
{
    host_addr_t *addr = calloc(1, sizeof *addr);
    if (addr == NULL) {
        ERROR("add_host_addr: no memory for record");
        return false;
    }

    while (*dest) {
        dest = &(*dest)->next;
    }
    *dest = addr;
    addr->rr = *rr;
    return true;
}

static bool
replace_zone_name(dns_name_t **nzp_in, dns_name_t *uzp, dns_name_t *replacement_zone)
{
    dns_name_t **nzp = nzp_in;
    while (*nzp != NULL && *nzp != uzp) {
        nzp = &((*nzp)->next);
    }
    if (*nzp == NULL) {
        ERROR("replace_zone: dns_name_subdomain_of returned bogus pointer.");
        return false;
    }

    // Free the suffix we're replacing
    dns_name_free(*nzp);

    // Replace it.
    *nzp = dns_name_copy(replacement_zone);
    if (*nzp == NULL) {
        ERROR("replace_zone_name: no memory for replacement zone");
        return false;
    }
    return true;
}

// We call advertise_finished when a client request has finished, successfully or otherwise.
static void
send_fail_response(comm_t *connection, message_t *message, int rcode)
{
    struct iovec iov;
    dns_wire_t response;

    INFO("rcode = " PUB_S_SRP, dns_rcode_name(rcode));
    memset(&response, 0, DNS_HEADER_SIZE);
    response.id = message->wire.id;
    response.bitfield = message->wire.bitfield;
    dns_rcode_set(&response, rcode);
    dns_qr_set(&response, dns_qr_response);

    iov.iov_base = &response;
    iov.iov_len = DNS_HEADER_SIZE;

    ioloop_send_message(connection, message, &iov, 1);
}

static int
make_delete(delete_t **delete_list, delete_t **delete_out, dns_rr_t *rr, dns_name_t *update_zone)
{
    int status = dns_rcode_noerror;
    delete_t *dp, **dpp;

    for (dpp = delete_list; *dpp;) {
        dp = *dpp;
        if (dns_names_equal(dp->name, rr->name)) {
            DNS_NAME_GEN_SRP(rr->name, name_buf);
            ERROR("two deletes for the same name: " PRI_DNS_NAME_SRP,
                  DNS_NAME_PARAM_SRP(rr->name, name_buf));
            return dns_rcode_formerr;
        }
        dpp = &dp->next;
    }
    dp = calloc(1, sizeof *dp);
    if (!dp) {
        ERROR("no memory.");
        return dns_rcode_servfail;
    }
    // Add to the deletes list
    *dpp = dp;

    // Make sure the name is a subdomain of the zone being updated.
    dp->zone = dns_name_subdomain_of(rr->name, update_zone);
    if (dp->zone == NULL) {
        DNS_NAME_GEN_SRP(update_zone, update_zone_buf);
        DNS_NAME_GEN_SRP(rr->name, name_buf);
        ERROR("delete for record not in update zone " PRI_DNS_NAME_SRP ": " PRI_DNS_NAME_SRP,
              DNS_NAME_PARAM_SRP(update_zone, update_zone_buf), DNS_NAME_PARAM_SRP(rr->name, name_buf));
        status = dns_rcode_formerr;
        goto out;
    }
    dp->name = rr->name;
    if (delete_out != NULL) {
        *delete_out = dp;
    }
out:
    if (status != dns_rcode_noerror) {
        free(dp);
    }
    return status;
}

// Find a delete in the delete list that has target as its target.
static delete_t *
srp_find_delete(delete_t *deletes, dns_rr_t *target)
{
    for (delete_t *dp = deletes; dp; dp = dp->next) {
        if (dns_names_equal(dp->name, target->name)) {
            return dp;
        }
    }
    return NULL;
}

static bool
srp_parse_lease_times(dns_message_t *message, uint32_t *r_lease, uint32_t *r_key_lease)
{
    // Get the lease time. We need this to differentiate between a mass host deletion and an add.
    uint32_t lease_time = 3600;
    uint32_t key_lease_time = 604800;
    bool found_lease = false;
    for (dns_edns0_t *edns0 = message->edns0; edns0; edns0 = edns0->next) {
        if (edns0->type == dns_opt_update_lease) {
            unsigned off = 0;
            if (edns0->length != 4 && edns0->length != 8) {
                ERROR("edns0 update-lease option length bogus: %d", edns0->length);
                return false;
            }
            dns_u32_parse(edns0->data, edns0->length, &off, &lease_time);
            if (edns0->length == 8) {
                dns_u32_parse(edns0->data, edns0->length, &off, &key_lease_time);
            } else {
                key_lease_time = 7 * lease_time;
            }
            found_lease = true;
        }
    }

    // Update-lease option is required for SRP.
    if (!found_lease) {
        ERROR("no update-lease edns0 option found in supposed SRP update");
        return false;
    }
    *r_lease = lease_time;
    *r_key_lease = key_lease_time;
    return true;
}

client_update_t *
srp_evaluate(const char *remote_name, dns_message_t **in_parsed_message, message_t *raw_message, int index)
{
    unsigned i;
    dns_host_description_t *host_description = NULL;
    delete_t *deletes = NULL, *dp, **dpp = NULL, **rpp = NULL, *removes = NULL;
    service_instance_t *service_instances = NULL, *sip, **sipp = &service_instances;
    service_t *services = NULL, *sp, **spp = &services;
    dns_rr_t *signature;
    struct timeval now;
    dns_name_t *update_zone = NULL, *replacement_zone = NULL;
    dns_name_t *uzp;
    dns_rr_t *key = NULL;
    dns_rr_t **keys = NULL;
    unsigned num_keys = 0;
    unsigned max_keys = 1;
    bool found_key = false;
#if SRP_PARSE_DEBUG_VERBOSE
    char namebuf2[DNS_MAX_NAME_SIZE];
#endif
    dns_message_t *message;


    client_update_t *ret = calloc(1, sizeof(*ret));
    if (ret == NULL) {
        ERROR("no memory for client update");
        return NULL;
    }

    ret->drop = false;
    ret->rcode = dns_rcode_servfail;

    if (in_parsed_message != NULL) {
        ret->parsed_message = *in_parsed_message;
        *in_parsed_message = NULL;
    } else {
        // Parse the UPDATE message.
        if (!dns_wire_parse(&ret->parsed_message, &raw_message->wire, raw_message->length, false)) {
            ERROR("dns_wire_parse failed.");
            goto out;
        }
    }
    ret->message = raw_message;
    RETAIN_HERE(ret->message, message);
    message = ret->parsed_message; // For brevity

    if (!srp_parse_lease_times(ret->parsed_message, &ret->host_lease, &ret->key_lease)) {
        ret->rcode = dns_rcode_formerr;
        goto out;
    }

    // Update requires a single SOA record as the question
    if (message->qdcount != 1) {
        ERROR("update received with qdcount > 1");
        ret->rcode = dns_rcode_formerr;
        return ret;
    }

    // Update should contain zero answers.
    if (message->ancount != 0) {
        ERROR("update received with ancount > 0");
        ret->rcode = dns_rcode_formerr;
        return ret;
    }

    if (message->questions[0].type != dns_rrtype_soa) {
        ERROR("update received with rrtype %d instead of SOA in question section.",
              message->questions[0].type);
        ret->rcode = dns_rcode_formerr;
        return ret;
    }

    if (remote_name == NULL) {
        raw_message->received_time = srp_time();
    }

    update_zone = message->questions[0].name;
    if (service_update_zone != NULL && dns_names_equal_text(update_zone, "default.service.arpa.")) {
#if SRP_PARSE_DEBUG_VERBOSE
        INFO(PRI_S_SRP " is in default.service.arpa, using replacement zone: " PUB_S_SRP,
             dns_name_print(update_zone, namebuf2, sizeof(namebuf2)),
             dns_name_print(service_update_zone, namebuf1, sizeof(namebuf1)));
#endif
        replacement_zone = service_update_zone;
    } else {
#if SRP_PARSE_DEBUG_VERBOSE
        INFO(PRI_S_SRP " is not in default.service.arpa, or no replacement zone (%p)",
             dns_name_print(update_zone, namebuf2, sizeof(namebuf2)), service_update_zone);
#endif
        replacement_zone = NULL;
    }

    // Scan over the authority RRs; do the delete consistency check.  We can't do other consistency checks
    // because we can't assume a particular order to the records other than that deletes have to come before
    // adds.
    for (i = 0; i < message->nscount; i++) {
        dns_rr_t *rr = &message->authority[i];

        // If this is a delete for all the RRs on a name, record it in the list of deletes.
        if (rr->type == dns_rrtype_any && rr->qclass == dns_qclass_any && rr->ttl == 0) {
            int status = make_delete(&deletes, NULL, rr, update_zone);
            if (status != dns_rcode_noerror) {
                ret->rcode = status;
                goto out;
            }
        }

        // The update should really only contain one key, but it's allowed for keys to appear on
        // service instance names as well, since that's what will be stored in the zone.   So if
        // we get one key, we'll assume it's a host key until we're done scanning, and then check.
        // If we get more than one, we allocate a buffer and store all the keys so that we can
        // check them all later.
        else if (rr->type == dns_rrtype_key) {
            if (num_keys < 1) {
                key = rr;
                num_keys++;
            } else {
                if (num_keys == 1) {
                    // We can't have more keys than there are authority records left, plus
                    // one for the key we already have, so allocate a buffer that large.
                    max_keys = message->nscount - i + 1;
                    keys = calloc(max_keys, sizeof *keys);
                    if (keys == NULL) {
                        ERROR("no memory");
                        goto out;
                    }
                    keys[0] = key;
                }
                if (num_keys >= max_keys) {
                    ERROR("coding error in key allocation");
                    goto out;
                }
                keys[num_keys++] = rr;
            }
        }

        // Otherwise if it's an A or AAAA record, it's part of a hostname entry.
        else if (rr->type == dns_rrtype_a || rr->type == dns_rrtype_aaaa) {
            // Allocate the hostname record
            if (!host_description) {
                host_description = calloc(1, sizeof *host_description);
                if (!host_description) {
                    ERROR("no memory");
                    goto out;
                }
            }

            // Make sure it's preceded by a deletion of all the RRs on the name.
            if (!host_description->delete) {
                dp = srp_find_delete(deletes, rr);
                if (dp == NULL) {
                    DNS_NAME_GEN_SRP(rr->name, name_buf);
                    ERROR("ADD for hostname " PRI_DNS_NAME_SRP " without a preceding delete.",
                          DNS_NAME_PARAM_SRP(rr->name, name_buf));
                    ret->rcode = dns_rcode_formerr;
                    goto out;
                }
                host_description->delete = dp;
                host_description->name = dp->name;
                dp->consumed = true; // This delete is accounted for.

                // In principle, we should be checking this name to see that it's a subdomain of the update
                // zone.  However, it turns out we don't need to, because the /delete/ has to be a subdomain
                // of the update zone, and we won't find that delete if it's not present.
            }

            if (rr->type == dns_rrtype_a || rr->type == dns_rrtype_aaaa) {
                if (!add_host_addr(&host_description->addrs, rr)) {
                    goto out;
                }
            }
        }

        // Otherwise if it's an SRV entry, that should be a service instance name.
        else if (rr->type == dns_rrtype_srv || rr->type == dns_rrtype_txt) {
            // Should be a delete that precedes this service instance.
            dp = srp_find_delete(deletes, rr);
            if (dp == NULL) {
                DNS_NAME_GEN_SRP(rr->name, name_buf);
                ERROR("ADD for service instance not preceded by delete: " PRI_DNS_NAME_SRP,
                      DNS_NAME_PARAM_SRP(rr->name, name_buf));
                ret->rcode = dns_rcode_formerr;
                goto out;
            }
            for (sip = service_instances; sip; sip = sip->next) {
                if (dns_names_equal(sip->name, rr->name)) {
                    break;
                }
            }
            if (!sip) {
                sip = calloc(1, sizeof *sip);
                if (sip == NULL) {
                    ERROR("no memory");
                    goto out;
                }
                sip->delete = dp;
                dp->consumed = true;
                sip->name = dp->name;
                // Add to the service instances list
                *sipp = sip;
                sipp = &sip->next;
            }
            if (rr->type == dns_rrtype_srv) {
                if (sip->srv != NULL) {
                    DNS_NAME_GEN_SRP(rr->name, name_buf);
                    ERROR("more than one SRV rr received for service instance: " PRI_DNS_NAME_SRP,
                          DNS_NAME_PARAM_SRP(rr->name, name_buf));
                    ret->rcode = dns_rcode_formerr;
                    goto out;
                }
                sip->srv = rr;
            } else if (rr->type == dns_rrtype_txt) {
                if (sip->txt != NULL) {
                    DNS_NAME_GEN_SRP(rr->name, name_buf);
                    ERROR("more than one TXT rr received for service instance: " PRI_DNS_NAME_SRP,
                          DNS_NAME_PARAM_SRP(rr->name, name_buf));
                    ret->rcode = dns_rcode_formerr;
                    goto out;
                }
                sip->txt = rr;
            }
        }

        // Otherwise if it's a PTR entry, that should be a service name
        else if (rr->type == dns_rrtype_ptr) {
            service_t *base_type = NULL;

            // See if the service is a subtype. If it is, it should be preceded in the list of RRs in
            // the update by a PTR record for the base service type. E.g., if there is a PTR for
            // _foo._sub._ipps._tcp.default.service.arpa, there should, earlier in the SRP update,
            // be a PTR for _ipps._tcp.default.service.arpa. Both the base type and the subtype PTR
            // records must have the same target.
            if (rr->name != NULL &&
                rr->name->next != NULL && rr->name->next->next != NULL && !strcmp(rr->name->next->data, "_sub"))
            {
                dns_name_t *base_type_name = rr->name->next->next;
                for (base_type = services; base_type != NULL; base_type = base_type->next) {
                    if (!dns_names_equal(base_type->rr->data.ptr.name, rr->data.ptr.name)) {
                        continue;
                    }
                    if (dns_names_equal(base_type->rr->name, base_type_name)) {
                        break;
                    }
                }
                if (base_type == NULL) {
                    DNS_NAME_GEN_SRP(rr->name, name_buf);
                    DNS_NAME_GEN_SRP(rr->data.ptr.name, target_name_buf);
                    ERROR("service subtype " PRI_DNS_NAME_SRP " for " PRI_DNS_NAME_SRP
                          " has no preceding base type ", DNS_NAME_PARAM_SRP(rr->name, name_buf),
                          DNS_NAME_PARAM_SRP(rr->data.ptr.name, target_name_buf));
                    ret->rcode = dns_rcode_formerr;
                    goto out;
                }
            }

            // If qclass is none and ttl is zero, this is a delete specific RR from RRset, not an add RR to RRset.
            if (rr->qclass == dns_qclass_none && rr->ttl == 0) {
                int status = make_delete(&deletes, &dp, rr, update_zone);
                if (status != dns_rcode_noerror) {
                    ret->rcode = status;
                    goto out;
                }
            } else {
                sp = calloc(1, sizeof *sp);
                if (sp == NULL) {
                    ERROR("no memory");
                    goto out;
                }

                // Add to the services list
                *spp = sp;
                spp = &sp->next;
                sp->rr = rr;
                if (base_type != NULL) {
                    sp->base_type = base_type;
                } else {
                    sp->base_type = sp;
                }

                // Make sure the service name is in the update zone.
                sp->zone = dns_name_subdomain_of(sp->rr->name, update_zone);
                if (sp->zone == NULL) {
                    DNS_NAME_GEN_SRP(rr->name, name_buf);
                    DNS_NAME_GEN_SRP(rr->data.ptr.name, data_name_buf);
                    ERROR("service name " PRI_DNS_NAME_SRP " for " PRI_DNS_NAME_SRP
                          " is not in the update zone", DNS_NAME_PARAM_SRP(rr->name, name_buf),
                          DNS_NAME_PARAM_SRP(rr->data.ptr.name, data_name_buf));
                    ret->rcode = dns_rcode_formerr;
                    goto out;
                }
            }
        }

        // Otherwise it's not a valid update
        else {
            DNS_NAME_GEN_SRP(rr->name, name_buf);
            ERROR("unexpected rrtype %d on " PRI_DNS_NAME_SRP " in update.", rr->type,
                  DNS_NAME_PARAM_SRP(rr->name, name_buf));
            ret->rcode = dns_rcode_formerr;
            goto out;
        }
    }

    // Now that we've scanned the whole update, do the consistency checks for updates that might
    // not have come in order.

    // If we don't yet have a host description, but this is a delete of the entire host registration (host_lease == 0) and
    // we do have a delete record and a key record for the host, create a host description with no addresses here.
    if (host_description == NULL && ret->host_lease == 0) {
        // If we get here and we have a key, then that suggests that this SRP update is a host remove with a KEY RR to
        // authenticate it (and possibly leave behind).
        if (key != NULL) {
            dp = srp_find_delete(deletes, key);
            if (dp != NULL) {
                host_description = calloc(1, sizeof *host_description);
                if (host_description == NULL) {
                    ERROR("no memory");
                    goto out;
                }
                host_description->delete = dp;
                host_description->name = dp->name;
                dp->consumed = true; // This delete is accounted for.
            }
        }
    }
    // Make sure there's a host description.
    if (!host_description) {
        ERROR("SRP update does not include a host description.");
        ret->rcode = dns_rcode_formerr;
        goto out;
    }

    // Make sure that each service add references a service instance that's in the same update.
    for (sp = services; sp; sp = sp->next) {
        // A service instance can never point to a service subtype--it has to point to the base type.
        if (sp->base_type != sp) {
            continue;
        }
        for (sip = service_instances; sip; sip = sip->next) {
            if (dns_names_equal(sip->name, sp->rr->data.ptr.name)) {
                // Note that we have already verified that there is only one service instance
                // with this name, so this could only ever happen once in this loop even without
                // the break statement.
                sip->service = sp;
                sip->num_instances++;
                break;
            }
        }
        // If this service doesn't point to a service instance that's in the update, then the
        // update fails validation.
        if (sip == NULL) {
            DNS_NAME_GEN_SRP(sp->rr->name, name_buf);
            ERROR("service points to an instance that's not included: " PRI_DNS_NAME_SRP,
                  DNS_NAME_PARAM_SRP(sp->rr->name, name_buf));
            ret->rcode = dns_rcode_formerr;
            goto out;
        }
    }

    for (sip = service_instances; sip; sip = sip->next) {
        // For each service instance, make sure that at least one service references it
        if (sip->num_instances == 0) {
            DNS_NAME_GEN_SRP(sip->name, name_buf);
            ERROR("service instance update for " PRI_DNS_NAME_SRP
                  " is not referenced by a service update.", DNS_NAME_PARAM_SRP(sip->name, name_buf));
            ret->rcode = dns_rcode_formerr;
            goto out;
        }

        // For each service instance, make sure that it references the host description
        if (dns_names_equal(host_description->name, sip->srv->data.srv.name)) {
            sip->host = host_description;
            host_description->num_instances++;
        }
    }

    // Make sure that at least one service instance references the host description, unless the update is deleting the host address records.
#ifdef REJECT_HOST_WITHOUT_SERVICES
    if (host_description->num_instances == 0 && host_description->addrs != NULL) {
        DNS_NAME_GEN_SRP(host_description->name, name_buf);
        ERROR("host description " PRI_DNS_NAME_SRP " is not referenced by any service instances.",
              DNS_NAME_PARAM_SRP(host_description->name, name_buf));
        ret->rcode = dns_rcode_formerr;
        goto out;
    }

    // Make sure the host description has at least one address record, unless we're deleting the host.
    if (host_description->addrs == NULL && host_description->num_instances != 0 && ret->host_lease != 0) {
        DNS_NAME_GEN_SRP(host_description->name, name_buf);
        ERROR("host description " PRI_DNS_NAME_SRP " doesn't contain any IP addresses, but services are being added.",
              DNS_NAME_PARAM_SRP(host_description->name, name_buf));
        ret->rcode = dns_rcode_formerr;
        goto out;
    }
#endif

    for (i = 0; i < num_keys; i++) {
        // If this isn't the only key, make sure it's got the same contents as the other keys.
        if (i > 0) {
            if (!dns_keys_rdata_equal(key, keys[i])) {
                ERROR("more than one key presented");
                ret->rcode = dns_rcode_formerr;
                goto out;
            }
            // This is a hack so that if num_keys == 1, we don't have to allocate keys[].
            // At the bottom of this if statement, key is always the key we are looking at.
            key = keys[i];
        }
        // If there is a key, and the host description doesn't currently have a key, check
        // there first since that's the default.
        if (host_description->key == NULL && dns_names_equal(key->name, host_description->name)) {
            host_description->key = key;
            found_key = true;
        } else {
            for (sip = service_instances; sip != NULL; sip = sip->next) {
                if (dns_names_equal(sip->name, key->name)) {
                    found_key = true;
                    break;
                }
            }
        }
        if (!found_key) {
            DNS_NAME_GEN_SRP(key->name, key_name_buf);
            ERROR("key present for name " PRI_DNS_NAME_SRP
                  " which is neither a host nor an instance name.", DNS_NAME_PARAM_SRP(key->name, key_name_buf));
            ret->rcode = dns_rcode_formerr;
            goto out;
        }
    }
    if (keys != NULL) {
        free(keys);
        keys = NULL;
    }

    // And make sure it has a key record
    if (host_description->key == NULL) {
        DNS_NAME_GEN_SRP(host_description->name, host_name_buf);
        ERROR("host description " PRI_DNS_NAME_SRP " doesn't contain a key.",
              DNS_NAME_PARAM_SRP(host_description->name, host_name_buf));
        ret->rcode = dns_rcode_formerr;
        goto out;
    }

    // Find any deletes that weren't consumed. These will be presumed to be removes of service instances previously
    // registered. These can't be validated here--we have to actually go look at the database.
    dpp = &deletes;
    rpp = &removes;
    while (*dpp) {
        dp = *dpp;
        if (!dp->consumed) {
            DNS_NAME_GEN_SRP(dp->name, delete_name_buf);
            INFO("delete for presumably previously-registered instance which is being withdrawn: " PRI_DNS_NAME_SRP,
                  DNS_NAME_PARAM_SRP(dp->name, delete_name_buf));
            *rpp = dp;
            rpp = &dp->next;
            *dpp = dp->next;
            dp->next = NULL;
        } else {
            dpp = &dp->next;
        }
    }

    // The signature should be the last thing in the additional section.   Even if the signature
    // is valid, if it's not at the end we reject it.   Note that we are just checking for SIG(0)
    // so if we don't find what we're looking for, we forward it to the DNS auth server which
    // will either accept or reject it.
    if (message->arcount < 1) {
        ERROR("signature not present");
        ret->rcode = dns_rcode_formerr;
        goto out;
    }
    signature = &message->additional[message->arcount -1];
    if (signature->type != dns_rrtype_sig) {
        ERROR("signature is not at the end or is not present");
        ret->rcode = dns_rcode_formerr;
        goto out;
    }

    // Make sure that the signer name is the hostname.   If it's not, it could be a legitimate
    // update with a different key, but it's not an SRP update, so we pass it on.
    if (!dns_names_equal(signature->data.sig.signer, host_description->name)) {
        DNS_NAME_GEN_SRP(signature->data.sig.signer, signer_name_buf);
        DNS_NAME_GEN_SRP(host_description->name, host_name_buf);
        ERROR("signer " PRI_DNS_NAME_SRP " doesn't match host " PRI_DNS_NAME_SRP,
              DNS_NAME_PARAM_SRP(signature->data.sig.signer, signer_name_buf),
              DNS_NAME_PARAM_SRP(host_description->name, host_name_buf));
        ret->rcode = dns_rcode_formerr;
        goto out;
    }

    // Make sure we're in the time limit for the signature.   Zeroes for the inception and expiry times
    // mean the host that send this doesn't have a working clock.   One being zero and the other not isn't
    // valid unless it's 1970.
    if (signature->data.sig.inception != 0 || signature->data.sig.expiry != 0) {
        gettimeofday(&now, NULL);
        if (raw_message->received_time != 0) {
            // The received time is in srp_time, but the signature time will be in wall clock time, so
            // convert from srpl_time to wall clock time.
            now.tv_sec = raw_message->received_time - srp_time() + now.tv_sec;
            now.tv_usec = 0;
        }

        // The sender does the bracketing, so we can just do a simple comparison.
        if ((uint32_t)(now.tv_sec & UINT32_MAX) > signature->data.sig.expiry ||
            (uint32_t)(now.tv_sec & UINT32_MAX) < signature->data.sig.inception) {
            ERROR("signature is not timely: %lu < %lu < %lu does not hold",
                  (unsigned long)signature->data.sig.inception, (unsigned long)now.tv_sec,
                  (unsigned long)signature->data.sig.expiry);
            goto badsig;
        }
    }

    // Now that we have the key, we can validate the signature.   If the signature doesn't validate,
    // there is no need to pass the message on.
    if (!srp_sig0_verify(&raw_message->wire, host_description->key, signature)) {
        ERROR("signature is not valid");
        goto badsig;
    }

    // Now that we have validated the SRP message, go through and fix up all instances of
    // *default.service.arpa to use the replacement zone, if this update is for
    // default.services.arpa and there is a replacement zone.
    if (replacement_zone != NULL) {
        // All of the service instances and the host use the name from the delete, so if
        // we update these, the names for those are taken care of.   We already found the
        // zone for which the delete is a subdomain, so we can just replace it without
        // finding it again.
        for (dp = deletes; dp; dp = dp->next) {
            replace_zone_name(&dp->name, dp->zone, replacement_zone);
        }

        // We also need to update the names of removes.
        for (dp = removes; dp; dp = dp->next) {
            replace_zone_name(&dp->name, dp->zone, replacement_zone);
        }

        // All services have PTR records, which point to names.   Both the service name and the
        // PTR name have to be fixed up.
        for (sp = services; sp; sp = sp->next) {
            replace_zone_name(&sp->rr->name, sp->zone, replacement_zone);
            uzp = dns_name_subdomain_of(sp->rr->data.ptr.name, update_zone);
            // We already validated that the PTR record points to something in the zone, so this
            // if condition should always be false.
            if (uzp == NULL) {
                ERROR("service PTR record zone match fail!!");
                ret->rcode = dns_rcode_formerr;
                goto out;
            }
            replace_zone_name(&sp->rr->data.ptr.name, uzp, replacement_zone);
        }

        // All service instances have SRV records, which point to names.  The service instance
        // name is already fixed up, because it's the same as the delete, but the name in the
        // SRV record must also be fixed.
        for (sip = service_instances; sip; sip = sip->next) {
            uzp = dns_name_subdomain_of(sip->srv->data.srv.name, update_zone);
            // We already validated that the SRV record points to something in the zone, so this
            // if condition should always be false.
            if (uzp == NULL) {
                ERROR("service instance SRV record zone match fail!!");
                ret->rcode = dns_rcode_formerr;
                goto out;
            }
            replace_zone_name(&sip->srv->data.srv.name, uzp, replacement_zone);
        }

        // We shouldn't need to replace the hostname zone because it's actually pointing to
        // the name of a delete.
    }

    // Start the update.
    DNS_NAME_GEN_SRP(host_description->name, host_description_name_buf);
    char time_buf[28];
    if (raw_message->received_time == 0) {
        static char msg[] = "not set";
        memcpy(time_buf, msg, sizeof(msg));
    } else {
        srp_format_time_offset(time_buf, sizeof(time_buf), srp_time() - raw_message->received_time);
    }

    INFO("update for " PRI_DNS_NAME_SRP " #%d, xid %x validates, key lease %d, host lease %d, message lease %d, receive_time "
         PUB_S_SRP ", remote " PRI_S_SRP " -> %p.",
         DNS_NAME_PARAM_SRP(host_description->name, host_description_name_buf), index, raw_message->wire.id,
         ret->key_lease, ret->host_lease, raw_message->lease, time_buf, remote_name == NULL ? "(none)" : remote_name, ret);
    ret->rcode = dns_rcode_noerror;
    goto out;

badsig:
    ret->drop = true;

out:
    // No matter how we get out of this, we free the delete structures that weren't dangling removes,
    // because they are not used to do the update.
    for (dp = deletes; dp; ) {
        delete_t *next = dp->next;
        free(dp);
        dp = next;
    }

    // We always return what we got, even if we failed
    ret->host = host_description;
    ret->instances = service_instances;
    ret->services = services;
    ret->removes = removes;
    ret->update_zone = replacement_zone == NULL ? update_zone : replacement_zone;
    return ret;
}

bool
srp_dns_evaluate(comm_t *connection, srp_server_t *server_state, message_t *message, dns_message_t **p_parsed_message)
{
    bool continuing = false;
    client_update_t *update = NULL;

    // Drop incoming responses--we're a server, so we only accept queries.
    if (dns_qr_get(&message->wire) == dns_qr_response) {
        ERROR("received a message that was a DNS response: %d", dns_opcode_get(&message->wire));
        goto out;
    }

    // Forward incoming messages that are queries to the dnssd-proxy code.
    if (dns_opcode_get(&message->wire) == dns_opcode_query)
    {
        dns_proxy_input_for_server(connection, server_state, message, NULL);
        goto out;
    }

    if (dns_opcode_get(&message->wire) != dns_opcode_update) {
        // dns_forward(connection)
        send_fail_response(connection, message, dns_rcode_refused);
        ERROR("received a message that was not a DNS update: %d", dns_opcode_get(&message->wire));
        goto out;
    }

    // We need the wire message to validate the signature...
    update = srp_evaluate(NULL, p_parsed_message, message, 0);
    if (update == NULL) {
        send_fail_response(connection, message, dns_rcode_servfail);
        goto out;
    }
    if (update->rcode != dns_rcode_noerror) {
        if (!update->drop) {
            send_fail_response(connection, message, update->rcode);
        }
        goto out;
    }

    update->connection = connection;
    ioloop_comm_retain(update->connection);
    update->server_state = server_state;

    continuing = srp_update_start(update);
    goto good;
out:
    srp_parse_client_updates_free(update);
good:
    return continuing;
}

#if SRP_FEATURE_REPLICATION
static bool
srp_parse_eliminate_shadowed_updates(srp_server_t *server_state, client_update_t *new_message, client_update_t *old_message)
{
    (void)server_state;

    // We only ever want the last host update.
    old_message->skip_host_updates = true;

    // Look for matching instances.
    for (service_instance_t *old = old_message->instances; old != NULL; old = old->next) {
        for (service_instance_t *new = new_message->instances; new != NULL; new = new->next) {
            if (dns_names_equal(old->name, new->name)) {
                old->skip_update = true;
            }
        }
        for (delete_t *delete = new_message->removes; delete != NULL; delete = delete->next) {
            DNS_NAME_GEN_SRP(old->name, old_name_buf);
            DNS_NAME_GEN_SRP(delete->name, delete_name_buf);
            INFO("old service " PRI_DNS_NAME_SRP ", delete " PRI_DNS_NAME_SRP,
                 DNS_NAME_PARAM_SRP(old->name, old_name_buf), DNS_NAME_PARAM_SRP(delete->name, delete_name_buf));
            if (dns_names_equal(old->name, delete->name)) {
                old->skip_update = true;
            }
        }
    }
    return true;
}

// For SRP replication
bool
srp_parse_host_messages_evaluate(srp_server_t *UNUSED server_state, srpl_connection_t *srpl_connection,
                                 message_t **messages, int num_messages)
{
    client_update_t *client_updates = NULL;
    bool ret = false;

    for (int i = 0; i < num_messages; i++) {
        message_t *message = messages[i];

        // Drop incoming responses--we're a server, so we only accept queries.
        if (dns_qr_get(&message->wire) == dns_qr_response) {
            ERROR("received a message that was a DNS response: %d", dns_opcode_get(&message->wire));
            goto out;
        }

        // Forward incoming messages that are queries but not updates.
        // XXX do this later--for now we operate only as a translator, not a proxy.
        if (dns_opcode_get(&message->wire) != dns_opcode_update) {
            ERROR("received a message that was not a DNS update: %d", dns_opcode_get(&message->wire));
            goto out;
        }

        // We need the wire message to validate the signature...
        INFO("evaluating message #%d from %s", i, srpl_connection->name);
        client_update_t *update = srp_evaluate(srpl_connection->name, NULL, message, i);
        if (update == NULL) {
            goto out;
        }
        if (update->rcode != dns_rcode_noerror) {
            update->next = client_updates;
            client_updates = update;
            goto out;
        }
        update->srpl_connection = srpl_connection;
        srpl_connection_retain(update->srpl_connection);
        update->server_state = server_state;
        update->index = i;

        // We build the list of messages so that message 0 winds up at the /end/ of the list; message 0 is
        // the earliest message.
        update->next = client_updates;
        client_updates = update;
    }

    // Now that we've parsed and validated everything, eliminate earlier updates that are in the shadow of
    // later updates.
    // The list off of client_updates is ordered with most recent messages first, so what we want to do
    // is, for each message on the list, see if any earlier messages update the same instance. If so, remove
    // the earlier update, since it is out of date and could create a conflict if we tried to apply it.
    // If we get an update that deletes the host, every update earlier than that is invalidated. It would
    // be weird for us to get an update that is earlier than a full delete, but we could well get a full
    // delete as the earliest recorded update.
    for (client_update_t *em = client_updates; em != NULL; em = em->next) {
        for (client_update_t *lem = em->next; lem != NULL; lem = lem->next) {
            srp_parse_eliminate_shadowed_updates(server_state, em, lem);
        }
    }

    // Now re-order the list oldest to newest.
    client_update_t *current = client_updates, *next = NULL, *prev = NULL;
    while (current != NULL) {
        next = current->next;
        current->next = prev;
        prev = current;
        current = next;
    }
    client_updates = prev;

    // Now that we've eliminated shadowed updates, we can actually call srp_update_start.
    ret = srp_update_start(client_updates);
    goto good;
out:
    srp_parse_client_updates_free(client_updates);
good:
    return ret;
}
#endif

void
dns_input(comm_t *comm, message_t *message, void *context)
{
    srp_server_t *server_state = context;
    srp_dns_evaluate(comm, server_state, message, NULL);
}

struct srp_proxy_listener_state {
    comm_t *NULLABLE tcp_listener;
    comm_t *NULLABLE tls_listener;
    comm_t *NULLABLE udp_listener;
};

comm_t *
srp_proxy_listen(uint16_t *avoid_ports, int num_avoid_ports, const char *interface_name, ready_callback_t ready,
                 cancel_callback_t cancel_callback, addr_t *address, finalize_callback_t context_release_callback,
                 void *context)
{
    unsigned ifindex = 0;
    if (interface_name != NULL) {
        ifindex = if_nametoindex(interface_name);
        if (ifindex == 0) {
            return false;
        }
    }

    // XXX UDP listeners should bind to interface addresses, not INADDR_ANY.
    return ioloop_listener_create(false, false, false, avoid_ports, num_avoid_ports, address, NULL, "SRP UDP listener",
                                  dns_input, NULL, cancel_callback, ready, context_release_callback, NULL, ifindex, context);
}

void
srp_proxy_init(const char *update_zone)
{
    // For now, hardcoded, should be configurable
    if (service_update_zone != NULL) {
        dns_name_free(service_update_zone);
    }
    service_update_zone = dns_pres_name_parse(update_zone);

}

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 108
// indent-tabs-mode: nil
// End:
