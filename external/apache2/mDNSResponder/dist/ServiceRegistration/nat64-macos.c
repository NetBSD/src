/* nat64-macos.c
 *
 * Copyright (c) 2021-2023 Apple Inc. All rights reserved.
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

#include "dns-msg.h"
#include "ioloop.h"
#include "srp-mdns-proxy.h"
#include "nat64-macos.h"
#include "nat64.h"
#include "interface-monitor-macos.h"
#include "srp-log.h"
#include "srp.h"
#include <CoreUtils/CoreUtils.h>
#include <mdns/pf.h>
#include <mdns/system.h>

static bool pass_all_rule_is_set = false;

#if SRP_FEATURE_NAT64
static struct sockaddr_in nat64_primary_ipv4;
static struct in6_addr nat64_prefix;
static ifmon_t nat64_ifmon = NULL;
static bool nat64_prefix_is_set = false;
static bool nat64_active = false;

static void
nat64_reset(void)
{
    OSStatus err;
    if (nat64_primary_ipv4.sin_family == AF_INET) {
        err = mdns_pf_set_thread_nat64_rules(nat64_prefix.s6_addr, NAT64_PREFIX_SLASH_96_BYTES * 8, nat64_primary_ipv4.sin_addr.s_addr);
        if (!err) {
            mdns_system_set_ipv4_forwarding(true);
            nat64_active = true;
        } else {
            ERROR("nat64_reset: failed to set NAT64 rules: %ld.", (long)err);
        }
    } else {
        err = mdns_pf_delete_thread_rules();
        if (err) {
            ERROR("nat64_reset: failed to delete NAT64 rules: %ld.", (long)err);
        }
        mdns_system_set_ipv4_forwarding(false);
    }
}

void
nat64_pass_all_pf_rule_delete(void)
{
    if (pass_all_rule_is_set) {
        OSStatus err = mdns_pf_delete_thread_pass_all_rule_for_conn_tracking();
        if (err != 0) {
            ERROR("failed to delete pass all rule: %ld.", (long)err);
        } else {
            pass_all_rule_is_set = false;
        }
    }
}
#endif // SRP_FEATURE_NAT64

void
nat64_pass_all_pf_rule_set(const char *interface)
{
    nat64_pass_all_pf_rule_delete();
    if (!pass_all_rule_is_set) {
        OSStatus err = mdns_pf_set_thread_pass_all_rule_for_conn_tracking(interface);
        if (err != 0) {
            ERROR("failed to set pass all rule: %ld.", (long)err);
        } else {
            pass_all_rule_is_set = true;
        }
    }
}

#if SRP_FEATURE_NAT64
void
nat64_stop_translation(void)
{
    OSStatus err;
    err = mdns_pf_delete_thread_rules();
    if (err) {
        ERROR("nat64_reset: failed to delete NAT64 rules: %ld.", (long)err);
    }
    mdns_system_set_ipv4_forwarding(false);
}

void
nat64_start_translation(const dispatch_queue_t queue)
{
    nat64_primary_ipv4.sin_family = AF_UNSPEC;
    nat64_ifmon = ifmon_create(queue);
    dispatch_block_t handler = ^{
        const sockaddr_ip new_primary = ifmon_get_primary_ipv4_address(nat64_ifmon);
        if (SockAddrCompareAddr(&nat64_primary_ipv4, &new_primary.v4) != 0) {
            nat64_primary_ipv4 = new_primary.v4;
            if (nat64_prefix_is_set) {
                nat64_reset();
            }
        }
    };
    ifmon_set_primary_ip_changed_handler(nat64_ifmon, handler);
    ifmon_activate(nat64_ifmon, handler);
}

void
nat64_set_ula_prefix(const struct in6_addr *const ula_prefix)
{
    bool changed;
    if (!nat64_prefix_is_set || (in6prefix_compare(&nat64_prefix, ula_prefix, 5) != 0)) {
        changed = true;
    } else {
        changed = false;
    }
    if (changed) {
        // Set the 48-bit ULA prefix (0xfd + global identifier), then set the next 16 bits to all-ones to make the
        // 64-bit IPv6 prefix.
        in6addr_zero(&nat64_prefix);
        in6prefix_copy(&nat64_prefix, ula_prefix, 6);
        memset(&nat64_prefix.s6_addr[6], 0xFF, 2);
        nat64_prefix_is_set = true;
        if (nat64_primary_ipv4.sin_family == AF_INET) {
            nat64_reset();
        }
    }
}

const struct in6_addr *
nat64_get_ipv6_prefix(void)
{
    return &nat64_prefix;
}

bool
nat64_is_active(void)
{
    return nat64_active;
}

#endif // SRP_FEATURE_NAT64
