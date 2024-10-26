/* advertising_proxy_services.h
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
 * This file contains the implementation of the SRP Advertising Proxy management
 * API on MacOS, which is private API used to control and manage the advertising
 * proxy.
 */



#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <stdlib.h>

#include "srp.h"
#include "dns-msg.h"
#include "ioloop.h"
#include "cti-proto.h"

#include "adv-ctl-common.h"
#include "advertising_proxy_services.h"

#ifndef IOLOOP_MACOS
int adv_host_created;
int adv_host_finalized;
int advertising_proxy_conn_ref_created;
int advertising_proxy_conn_ref_finalized;
os_log_t global_os_log;
#endif

static void
adv_host_finalize(advertising_proxy_host_t *host)
{
    int i;

    free(host->hostname);
    free(host->regname);

    if (host->addresses != NULL) {
        for (i = 0; i < host->num_addresses; i++) {
            free(host->addresses[i].rdata);
        }
        free(host->addresses);
    }
    if (host->instances != NULL) {
        for (i = 0; i < host->num_instances; i++) {
            free(host->instances[i].instance_name);
            free(host->instances[i].service_type);
            free(host->instances[i].txt_data);
        }
    }
    free(host);
}

#define adv_host_allocate() adv_host_allocate_(__FILE__, __LINE__)
static advertising_proxy_host_t *
adv_host_allocate_(const char *file, int line)
{
    advertising_proxy_host_t *host = calloc(1, sizeof(*host));
    if (host == NULL) {
        return host;
    }
    RETAIN(host, adv_host);
    return host;
}



static void
adv_fd_finalize(void *context)
{
    advertising_proxy_conn_ref connection = context;
    connection->io_context = NULL;
    RELEASE_HERE(connection, advertising_proxy_conn_ref);
}

void
advertising_proxy_ref_dealloc(advertising_proxy_conn_ref conn_ref)
{
    if (conn_ref == NULL) {
        ERROR("advertising_proxy_ref_dealloc called with NULL advertising_proxy_conn_ref");
        return;
    }
    conn_ref->callback.callback = NULL;
    cti_connection_close(conn_ref);

    // This is releasing the caller's reference. We may still have an internal reference.
    RELEASE_HERE(conn_ref, advertising_proxy_conn_ref);
    ERROR("advertising_proxy_ref_dealloc successfully released conn_ref");
}

static void
adv_message_parse(cti_connection_t connection)
{
    int err = kDNSSDAdvertisingProxyStatus_NoError;
    int32_t status;

    cti_connection_parse_start(connection);
    if (!cti_connection_u16_parse(connection, &connection->message_type)) {
        err = kDNSSDAdvertisingProxyStatus_Disconnected;
        goto out;
    }
    if (connection->message_type != kDNSSDAdvertisingProxyResponse) {
        syslog(LOG_ERR, "adv_message_parse: unexpected message type %d", connection->message_type);
        err = kDNSSDAdvertisingProxyStatus_Disconnected;
        goto out;
    }
    if (!cti_connection_u32_parse(connection, (uint32_t *)&status)) {
        err = kDNSSDAdvertisingProxyStatus_Disconnected;
        goto out;
    }
    if (status != kDNSSDAdvertisingProxyStatus_NoError) {
        goto out;
    }
    if (connection->internal_callback != NULL) {
        (*connection->internal_callback)(connection, NULL, status);
    } else {
        if (!cti_connection_parse_done(connection)) {
            err = kDNSSDAdvertisingProxyStatus_Disconnected;
            goto out;
        }
        if (connection->callback.reply != NULL) {
            connection->callback.reply(connection, NULL, status);
        }
    }
    return;
out:
    cti_connection_close(connection);
    if (connection->callback.reply != NULL) {
        connection->callback.reply(connection, NULL, err);
    }
    return;
}

static void
adv_read_callback(io_t *UNUSED io, void *context)
{
    cti_connection_t connection = context;

    cti_read(connection, adv_message_parse);
}

static void
adv_service_list_callback(cti_connection_t connection, void *UNUSED object, cti_status_t UNUSED status)
{
    int i;
    advertising_proxy_host_t *host = NULL;
    uint32_t num_hosts;

	if (!cti_connection_u32_parse(connection, &num_hosts)) {
		ERROR("adv_ctl_list_callback: error parsing host count");
        goto fail;
	}
	for (i = 0; i < num_hosts; i++) {
        host = adv_host_allocate();
        if (host == NULL) {
            ERROR("adv_ctl_list_callback: no memory for host object");
            cti_connection_close(connection);
            goto fail;
        }
		if (!cti_connection_string_parse(connection, &host->hostname) ||
			!cti_connection_string_parse(connection, &host->regname) ||
			!cti_connection_u32_parse(connection, &host->lease_time) ||
            !cti_connection_bool_parse(connection, &host->removed) ||
            !cti_connection_u64_parse(connection, &host->server_id))
		{
			ERROR("adv_ctl_list_callback: unable to parse host info for host %d", i);
			cti_connection_close(connection);
            goto fail;
		}

		if (!cti_connection_u16_parse(connection, &host->num_addresses)) {
			ERROR("adv_ctl_list_callback: unable to parse host address count for host %s", host->hostname);
			cti_connection_close(connection);
            goto fail;
        }
        if (host->num_addresses > 0) {
            host->addresses = calloc(host->num_addresses, sizeof(advertising_proxy_host_address_t));
            if (host->addresses == NULL) {
					ERROR("adv_ctl_list_callback: no memory for addresses for host %s", host->hostname);
                    goto fail;
            }
        }
		for (i = 0; i < host->num_addresses; i++) {
            if (!cti_connection_u16_parse(connection, &host->addresses[i].rrtype) ||
                !cti_connection_data_parse(connection, (void **)&host->addresses[i].rdata, &host->addresses[i].rdlen))
            {
                ERROR("adv_ctl_list_callback: unable to parse address %d for host %s", i, host->hostname);
                goto fail;
            }
		}

        if (!cti_connection_u64_parse(connection, &host->server_id)) {
            ERROR("adv_ctl_list_callback: unable to parse stable server ID for host %s", host->hostname);
            goto fail;
        }

        if (!cti_connection_u16_parse(connection, &host->num_instances)) {
			ERROR("adv_ctl_list_callback: unable to parse host address count for host %s", host->hostname);
			cti_connection_close(connection);
            goto fail;
        }
        if (host->num_instances > 0) {
            host->instances = calloc(host->num_instances, sizeof(advertising_proxy_instance_t));
            if (host->instances == NULL) {
                ERROR("adv_ctl_list_callback: no memory for instances for host %s", host->hostname);
                    goto fail;
            }
        }
		for (i = 0; i < host->num_instances; i++) {
            if (!cti_connection_string_parse(connection, &host->instances[i].instance_name) ||
                !cti_connection_string_parse(connection, &host->instances[i].service_type) ||
                !cti_connection_u16_parse(connection, &host->instances[i].port) ||
                !cti_connection_data_parse(connection, (void **)&host->instances[i].txt_data, &host->instances[i].txt_len))
            {
                ERROR("adv_ctl_list_callback: unable to write address %d for host %s", i, host->hostname);
                goto fail;
            }
		}
        if (connection->callback.reply != NULL) {
            connection->callback.reply(connection, host, kDNSSDAdvertisingProxyStatus_NoError);
        }
        RELEASE_HERE(host, advertising_proxy_conn_ref);
        host = NULL;
    }

    if (!cti_connection_parse_done(connection)) {
    fail:
        if (connection->callback.reply != NULL) {
            connection->callback.reply(connection, NULL, kDNSSDAdvertisingProxyStatus_Disconnected);
        }
    } else {
        if (connection->callback.reply != NULL) {
            connection->callback.reply(connection, NULL, kDNSSDAdvertisingProxyStatus_NoError);
        }
    }
    if (host != NULL) {
        RELEASE_HERE(host, advertising_proxy_conn_ref);
    }
}

static void
adv_ula_callback(cti_connection_t connection, void *UNUSED object, cti_status_t UNUSED status)
{
    uint64_t ula_prefix;

	if (!cti_connection_u64_parse(connection, &ula_prefix)) {
		ERROR("error parsing ula prefix");
        goto fail;
	}

    if (!cti_connection_parse_done(connection)) {
    fail:
        if (connection->callback.reply != NULL) {
            connection->callback.reply(connection, NULL, kDNSSDAdvertisingProxyStatus_Disconnected);
        }
    } else {
        if (connection->callback.reply != NULL) {
            connection->callback.reply(connection, &ula_prefix, kDNSSDAdvertisingProxyStatus_NoError);
        }
    }
}

#define adv_send_command_with_data(ref, client_queue, command_name, command, app_callback, internal_callback, allocation, data, len)  \
    adv_send_command_(ref, client_queue, command_name, command, app_callback, internal_callback, allocation, __FILE__, __LINE__)
#define adv_send_command(ref, client_queue, command_name, command, app_callback, internal_callback, allocation)  \
    adv_send_command_(ref, client_queue, command_name, command, app_callback, internal_callback, allocation, __FILE__, __LINE__)
static advertising_proxy_error_type
adv_send_command_(advertising_proxy_conn_ref *ref, run_context_t client_queue, const char *command_name, int command,
                  advertising_proxy_reply app_callback, cti_internal_callback_t internal_callback, size_t allocation,
                  const char *file, int line)
{
    int fd;
    cti_connection_t connection;

    fd = cti_make_unix_socket(ADV_CTL_SERVER_SOCKET_NAME, sizeof(ADV_CTL_SERVER_SOCKET_NAME), false);
    if (fd < 0) {
        return kDNSSDAdvertisingProxyStatus_DaemonNotRunning;
    }
    connection = cti_connection_allocate(allocation);
    if (connection == NULL) {
        close(fd);
        return kDNSSDAdvertisingProxyStatus_NoMemory;
    }
    connection->fd = fd;
    RETAIN(connection, advertising_proxy_conn_ref);

    connection->io_context = ioloop_file_descriptor_create(connection->fd, connection, adv_fd_finalize);
    if (connection->io_context == NULL) {
        ERROR("cti_listen_callback: no memory for io context.");
		close(fd);
		RELEASE_HERE(connection, advertising_proxy_conn_ref);
        return kDNSSDAdvertisingProxyStatus_NoMemory;
    }
    ioloop_add_reader(connection->io_context, adv_read_callback);
    connection->callback.reply = app_callback;
    connection->internal_callback = internal_callback;
    cti_connection_message_create(connection, command, 2);
    cti_connection_message_send(connection);
    *ref = connection;
    return kDNSSDAdvertisingProxyStatus_NoError;
}


advertising_proxy_error_type
advertising_proxy_flush_entries(advertising_proxy_conn_ref *conn_ref,
                                run_context_t client_queue, advertising_proxy_reply callback)
{
    advertising_proxy_error_type errx;
    errx = adv_send_command(conn_ref, client_queue, "advertising_proxy_flush_entries",
                            kDNSSDAdvertisingProxyFlushEntries, callback, NULL, 0);
    return errx;
}

advertising_proxy_error_type
advertising_proxy_get_service_list(advertising_proxy_conn_ref *conn_ref,
                                   run_context_t client_queue, advertising_proxy_reply callback)
{
    advertising_proxy_error_type errx;
    errx = adv_send_command(conn_ref, client_queue, "advertising_proxy_get_service_list",
                            kDNSSDAdvertisingProxyListServices, callback, adv_service_list_callback, 4096);
    return errx;
}

advertising_proxy_error_type
advertising_proxy_block_service(advertising_proxy_conn_ref *conn_ref,
                                run_context_t client_queue, advertising_proxy_reply callback)
{
    advertising_proxy_error_type errx;
    errx = adv_send_command(conn_ref, client_queue, "advertising_proxy_block_service",
                            kDNSSDAdvertisingProxyBlockService, callback, NULL, 0);
    return errx;
}

advertising_proxy_error_type
advertising_proxy_unblock_service(advertising_proxy_conn_ref *conn_ref,
                                  run_context_t client_queue, advertising_proxy_reply callback)
{
    advertising_proxy_error_type errx;
    errx = adv_send_command(conn_ref, client_queue, "advertising_proxy_unblock_service",
                            kDNSSDAdvertisingProxyUnblockService, callback, NULL, 0);
    return errx;
}

advertising_proxy_error_type
advertising_proxy_regenerate_ula(advertising_proxy_conn_ref *conn_ref,
                                 run_context_t client_queue, advertising_proxy_reply callback)
{
    advertising_proxy_error_type errx;
    errx = adv_send_command(conn_ref, client_queue, "advertising_proxy_regenerate_ula",
                            kDNSSDAdvertisingProxyRegenerateULA, callback, NULL, 0);
    return errx;
}

advertising_proxy_error_type
advertising_proxy_advertise_prefix(advertising_proxy_conn_ref *conn_ref, bool high,
                                   run_context_t client_queue, advertising_proxy_reply callback)
{
    advertising_proxy_error_type errx;
    if (high) {
        errx = adv_send_command(conn_ref, client_queue, "advertising_proxy_advertise_prefix",
                                kDNSSDAdvertisingProxyAdvertisePrefixPriorityHigh, callback, NULL, 0);
    } else {
        errx = adv_send_command(conn_ref, client_queue, "advertising_proxy_advertise_prefix",
                                kDNSSDAdvertisingProxyAdvertisePrefix, callback, NULL, 0);
    }
    return errx;
}

advertising_proxy_error_type
advertising_proxy_add_prefix(advertising_proxy_conn_ref *conn_ref, run_context_t client_queue,
                             advertising_proxy_reply callback, const uint8_t *prefix_buf, size_t buf_len)
{
    advertising_proxy_error_type errx;
    errx = adv_send_command_with_data(conn_ref, client_queue, "advertising_proxy_add_prefix",
                                      kDNSSDAdvertisingProxyAddPrefix, callback, NULL, 0,
                                      prefix_buf, buf_len);
    return errx;
}

advertising_proxy_error_type
advertising_proxy_remove_prefix(advertising_proxy_conn_ref *conn_ref, run_context_t client_queue,
                                advertising_proxy_reply callback, const uint8_t *prefix_buf, size_t buf_len)
{
    advertising_proxy_error_type errx;
    errx = adv_send_command_with_data(conn_ref, client_queue, "advertising_proxy_remove_prefix",
                                      kDNSSDAdvertisingProxyRemovePrefix, callback, NULL, 0,
                                      prefix_buf, buf_len);
    return errx;
}

advertising_proxy_error_type
advertising_proxy_add_nat64_prefix(advertising_proxy_conn_ref *conn_ref, run_context_t client_queue,
                             advertising_proxy_reply callback, const uint8_t *prefix_buf, size_t buf_len)
{
    advertising_proxy_error_type errx;
    errx = adv_send_command_with_data(conn_ref, client_queue, "advertising_proxy_add_nat64_prefix",
                                      kDNSSDAdvertisingProxyAddNAT64Prefix, callback, NULL, 0,
                                      prefix_buf, buf_len);
    return errx;
}

advertising_proxy_error_type
advertising_proxy_remove_nat64_prefix(advertising_proxy_conn_ref *conn_ref, run_context_t client_queue,
                                advertising_proxy_reply callback, const uint8_t *prefix_buf, size_t buf_len)
{
    advertising_proxy_error_type errx;
    errx = adv_send_command_with_data(conn_ref, client_queue, "advertising_proxy_remove_nat64_prefix",
                                      kDNSSDAdvertisingProxyRemoveNAT64Prefix, callback, NULL, 0,
                                      prefix_buf, buf_len);
    return errx;
}

advertising_proxy_error_type
advertising_proxy_stop(advertising_proxy_conn_ref *conn_ref,
                       run_context_t client_queue, advertising_proxy_reply callback)
{
    advertising_proxy_error_type errx;
    errx = adv_send_command(conn_ref, client_queue, "advertising_proxy_stop",
                            kDNSSDAdvertisingProxyStop, callback, NULL, 0);
    return errx;
}

advertising_proxy_error_type
advertising_proxy_get_ula(advertising_proxy_conn_ref *conn_ref,
                          run_context_t client_queue, advertising_proxy_reply callback)
{
    advertising_proxy_error_type errx;
    errx = adv_send_command(conn_ref, client_queue, "advertising_proxy_get_ula",
                            kDNSSDAdvertisingProxyGetULA, callback, adv_ula_callback, 128);
    return errx;
}

advertising_proxy_error_type
advertising_proxy_disable_srp_replication(advertising_proxy_conn_ref *conn_ref,
                                          run_context_t client_queue, advertising_proxy_reply callback)
{
    advertising_proxy_error_type errx;
    errx = adv_send_command(conn_ref, client_queue, "advertising_proxy_disable_srp_replication",
                            kDNSSDAdvertisingProxyDisableReplication, callback, NULL, 0);
    return errx;
}

advertising_proxy_error_type
advertising_proxy_drop_srpl_connection(advertising_proxy_conn_ref *conn_ref,
                                       run_context_t client_queue, advertising_proxy_reply callback)
{
    advertising_proxy_error_type errx;
    errx = adv_send_command(conn_ref, client_queue, "advertising_proxy_drop_srpl_connection",
                            kDNSSDAdvertisingProxyDropSrplConnection, callback, NULL, 0);
    return errx;
}

advertising_proxy_error_type
advertising_proxy_undrop_srpl_connection(advertising_proxy_conn_ref *conn_ref,
                                         run_context_t client_queue, advertising_proxy_reply callback)
{
    advertising_proxy_error_type errx;
    errx = adv_send_command(conn_ref, client_queue, "advertising_proxy_undrop_srpl_connection",
                            kDNSSDAdvertisingProxyUndropSrplConnection, callback, NULL, 0);
    return errx;
}

advertising_proxy_error_type
advertising_proxy_drop_srpl_advertisement(advertising_proxy_conn_ref *conn_ref,
                                          run_context_t client_queue, advertising_proxy_reply callback)
{
    advertising_proxy_error_type errx;
    errx = adv_send_command(conn_ref, client_queue, "advertising_proxy_drop_srpl_advertisement",
                            kDNSSDAdvertisingProxyDropSrplAdvertisement, callback, NULL, 0);
    return errx;
}

advertising_proxy_error_type
advertising_proxy_undrop_srpl_advertisement(advertising_proxy_conn_ref *conn_ref,
                                            run_context_t client_queue, advertising_proxy_reply callback)
{
    advertising_proxy_error_type errx;
    errx = adv_send_command(conn_ref, client_queue, "advertising_proxy_disable_undrop_srpl_advertisement",
                            kDNSSDAdvertisingProxyUndropSrplAdvertisement, callback, NULL, 0);
    return errx;
}

advertising_proxy_error_type
advertising_proxy_start_dropping_push_connections(advertising_proxy_conn_ref *conn_ref,
                                                  run_context_t client_queue, advertising_proxy_reply callback)
{
    advertising_proxy_error_type errx;
    errx = adv_send_command(conn_ref, client_queue, "advertising_proxy_disable_start_dropping_push_connections",
                            kDNSSDAdvertisingProxyStartDroppingPushConnections, callback, NULL, 0);
    return errx;
}

advertising_proxy_error_type
advertising_proxy_start_breaking_time_validation(advertising_proxy_conn_ref *conn_ref,
                                                 run_context_t client_queue, advertising_proxy_reply callback)
{
    advertising_proxy_error_type errx;
    errx = adv_send_command(conn_ref, client_queue, "advertising_proxy_start_breaking_time_validation",
                            kDNSSDAdvertisingProxyStartBreakingTimeValidation, callback, NULL, 0);
    return errx;
}

advertising_proxy_error_type
advertising_proxy_block_anycast_service(advertising_proxy_conn_ref *conn_ref,
                                        run_context_t client_queue, advertising_proxy_reply callback)
{
    advertising_proxy_error_type errx;
    errx = adv_send_command(conn_ref, client_queue, "advertising_proxy_block_anycast_service",
                            kDNSSDAdvertisingProxyBlockAnycastService, callback, NULL, 0);
    return errx;
}

advertising_proxy_error_type
advertising_proxy_unblock_anycast_service(advertising_proxy_conn_ref *conn_ref,
                                          run_context_t client_queue, advertising_proxy_reply callback)
{
    advertising_proxy_error_type errx;
    errx = adv_send_command(conn_ref, client_queue, "advertising_proxy_disable_unblock_anycast_service",
                            kDNSSDAdvertisingProxyUnblockAnycastService, callback, NULL, 0);
    return errx;
}

advertising_proxy_error_type
advertising_proxy_start_thread_shutdown(advertising_proxy_conn_ref *conn_ref,
                                        run_context_t client_queue, advertising_proxy_reply callback)
{
    advertising_proxy_error_type errx;
    errx = adv_send_command(conn_ref, client_queue, "advertising_proxy_start_thread_shutdown",
                            kDNSSDAdvertisingProxyStartThreadShutdown, callback, NULL, 0);
    return errx;
}

static void
adv_set_variable_callback(advertising_proxy_conn_ref conn_ref, xpc_object_t *response, advertising_proxy_error_type status)
{
    if (conn_ref->app_response_callback != NULL) {
        conn_ref->app_response_callback(conn_ref, conn_ref->context, response, status);
    }
}

advertising_proxy_error_type
advertising_proxy_set_variable(advertising_proxy_conn_ref *conn_ref,
                               run_context_t client_queue, advertising_proxy_response_reply callback, void *context,
                               const char *name, const char *value)
{
    advertising_proxy_error_type errx;
    size_t name_len = strlen(name), value_len = strlen(value);
    size_t total_len = name_len + value_len + 2;
    uint8_t *buf = malloc(total_len);
    if (buf == NULL) {
        return kDNSSDAdvertisingProxyStatus_NoMemory;
    }
    memcpy(buf, name, name_len + 1);
    memcpy(buf + name_len + 1, value, value_len + 1);
    errx = adv_send_command_with_data(conn_ref, client_queue, "advertising_proxy_get_service_list",
                                      kDNSSDAdvertisingProxySetVariable, NULL, adv_set_variable_callback,
                                      0, buf, total_len);
    free(buf);
    if (errx == kDNSSDAdvertisingProxyStatus_NoError) {
        (*conn_ref)->context = context;
        (*conn_ref)->app_response_callback = callback;
    }
    return errx;
}

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 120
// indent-tabs-mode: nil
// End:
