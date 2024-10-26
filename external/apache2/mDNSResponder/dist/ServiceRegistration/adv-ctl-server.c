/* adv-ctl-proxy.c
 *
 * Copyright (c) 2019-2024 Apple Inc. All rights reserved.
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
 * This file contains the SRP Advertising Proxy control interface, which allows clients to control the advertising proxy
 * and discover information about its internal state. This is largely used for testing.
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

#include "srp.h"
#include "dns-msg.h"
#include "ioloop.h"
#include "srp-gw.h"
#include "srp-proxy.h"
#include "cti-services.h"
#include "srp-mdns-proxy.h"
#include "route.h"
#include "nat64.h"
#include "adv-ctl-server.h"
#include "srp-replication.h"
#include "dnssd-proxy.h"
#include "thread-device.h"

#include "state-machine.h"
#include "thread-service.h"
#include "omr-watcher.h"
#include "omr-publisher.h"
#include "dnssd-client.h"
#include "service-publisher.h"
#include "thread-tracker.h"
#include "service-tracker.h"
#include "route-tracker.h"
#include "ifpermit.h"

#include "cti-proto.h"
#include "adv-ctl-common.h"
#include "advertising_proxy_services.h"

static void srp_xpc_client_finalize(srp_wanted_state_t *wanted);


static int
adv_ctl_block_service(bool enable, void *context)
{
    int status = kDNSSDAdvertisingProxyStatus_NoError;
#if THREAD_BORDER_ROUTER
    srp_server_t *server_state = context;
    route_state_t *route_state = server_state->route_state;
    if (enable) {
        if (route_state->srp_listener != NULL) {
            ioloop_comm_cancel(route_state->srp_listener);
            server_state->srp_unicast_service_blocked = true;
        } else {
            status = kDNSSDAdvertisingProxyStatus_UnknownErr;
        }
    } else {
        if (route_state->srp_listener == NULL) {
            server_state->srp_unicast_service_blocked = false;
            route_refresh_interface_list(route_state);
        } else {
            status = kDNSSDAdvertisingProxyStatus_UnknownErr;
        }
    }
#else
    (void)enable;
    (void)context;
#endif // THREAD_DEVICE
    return status;
}

static bool
adv_ctl_regenerate_ula(void *context)
{
    int status = kDNSSDAdvertisingProxyStatus_NoError;
#if STUB_ROUTER
    srp_server_t *server_state = context;

    infrastructure_network_shutdown(server_state->route_state);
    route_ula_generate(server_state->route_state);
    infrastructure_network_startup(server_state->route_state);
#else
    (void)context;
#endif
    return status;
}

static int
adv_ctl_advertise_prefix(void *context, omr_prefix_priority_t priority)
{
    int status = kDNSSDAdvertisingProxyStatus_NoError;
#if STUB_ROUTER
    srp_server_t *server_state = context;

    if (server_state->route_state != NULL && server_state->route_state->omr_publisher != NULL) {
        omr_publisher_force_publication(server_state->route_state->omr_publisher, priority);
    }
#else
    (void)context;
    (void)priority;
#endif
    return status;
}

static int
adv_ctl_stop_advertising_service(void *context)
{
    int status = kDNSSDAdvertisingProxyStatus_NoError;
#if THREAD_DEVICE
    srp_server_t *server_state = context;

#if STUB_ROUTER
    if (server_state->stub_router_enabled) {
        partition_discontinue_all_srp_service(server_state->route_state);
    } else
#endif
    {
        thread_device_stop(server_state);
    }
#else
    (void)context;
#endif
    return status;
}

static int
adv_ctl_disable_replication(void *context)
{
    int status = kDNSSDAdvertisingProxyStatus_NoError;

#if SRP_FEATURE_REPLICATION
    srp_server_t *server_state = context;
    srpl_disable(server_state);
#else
    (void)context;
#endif
    return status;
}

static int
adv_ctl_drop_srpl_connection(void *context)
{
    int status = kDNSSDAdvertisingProxyStatus_NoError;

#if SRP_FEATURE_REPLICATION
    srp_server_t *server_state = context;
    srpl_drop_srpl_connection(server_state);
#else
    (void)context;
#endif
    return status;
}

static int
adv_ctl_undrop_srpl_connection(void *context)
{
    int status = kDNSSDAdvertisingProxyStatus_NoError;

#if SRP_FEATURE_REPLICATION
    srp_server_t *server_state = context;
    srpl_undrop_srpl_connection(server_state);
#else
    (void)context;
#endif
    return status;
}

static int
adv_ctl_drop_srpl_advertisement(void *context)
{
    int status = kDNSSDAdvertisingProxyStatus_NoError;

#if SRP_FEATURE_REPLICATION
    srp_server_t *server_state = context;
    srpl_drop_srpl_advertisement(server_state);
#else
    (void)context;
#endif
    return status;
}

static int
adv_ctl_undrop_srpl_advertisement(void *context)
{
    int status = kDNSSDAdvertisingProxyStatus_NoError;

#if SRP_FEATURE_REPLICATION
    srp_server_t *server_state = context;
    srpl_undrop_srpl_advertisement(server_state);
#else
    (void)context;
#endif
    return status;
}

static void
adv_ctl_start_breaking_time(void *context)
{
    srp_server_t *server_state = context;

    server_state->break_srpl_time = true;
}

#if STUB_ROUTER || THREAD_DEVICE
static void
adv_ctl_thread_shutdown_continue(void *context)
{
    srp_server_t *server_state = context;

    if (server_state->shutdown_timeout != NULL) {
        ioloop_cancel_wake_event(server_state->shutdown_timeout);
        ioloop_wakeup_release(server_state->shutdown_timeout);
        server_state->shutdown_timeout = NULL;
    }

    xpc_object_t response;
    response = xpc_dictionary_create_reply(server_state->shutdown_request);
    if (response == NULL) {
        ERROR("adv_xpc_message: Unable to create reply dictionary.");
        return;
    }
    xpc_dictionary_set_uint64(response, kDNSSDAdvertisingProxyResponseStatus, 0);
    xpc_connection_send_message(server_state->shutdown_connection, response);
    xpc_release(response);
    xpc_release(server_state->shutdown_request);
    server_state->shutdown_request = NULL;
    xpc_release(server_state->shutdown_connection);
    server_state->shutdown_connection = NULL;

    server_state->awaiting_service_removal = false;
    server_state->awaiting_prefix_removal = false;

    if (server_state->wanted != NULL) {
        INFO("clearing server_state->wanted (%p) unconditionally.", server_state->wanted);
        srp_xpc_client_finalize(server_state->wanted);
    }
}

static void
adv_ctl_start_thread_shutdown_timer(srp_server_t *server_state)
{
    if (server_state->shutdown_timeout == NULL) {
        server_state->shutdown_timeout = ioloop_wakeup_create();
    }
    // If we can't allocate the shutdown timer, send the response immediately (which also probably won't
    // work, oh well).
    if (server_state->shutdown_timeout == NULL) {
        adv_ctl_thread_shutdown_continue(server_state);
        return;
    }
    // Wait no longer than two seconds for thread network data update
    ioloop_add_wake_event(server_state->shutdown_timeout,
                          server_state, adv_ctl_thread_shutdown_continue, NULL, 2 * IOLOOP_SECOND);
}

static bool
adv_ctl_start_thread_shutdown(xpc_object_t request, xpc_connection_t connection, void *context)
{
    srp_server_t *server_state = context;

    server_state->shutdown_connection = connection;
    xpc_retain(server_state->shutdown_connection);
    server_state->shutdown_request = request;
    xpc_retain(server_state->shutdown_request);
    if (0) {
#if STUB_ROUTER
    } else if (server_state->stub_router_enabled) {
        if (server_state->service_tracker != NULL &&
            service_tracker_local_service_seen(server_state->service_tracker))
        {
            service_tracker_cancel_probes(server_state->service_tracker); // This is a no-op for now.
            server_state->awaiting_service_removal = true;
        }
        if (server_state->route_state->omr_publisher != NULL &&
            omr_publisher_publishing_prefix(server_state->route_state->omr_publisher))
        {
            omr_publisher_unpublish_prefix(server_state->route_state->omr_publisher);
            if (server_state->route_state->omr_watcher != NULL) {
                server_state->awaiting_prefix_removal = true;
            }
        }
        if (route_tracker_local_routes_seen(server_state->route_state->route_tracker)) {
            nat64_thread_shutdown(server_state->route_state);
            route_tracker_shutdown(server_state->route_state);
            server_state->awaiting_route_removal = true;
        }
        srpl_shutdown(server_state);
        partition_discontinue_all_srp_service(server_state->route_state);
        adv_ctl_start_thread_shutdown_timer(server_state);
        adv_ctl_thread_shutdown_status_check(server_state);
#endif
#if THREAD_DEVICE
    } else {
        if (server_state->service_tracker != NULL) {
            service_tracker_cancel_probes(server_state->service_tracker);
        }
        if (server_state->dnssd_client != NULL) {
            dnssd_client_cancel(server_state->dnssd_client);
            dnssd_client_release(server_state->dnssd_client);
            server_state->dnssd_client = NULL;
        }
        if (server_state->service_publisher != NULL) {
            service_publisher_stop_publishing(server_state->service_publisher);
            service_publisher_cancel(server_state->service_publisher);
            service_publisher_release(server_state->service_publisher);
            server_state->service_publisher = NULL;
            if (server_state->service_tracker != NULL &&
                service_tracker_local_service_seen(server_state->service_tracker))
            {
                server_state->awaiting_service_removal = true;
            }
        }
        adv_ctl_thread_shutdown_status_check(server_state);
        adv_ctl_start_thread_shutdown_timer(server_state);
#endif
    }
    return true;
}

void
adv_ctl_thread_shutdown_status_check(srp_server_t *server_state)
{
    if (0) {
#if STUB_ROUTER
    } else if (server_state->stub_router_enabled) {
        if (!server_state->awaiting_prefix_removal &&
            !server_state->awaiting_service_removal &&
            !server_state->awaiting_route_removal)
        {
            adv_ctl_thread_shutdown_continue(server_state);
        }
#endif
#if THREAD_DEVICE
    } else {
        if (!server_state->awaiting_service_removal) {
            adv_ctl_thread_shutdown_continue(server_state);
        }
#endif
    }
}
#endif // STUB_ROUTER || THREAD_DEVICE

static int
adv_ctl_block_anycast_service(bool block, void *context)
{
    int status = kDNSSDAdvertisingProxyStatus_NoError;

#if STUB_ROUTER
    srp_server_t *server_state = context;
    partition_block_anycast_service(server_state->route_state, block);
#else
    (void)context;
    (void)block;
#endif
    return status;
}

typedef struct variable variable_t;
static void adv_ctl_set_int(srp_server_t *server_state, variable_t *which, const char *value);

struct variable {
    const char *name;
    enum { type_int, type_bool, type_string } format;
    void (*set_function)(srp_server_t *server_state, variable_t *which, const char *value);
    size_t offset;
} variables[] = {
    { "min-lease-time", type_int, adv_ctl_set_int, offsetof(srp_server_t, min_lease_time) },
    { "max-lease-time", type_int, adv_ctl_set_int, offsetof(srp_server_t, min_lease_time) },
    { "key-min-lease-time", type_int, adv_ctl_set_int, offsetof(srp_server_t, key_min_lease_time) },
    { "key-min-lease-time", type_int, adv_ctl_set_int, offsetof(srp_server_t, key_min_lease_time) },
    { NULL, type_int, NULL, 0 },
};

static void
adv_ctl_set_int(srp_server_t *server_state, variable_t *which, const char *value)
{
    int *dest = (int *)((char *)server_state + which->offset);
    long new_value;
    char *endptr;

    // Sanity check
    if (which->offset > sizeof(*server_state)) {
        ERROR("which->offset out of range: %zu vs %zu", which->offset, sizeof(*server_state));
        return;
    }
    if (value[0] == '0' && value[1] == 'x') {
        new_value = strtol(value + 2, &endptr, 16);
    } else {
        new_value = strtol(value, &endptr, 10);
    }
    if (endptr == value || (endptr != NULL && *endptr != '\0') || new_value < INT_MIN || new_value > INT_MAX) {
        ERROR("invalid int for " PUB_S_SRP ": " PUB_S_SRP, which->name, value);
        return;
    }

    INFO("setting " PUB_S_SRP " to '" PUB_S_SRP "' %ld (%lx), originally %d (%x)",
         which->name, value, new_value, new_value, *dest, *dest);
    *dest = (int)new_value;
}

static int
adv_ctl_set_variable(void *context, const uint8_t *data, size_t data_len)
{
    srp_server_t *server_state = context;
    char *name = (char *)data, *value, *value_end;
    size_t remain;
    char hexbuf[1000];
    char *hp = hexbuf;

    for (size_t i = 0; i < data_len && hp < hexbuf + sizeof(hexbuf); ) {
        size_t len = snprintf(hp, hexbuf + sizeof(hexbuf) - hp, "%02x ", data[i++]);
        hp += len;
    }
    INFO("hexbuf: " PUB_S_SRP, hexbuf);

    // Find the end of the name
    value = memchr(data, 0, data_len);
    if (value == NULL) {
        ERROR("name not NUL-terminated");
        return kDNSSDAdvertisingProxyStatus_BadParam;
    }
    value++;
    remain = value - name;
    if (remain >= data_len) {
        ERROR("no value");
        return kDNSSDAdvertisingProxyStatus_BadParam;
    }
    value_end = memchr(value, 0, remain);
    if (value_end == NULL) {
        ERROR("value not NUL-terminated");
        return kDNSSDAdvertisingProxyStatus_BadParam;
    }
    // value_end - name is the length of all the data but the final NUL.
    if ((size_t)(value_end - name) != data_len - 1) {
        ERROR("extra bytes at end of name/value buffer: %zd != %zd %p %p %p %zu",
              value_end - name, data_len, name, value, value_end, remain);
        return kDNSSDAdvertisingProxyStatus_BadParam;
    }
    for (int i = 0; variables[i].name != NULL; i++) {
        if (!strcmp(name, variables[i].name)) {
            if (variables[i].set_function != NULL) {
                variables[i].set_function(server_state, &variables[i], value);
            }
            break;
        }
    }
    return kDNSSDAdvertisingProxyStatus_NoError;
}


static void
adv_ctl_fd_finalize(void *context)
{
    advertising_proxy_conn_ref connection = context;
    connection->io_context = NULL;
    RELEASE_HERE(connection, advertising_proxy_conn_ref);
}

static bool
adv_ctl_list_services(advertising_proxy_conn_ref connection, void *context)
{
    srp_server_t *server_state = context;
    adv_host_t *host;
    int i;
    int64_t now = ioloop_timenow();
    int num_hosts = 0;

    for (host = server_state->hosts; host != NULL; host = host->next) {
        num_hosts++;
    }
    if (!cti_connection_message_create(connection, kDNSSDAdvertisingProxyResponse, 200) ||
        !cti_connection_u32_put(connection, (uint32_t)kDNSSDAdvertisingProxyStatus_NoError) ||
        !cti_connection_u32_put(connection, num_hosts))
    {
        ERROR("adv_ctl_list_services: error starting response");
        cti_connection_close(connection);
        return false;
    }
    for (host = server_state->hosts; host != NULL; host = host->next) {
        int num_addresses = 0;
        int num_instances = 0;
        if (!cti_connection_string_put(connection, host->name) ||
            !cti_connection_string_put(connection, host->registered_name) ||
            !cti_connection_u32_put(connection, host->lease_expiry >= now ? host->lease_expiry - now : 0) ||
            !cti_connection_bool_put(connection, host->removed) ||
            !cti_connection_u64_put(connection, host->update_server_id))
        {
            ERROR("adv_ctl_list_services: unable to write host info for host %s", host->name);
            cti_connection_close(connection);
            return false;
        }

        cti_connection_u64_put(connection, host->server_stable_id);

        if (host->addresses != NULL) {
            for (i = 0; i < host->addresses->num; i++) {
                if (host->addresses->vec[i] != NULL) {
                    num_addresses++;
                }
            }
        }
        cti_connection_u16_put(connection, num_addresses);
        if (host->addresses != NULL) {
            for (i = 0; i < host->addresses->num; i++) {
                if (host->addresses->vec[i] != NULL) {
                    if (!cti_connection_u16_put(connection, host->addresses->vec[i]->rrtype) ||
                        !cti_connection_data_put(connection, host->addresses->vec[i]->rdata, host->addresses->vec[i]->rdlen))
                    {
                        ERROR("adv_ctl_list_services: unable to write address %d for host %s", i, host->name);
                        cti_connection_close(connection);
                        return false;
                    }
                }
            }
        }
        if (host->instances != NULL) {
            for (i = 0; i < host->instances->num; i++) {
                if (host->instances->vec[i] != NULL) {
                    num_instances++;
                }
            }
        }
        cti_connection_u16_put(connection, num_instances);
        if (host->instances != NULL) {
            char *regtype;
            if (memcmp(&host->server_stable_id, &server_state->ula_prefix, sizeof(host->server_stable_id))) {
                regtype = "replicated";
            } else {
                regtype = instance->anycast ? "anycast" : "unicast";
            }
            for (i = 0; i < host->instances->num; i++) {
                adv_instance_t *instance = host->instances->vec[i];
                if (instance != NULL) {
                    if (!cti_connection_string_put(connection, instance->instance_name) ||
                        !cti_connection_string_put(connection, instance->service_type) ||
                        !cti_connection_u16_put(connection, instance->port) ||
                        !cti_connection_data_put(connection, instance->txt_data, instance->txt_length) ||
                        !cti_connection_string_put(connection, regtype))
                    {
                        ERROR("adv_ctl_list_services: unable to write address %d for host %s", i, host->name);
                        cti_connection_close(connection);
                        return false;
                    }
                }
            }
        }
    }
    return cti_connection_message_send(connection);
}

static bool
adv_ctl_get_ula(advertising_proxy_conn_ref connection, void *context)
{
    srp_server_t *server_state = context;

    if (!cti_connection_message_create(connection, kDNSSDAdvertisingProxyResponse, 200) ||
        !cti_connection_u32_put(connection, (uint32_t)kDNSSDAdvertisingProxyStatus_NoError))
    {
        ERROR("error starting response");
        cti_connection_close(connection);
        return false;
    }
    // Copy out just the global ID part of the ULA prefix.
    uint64_t ula = 0;
    for (int j = 1; j < 6; j++) {
        ula = ula << 8 | (((uint8_t *)&server_state->ula_prefix)[j]);
    }
    if (!cti_connection_u64_put(connection, ula)) {
        ERROR("error sending ula");
        cti_connection_close(connection);
        return false;
    }
    return cti_connection_message_send(connection);
}

static void
adv_ctl_message_parse(advertising_proxy_conn_ref connection)
{
    int status = kDNSSDAdvertisingProxyStatus_NoError;
    cti_connection_parse_start(connection);
    if (!cti_connection_u16_parse(connection, &connection->message_type)) {
        return;
    }
    switch(connection->message_type) {
    case kDNSSDAdvertisingProxyEnable:
        INFO("Client uid %d pid %d sent a kDNSSDAdvertisingProxyEnable request.",
             connection->uid, connection->gid);
        break;
    case kDNSSDAdvertisingProxyListServiceTypes:
        INFO("Client uid %d pid %d sent a kDNSSDAdvertisingProxyListServiceTypes request.",
             connection->uid, connection->gid);
        break;
    case kDNSSDAdvertisingProxyListServices:
        INFO("Client uid %d pid %d sent a kDNSSDAdvertisingProxyListServices request.",
             connection->uid, connection->gid);
        adv_ctl_list_services(connection, connection->context);
        return;
    case kDNSSDAdvertisingProxyListHosts:
        INFO("Client uid %d pid %d sent a kDNSSDAdvertisingProxyListHosts request.",
             connection->uid, connection->gid);
        break;
    case kDNSSDAdvertisingProxyGetHost:
        INFO("Client uid %d pid %d sent a kDNSSDAdvertisingProxyGetHost request.",
             connection->uid, connection->gid);
        break;
    case kDNSSDAdvertisingProxyFlushEntries:
        INFO("Client uid %d pid %d sent a kDNSSDAdvertisingProxyFlushEntries request.",
             connection->uid, connection->gid);
        srp_mdns_flush(connection->context);
        break;
    case kDNSSDAdvertisingProxyBlockService:
        INFO("Client uid %d pid %d sent a kDNSSDAdvertisingProxyBlockService request.",
             connection->uid, connection->gid);
        adv_ctl_block_service(true, connection->context);
        break;
    case kDNSSDAdvertisingProxyUnblockService:
        INFO("Client uid %d pid %d sent a kDNSSDAdvertisingProxyUnblockService request.",
             connection->uid, connection->gid);
        adv_ctl_block_service(false, connection->context);
        break;
    case kDNSSDAdvertisingProxyRegenerateULA:
        INFO("Client uid %d pid %d sent a kDNSSDAdvertisingProxyRegenerateULA request.",
             connection->uid, connection->gid);
        adv_ctl_regenerate_ula(connection->context);
        break;
    case kDNSSDAdvertisingProxyAdvertisePrefix:
        INFO("Client uid %d pid %d sent a kDNSSDAdvertisingProxyAdvertisePrefix request.",
             connection->uid, connection->gid);
        adv_ctl_advertise_prefix(connection->context);
        break;
    case kDNSSDAdvertisingProxyAddPrefix: {
        void *data = NULL;
        uint16_t data_len;
        INFO("Client uid %d pid %d sent a kDNSSDAdvertisingProxyAddPrefix request.",
             connection->uid, connection->gid);
        if (!cti_connection_data_parse(connection, &data, &data_len)) {
            ERROR("faile to parse data for kDNSSDAdvertisingProxyAddPrefix request.");
            status = kDNSSDAdvertisingProxyStatus_BadParam;
        } else {
            if (data != NULL && data_len == 16) {
                SEGMENTED_IPv6_ADDR_GEN_SRP(data, prefix_buf);
                INFO("got prefix " PRI_SEGMENTED_IPv6_ADDR_SRP, SEGMENTED_IPv6_ADDR_PARAM_SRP(data, prefix_buf));
                adv_ctl_add_prefix(connection->context, data);
                status = kDNSSDAdvertisingProxyStatus_NoError;
            } else {
                ERROR("invalid add prefix request, data[%p], data_len[%d]", data, data_len);
                status = kDNSSDAdvertisingProxyStatus_BadParam;
            }
        }
        break;
    }
    case kDNSSDAdvertisingProxyRemovePrefix: {
        void *data = NULL;
        uint16_t data_len;
        INFO("Client uid %d pid %d sent a kDNSSDAdvertisingProxyRemovePrefix request.",
             connection->uid, connection->gid);
        if (!cti_connection_data_parse(connection, &data, &data_len)) {
            ERROR("faile to parse data for kDNSSDAdvertisingProxyRemovePrefix request.");
            status = kDNSSDAdvertisingProxyStatus_BadParam;
        } else {
            if (data != NULL && data_len == 16) {
                SEGMENTED_IPv6_ADDR_GEN_SRP(data, prefix_buf);
                INFO("got prefix " PRI_SEGMENTED_IPv6_ADDR_SRP, SEGMENTED_IPv6_ADDR_PARAM_SRP(data, prefix_buf));
                adv_ctl_remove_prefix(connection->context, data);
                status = kDNSSDAdvertisingProxyStatus_NoError;
            } else {
                ERROR("invalid add prefix request, data[%p], data_len[%d]", data, data_len);
                status = kDNSSDAdvertisingProxyStatus_BadParam;
            }
        }
        break;
    }
    case kDNSSDAdvertisingProxyStop:
        INFO("Client uid %d pid %d sent a kDNSSDAdvertisingProxyStop request.",
             connection->uid, connection->gid);
        adv_ctl_stop_advertising_service(connection->context);
        break;
    case kDNSSDAdvertisingProxyGetULA:
        INFO("Client uid %d pid %d sent a kDNSSDAdvertisingProxyULA request.",
             connection->uid, connection->gid);
        adv_ctl_get_ula(connection, connection->context);
        break;
    case kDNSSDAdvertisingProxyDisableReplication:
        INFO("Client uid %d pid %d sent a kDNSSDAdvertisingProxyDisableReplication request.",
             connection->uid, connection->gid);
        adv_ctl_disable_replication(connection->context);
        break;
    case kDNSSDAdvertisingProxyDropSrplConnection:
        INFO("Client uid %d pid %d sent a kDNSSDAdvertisingProxyDropSrplConnection request.",
             connection->uid, connection->gid);
        adv_ctl_drop_srpl_connection(connection->context);
        break;
    case kDNSSDAdvertisingProxyUndropSrplConnection:
        INFO("Client uid %d pid %d sent a kDNSSDAdvertisingProxyUndropSrplConnection request.",
             connection->uid, connection->gid);
        adv_ctl_undrop_srpl_connection(connection->context);
        break;
    case kDNSSDAdvertisingProxyDropSrplAdvertisement:
        INFO("Client uid %d pid %d sent a kDNSSDAdvertisingProxyDropSrplAdvertisement request.",
             connection->uid, connection->gid);
        adv_ctl_drop_srpl_advertisement(connection->context);
        break;
    case kDNSSDAdvertisingProxyUndropSrplAdvertisement:
        INFO("Client uid %d pid %d sent a kDNSSDAdvertisingProxyUndropSrplAdvertisement request.",
             connection->uid, connection->gid);
        adv_ctl_undrop_srpl_advertisement(connection->context);
        break;

    case kDNSSDAdvertisingProxyStartDroppingPushConnections:
        INFO("Client uid %d pid %d sent a kDNSSDAdvertisingProxyStartDroppingPushConnections request.",
             connection->uid, connection->gid);
        dp_start_dropping();
        break;

    case kDNSSDAdvertisingProxyStartBreakingTimeValidation:
        INFO("Client uid %d pid %d sent a kDNSSDAdvertisingProxyStartBreakingTimeValidation request.",
             connection->uid, connection->pid);
        adv_ctl_start_breaking_time(connection->context);
        break;

    case kDNSSDAdvertisingProxyStartThreadShutdown:
        INFO("Client uid %d pid %d sent a kDNSSDAdvertisingProxyStartThreadShutdown request.",
             connection->uid, connection->pid);
        return adv_ctl_start_thread_shutdown(request, connection, context);

    case kDNSSDAdvertisingProxySetVariable:
        void *data = NULL;
        uint16_t data_len;
        INFO("Client uid %d pid %d sent a kDNSSDAdvertisingProxySetVariable request.",
             connection->uid, connection->pid);
        if (!cti_connection_data_parse(connection, &data, &data_len)) {
            ERROR("faile to parse data for kDNSSDAdvertisingProxySetVariable request.");
            status = kDNSSDAdvertisingProxyStatus_BadParam;
        } else {
            status = adv_ctl_set_variable(connection->context, data, data_len);
        }
        break;

    case kDNSSDAdvertisingProxyBlockAnycastService:
        INFO("Client uid %d pid %d sent a kDNSSDAdvertisingProxyBlockAnycastService request.",
             connection->uid, connection->gid);
        adv_ctl_block_anycast_service(true, connection->context);
        break;

    case kDNSSDAdvertisingProxyUnblockBlockAnycastService:
        INFO("Client uid %d pid %d sent a kDNSSDAdvertisingProxyUnblockAnycastService request.",
             connection->uid, connection->gid);
        adv_ctl_block_anycast_service(false, connection->context);
        break;

    default:
        ERROR("Client uid %d pid %d sent a request with unknown message type %d.",
              connection->uid, connection->gid, connection->message_type);
        status = kDNSSDAdvertisingProxyStatus_Invalid;
        break;
    }
    cti_send_response(connection, status);
    cti_connection_close(connection);
}

static void
adv_ctl_read_callback(io_t *UNUSED io, void *context)
{
    advertising_proxy_conn_ref connection = context;

    cti_read(connection, adv_ctl_message_parse);
}

static void
adv_ctl_listen_callback(io_t *UNUSED io, void *context)
{
    srp_server_t *server_state = context;
    uid_t uid;
    gid_t gid;
    pid_t pid;

    int fd = cti_accept(server_state->adv_ctl_listener->fd, &uid, &gid, &pid);
    if (fd < 0) {
        return;
    }

    advertising_proxy_conn_ref connection = cti_connection_allocate(500);
    if (connection == NULL) {
        ERROR("cti_listen_callback: no memory for connection.");
        close(fd);
        return;
    }
    RETAIN_HERE(connection, advertising_proxy_conn_ref);

    connection->fd = fd;
    connection->uid = uid;
    connection->gid = gid;
    connection->pid = pid;
    connection->io_context = ioloop_file_descriptor_create(connection->fd, connection, adv_ctl_fd_finalize);
    if (connection->io_context == NULL) {
        ERROR("cti_listen_callback: no memory for io context.");
        close(fd);
        RELEASE_HERE(connection, advertising_proxy_conn_ref);
        return;
    }
    ioloop_add_reader(connection->io_context, adv_ctl_read_callback);
    connection->context = context;
    connection->callback.reply = NULL;
    connection->internal_callback = NULL;
    return;
}

static int
adv_ctl_listen(srp_server_t *server_state)
{
    int fd = cti_make_unix_socket(ADV_CTL_SERVER_SOCKET_NAME, sizeof(ADV_CTL_SERVER_SOCKET_NAME), true);
    if (fd < 0) {
        int ret = (errno == ECONNREFUSED
                   ? kDNSSDAdvertisingProxyStatus_DaemonNotRunning
                   : errno == EPERM ? kDNSSDAdvertisingProxyStatus_NotPermitted : kDNSSDAdvertisingProxyStatus_UnknownErr);
        ERROR("adv_ctl_listener: socket: %s", strerror(errno));
        return ret;
    }

    server_state->adv_ctl_listener = ioloop_file_descriptor_create(fd, server_state, NULL);
    if (server_state->adv_ctl_listener == NULL) {
        ERROR("adv_ctl_listener: no memory for io_t object.");
        close(fd);
        return kDNSSDAdvertisingProxyStatus_NoMemory;
    }
    RETAIN_HERE(server_state->adv_ctl_listener, ioloop_file_descriptor);

    ioloop_add_reader(server_state->adv_ctl_listener, adv_ctl_listen_callback);
    return kDNSSDAdvertisingProxyStatus_NoError;
}

bool
adv_ctl_init(void *context)
{
    srp_server_t *server_state = context;
    return adv_ctl_listen(server_state);
}

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 108
// indent-tabs-mode: nil
// End:
