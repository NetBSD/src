/* srputil.c
 *
 * Copyright (c) 2020-2024 Apple Inc. All rights reserved.
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
 * SRP Advertising Proxy utility program, allows:
 *   start/stop advertising proxy
 *   get/track list of service types
 *   get/track list of services of a particular type
 *   get/track list of hosts
 *   get/track information about a particular host
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
#include <time.h>
#include <dns_sd.h>
#include <net/if.h>
#include <inttypes.h>

void *main_queue = NULL;

#include "srp.h"
#include "dns-msg.h"
#include "ioloop.h"
#include "advertising_proxy_services.h"
#include "route-tracker.h"
#include "state-machine.h"
#include "thread-service.h"
#include "service-tracker.h"
#include "probe-srp.h"
#include "cti-services.h"
#include "adv-ctl-server.h"


static void
flushed_callback(advertising_proxy_conn_ref cref, void *result, advertising_proxy_error_type err)
{
    INFO("flushed: cref %p  response %p   err %d.", cref, result, err);
    if (err != kDNSSDAdvertisingProxyStatus_NoError) {
        exit(1);
    }
    // We don't need to wait around after flushing.
    exit(0);
}

static void
block_callback(advertising_proxy_conn_ref cref, void *result, advertising_proxy_error_type err)
{
    INFO("blocked: cref %p  response %p   err %d.", cref, result, err);
    if (err != kDNSSDAdvertisingProxyStatus_NoError) {
        exit(1);
    }
    // We don't need to wait around after blocking.
    exit(0);
}

static void
unblock_callback(advertising_proxy_conn_ref cref, void *result, advertising_proxy_error_type err)
{
    INFO("unblocked: cref %p  response %p   err %d.", cref, result, err);
    if (err != kDNSSDAdvertisingProxyStatus_NoError) {
        exit(1);
    }
    // We don't need to wait around after unblocking.
    exit(0);
}

static void
regenerate_callback(advertising_proxy_conn_ref cref, void *result, advertising_proxy_error_type err)
{
    INFO("regenerated ula: cref %p  response %p   err %d.", cref, result, err);
    if (err != kDNSSDAdvertisingProxyStatus_NoError) {
        exit(1);
    }
    // We don't need to wait around after unblocking.
    exit(0);
}

static void
prefix_advertise_callback(advertising_proxy_conn_ref cref, void *result, advertising_proxy_error_type err)
{
    INFO("advertise prefix: cref %p  response %p   err %d.", cref, result, err);
    if (err != kDNSSDAdvertisingProxyStatus_NoError) {
        exit(1);
    }
    // We don't need to wait around after advertising prefix.
    exit(0);
}

static void
add_prefix_callback(advertising_proxy_conn_ref cref, void *result, advertising_proxy_error_type err)
{
    INFO("add prefix: cref %p  response %p   err %d.", cref, result, err);
    if (err != kDNSSDAdvertisingProxyStatus_NoError) {
        exit(1);
    }
    // We don't need to wait around after advertising prefix.
    exit(0);
}

static void
remove_prefix_callback(advertising_proxy_conn_ref cref, void *result, advertising_proxy_error_type err)
{
    INFO("remove prefix: cref %p  response %p   err %d.", cref, result, err);
    if (err != kDNSSDAdvertisingProxyStatus_NoError) {
        exit(1);
    }
    // We don't need to wait around after advertising prefix.
    exit(0);
}

static void
add_nat64_prefix_callback(advertising_proxy_conn_ref cref, void *result, advertising_proxy_error_type err)
{
    INFO("add nat64 prefix: cref %p  response %p   err %d.", cref, result, err);
    if (err != kDNSSDAdvertisingProxyStatus_NoError) {
        exit(1);
    }
    exit(0);
}

static void
remove_nat64_prefix_callback(advertising_proxy_conn_ref cref, void *result, advertising_proxy_error_type err)
{
    INFO("remove nat64 prefix: cref %p  response %p   err %d.", cref, result, err);
    if (err != kDNSSDAdvertisingProxyStatus_NoError) {
        exit(1);
    }
    exit(0);
}

static void
stop_callback(advertising_proxy_conn_ref cref, void *result, advertising_proxy_error_type err)
{
    INFO("stopped: cref %p  response %p   err %d.", cref, result, err);
    if (err != kDNSSDAdvertisingProxyStatus_NoError) {
        exit(1);
    }
    // We don't need to wait around after stopping.
    exit(0);
}

static const char *
print_address(advertising_proxy_host_address_t *address, char *addrbuf, size_t addrbuf_size)
{
    if (address->rrtype == 0) {
        return (char *)address->rdata;
    } else if (address->rrtype == dns_rrtype_a && address->rdlen == 4) {
        inet_ntop(AF_INET, address->rdata, addrbuf, (socklen_t)addrbuf_size);
        return addrbuf;
    } else if (address->rrtype == dns_rrtype_aaaa && address->rdlen == 16) {
        inet_ntop(AF_INET6, address->rdata, addrbuf, (socklen_t)addrbuf_size);
        return addrbuf;
    } else {
        sprintf(addrbuf, "Family-%d", address->rrtype);
        return addrbuf;
    }
}

static void
services_callback(advertising_proxy_conn_ref cref, void *result, advertising_proxy_error_type err)
{
    int i;
    int64_t lease, hours, minutes, seconds;
    advertising_proxy_host_t *host = result;
    const char *address = "<no address>";
    char *addrbuf = NULL;
    size_t addrbuflen;
    uint64_t ula;

    if (err != kDNSSDAdvertisingProxyStatus_NoError) {
        INFO("services: cref %p  response %p   err %d.", cref, result, err);
        exit(1);
    }
    if (result == NULL) {
        INFO("services: cref %p  response %p   err %d.", cref, result, err);
        exit(0);
    }

    if (host->num_instances == 0) {
        i = -1;
    } else {
        i = 0;
    }
    for (; i < host->num_instances; i++) {
        const char *instance_name, *service_type, *reg_type;
        char port[6]; // uint16_t as ascii

        if (i == -1 || host->instances[i].instance_name == NULL) {
            instance_name = "<no instances>";
            service_type = "";
            port[0] = 0;
            reg_type = "";
        } else {
            instance_name = host->instances[i].instance_name;
            service_type = host->instances[i].service_type;
            snprintf(port, sizeof(port), "%u", host->instances[i].port);
            reg_type = host->instances[i].reg_type;
        }

        if (host->num_addresses > 0) {
            addrbuflen = host->num_addresses * (INET6_ADDRSTRLEN + 1);
            addrbuf = malloc(addrbuflen);
            if (addrbuf == NULL) {
                address = "<no memory for address buffer>";
            } else {
                char *ap = addrbuf;
                for (int j = 0; j < host->num_addresses; j++) {
                    *ap++ = ' ';
                    address = print_address(&host->addresses[j], ap, addrbuflen - (ap - addrbuf));
                    size_t len = strlen(address);
                    if (address != ap) {
                        if (len + ap + 1 > addrbuf + addrbuflen) {
                            len = addrbuflen - (ap - addrbuf) - 1;
                        }
                        memcpy(ap, address, len + 1); // Includes NUL
                    }
                    ap += len;
                }
                address = addrbuf;
            }
        }
        lease = host->lease_time;
        hours = lease / 3600 / 1000;
        lease -= hours * 3600 * 1000;
        minutes = lease / 60 / 1000;
        lease -= minutes * 60 * 1000;
        seconds = lease / 1000;
        lease -= seconds * 1000;

        // Our implementation of the stable server ID uses the server ULA, so just copy out those 48 bits,
        // which are in network byte order.
        ula = 0;
        for (int j = 1; j < 6; j++) {
            ula = ula << 8 | (((uint8_t *)&host->server_id)[j]);
        }
        printf("\"%s\" \"%s\" %s %s %s %" PRIu64 ":%" PRIu64 ":%" PRIu64 ".%" PRIu64 " \"%s\" \"%s\" %s %" PRIx64 "\n",
               host->regname, instance_name, service_type, port,
               address == NULL ? "" : address, hours, minutes, seconds, lease, host->hostname,
               reg_type, host->removed ? "invalid" : "valid", ula);
        if (addrbuf != NULL) {
            free(addrbuf);
        }
    }
}

static void
ula_callback(advertising_proxy_conn_ref cref, void *result, advertising_proxy_error_type err)
{
    INFO("get_ula: cref %p  response %p   err %d.", cref, result, err);
    if (err != kDNSSDAdvertisingProxyStatus_NoError) {
        fprintf(stderr, "ULA get failed: %d\n", err);
        exit(1);
    }
    uint64_t ula = *((uint64_t *)result);
    printf("ULA: %" PRIx64 "\n", ula);
    exit(0);
}

static void
disable_srp_replication_callback(advertising_proxy_conn_ref cref, void *result, advertising_proxy_error_type err)
{
    INFO("disable_srp_replication: cref %p  response %p   err %d.", cref, result, err);
    if (err != kDNSSDAdvertisingProxyStatus_NoError) {
        exit(1);
    }
    // We don't need to wait around after SRP replication disabled.
    exit(0);
}

static void
drop_srpl_connection_callback(advertising_proxy_conn_ref cref, void *result, advertising_proxy_error_type err)
{
    INFO("drop_srpl_connection: cref %p  response %p   err %d.", cref, result, err);
    if (err != kDNSSDAdvertisingProxyStatus_NoError) {
        exit(1);
    }
    exit(0);
}

static void
undrop_srpl_connection_callback(advertising_proxy_conn_ref cref, void *result, advertising_proxy_error_type err)
{
    INFO("undrop_srpl_connection: cref %p  response %p   err %d.", cref, result, err);
    if (err != kDNSSDAdvertisingProxyStatus_NoError) {
        exit(1);
    }
    exit(0);
}

static void
drop_srpl_advertisement_callback(advertising_proxy_conn_ref cref, void *result, advertising_proxy_error_type err)
{
    INFO("drop_srpl_advertisement: cref %p  response %p   err %d.", cref, result, err);
    if (err != kDNSSDAdvertisingProxyStatus_NoError) {
        exit(1);
    }
    exit(0);
}

static void
undrop_srpl_advertisement_callback(advertising_proxy_conn_ref cref, void *result, advertising_proxy_error_type err)
{
    INFO("undrop_srpl_advertisement: cref %p  response %p   err %d.", cref, result, err);
    if (err != kDNSSDAdvertisingProxyStatus_NoError) {
        exit(1);
    }
    exit(0);
}

static void
start_dropping_push_connections_callback(advertising_proxy_conn_ref cref, void *result, advertising_proxy_error_type err)
{
    INFO("start_dropping_push_connections: cref %p  response %p   err %d.", cref, result, err);
    if (err != kDNSSDAdvertisingProxyStatus_NoError) {
        exit(1);
    }
    exit(0);
}

static void
start_breaking_time_validation_callback(advertising_proxy_conn_ref cref, void *result, advertising_proxy_error_type err)
{
    INFO("start_breaking_time_validation: cref %p  response %p   err %d.", cref, result, err);
    if (err != kDNSSDAdvertisingProxyStatus_NoError) {
        exit(1);
    }
    exit(0);
}

static void
block_anycast_service_callback(advertising_proxy_conn_ref cref, void *result, advertising_proxy_error_type err)
{
    INFO("block_anycast_service: cref %p  response %p   err %d.", cref, result, err);
    if (err != kDNSSDAdvertisingProxyStatus_NoError) {
        exit(1);
    }
    exit(0);
}

static void
unblock_anycast_service_callback(advertising_proxy_conn_ref cref, void *result, advertising_proxy_error_type err)
{
    INFO("unblock_anycast_service: cref %p  response %p   err %d.", cref, result, err);
    if (err != kDNSSDAdvertisingProxyStatus_NoError) {
        exit(1);
    }
    exit(0);
}

static void
start_thread_shutdown_callback(advertising_proxy_conn_ref cref, void *result, advertising_proxy_error_type err)
{
    INFO("cref %p  response %p   err %d.", cref, result, err);
    if (err != kDNSSDAdvertisingProxyStatus_NoError) {
        exit(1);
    }
    exit(0);
}

typedef struct variable variable_t;
struct variable {
    variable_t *next;
    const char *name, *value;
};

static void
set_variable_callback(advertising_proxy_conn_ref cref, void *context, void *result, advertising_proxy_error_type err)
{
    variable_t *variable = context;
    INFO("set_variable: cref %p  response %p   err %d, variable name %s, value %s.",
         cref, result, err, variable->name, variable->value);
    if (err != kDNSSDAdvertisingProxyStatus_NoError) {
        if (variable->next == NULL) {
            exit(1);
        }
    }
    if (variable->next == NULL) {
        exit(0);
    }
}

static comm_t *tcp_connection;
bool do_tcp_zero_test = false;
bool do_tcp_fin_length = false;
bool do_tcp_fin_payload = false;

service_tracker_t *tracker;

// Dummy functions required to use service tracker here
void
adv_ctl_thread_shutdown_status_check(srp_server_t *UNUSED server_state) {
}

static void
service_done_callback(void *context, cti_status_t status)
{
    const char *action = context;
    if (status != kCTIStatus_NoError) {
        fprintf(stderr, PUB_S_SRP " failed, status %d", action, status);
        exit(1);
    } else {
        fprintf(stderr, PUB_S_SRP " done", action);
        exit(0);
    }
}

static void
service_set_changed(bool unicast)
{
    thread_service_t *winner = NULL;
    for (thread_service_t *service = service_tracker_services_get(tracker); service != NULL; service = service->next) {
        if (service->ignore) {
            continue;
        }
        if (unicast && service->service_type == unicast_service) {
            if (winner == NULL || in6addr_compare(&service->u.unicast.address, &winner->u.unicast.address) < 0) {
                winner = service;
            }
        }
    }
    if (winner == NULL) {
        fprintf(stderr, "no services present!");
        exit(1);
    }
    winner->u.unicast.address.s6_addr[15] = 0;
    uint8_t service_data[1] = { THREAD_SRP_SERVER_OPTION };
    uint8_t server_data[18];
    memcpy(server_data, &winner->u.unicast.address, 15);
    server_data[15] = 0;
    server_data[16] = winner->u.unicast.port[0];
    server_data[17] = winner->u.unicast.port[1];
    int ret = cti_add_service(NULL, "add", service_done_callback, NULL,
                              THREAD_ENTERPRISE_NUMBER, service_data, 1, server_data, 18);
    if (ret != kCTIStatus_NoError) {
        fprintf(stderr, "add_service failed: %d", ret);
        exit(1);
    }
}

static void
unicast_service_set_changed(void *UNUSED context)
{
    service_set_changed(true);
}

static void
start_advertising_winning_unicast_service(void)
{
    tracker = service_tracker_create(NULL);
    if (tracker != NULL) {
        service_tracker_callback_add(tracker, unicast_service_set_changed, NULL, NULL);
        service_tracker_start(tracker);
    } else {
        fprintf(stderr, "unable to allocate tracker");
        exit(1);
    }
}

static void
start_removing_unicast_service(void)
{
    uint8_t service_data[1] = { THREAD_SRP_SERVER_OPTION };
    int ret = cti_remove_service(NULL, "remove", service_done_callback, NULL, THREAD_ENTERPRISE_NUMBER, service_data, 1);
    if (ret != kCTIStatus_NoError) {
        fprintf(stderr, "remove_service failed: %d", ret);
        exit(1);
    }
}

static void
tcp_datagram_callback(comm_t *NONNULL comm, message_t *NONNULL message, void *NULLABLE context)
{
    (void)comm;
    (void)context;
    fprintf(stderr, "tcp datagram received, length %d", message->length);
}

static void
tcp_connect_callback(comm_t *NONNULL connection, void *NULLABLE context)
{
    fprintf(stderr, "tcp connection succeeded...\n");
    uint8_t length[2];
    struct iovec iov[2];
    char databuf[128];
    memset(databuf, 0, sizeof(databuf));
    memset(iov, 0, sizeof(iov));

    (void)context;

    if (do_tcp_zero_test) {
        memset(length, 0, sizeof(length));
        iov[0].iov_len = 2;
        iov[0].iov_base = length;
        ioloop_send_data(connection, NULL, iov, 1);
    } else if (do_tcp_fin_length) {
        memset(length, 0, sizeof(length));
        iov[0].iov_len = 1;
        iov[0].iov_base = &length;
        ioloop_send_final_data(connection, NULL, iov, 1);
    } else if (do_tcp_fin_payload) {
        length[0] = 0;
        length[1] = 255;
        iov[0].iov_len = 2;
        iov[0].iov_base = length;
        iov[1].iov_len = 128;
        iov[1].iov_base = databuf;
        ioloop_send_final_data(connection, NULL, iov, 2);
    }
}

static void
tcp_disconnect_callback(comm_t *NONNULL comm, void *NULLABLE context, int error)
{
    (void)comm;
    (void)context;
    (void)error;
    fprintf(stderr, "tcp remote close.\n");
    exit(0);
}

static int
start_tcp_test(void)
{
    addr_t address;
    memset(&address, 0, sizeof(address));
    address.sa.sa_family = AF_INET;
    address.sin.sin_addr.s_addr = htonl(0x7f000001);
    address.sin.sin_port = htons(53);
#ifndef NOT_HAVE_SA_LEN
    address.sin.sin_len = sizeof(address.sin);
#endif
    tcp_connection = ioloop_connection_create(&address, false, true, false, false, tcp_datagram_callback,
                                              tcp_connect_callback, tcp_disconnect_callback, NULL, NULL);
    if (tcp_connection == NULL) {
        return kDNSSDAdvertisingProxyStatus_NoMemory;
    }
    return kDNSSDAdvertisingProxyStatus_NoError;
}

const char *need_name, *need_service;
bool needed_flag;

advertising_proxy_conn_ref service_sub;

static void
service_callback(advertising_proxy_conn_ref UNUSED sub, void *UNUSED context, advertising_proxy_error_type error)
{
    fprintf(stderr, "service callback: %d\n", error);
    exit(0);
}

static void
start_needing_service(void)
{
    advertising_proxy_error_type ret = advertising_proxy_set_service_needed(&service_sub, dispatch_get_main_queue(),
                                                                            service_callback, NULL, NULL, need_service,
                                                                            needed_flag);
    if (ret != kDNSSDAdvertisingProxyStatus_NoError) {
        fprintf(stderr, "advertising_proxy_service_create failed: %d\n", ret);
        exit(1);
    }
}

advertising_proxy_conn_ref instance_sub;

static void
instance_callback(advertising_proxy_conn_ref UNUSED sub, void *UNUSED context, advertising_proxy_error_type error)
{
    fprintf(stderr, "instance callback: %d\n", error);
    exit(0);
}

static void
start_needing_instance(void)
{
    advertising_proxy_error_type ret = advertising_proxy_set_service_needed(&instance_sub, dispatch_get_main_queue(),
                                                                            instance_callback, NULL, need_name,
                                                                            need_service, needed_flag);
    if (ret != kDNSSDAdvertisingProxyStatus_NoError) {
        fprintf(stderr, "advertising_proxy_set_service_needed failed: %d\n", ret);
        exit(1);
    }
}

const char *browse_service, *resolve_name, *resolve_service;

advertising_proxy_subscription_t *browse_sub;

static void
browse_callback(advertising_proxy_subscription_t *UNUSED sub, advertising_proxy_error_type error, uint32_t interface_index,
                bool add, const char *instance_name, const char *service_type, void *UNUSED context)
{
    if (error != kDNSSDAdvertisingProxyStatus_NoError) {
        fprintf(stderr, "browse_callback: %d\n", error);
        exit(1);
    }

    fprintf(stderr, "browse: %d %s %s %s\n", interface_index, add ? "add" : "rmv", instance_name, service_type);
}

static void
start_browsing_service(void)
{
    advertising_proxy_error_type ret = advertising_proxy_browse_create(&browse_sub, dispatch_get_main_queue(),
                                                                        browse_service, browse_callback, NULL);
    if (ret != kDNSSDAdvertisingProxyStatus_NoError) {
        fprintf(stderr, "advertising_proxy_browse_create failed: %d\n", ret);
        exit(1);
    }
}

advertising_proxy_subscription_t *resolve_sub;

static void
resolve_callback(advertising_proxy_subscription_t *UNUSED sub, advertising_proxy_error_type error,
                 uint32_t interface_index, bool add, const char *fullname, const char *hostname, uint16_t port,
                 uint16_t txt_length, const uint8_t *UNUSED txt_record, void *UNUSED context)
{
    if (error != kDNSSDAdvertisingProxyStatus_NoError) {
        fprintf(stderr, "resolve_create callback: %d\n", error);
        exit(1);
    }

    fprintf(stderr, "resolved: %d %s %s %s %d %d\n", interface_index, add ? "add" : "rmv",
            fullname, hostname, port, txt_length);
}

static void
start_resolving_service(void)
{
    advertising_proxy_error_type ret = advertising_proxy_resolve_create(&resolve_sub, dispatch_get_main_queue(),
                                                                        resolve_name, resolve_service, NULL,
                                                                        resolve_callback, NULL);
    if (ret != kDNSSDAdvertisingProxyStatus_NoError) {
        fprintf(stderr, "advertising_proxy_resolve_create failed: %d\n", ret);
        exit(1);
    }
}

advertising_proxy_subscription_t *registrar_sub;

static void
registrar_callback(advertising_proxy_subscription_t *UNUSED sub,
                   advertising_proxy_error_type error, void *UNUSED context)
{
    if (error != kDNSSDAdvertisingProxyStatus_NoError) {
        fprintf(stderr, "registrar_callback: %d\n", error);
        exit(1);
    }

    INFO("SRP registrar is enabled.");
}

static void
start_registrar(void)
{
    advertising_proxy_error_type ret = advertising_proxy_registrar_create(&registrar_sub, dispatch_get_main_queue(),
                                                                          registrar_callback, NULL);
    if (ret != kDNSSDAdvertisingProxyStatus_NoError) {
        fprintf(stderr, "advertising_proxy_registrar_create failed: %d", ret);
        exit(1);
    }
}


static void
usage(void)
{
    fprintf(stderr, "srputil start [if1 .. ifN -]            -- start the SRP MDNS Proxy through launchd\n");
    fprintf(stderr, "  tcp-zero                              -- connect to port 53, send a DNS message that's got a zero-length payload\n");
    fprintf(stderr, "  tcp-fin-length                        -- connect to port 53, send a DNS message that ends before length is complete\n");
    fprintf(stderr, "  tcp-fin-payload                       -- connect to port 53, send a DNS message that ends before payload is complete\n");
    fprintf(stderr, "  services                              -- get the list of services currently being advertised\n");
    fprintf(stderr, "  block                                 -- block the SRP listener\n");
    fprintf(stderr, "  unblock                               -- unblock the SRP listener\n");
    fprintf(stderr, "  regenerate-ula                        -- generate a new ULA and restart the network\n");
    fprintf(stderr, "  adv-prefix-high                       -- advertise high-priority prefix to thread network\n");
    fprintf(stderr, "  adv-prefix                            -- advertise prefix to thread network\n");
    fprintf(stderr, "  stop                                  -- stop advertising as SRP server\n");
    fprintf(stderr, "  get-ula                               -- fetch the current ULA prefix configured on the SRP server\n");
    fprintf(stderr, "  disable-srpl                          -- disable SRP replication\n");
    fprintf(stderr, "  add-prefix <ipv6 prefix>              -- add an OMR prefix\n");
    fprintf(stderr, "  remove-prefix <ipv6 prefix            -- remove an OMR prefix\n");
    fprintf(stderr, "  add-nat64-prefix <nat64 prefix>       -- add an nat64 prefix\n");
    fprintf(stderr, "  remove-nat64-prefix <nat64 prefix>    -- remove an nat64 prefix\n");
    fprintf(stderr, "  drop-srpl-connection                  -- drop existing srp replication connections\n");
    fprintf(stderr, "  undrop-srpl-connection                -- restart srp replication connections that were dropped \n");
    fprintf(stderr, "  drop-srpl-advertisement               -- stop advertising srpl service (but keep it around)\n");
    fprintf(stderr, "  undrop-srpl-advertisement             -- resume advertising srpl service\n");
    fprintf(stderr, "  start-dropping-push                   -- start repeatedly dropping any active push connections after 90 seconds\n");
    fprintf(stderr, "  start-breaking-time                   -- start breaking time validation on replicated SRP registrations\n");
    fprintf(stderr, "  set [variable] [value]                -- set the value of variable to value (e.g. set min-lease-time 100)\n");
    fprintf(stderr, "  block-anycast-service                 -- block advertising anycast service\n");
    fprintf(stderr, "  unblock-anycast-service               -- unblock advertising anycast service\n");
    fprintf(stderr, "  start-thread-shutdown                 -- start thread network shutdown\n");
    fprintf(stderr, "  advertise-winning-unicast-service     -- advertise a unicast service that wins over the current service\n");
    fprintf(stderr, "  browse <service>                      -- start an advertising_proxy_browse on the specified service\n");
    fprintf(stderr, "  resolve <name> <service>              -- start an advertising_proxy_resolve on the specified service instance\n");
    fprintf(stderr, "  need-service <service> <flag>         -- signal to srp-mdns-proxy that we need to discover a service\n");
    fprintf(stderr, "  need-instance <name> <service> <flag> -- signal to srp-mdns-proxy that we need to discover a service\n");
    fprintf(stderr, "  start-srp                             -- on thread device, enable srp registration\n");
#ifdef NOTYET
    fprintf(stderr, "  flush                                 -- flush all entries from the SRP proxy (for testing only)\n");
#endif
}

bool start_proxy = false;
bool flush_entries = false;
bool list_services = false;
bool block = false;
bool unblock = false;
bool regenerate_ula = false;
bool adv_prefix = false;
bool adv_prefix_high = false;
bool stop_proxy = false;
bool dump_stdin = false;
bool get_ula = false;
bool disable_srp_replication = false;
bool dso_test = false;
bool drop_srpl_connection;
bool undrop_srpl_connection;
bool drop_srpl_advertisement;
bool undrop_srpl_advertisement;
bool start_dropping_push_connections;
bool add_thread_prefix = false;
bool remove_thread_prefix = false;
bool add_nat64_prefix = false;
bool remove_nat64_prefix = false;
bool start_breaking_time_validation = false;
bool test_route_tracker = false;
bool block_anycast_service = false;
bool unblock_anycast_service = false;
bool start_thread_shutdown = false;
bool advertise_winning_unicast_service = false;
bool remove_unicast_service = false;
bool start_srp = false;
uint8_t prefix_buf[16];
#ifdef NOTYET
bool watch = false;
bool get = false;
#endif
variable_t *variables;
int num_permitted_interfaces;
char **permitted_interfaces;

static void
start_activities(void *context)
{
    advertising_proxy_error_type err = kDNSSDAdvertisingProxyStatus_NoError;;
    advertising_proxy_conn_ref cref = NULL;
    (void)context;

    if (err == kDNSSDAdvertisingProxyStatus_NoError && flush_entries) {
        err = advertising_proxy_flush_entries(&cref, main_queue, flushed_callback);
    }
    if (err == kDNSSDAdvertisingProxyStatus_NoError && (do_tcp_zero_test ||
                                                        do_tcp_fin_length || do_tcp_fin_payload)) {
        err = start_tcp_test();
    }
    if (err == kDNSSDAdvertisingProxyStatus_NoError && list_services) {
        err = advertising_proxy_get_service_list(&cref, main_queue, services_callback);
    }
    if (err == kDNSSDAdvertisingProxyStatus_NoError && block) {
        err = advertising_proxy_block_service(&cref, main_queue, block_callback);
    }
    if (err == kDNSSDAdvertisingProxyStatus_NoError && unblock) {
        err = advertising_proxy_unblock_service(&cref, main_queue, unblock_callback);
    }
    if (err == kDNSSDAdvertisingProxyStatus_NoError && regenerate_ula) {
        err = advertising_proxy_regenerate_ula(&cref, main_queue, regenerate_callback);
    }
    if (err == kDNSSDAdvertisingProxyStatus_NoError && adv_prefix) {
        err = advertising_proxy_advertise_prefix(&cref, adv_prefix_high, main_queue, prefix_advertise_callback);
    }
    if (err == kDNSSDAdvertisingProxyStatus_NoError && stop_proxy) {
        err = advertising_proxy_stop(&cref, main_queue, stop_callback);
    }
    if (err == kDNSSDAdvertisingProxyStatus_NoError && get_ula) {
        err = advertising_proxy_get_ula(&cref, main_queue, ula_callback);
    }
    if (err == kDNSSDAdvertisingProxyStatus_NoError && disable_srp_replication) {
        err = advertising_proxy_disable_srp_replication(&cref, main_queue, disable_srp_replication_callback);
    }
    if (err == kDNSSDAdvertisingProxyStatus_NoError && add_thread_prefix) {
        err = advertising_proxy_add_prefix(&cref, main_queue, add_prefix_callback, prefix_buf, sizeof(prefix_buf));
    }
    if (err == kDNSSDAdvertisingProxyStatus_NoError && remove_thread_prefix) {
        err = advertising_proxy_remove_prefix(&cref, main_queue, remove_prefix_callback, prefix_buf, sizeof(prefix_buf));
    }
    if (err == kDNSSDAdvertisingProxyStatus_NoError && add_nat64_prefix) {
        err = advertising_proxy_add_nat64_prefix(&cref, main_queue, add_nat64_prefix_callback, prefix_buf, sizeof(prefix_buf));
    }
    if (err == kDNSSDAdvertisingProxyStatus_NoError && remove_nat64_prefix) {
        err = advertising_proxy_remove_nat64_prefix(&cref, main_queue, remove_nat64_prefix_callback, prefix_buf, sizeof(prefix_buf));
    }
    if (err == kDNSSDAdvertisingProxyStatus_NoError && drop_srpl_connection) {
        err = advertising_proxy_drop_srpl_connection(&cref, main_queue, drop_srpl_connection_callback);
    }
    if (err == kDNSSDAdvertisingProxyStatus_NoError && undrop_srpl_connection) {
        err = advertising_proxy_undrop_srpl_connection(&cref, main_queue, undrop_srpl_connection_callback);
    }
    if (err == kDNSSDAdvertisingProxyStatus_NoError && drop_srpl_advertisement) {
        err = advertising_proxy_drop_srpl_advertisement(&cref, main_queue, drop_srpl_advertisement_callback);
    }
    if (err == kDNSSDAdvertisingProxyStatus_NoError && undrop_srpl_advertisement) {
        err = advertising_proxy_undrop_srpl_advertisement(&cref, main_queue, undrop_srpl_advertisement_callback);
    }
    if (err == kDNSSDAdvertisingProxyStatus_NoError && start_dropping_push_connections) {
        err = advertising_proxy_start_dropping_push_connections(&cref, main_queue, start_dropping_push_connections_callback);
    }
    if (err == kDNSSDAdvertisingProxyStatus_NoError && start_breaking_time_validation) {
        err = advertising_proxy_start_breaking_time_validation(&cref, main_queue, start_breaking_time_validation_callback);
    }
    if (err == kDNSSDAdvertisingProxyStatus_NoError && block_anycast_service) {
        err = advertising_proxy_block_anycast_service(&cref, main_queue, block_anycast_service_callback);
    }
    if (err == kDNSSDAdvertisingProxyStatus_NoError && unblock_anycast_service) {
        err = advertising_proxy_unblock_anycast_service(&cref, main_queue, unblock_anycast_service_callback);
    }
    if (err == kDNSSDAdvertisingProxyStatus_NoError && start_thread_shutdown) {
        err = advertising_proxy_start_thread_shutdown(&cref, main_queue, start_thread_shutdown_callback);
    }
    if (err == kDNSSDAdvertisingProxyStatus_NoError && test_route_tracker) {
        route_tracker_test_start(1000);
    }
    if (err == kDNSSDAdvertisingProxyStatus_NoError && advertise_winning_unicast_service) {
        start_advertising_winning_unicast_service();
    }
    if (err == kDNSSDAdvertisingProxyStatus_NoError && remove_unicast_service) {
        start_removing_unicast_service();
    }
    if (err == kDNSSDAdvertisingProxyStatus_NoError && advertise_winning_unicast_service) {
        start_advertising_winning_unicast_service();
    }
    if (err == kDNSSDAdvertisingProxyStatus_NoError && browse_service != NULL) {
        start_browsing_service();
    }
    if (err == kDNSSDAdvertisingProxyStatus_NoError && resolve_service != NULL) {
        start_resolving_service();
    }
    if (err == kDNSSDAdvertisingProxyStatus_NoError && need_service != NULL && need_name == NULL) {
        start_needing_service();
    }
    if (err == kDNSSDAdvertisingProxyStatus_NoError && need_service != NULL && need_name != NULL) {
        start_needing_instance();
    }
    if (err == kDNSSDAdvertisingProxyStatus_NoError && start_srp) {
        start_registrar();
    }
    if (err == kDNSSDAdvertisingProxyStatus_NoError && variables != NULL) {
        for (variable_t *variable = variables; variable != NULL; variable = variable->next) {
            err = advertising_proxy_set_variable(&cref, main_queue, set_variable_callback, variable, variable->name, variable->value);
            if (err != kDNSSDAdvertisingProxyStatus_NoError) {
                break;
            }
        }
    }
    if (err != kDNSSDAdvertisingProxyStatus_NoError) {
        exit(1);
    }
}

static void
dump_packet(void)
{
    ssize_t len;
    dns_message_t *message = NULL;
    dns_wire_t wire;

    len = read(0, &wire, sizeof(wire));
    if (len < 0) {
        ERROR("stdin: %s", strerror(errno));
        return;
    }
    if (len < DNS_HEADER_SIZE) {
        ERROR("stdin: too short: %zd bytes", len);
        return;
    }
    if (!dns_wire_parse(&message, &wire, (unsigned)len, true)) {
        fprintf(stderr, "DNS message parse failed\n");
        return;
    }
}

int
main(int argc, char **argv)
{
    int i;
    bool something = false;
    bool log_stderr = false;

    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "start")) {
			start_proxy = true;
            something = true;
            int j;
            for (j = i + 1; j < argc; j++) {
                if (!strcmp(argv[j], "-")) {
                    break;
                }
            }
            num_permitted_interfaces = j - i - 1;
            permitted_interfaces = argv + i + 1;
            i = j;
        } else if (!strcmp(argv[i], "tcp-zero")) {
            do_tcp_zero_test = true;
            something = true;
        } else if (!strcmp(argv[i], "tcp-fin-length")) {
            do_tcp_fin_length = true;
            something = true;
        } else if (!strcmp(argv[i], "tcp-fin-payload")) {
            do_tcp_fin_payload = true;
            something = true;
		} else if (!strcmp(argv[i], "flush")) {
            flush_entries = true;
            something = true;
		} else if (!strcmp(argv[i], "services")) {
            list_services = true;
            something = true;
		} else if (!strcmp(argv[i], "block")) {
            block = true;
            something = true;
		} else if (!strcmp(argv[i], "unblock")) {
            unblock = true;
            something = true;
		} else if (!strcmp(argv[i], "regenerate-ula")) {
            regenerate_ula = true;
            something = true;
        } else if (!strcmp(argv[i], "adv-prefix")) {
            adv_prefix = true;
            something = true;
        } else if (!strcmp(argv[i], "adv-prefix-high")) {
            adv_prefix = true;
            adv_prefix_high = true;
            something = true;
        } else if (!strcmp(argv[i], "stop")) {
            stop_proxy = true;
            something = true;
        } else if (!strcmp(argv[i], "dump")) {
            dump_packet();
            exit(0);
        } else if (!strcmp(argv[i], "get-ula")) {
            get_ula = true;
            something = true;
        } else if (!strcmp(argv[i], "disable-srpl")) {
            disable_srp_replication = true;
            something = true;
        } else if (!strcmp(argv[i], "add-prefix")) {
            if (i + 1 >= argc) {
                usage();
            }
            if (inet_pton(AF_INET6, argv[i + 1], prefix_buf) < 1) {
                fprintf(stderr, "Invalid ipv6 prefix %s.\n", argv[i + 1]);
                usage();
            } else {
                add_thread_prefix = true;
                something = true;
                i++;
            }
        } else if (!strcmp(argv[i], "remove-prefix")) {
            if (i + 1 >= argc) {
                usage();
            }
            if (inet_pton(AF_INET6, argv[i + 1], prefix_buf) < 1) {
                fprintf(stderr, "Invalid ipv6 prefix %s.\n", argv[i + 1]);
                usage();
            } else {
                remove_thread_prefix = true;
                something = true;
                i++;
            }
        } else if (!strcmp(argv[i], "add-nat64-prefix")) {
            if (i + 1 >= argc) {
                usage();
            }
            if (inet_pton(AF_INET6, argv[i + 1], prefix_buf) < 1) {
                fprintf(stderr, "Invalid ipv6 prefix %s.\n", argv[i + 1]);
                usage();
            } else {
                add_nat64_prefix = true;
                something = true;
                i++;
            }
        } else if (!strcmp(argv[i], "remove-nat64-prefix")) {
            if (i + 1 >= argc) {
                usage();
            }
            if (inet_pton(AF_INET6, argv[i + 1], prefix_buf) < 1) {
                fprintf(stderr, "Invalid ipv6 prefix %s.\n", argv[i + 1]);
                usage();
            } else {
                remove_nat64_prefix = true;
                something = true;
                i++;
            }
        } else if (!strcmp(argv[i], "browse")) {
            if (i + 1 >= argc) {
                usage();
            }
            browse_service = argv[i + 1];
            i++;
            something = true;
        } else if (!strcmp(argv[i], "resolve")) {
            if (i + 2 >= argc) {
                usage();
            }
            resolve_name = argv[i + 1];
            resolve_service = argv[i + 2];
            i += 2;
            something = true;
        } else if (!strcmp(argv[i], "need-service")) {
            if (i + 2 >= argc) {
                usage();
            }
            need_service = argv[i + 1];
            if (!strcmp(argv[i + 2], "true")) {
                needed_flag = true;
            } else {
                needed_flag = false;
            }
            i += 2;
            something = true;
        } else if (!strcmp(argv[i], "need-instance")) {
            if (i + 3 >= argc) {
                usage();
            }
            need_name = argv[i + 1];
            need_service = argv[i + 2];
            if (!strcmp(argv[i + 3], "true")) {
                needed_flag = true;
            } else {
                needed_flag = false;
            }
            i += 3;
            something = true;
        } else if (!strcmp(argv[i], "start-srp")) {
            start_srp = true;
            something = true;
        } else if (!strcmp(argv[i], "drop-srpl-connection")) {
            drop_srpl_connection = true;
            something = true;
        } else if (!strcmp(argv[i], "undrop-srpl-connection")) {
            undrop_srpl_connection = true;
            something = true;
        } else if (!strcmp(argv[i], "drop-srpl-advertisement")) {
            drop_srpl_advertisement = true;
            something = true;
        } else if (!strcmp(argv[i], "undrop-srpl-advertisement")) {
            undrop_srpl_advertisement = true;
            something = true;
        } else if (!strcmp(argv[i], "start-dropping-push")) {
            start_dropping_push_connections = true;
            something = true;
        } else if (!strcmp(argv[i], "start-breaking-time")) {
            start_breaking_time_validation = true;
            something = true;
        } else if (!strcmp(argv[i], "block-anycast-service")) {
            block_anycast_service = true;
            something = true;
        } else if (!strcmp(argv[i], "unblock-anycast-service")) {
            unblock_anycast_service = true;
            something = true;
        } else if (!strcmp(argv[i], "test-route-tracker")) {
            test_route_tracker = true;
            something = true;
        } else if (!strcmp(argv[i], "start-thread-shutdown")) {
            start_thread_shutdown = true;
            something = true;
        } else if (!strcmp(argv[i], "advertise-winning-unicast-service")) {
            advertise_winning_unicast_service = true;
            something = true;
        } else if (!strcmp(argv[i], "remove-unicast-service")) {
            remove_unicast_service = true;
            something = true;
        } else if (!strcmp(argv[i], "set")) {
            if (i + 2 >= argc) {
                usage();
            }
            variable_t *variable = calloc(1, sizeof(*variable));
            if (variable == NULL) {
                fprintf(stderr, "no memory for variable %s", argv[i + 1]);
                exit(1);
            }
            variable->name = argv[i + 1];
            variable->value = argv[i + 2];
            variable->next = variables;
            variables = variable;
            i += 2;
            something = true;
#ifdef NOTYET
		} else if (!strcmp(argv[i], "watch")) {
            fprintf(stderr, "Watching not implemented yet.\n");
            exit(1);
		} else if (!strcmp(argv[i], "get")) {
            fprintf(stderr, "Getting not implemented yet.\n");
            exit(1);
#endif
        } else if (!strcmp(argv[i], "--debug")) {
            OPENLOG("srputil", true);
            log_stderr = true;
        } else {
            usage();
            exit(1);
        }
    }

    if (!something) {
        usage();
        exit(1);
    }

    if (log_stderr == false) {
        OPENLOG("srputil", log_stderr);
    }

    ioloop_init();
    // Start the queue, //then// do the work
    ioloop_run_async(start_activities, NULL);
    ioloop();
}

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 108
// indent-tabs-mode: nil
// End:
