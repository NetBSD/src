/* icmp.c
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
 * This code implements ICMP I/O functions for the Thread border router.
 */

#ifndef LINUX
#include <netinet/in.h>
#include <net/if.h>
#include <netinet6/in6_var.h>
#include <netinet6/nd6.h>
#include <net/if_media.h>
#include <sys/stat.h>
#else
#define _GNU_SOURCE
#include <netinet/in.h>
#include <fcntl.h>
#include <bsd/stdlib.h>
#include <net/if.h>
#endif
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/route.h>
#include <netinet/icmp6.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <arpa/inet.h>
#if !USE_SYSCTL_COMMAND_TO_ENABLE_FORWARDING
#ifndef LINUX
#include <sys/sysctl.h>
#endif // LINUX
#endif // !USE_SYSCTL_COMMAND_TO_ENABLE_FORWARDING
#include <stdlib.h>
#include <stddef.h>
#include <dns_sd.h>
#include <inttypes.h>
#include <signal.h>

#ifdef IOLOOP_MACOS
#include <xpc/xpc.h>

#include <TargetConditionals.h>
#include <SystemConfiguration/SystemConfiguration.h>
#include <SystemConfiguration/SCPrivate.h>
#include <SystemConfiguration/SCNetworkConfigurationPrivate.h>
#include <SystemConfiguration/SCNetworkSignature.h>
#include <network_information.h>

#include <CoreUtils/CoreUtils.h>
#endif // IOLOOP_MACOS

#include "srp.h"
#include "dns-msg.h"
#include "ioloop.h"
#include "cti-services.h"
#include "srp-mdns-proxy.h"
#include "route.h"
#include "icmp.h"
#include "state-machine.h"
#include "thread-service.h"
#include "omr-watcher.h"


icmp_listener_t icmp_listener;

void
icmp_message_free(icmp_message_t *message)
{
    if (message->options != NULL) {
        free(message->options);
    }
    if (message->wakeup != NULL) {
        ioloop_cancel_wake_event(message->wakeup);
        ioloop_wakeup_release(message->wakeup);
    }
    free(message);
}

void
icmp_message_dump(icmp_message_t *message,
                  struct in6_addr *source_address, struct in6_addr *destination_address)
{
    link_layer_address_t *lladdr;
    prefix_information_t *prefix_info;
    route_information_t *route_info;
    uint8_t *flags;
    int i;
    char retransmission_timer_buf[11]; // Maximum size of a uint32_t printed as decimal.
    char *retransmission_timer = "infinite";

    if (message->retransmission_timer != ND6_INFINITE_LIFETIME) {
        snprintf(retransmission_timer_buf, sizeof(retransmission_timer_buf), "%" PRIu32, message->retransmission_timer);
        retransmission_timer = retransmission_timer_buf;
    }

    SEGMENTED_IPv6_ADDR_GEN_SRP(source_address->s6_addr, src_addr_buf);
    SEGMENTED_IPv6_ADDR_GEN_SRP(destination_address->s6_addr, dst_addr_buf);
    if (message->type == icmp_type_router_advertisement) {
        INFO("router advertisement from " PRI_SEGMENTED_IPv6_ADDR_SRP " to " PRI_SEGMENTED_IPv6_ADDR_SRP
             " hop_limit %d on " PUB_S_SRP ": checksum = %x "
             "cur_hop_limit = %d flags = %x router_lifetime = %d reachable_time = %" PRIu32
             " retransmission_timer = " PUB_S_SRP,
             SEGMENTED_IPv6_ADDR_PARAM_SRP(source_address->s6_addr, src_addr_buf),
             SEGMENTED_IPv6_ADDR_PARAM_SRP(destination_address->s6_addr, dst_addr_buf),
             message->hop_limit, message->interface->name, message->checksum, message->cur_hop_limit, message->flags,
             message->router_lifetime, message->reachable_time, retransmission_timer);
    } else if (message->type == icmp_type_router_solicitation) {
        INFO("router solicitation from " PRI_SEGMENTED_IPv6_ADDR_SRP " to " PRI_SEGMENTED_IPv6_ADDR_SRP
             " hop_limit %d on " PUB_S_SRP ": code = %d checksum = %x",
             SEGMENTED_IPv6_ADDR_PARAM_SRP(source_address->s6_addr, src_addr_buf),
             SEGMENTED_IPv6_ADDR_PARAM_SRP(destination_address->s6_addr, dst_addr_buf),
             message->hop_limit, message->interface->name,
             message->code, message->checksum);
    } else {
        INFO("icmp message from " PRI_SEGMENTED_IPv6_ADDR_SRP " to " PRI_SEGMENTED_IPv6_ADDR_SRP " hop_limit %d on "
             PUB_S_SRP ": type = %d code = %d checksum = %x",
             SEGMENTED_IPv6_ADDR_PARAM_SRP(source_address->s6_addr, src_addr_buf),
             SEGMENTED_IPv6_ADDR_PARAM_SRP(destination_address->s6_addr, dst_addr_buf),
             message->hop_limit, message->interface->name, message->type,
             message->code, message->checksum);
    }

    for (i = 0; i < message->num_options; i++) {
        icmp_option_t *option = &message->options[i];
        switch(option->type) {
        case icmp_option_source_link_layer_address:
            lladdr = &option->option.link_layer_address;
            INFO("  source link layer address " PRI_MAC_ADDR_SRP, MAC_ADDR_PARAM_SRP(lladdr->address));
            break;
        case icmp_option_target_link_layer_address:
            lladdr = &option->option.link_layer_address;
            INFO("  destination link layer address " PRI_MAC_ADDR_SRP, MAC_ADDR_PARAM_SRP(lladdr->address));
            break;
        case icmp_option_prefix_information:
            prefix_info = &option->option.prefix_information;
            SEGMENTED_IPv6_ADDR_GEN_SRP(prefix_info->prefix.s6_addr, prefix_buf);
            INFO("  prefix info: " PRI_SEGMENTED_IPv6_ADDR_SRP "/%d %x %" PRIu32 " %" PRIu32,
                 SEGMENTED_IPv6_ADDR_PARAM_SRP(prefix_info->prefix.s6_addr, prefix_buf), prefix_info->length,
                 prefix_info->flags, prefix_info->valid_lifetime, prefix_info->preferred_lifetime);
            break;
        case icmp_option_route_information:
            route_info = &option->option.route_information;
                SEGMENTED_IPv6_ADDR_GEN_SRP(route_info->prefix.s6_addr, router_prefix_buf);
            INFO("  route info: " PRI_SEGMENTED_IPv6_ADDR_SRP "/%d %x %d",
                 SEGMENTED_IPv6_ADDR_PARAM_SRP(route_info->prefix.s6_addr, router_prefix_buf), route_info->length,
                 route_info->flags, route_info->route_lifetime);
            break;
        case icmp_option_ra_flags_extension:
            flags = option->option.ra_flags_extension;
            INFO("  ra flags extension: %x %x %x %x %x %x", flags[0], flags[1], flags[2], flags[3], flags[4], flags[5]);
            break;
        default:
            INFO("  option type %d", option->type);
            break;
        }
    }
}

static bool
icmp_message_parse_options(icmp_message_t *message, uint8_t *icmp_buf, unsigned length, unsigned *offset)
{
    uint8_t option_type, option_length_8;
    unsigned option_length;
    unsigned scan_offset = *offset;
    icmp_option_t *option;
    uint32_t reserved32;
    prefix_information_t *prefix_information;
    route_information_t *route_information;

    int prefix_bytes;

    // Count the options and validate the lengths
    message->num_options = 0;
    while (scan_offset < length) {
        if (!dns_u8_parse(icmp_buf, length, &scan_offset, &option_type)) {
            return false;
        }
        if (!dns_u8_parse(icmp_buf, length, &scan_offset, &option_length_8)) {
            return false;
        }
        if (option_length_8 == 0) { // RFC4191 section 4.6: The value 0 is invalid.
            ERROR("icmp_option_parse: option type %d length 0 is invalid.", option_type);
            return false;
        }
        if (scan_offset + option_length_8 * 8 - 2 > length) {
            ERROR("icmp_option_parse: option type %d length %d is longer than remaining available space %u",
                  option_type, option_length_8 * 8, length - scan_offset + 2);
            return false;
        }
        scan_offset += option_length_8 * 8 - 2;
        message->num_options++;
    }
    // If there are no options, we're done. No options is valid, so return true.
    if (message->num_options == 0) {
        return true;
    }
    message->options = calloc(message->num_options, sizeof(*message->options));
    if (message->options == NULL) {
        ERROR("No memory for icmp options.");
        return false;
    }
    option = message->options;
    while (*offset < length) {
        scan_offset = *offset;
        if (!dns_u8_parse(icmp_buf, length, &scan_offset, &option_type)) {
            return false;
        }
        if (!dns_u8_parse(icmp_buf, length, &scan_offset, &option_length_8)) {
            return false;
        }
        // We already validated the length in the previous pass.
        option->type = option_type;
        option_length = option_length_8 * 8;

        switch(option_type) {
        case icmp_option_source_link_layer_address:
        case icmp_option_target_link_layer_address:
            // At this juncture we are assuming that everything we care about looks like an
            // ethernet interface.  So for this case, length should be 8.
            if (option_length != 8) {
                INFO("Ignoring unexpectedly long link layer address: %d", option_length);
                // Don't store the option.
                message->num_options--;
                *offset += option_length;
                continue;
            }
            option->option.link_layer_address.length = 6;
            memcpy(option->option.link_layer_address.address, &icmp_buf[scan_offset], 6);
            break;
        case icmp_option_prefix_information:
            prefix_information = &option->option.prefix_information;
            // Only a length of 32 is valid.  This is an invalid ICMP packet, not just misunderunderstood
            if (option_length != 32) {
                return false;
            }
            // prefix length 8
            if (!dns_u8_parse(icmp_buf, length, &scan_offset, &prefix_information->length)) {
                return false;
            }
            // flags 8a
            if (!dns_u8_parse(icmp_buf, length, &scan_offset, &prefix_information->flags)) {
                return false;
            }
            // valid lifetime 32
            if (!dns_u32_parse(icmp_buf, length, &scan_offset,
                               &prefix_information->valid_lifetime)) {
                return false;
            }
            // preferred lifetime 32
            if (!dns_u32_parse(icmp_buf, length, &scan_offset,
                               &prefix_information->preferred_lifetime)) {
                return false;
            }
            // reserved2 32
            if (!dns_u32_parse(icmp_buf, length, &scan_offset, &reserved32)) {
                return false;
            }
            // prefix 128
            in6prefix_copy_from_data(&prefix_information->prefix, &icmp_buf[scan_offset], 16);
            break;
        case icmp_option_route_information:
            route_information = &option->option.route_information;

            // route length 8
            if (!dns_u8_parse(icmp_buf, length, &scan_offset, &route_information->length)) {
                return false;
            }
            switch(option_length) {
            case 8:
                prefix_bytes = 0;
                break;
            case 16:
                prefix_bytes = 8;
                break;
            case 24:
                prefix_bytes = 16;
                break;
            default:
                ERROR("invalid route information option length %d for route length %d",
                      option_length, route_information->length);
                return false;
            }
            // flags 8
            if (!dns_u8_parse(icmp_buf, length, &scan_offset, &route_information->flags)) {
                return false;
            }
            // route lifetime 32
            if (!dns_u32_parse(icmp_buf, length, &scan_offset, &route_information->route_lifetime)) {
                return false;
            }
            // route (64, 96 or 128)
            in6prefix_copy_from_data(&route_information->prefix, &icmp_buf[scan_offset], prefix_bytes);
            break;
        case icmp_option_ra_flags_extension:
            // The RA Flags extension as defined in RFC 5175 must have a length of 1 (meaning 8 bytes).
            // It's possible that a later spec will define a length > 1, but since we are implementing
            // RFC5175, we are required to silently ignore anything after the first 8 bytes. Since
            // we've already checked for length=0 (invalid), we can just take our six bytes of flags
            // and not bounds-check further.
            memcpy(option->option.ra_flags_extension, &icmp_buf[scan_offset], sizeof(option->option.ra_flags_extension));
            break;
        default:
        case icmp_option_mtu:
        case icmp_option_redirected_header:
            // don't care
            break;
        }
        *offset += option_length;
        option++;
    }
    return true;
}


static void
icmp_message(route_state_t *route_state, uint8_t *icmp_buf, unsigned length, int ifindex, int hop_limit, addr_t *src, addr_t *dest)
{
    unsigned offset = 0;
    uint32_t reserved32;
    interface_t *interface;
    icmp_message_t *message = calloc(1, sizeof(*message));
    if (message == NULL) {
        ERROR("Unable to allocate icmp_message_t for parsing");
        return;
    }

    message->source = src->sin6.sin6_addr;
    message->destination = dest->sin6.sin6_addr;
    message->hop_limit = hop_limit;
    for (interface = route_state->interfaces; interface; interface = interface->next) {
        if (interface->index == ifindex) {
            message->interface = interface;
            break;
        }
    }
    message->received_time = ioloop_timenow();
    message->received_time_already_adjusted = false;
    message->new_router = true;
    message->route_state = route_state;

    if (message->interface == NULL) {
        SEGMENTED_IPv6_ADDR_GEN_SRP(message->source.s6_addr, src_buf);
        SEGMENTED_IPv6_ADDR_GEN_SRP(message->destination.s6_addr, dst_buf);
        INFO("ICMP message type %d from " PRI_SEGMENTED_IPv6_ADDR_SRP " to " PRI_SEGMENTED_IPv6_ADDR_SRP
             " on interface index %d, which isn't listed.",
             icmp_buf[0], SEGMENTED_IPv6_ADDR_PARAM_SRP(message->source.s6_addr, src_buf),
             SEGMENTED_IPv6_ADDR_PARAM_SRP(message->destination.s6_addr, dst_buf), ifindex);
        icmp_message_free(message);
        return;
    }

    if (length < sizeof (struct icmp6_hdr)) {
        ERROR("Short ICMP message: length %d is shorter than ICMP header length %zd", length, sizeof(struct icmp6_hdr));
        icmp_message_free(message);
        return;
    }
    INFO("length %d", length);

    // The increasingly innaccurately named dns parse functions will work fine for this.
    if (!dns_u8_parse(icmp_buf, length, &offset, &message->type)) {
        goto out;
    }
    if (!dns_u8_parse(icmp_buf, length, &offset, &message->code)) {
        goto out;
    }
    // XXX check the checksum
    if (!dns_u16_parse(icmp_buf, length, &offset, &message->checksum)) {
        goto out;
    }
    switch(message->type) {
    case icmp_type_router_advertisement:
        if (!dns_u8_parse(icmp_buf, length, &offset, &message->cur_hop_limit)) {
            goto out;
        }
        if (!dns_u8_parse(icmp_buf, length, &offset, &message->flags)) {
            goto out;
        }
        if (!dns_u16_parse(icmp_buf, length, &offset, &message->router_lifetime)) {
            goto out;
        }
        if (!dns_u32_parse(icmp_buf, length, &offset, &message->reachable_time)) {
            goto out;
        }
        if (!dns_u32_parse(icmp_buf, length, &offset, &message->retransmission_timer)) {
            goto out;
        }

        if (!icmp_message_parse_options(message, icmp_buf, length, &offset)) {
            goto out;
        }
        icmp_message_dump(message, &message->source, &message->destination);
        router_advertisement(message);
        // router_advertisement() is given ownership of the message
        return;

    case icmp_type_router_solicitation:
        if (!dns_u32_parse(icmp_buf, length, &offset, &reserved32)) {
            goto out;
        }
        if (!icmp_message_parse_options(message, icmp_buf, length, &offset)) {
            goto out;
        }
        icmp_message_dump(message, &message->source, &message->destination);
        router_solicit(message);
        // router_solicit() is given ownership of the message.
        return;

    case icmp_type_neighbor_advertisement:
        icmp_message_dump(message, &message->source, &message->destination);
        neighbor_advertisement(message);
        break;

    case icmp_type_neighbor_solicitation:
    case icmp_type_echo_request:
    case icmp_type_echo_reply:
    case icmp_type_redirect:
        break;
    }

out:
    icmp_message_free(message);
    return;
}

#ifndef FUZZING
static
#endif
void
icmp_callback(io_t *NONNULL io, void *UNUSED context)
{
    ssize_t rv;
    uint8_t icmp_buf[1500];
    int ifindex = 0;
    addr_t src, dest;
    int hop_limit = 0;

#ifndef FUZZING
    rv = ioloop_recvmsg(io->fd, &icmp_buf[0], sizeof(icmp_buf), &ifindex, &hop_limit, &src, &dest);
#else
    rv = read(io->fd, &icmp_buf, sizeof(icmp_buf));
#endif
    if (rv < 0) {
        ERROR("icmp_callback: can't read ICMP message: " PUB_S_SRP, strerror(errno));
        return;
    }
    for (route_state_t *route_state = route_states; route_state != NULL; route_state = route_state->next) {
        icmp_message(route_state, icmp_buf, (unsigned)rv, ifindex, hop_limit, &src, &dest); // rv will never be > sizeof(icmp_buf)
    }
}

static void
route_information_to_wire(dns_towire_state_t *towire, void *prefix_data,
                          const char *source_interface, const char *dest_interface)
{
    uint8_t *prefix = prefix_data;

#ifndef ND_OPT_ROUTE_INFORMATION
#define ND_OPT_ROUTE_INFORMATION 24
#endif
    dns_u8_to_wire(towire, ND_OPT_ROUTE_INFORMATION);
    dns_u8_to_wire(towire, 2); // length / 8
    dns_u8_to_wire(towire, 64); // Interface prefixes are always 64 bits
    dns_u8_to_wire(towire, 0); // There's no reason at present to prefer one Thread BR over another
    dns_u32_to_wire(towire, BR_PREFIX_LIFETIME); // Route lifetime 1800 seconds (30 minutes)
    dns_rdata_raw_data_to_wire(towire, prefix, 8); // /64 requires 8 bytes.
    SEGMENTED_IPv6_ADDR_GEN_SRP(prefix, thread_prefix_buf);
    INFO("Sending route to " PRI_SEGMENTED_IPv6_ADDR_SRP "%%" PUB_S_SRP " on " PUB_S_SRP,
         SEGMENTED_IPv6_ADDR_PARAM_SRP(prefix, thread_prefix_buf), source_interface, dest_interface);
}

static void
icmp_send(uint8_t *message, size_t length, interface_t *interface, const struct in6_addr *destination)
{
#ifdef FUZZING
    char buffer[length];
    memcpy(buffer, message, length);
    return;
#endif
    struct iovec iov;
    struct in6_pktinfo *packet_info;
    socklen_t cmsg_length = CMSG_SPACE(sizeof(*packet_info)) + CMSG_SPACE(sizeof (int));
    uint8_t *cmsg_buffer;
    struct msghdr msg_header;
    struct cmsghdr *cmsg_pointer;
    int hop_limit = 255;
    ssize_t rv;
    struct sockaddr_in6 dest;

    // Make space for the control message buffer.
    cmsg_buffer = calloc(1, cmsg_length);
    if (cmsg_buffer == NULL) {
        ERROR("Unable to construct ICMP Router Advertisement: no memory");
        return;
    }

    // Send the message
    memset(&dest, 0, sizeof(dest));
    dest.sin6_family = AF_INET6;
    dest.sin6_scope_id = interface->index;
#ifndef NOT_HAVE_SA_LEN
    dest.sin6_len = sizeof(dest);
#endif
    msg_header.msg_namelen = sizeof(dest);
    dest.sin6_addr = *destination;

    msg_header.msg_name = &dest;
    iov.iov_base = message;
    iov.iov_len = length;
    msg_header.msg_iov = &iov;
    msg_header.msg_iovlen = 1;
    msg_header.msg_control = cmsg_buffer;
    msg_header.msg_controllen = cmsg_length;

    // Specify the interface
    cmsg_pointer = CMSG_FIRSTHDR(&msg_header);
    cmsg_pointer->cmsg_level = IPPROTO_IPV6;
    cmsg_pointer->cmsg_type = IPV6_PKTINFO;
    cmsg_pointer->cmsg_len = CMSG_LEN(sizeof(*packet_info));
    packet_info = (struct in6_pktinfo *)CMSG_DATA(cmsg_pointer);
    memset(packet_info, 0, sizeof(*packet_info));
    packet_info->ipi6_ifindex = interface->index;

    // Router advertisements and solicitations have a hop limit of 255
    cmsg_pointer = CMSG_NXTHDR(&msg_header, cmsg_pointer);
    cmsg_pointer->cmsg_level = IPPROTO_IPV6;
    cmsg_pointer->cmsg_type = IPV6_HOPLIMIT;
    cmsg_pointer->cmsg_len = CMSG_LEN(sizeof(int));
    memcpy(CMSG_DATA(cmsg_pointer), &hop_limit, sizeof(hop_limit));


    // Send it
    rv = sendmsg(icmp_listener.io_state->fd, &msg_header, 0);
    if (rv < 0) {
        uint8_t *in6_addr_bytes = ((struct sockaddr_in6 *)(msg_header.msg_name))->sin6_addr.s6_addr;
        SEGMENTED_IPv6_ADDR_GEN_SRP(in6_addr_bytes, in6_addr_buf);
        ERROR("icmp_send: sending " PUB_S_SRP " to " PRI_SEGMENTED_IPv6_ADDR_SRP " on interface " PUB_S_SRP
              " index %d: " PUB_S_SRP, message[0] == ND_ROUTER_SOLICIT ? "solicit" : "advertise",
              SEGMENTED_IPv6_ADDR_PARAM_SRP(in6_addr_bytes, in6_addr_buf),
              interface->name, interface->index, strerror(errno));
    } else if ((size_t)rv != iov.iov_len) {
        ERROR("icmp_send: short send to interface " PUB_S_SRP ": %zd < %zd", interface->name, rv, iov.iov_len);
    }
    free(cmsg_buffer);
}

void
router_advertisement_send(interface_t *interface, const struct in6_addr *destination)
{
    uint8_t *message;
    dns_towire_state_t towire;
    route_state_t *route_state = interface->route_state;

    // Thread blocks RAs so no point sending them.
    if (interface->inactive
#ifndef RA_TESTER
        || interface->is_thread
#endif
        ) {
        return;
    }

#define MAX_ICMP_MESSAGE 1280
    message = malloc(MAX_ICMP_MESSAGE);
    if (message == NULL) {
        ERROR("router_advertisement_send: unable to construct ICMP Router Advertisement: no memory");
        return;
    }

    // Construct the ICMP header and options for each interface.
    memset(&towire, 0, sizeof towire);
    towire.p = message;
    towire.lim = message + MAX_ICMP_MESSAGE;

    // Construct the ICMP header.
    // We use the DNS message construction functions because it's easy; probably should just make
    // the towire functions more generic.
    dns_u8_to_wire(&towire, ND_ROUTER_ADVERT);  // icmp6_type
    dns_u8_to_wire(&towire, 0);                 // icmp6_code
    dns_u16_to_wire(&towire, 0);                // The kernel computes the checksum (we don't technically have it).
    dns_u8_to_wire(&towire, 0);                 // Hop limit, we don't set.
    dns_u8_to_wire(&towire, 0);                 // Flags.  We don't offer DHCP, so We set neither the M nor the O bit.
    // We are not a home agent, so no H bit.  Lifetime is 0, so Prf is 0.
#ifdef ROUTER_LIFETIME_HACK
    dns_u16_to_wire(&towire, BR_PREFIX_LIFETIME); // Router lifetime, hacked.  This shouldn't ever be enabled.
#else
#ifdef RA_TESTER
    // Advertise a default route on the simulated thread network
    if (!strcmp(interface->name, route_state->thread_interface_name)) {
        dns_u16_to_wire(&towire, BR_PREFIX_LIFETIME); // Router lifetime for default route
    } else {
#endif
        dns_u16_to_wire(&towire, 0);            // Router lifetime for non-default default route(s).
#ifdef RA_TESTER
    }
#endif // RA_TESTER
#endif // ROUTER_LIFETIME_HACK
    dns_u32_to_wire(&towire, 0);                // Reachable time for NUD, we have no opinion on this.
    dns_u32_to_wire(&towire, 0);                // Retransmission timer, again we have no opinion.

#ifndef RA_TESTER
    // Send MTU of 1280 for Thread?
    if (interface->is_thread) {
        dns_u8_to_wire(&towire, ND_OPT_MTU);
        dns_u8_to_wire(&towire, 1); // length / 8
        dns_u32_to_wire(&towire, 1280);
        INFO("advertising MTU of 1280 on " PUB_S_SRP, interface->name);
    }
#endif

    // Send Prefix Information option if there's no IPv6 on the link.
    if (interface->our_prefix_advertised && !interface->suppress_ipv6_prefix && route_state->have_xpanid_prefix) {
        dns_u8_to_wire(&towire, ND_OPT_PREFIX_INFORMATION);
        dns_u8_to_wire(&towire, 4); // length / 8
        dns_u8_to_wire(&towire, 64); // On-link prefix is always 64 bits
        dns_u8_to_wire(&towire, ND_OPT_PI_FLAG_ONLINK | ND_OPT_PI_FLAG_AUTO); // On link, autoconfig
        dns_u32_to_wire(&towire, interface->valid_lifetime);
        dns_u32_to_wire(&towire, interface->preferred_lifetime);
        dns_u32_to_wire(&towire, 0); // Reserved
        dns_rdata_raw_data_to_wire(&towire, &interface->ipv6_prefix, sizeof interface->ipv6_prefix);
        SEGMENTED_IPv6_ADDR_GEN_SRP(interface->ipv6_prefix.s6_addr, ipv6_prefix_buf);
        INFO("advertising on-link prefix " PRI_SEGMENTED_IPv6_ADDR_SRP " on " PUB_S_SRP,
             SEGMENTED_IPv6_ADDR_PARAM_SRP(interface->ipv6_prefix.s6_addr, ipv6_prefix_buf), interface->name);

    }

    // In principle we can either send routes to links that are reachable by this router,
    // or just advertise a router to the entire ULA /48.   In theory it doesn't matter
    // which we do; if we support HNCP at some point we probably need to be specific, but
    // for now being general is fine because we have no way to share a ULA.
    // Unfortunately, some RIO implementations do not work with specific routes, so for now
    // We are doing it the easy way and just advertising the /48.
#define SEND_INTERFACE_SPECIFIC_RIOS 1
#ifdef SEND_INTERFACE_SPECIFIC_RIOS

    // If neither ROUTE_BETWEEN_NON_THREAD_LINKS nor RA_TESTER are defined, then we never want to
    // send an RIO other than for the thread network prefix.
#if defined (ROUTE_BETWEEN_NON_THREAD_LINKS) || defined(RA_TESTER)
    interface_t *ifroute;
    // Send Route Information option for other interfaces.
    for (ifroute = route_state->interfaces; ifroute; ifroute = ifroute->next) {
        if (ifroute->inactive) {
            continue;
        }
        if (want_routing(route_state) &&
            ifroute->our_prefix_advertised &&
#ifdef SEND_ON_LINK_ROUTE
            // In theory we don't want to send RIO for the on-link prefix, but there's this bug, see.
            true &&
#else
            ifroute != interface &&
#endif
#ifdef RA_TESTER
            // For the RA tester, we don't need to send an RIO to the thread network because we're the
            // default router for that network.
            strcmp(interface->name, route_state->thread_interface_name)
#else
            true
#endif
            )
        {
            route_information_to_wire(&towire, &ifroute->ipv6_prefix, ifroute->name, interface->name);
        }
    }
#endif // ROUTE_BETWEEN_NON_THREAD_LINKS || RA_TESTER

#ifndef RA_TESTER
    // Send route information option for thread prefix
    if (route_state->omr_watcher != NULL) {
        omr_prefix_t *thread_prefixes = omr_watcher_prefixes_get(route_state->omr_watcher);

        // Send RIOs for any other prefixes that appear on the Thread network
        for (struct omr_prefix *prefix = thread_prefixes; prefix != NULL; prefix = prefix->next) {
            route_information_to_wire(&towire, &prefix->prefix, route_state->thread_interface_name, interface->name);
        }
    }
#endif
#else
#ifndef SKIP_SLASH_48
    dns_u8_to_wire(&towire, ND_OPT_ROUTE_INFORMATION);
    dns_u8_to_wire(&towire, 3); // length / 8
    dns_u8_to_wire(&towire, 48); // ULA prefixes are always 48 bits
    dns_u8_to_wire(&towire, 0); // There's no reason at present to prefer one Thread BR over another
    dns_u32_to_wire(&towire, BR_PREFIX_LIFETIME); // Route lifetime 1800 seconds (30 minutes)
    dns_rdata_raw_data_to_wire(&towire, &route_state->srp_server->ula_prefix, 16); // /48 requires 16 bytes
#endif // SKIP_SLASH_48
#endif // SEND_INTERFACE_SPECIFIC_RIOS

    // Send the stub router flag
    dns_u8_to_wire(&towire, ND_OPT_RA_FLAGS_EXTENSION);
    dns_u8_to_wire(&towire, 1); // length / 8
    dns_u8_to_wire(&towire, RA_FLAGS1_STUB_ROUTER);
    dns_u8_to_wire(&towire, 0); // Five bytes of zero flag bits
    dns_u32_to_wire(&towire, 0);

    // Send Source link-layer address option
    if (interface->have_link_layer_address) {
        dns_u8_to_wire(&towire, ND_OPT_SOURCE_LINKADDR);
        dns_u8_to_wire(&towire, 1); // length / 8
        dns_rdata_raw_data_to_wire(&towire, &interface->link_layer, sizeof(interface->link_layer));
    }

    if (towire.error) {
        ERROR("No space in ICMP output buffer for " PUB_S_SRP " at route.c:%d", interface->name, towire.line);
        towire.error = 0;
    } else {
        SEGMENTED_IPv6_ADDR_GEN_SRP(destination->s6_addr, destination_buf);
        INFO("sending advertisement to " PRI_SEGMENTED_IPv6_ADDR_SRP " on " PUB_S_SRP,
             SEGMENTED_IPv6_ADDR_PARAM_SRP(destination->s6_addr, destination_buf),
             interface->name);
        icmp_send(message, towire.p - message, interface, destination);
    }
    free(message);
}

void
router_solicit_send(interface_t *interface)
{
    uint8_t *message;
    dns_towire_state_t towire;

    // Thread blocks RSs so no point sending them.
    if (interface->inactive
#ifndef RA_TESTER
        || interface->is_thread
#endif
        ) {
        return;
    }
#define MAX_ICMP_MESSAGE 1280
    message = malloc(MAX_ICMP_MESSAGE);
    if (message == NULL) {
        ERROR("Unable to construct ICMP Router Advertisement: no memory");
        return;
    }

    // Construct the ICMP header and options for each interface.
    memset(&towire, 0, sizeof towire);
    towire.p = message;
    towire.lim = message + MAX_ICMP_MESSAGE;

    // Construct the ICMP header.
    // We use the DNS message construction functions because it's easy; probably should just make
    // the towire functions more generic.
    dns_u8_to_wire(&towire, ND_ROUTER_SOLICIT);  // icmp6_type
    dns_u8_to_wire(&towire, 0);                  // icmp6_code
    dns_u16_to_wire(&towire, 0);                 // The kernel computes the checksum (we don't technically have it).
    dns_u32_to_wire(&towire, 0);                 // Reserved32

    // Send Source link-layer address option
    if (interface->have_link_layer_address) {
        dns_u8_to_wire(&towire, ND_OPT_SOURCE_LINKADDR);
        dns_u8_to_wire(&towire, 1); // length / 8
        dns_rdata_raw_data_to_wire(&towire, &interface->link_layer, sizeof(interface->link_layer));
    }

    if (towire.error) {
        ERROR("No space in ICMP output buffer for " PUB_S_SRP " at route.c:%d", interface->name, towire.line);
    } else {
        if (interface->have_link_layer_address) {
            INFO("sending router solicit on " PUB_S_SRP " to all routers with source " PRI_MAC_ADDR_SRP,
                 interface->name, MAC_ADDR_PARAM_SRP(interface->link_layer));
        } else {
            INFO("sending router solicit on " PUB_S_SRP " to all routers", interface->name);
        }
        icmp_send(message, towire.p - message, interface, &in6addr_linklocal_allrouters);
    }
    free(message);
}

void
neighbor_solicit_send(interface_t *interface, struct in6_addr *destination)
{
    uint8_t *message;
    dns_towire_state_t towire;

#define MAX_ICMP_MESSAGE 1280
    message = malloc(MAX_ICMP_MESSAGE);
    if (message == NULL) {
        ERROR("Unable to construct ICMP Router Advertisement: no memory");
        return;
    }

    // Construct the ICMP header and options for each interface.
    memset(&towire, 0, sizeof towire);
    towire.p = message;
    towire.lim = message + MAX_ICMP_MESSAGE;

    // Construct the ICMP header.
    // We use the DNS message construction functions because it's easy; probably should just make
    // the towire functions more generic.
    dns_u8_to_wire(&towire, ND_NEIGHBOR_SOLICIT);  // icmp6_type
    dns_u8_to_wire(&towire, 0);                    // icmp6_code
    dns_u16_to_wire(&towire, 0);                   // The kernel computes the checksum (we don't technically have it).
    dns_u32_to_wire(&towire, 0);                   // Reserved32
    dns_rdata_raw_data_to_wire(&towire, destination, sizeof(*destination)); // Target address of solicit

    // Send Source link-layer address option
    if (interface->have_link_layer_address) {
        dns_u8_to_wire(&towire, ND_OPT_SOURCE_LINKADDR);
        dns_u8_to_wire(&towire, 1); // length / 8
        dns_rdata_raw_data_to_wire(&towire, &interface->link_layer, sizeof(interface->link_layer));
    }

    if (towire.error) {
        ERROR("No space in ICMP output buffer for " PUB_S_SRP " at route.c:%d", interface->name, towire.line);
    } else {
        SEGMENTED_IPv6_ADDR_GEN_SRP(destination, dest_buf);
        if (interface->have_link_layer_address) {
            INFO("sending neighbor solicit on " PUB_S_SRP " to " PRI_SEGMENTED_IPv6_ADDR_SRP " with source " PRI_MAC_ADDR_SRP,
                 interface->name, SEGMENTED_IPv6_ADDR_PARAM_SRP(destination, dest_buf), MAC_ADDR_PARAM_SRP(interface->link_layer));
        } else {
            INFO("sending neighbor solicit on " PUB_S_SRP " to " PRI_SEGMENTED_IPv6_ADDR_SRP,
                 interface->name, SEGMENTED_IPv6_ADDR_PARAM_SRP(destination, dest_buf));
        }
        icmp_send(message, towire.p - message, interface, destination);
    }
    free(message);
}

bool
start_icmp_listener(void)
{
#ifndef SRP_TEST_SERVER
    int sock = socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6);
    int true_flag = 1;
#ifdef CONFIGURE_STATIC_INTERFACE_ADDRESSES
    int false_flag = 0;
#endif
    struct icmp6_filter filter;
    ssize_t rv;

    if (sock < 0) {
        ERROR("Unable to listen for icmp messages: " PUB_S_SRP, strerror(errno));
        close(sock);
        return false;
    }

    // Only accept router advertisements and router solicits.
    ICMP6_FILTER_SETBLOCKALL(&filter);
    ICMP6_FILTER_SETPASS(ND_ROUTER_SOLICIT, &filter);
    ICMP6_FILTER_SETPASS(ND_ROUTER_ADVERT, &filter);
    ICMP6_FILTER_SETPASS(ND_NEIGHBOR_ADVERT, &filter);
    rv = setsockopt(sock, IPPROTO_ICMPV6, ICMP6_FILTER, &filter, sizeof(filter));
    if (rv < 0) {
        ERROR("Can't set IPV6_RECVHOPLIMIT: " PUB_S_SRP ".", strerror(errno));
        close(sock);
        return false;
    }

    // We want a source address and interface index
    rv = setsockopt(sock, IPPROTO_IPV6, IPV6_RECVPKTINFO, &true_flag, sizeof(true_flag));
    if (rv < 0) {
        ERROR("Can't set IPV6_RECVPKTINFO: " PUB_S_SRP ".", strerror(errno));
        close(sock);
        return false;
    }

    // We need to be able to reject RAs arriving from off-link.
    rv = setsockopt(sock, IPPROTO_IPV6, IPV6_RECVHOPLIMIT, &true_flag, sizeof(true_flag));
    if (rv < 0) {
        ERROR("Can't set IPV6_RECVHOPLIMIT: " PUB_S_SRP ".", strerror(errno));
        close(sock);
        return false;
    }

#ifdef CONFIGURE_STATIC_INTERFACE_ADDRESSES
    // Prevent our router advertisements from updating our routing table.
    rv = setsockopt(sock, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, &false_flag, sizeof(false_flag));
    if (rv < 0) {
        ERROR("Can't set IPV6_RECVHOPLIMIT: " PUB_S_SRP ".", strerror(errno));
        close(sock);
        return false;
    }
#endif

    icmp_listener.io_state = ioloop_file_descriptor_create(sock, NULL, NULL);
    if (icmp_listener.io_state == NULL) {
        ERROR("No memory for ICMP I/O structure.");
        close(sock);
        return false;
    }

    // Beacon out a router advertisement every three minutes.
    icmp_listener.unsolicited_interval = 3 * 60 * 1000;
    ioloop_add_reader(icmp_listener.io_state, icmp_callback);
#else
    (void)icmp_callback;
#endif // !SRP_TEST_SERVER

    return true;
}

void
icmp_interface_subscribe(interface_t *interface, bool added)
{
    struct ipv6_mreq req;
    int rv;

    if (icmp_listener.io_state == NULL) {
        ERROR("Interface subscribe without ICMP listener.");
        return;
    }

    memset(&req, 0, sizeof req);
    if (interface->index == -1) {
        ERROR("icmp_interface_subscribe called before interface index fetch for " PUB_S_SRP, interface->name);
        return;
    }

    req.ipv6mr_multiaddr = in6addr_linklocal_allrouters;
    req.ipv6mr_interface = interface->index;
    rv = setsockopt(icmp_listener.io_state->fd, IPPROTO_IPV6, added ? IPV6_JOIN_GROUP : IPV6_LEAVE_GROUP, &req,
                    sizeof req);
    if (rv < 0) {
        ERROR("Unable to " PUB_S_SRP " all-routers multicast group on " PUB_S_SRP ": " PUB_S_SRP,
              added ? "join" : "leave", interface->name, strerror(errno));
        return;
    } else {
        INFO(PUB_S_SRP "subscribed on interface " PUB_S_SRP, added ? "" : "un",
             interface->name);
    }

    req.ipv6mr_multiaddr = in6addr_linklocal_allnodes;
    req.ipv6mr_interface = interface->index;
    rv = setsockopt(icmp_listener.io_state->fd, IPPROTO_IPV6, added ? IPV6_JOIN_GROUP : IPV6_LEAVE_GROUP, &req,
                    sizeof req);
    if (rv < 0) {
        ERROR("Unable to " PUB_S_SRP " all-nodes multicast group on " PUB_S_SRP ": " PUB_S_SRP,
              added ? "join" : "leave", interface->name, strerror(errno));
        return;
    } else {
        INFO(PUB_S_SRP "subscribed on interface " PUB_S_SRP, added ? "" : "un",
             interface->name);
    }

}


// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 120
// indent-tabs-mode: nil
// End:
