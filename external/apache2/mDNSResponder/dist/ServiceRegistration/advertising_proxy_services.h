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
 * This file contains definitions for the SRP Advertising Proxy management
 * API on MacOS, which is private API used to control and manage the advertising
 * proxy.
 */

#ifndef DNSSD_PROXY_SERVICES_H
#define DNSSD_PROXY_SERVICES_H

#if !defined(__BEGIN_DECLS)
    #if defined(__cplusplus)
        #define __BEGIN_DECLS               extern "C" {
        #define __END_DECLS                     }
    #else
        #define __BEGIN_DECLS
        #define __END_DECLS
    #endif
#endif

__BEGIN_DECLS
typedef void *run_context_t;
typedef struct _cti_connection_t *advertising_proxy_conn_ref;
#define ADV_CTL_SERVER_SOCKET_NAME "/var/run/adv-ctl-server-socket"
#define RCHAR char
#define RUCHAR uint8_t

typedef struct advertising_proxy_subscription advertising_proxy_subscription_t;

typedef struct advertising_proxy_host_address {
    uint16_t rrtype;
    RUCHAR *NULLABLE rdata;
    uint16_t rdlen;
} advertising_proxy_host_address_t;

typedef struct advertising_proxy_instance {
    RCHAR *NULLABLE instance_name;
    RCHAR *NULLABLE service_type;
    RCHAR *NULLABLE reg_type;
    uint16_t port;
    RUCHAR *NULLABLE txt_data;
    uint16_t txt_len;
} advertising_proxy_instance_t;

typedef struct advertising_proxy_host {
    int ref_count;
    RCHAR *NULLABLE hostname;
    RCHAR *NULLABLE regname;
    uint16_t num_addresses;
    uint32_t lease_time;
    advertising_proxy_host_address_t *NULLABLE addresses;
    uint16_t num_instances;
    advertising_proxy_instance_t *NULLABLE instances;
    bool removed;
    uint64_t server_id;
} advertising_proxy_host_t;

#if (defined(__GNUC__) && (__GNUC__ >= 4))
#define DNS_SERVICES_EXPORT __attribute__((visibility("default")))
#else
#define DNS_SERVICES_EXPORT
#endif

typedef enum
{
    kDNSSDAdvertisingProxyStatus_NoError                   =  0,
    kDNSSDAdvertisingProxyStatus_UnknownErr                = -65537,   /* 0xFFFE FFFF */
    kDNSSDAdvertisingProxyStatus_NoMemory                  = -65539,   /* No Memory   */
    kDNSSDAdvertisingProxyStatus_BadParam                  = -65540,   /* Client passed invalid arg */
    kDNSSDAdvertisingProxyStatus_Invalid                   = -65549,   /* Invalid message */
    kDNSSDAdvertisingProxyStatus_DaemonNotRunning          = -65563,   /* Daemon not running */
    kDNSSDAdvertisingProxyStatus_Disconnected              = -65569,   /* Daemon disconnected */
    kDNSSDAdvertisingProxyStatus_NotPermitted              = -65571
} advertising_proxy_error_type;

// Register for notification that TLS key has changed
#define kDNSSDAdvertisingProxyTLSKeyUpdateNotification "com.apple.srp-mdns-proxy.tls-key-update"

/*********************************************************************************************
 *
 *  DNSSD Advertising Proxy control/management library functions
 *
 *********************************************************************************************/

/* advertising_proxy_reply: Callback from all DNSSD Advertising proxy library functions
 *
 * advertising_proxy_reply() parameters:
 *
 * conn_ref:                   The advertising_proxy_conn_ref initialized by the library function.
 *
 * errCode:                    Will be kDNSSDAdvertisingProxy_NoError on success, otherwise will indicate the
 *                             failure that occurred.
 *
 */

typedef void (*advertising_proxy_reply)
(
    advertising_proxy_conn_ref NULLABLE conn_ref,
    void *                     NULLABLE data,
    advertising_proxy_error_type        errCode
);

/* advertising_proxy_response_reply: Callback from all DNSSD Advertising proxy library functions
 *
 * advertising_proxy_response_reply() parameters:
 *
 * conn_ref:                   The advertising_proxy_conn_ref initialized by the library function.  Call advertising_proxy_
 *
 * context:                    context passed to advertising proxy call
 *
 * response:                   Any data returned by the advertising proxy in response to the request.
 *
 * errCode:                    Will be kDNSSDAdvertisingProxy_NoError on success, otherwise will indicate the
 *                             failure that occurred.
 *
 */

typedef void (*advertising_proxy_response_reply)
(
    advertising_proxy_conn_ref  NULLABLE conn_ref,
    void *                      NULLABLE context;
    void *                      NULLABLE response,
    advertising_proxy_error_type         errCode
);



/* advertising_proxy_flush_entries
 *
 * Flushes any host entries that have been registered with the advertising proxy.   For testing only:
 * this is never the right thing to do in production.
 *
 * advertising_proxy_flush_entries() Parameters:
 *
 * conn_ref:                  A pointer to advertising_proxy_conn_ref that is initialized to NULL.
 *                            If the call succeeds it will be initialized to a non-NULL value.
 *                            The same conn_ref can be used for more than one call.
 *
 * clientq:                   Queue the client wants to schedule the callback on (Note: Must not be NULL)
 *
 * callback:                  CallBack function for the client that indicates success or failure.
 *                            Callback is not called until either the command has failed, or has completed.
 *
 * return value:              Returns kDNSSDAdvertisingProxy_NoError when no error otherwise returns an
 *                            error code indicating the error that occurred. Note: A return value of
 *                            kDNSSDAdvertisingProxy_NoError does not mean that DNSSD Advertising Proxy host
 *                            table was successfully flushed. The callback may asynchronously return an
 *                            error (such as kDNSSDAdvertisingProxy_DaemonNotRunning)
 *
 */

DNS_SERVICES_EXPORT
advertising_proxy_error_type advertising_proxy_flush_entries
(
    advertising_proxy_conn_ref NONNULL *NULLABLE conn_ref,
    run_context_t                        NONNULL clientq,
    advertising_proxy_reply             NULLABLE callback
);

/* advertising_proxy_get_service_list
 *
 * Returns a list of registered services on the advertising proxy.
 *
 * advertising_proxy_get_service_list() Parameters:
 *
 * conn_ref:                  A pointer to advertising_proxy_conn_ref that is initialized to NULL.
 *                            If the call succeeds it will be initialized to a non-NULL value.
 *                            The same conn_ref can be used for more than one call.
 *
 * clientq:                   Queue the client wants to schedule the callback on (Note: Must not be NULL)
 *
 * callback:                  CallBack function for the client that indicates success or failure.
 *                            Callback is not called until either the command has failed, or has completed.
 *
 * return value:              Returns kDNSSDAdvertisingProxy_NoError when no error otherwise returns an
 *                            error code indicating the error that occurred. Note: A return value of
 *                            kDNSSDAdvertisingProxy_NoError does not mean that DNSSD Advertising Proxy host
 *                            table was successfully flushed. The callback may asynchronously return an
 *                            error (such as kDNSSDAdvertisingProxy_DaemonNotRunning)
 *
 */

DNS_SERVICES_EXPORT
advertising_proxy_error_type advertising_proxy_get_service_list
(
    advertising_proxy_conn_ref NONNULL *NULLABLE conn_ref,
    run_context_t                        NONNULL clientq,
    advertising_proxy_reply             NULLABLE callback
);

/* advertising_proxy_block_service
 *
 * For testing, block advertisement of SRP service on the thread network.
 *
 * advertising_proxy_block_service() Parameters:
 *
 * conn_ref:                  A pointer to advertising_proxy_conn_ref that is initialized to NULL.
 *                            If the call succeeds it will be initialized to a non-NULL value.
 *                            The same conn_ref can be used for more than one call.
 *
 * clientq:                   Queue the client wants to schedule the callback on (Note: Must not be NULL)
 *
 * callback:                  CallBack function for the client that indicates success or failure.
 *                            Callback is not called until either the command has failed, or has completed.
 *
 * return value:              Returns kDNSSDAdvertisingProxy_NoError when no error otherwise returns an
 *                            error code indicating the error that occurred. Note: A return value of
 *                            kDNSSDAdvertisingProxy_NoError does not mean that DNSSD Advertising Proxy host
 *                            table was successfully flushed. The callback may asynchronously return an
 *                            error (such as kDNSSDAdvertisingProxy_DaemonNotRunning)
 *
 */

DNS_SERVICES_EXPORT
advertising_proxy_error_type advertising_proxy_block_service
(
    advertising_proxy_conn_ref NONNULL *NULLABLE conn_ref,
    run_context_t                        NONNULL clientq,
    advertising_proxy_reply             NULLABLE callback
);

/* advertising_proxy_unblock_service
 *
 * For testing, unblock advertisement of SRP service on the thread network.
 *
 * advertising_proxy_unblock_service() Parameters:
 *
 * conn_ref:                  A pointer to advertising_proxy_conn_ref that is initialized to NULL.
 *                            If the call succeeds it will be initialized to a non-NULL value.
 *                            The same conn_ref can be used for more than one call.
 *
 * clientq:                   Queue the client wants to schedule the callback on (Note: Must not be NULL)
 *
 * callback:                  CallBack function for the client that indicates success or failure.
 *                            Callback is not called until either the command has failed, or has completed.
 *
 * return value:              Returns kDNSSDAdvertisingProxy_NoError when no error otherwise returns an
 *                            error code indicating the error that occurred. Note: A return value of
 *                            kDNSSDAdvertisingProxy_NoError does not mean that DNSSD Advertising Proxy host
 *                            table was successfully flushed. The callback may asynchronously return an
 *                            error (such as kDNSSDAdvertisingProxy_DaemonNotRunning)
 *
 */

DNS_SERVICES_EXPORT
advertising_proxy_error_type advertising_proxy_unblock_service
(
    advertising_proxy_conn_ref NONNULL *NULLABLE conn_ref,
    run_context_t                        NONNULL clientq,
    advertising_proxy_reply             NULLABLE callback
);

/* advertising_proxy_regenerate_ula
 *
 * For testing, generate a new ULA prefix
 *
 * advertising_proxy_regenerate_ula() Parameters:
 *
 * conn_ref:                  A pointer to advertising_proxy_conn_ref that is initialized to NULL.
 *                            If the call succeeds it will be initialized to a non-NULL value.
 *                            The same conn_ref can be used for more than one call.
 *
 * clientq:                   Queue the client wants to schedule the callback on (Note: Must not be NULL)
 *
 * callback:                  CallBack function for the client that indicates success or failure.
 *                            Callback is not called until either the command has failed, or has completed.
 *
 * return value:              Returns kDNSSDAdvertisingProxy_NoError when no error otherwise returns an
 *                            error code indicating the error that occurred. Note: A return value of
 *                            kDNSSDAdvertisingProxy_NoError does not mean that DNSSD Advertising Proxy host
 *                            table was successfully flushed. The callback may asynchronously return an
 *                            error (such as kDNSSDAdvertisingProxy_DaemonNotRunning)
 *
 */

DNS_SERVICES_EXPORT
advertising_proxy_error_type advertising_proxy_regenerate_ula
(
    advertising_proxy_conn_ref NONNULL *NULLABLE conn_ref,
    run_context_t                        NONNULL clientq,
    advertising_proxy_reply             NULLABLE callback
);

/* advertising_proxy_advertise_prefix
 *
 * For testing, advertise it's own prefix to thread network
 *
 * advertising_proxy_advertise_prefix() Parameters:
 *
 * conn_ref:                  A pointer to advertising_proxy_conn_ref that is initialized to NULL.
 *                            If the call succeeds it will be initialized to a non-NULL value.
 *                            The same conn_ref can be used for more than one call.
 *
 * high:                      If true, advertise the prefix as high priority.
 *
 * clientq:                   Queue the client wants to schedule the callback on (Note: Must not be NULL)
 *
 * callback:                  Callback function for the client that indicates success or failure.
 *                            Callback is not called until either the command has failed, or has completed.
 *
 * return value:              Returns kDNSSDAdvertisingProxy_NoError when no error otherwise returns an
 *                            error code indicating the error that occurred. Note: A return value of
 *                            kDNSSDAdvertisingProxy_NoError does not mean that the prefix advertising
 *                            was successful. The callback may asynchronously return an
 *                            error (such as kDNSSDAdvertisingProxy_DaemonNotRunning)
 *
 */

DNS_SERVICES_EXPORT
advertising_proxy_error_type advertising_proxy_advertise_prefix
(
    advertising_proxy_conn_ref NONNULL *NULLABLE conn_ref,
    bool                                         high,
    run_context_t                        NONNULL clientq,
    advertising_proxy_reply             NULLABLE callback
);

/* advertising_proxy_add_prefix
 *
 * For testing, stop advertising service
 *
 * advertising_proxy_add_prefix() Parameters:
 *
 * conn_ref:                  A pointer to advertising_proxy_conn_ref that is initialized to NULL.
 *                            If the call succeeds it will be initialized to a non-NULL value.
 *                            The same conn_ref can be used for more than one call.
 *
 * clientq:                   Queue the client wants to schedule the callback on (Note: Must not be NULL).
 *
 * callback:                  Callback function for the client that indicates success or failure.
 *                            Callback is not called until either the command has failed, or has completed.
 *
 * prefix_buf:                 Prefix to be added.
 *
 * buf_len:                    Length of the prefixbuf.
 *
 * return value:              Returns kDNSSDAdvertisingProxy_NoError when no error otherwise returns an
 *                            error code indicating the error that occurred. Note: A return value of
 *                            kDNSSDAdvertisingProxy_NoError does not mean that the service advertising
 *                            was stopped successfully. The callback may asynchronously return an
 *                            error (such as kDNSSDAdvertisingProxy_DaemonNotRunning).
 *
 */

DNS_SERVICES_EXPORT
advertising_proxy_error_type advertising_proxy_add_prefix
(
    advertising_proxy_conn_ref NONNULL *NULLABLE conn_ref,
    run_context_t                        NONNULL clientq,
    advertising_proxy_reply             NULLABLE callback,
    const uint8_t                       *NONNULL prefix_buf,
    size_t                                       buf_len
);

/* advertising_proxy_remove_prefix
 *
 * For testing, stop advertising service
 *
 * advertising_proxy_remove_prefix() Parameters:
 *
 * conn_ref:                  A pointer to advertising_proxy_conn_ref that is initialized to NULL.
 *                            If the call succeeds it will be initialized to a non-NULL value.
 *                            The same conn_ref can be used for more than one call.
 *
 * clientq:                   Queue the client wants to schedule the callback on (Note: Must not be NULL).
 *
 * callback:                  Callback function for the client that indicates success or failure.
 *                            Callback is not called until either the command has failed, or has completed.
 *
 * prefix_buf:                Prefix to be added.
 *
 * buf_len:                   Length of the prefixbuf.
 *
 * return value:              Returns kDNSSDAdvertisingProxy_NoError when no error otherwise returns an
 *                            error code indicating the error that occurred. Note: A return value of
 *                            kDNSSDAdvertisingProxy_NoError does not mean that the service advertising
 *                            was stopped successfully. The callback may asynchronously return an
 *                            error (such as kDNSSDAdvertisingProxy_DaemonNotRunning).
 *
 */

/* advertising_proxy_add_nat64_prefix
 *
 * For testing, add nat64 prefix
 *
 * advertising_proxy_add_nat64_prefix() Parameters:
 *
 * conn_ref:                  A pointer to advertising_proxy_conn_ref that is initialized to NULL.
 *                            If the call succeeds it will be initialized to a non-NULL value.
 *                            The same conn_ref can be used for more than one call.
 *
 * clientq:                   Queue the client wants to schedule the callback on (Note: Must not be NULL).
 *
 * callback:                  Callback function for the client that indicates success or failure.
 *                            Callback is not called until either the command has failed, or has completed.
 *
 * prefix_buf:                Prefix to be added.
 *
 * buf_len:                   Length of the prefixbuf.
 *
 * return value:              Returns kDNSSDAdvertisingProxy_NoError when no error otherwise returns an
 *                            error code indicating the error that occurred. Note: A return value of
 *                            kDNSSDAdvertisingProxy_NoError does not mean that the service advertising
 *                            was stopped successfully. The callback may asynchronously return an
 *                            error (such as kDNSSDAdvertisingProxy_DaemonNotRunning).
 *
 */

DNS_SERVICES_EXPORT
advertising_proxy_error_type advertising_proxy_add_nat64_prefix
(
    advertising_proxy_conn_ref NONNULL *NULLABLE conn_ref,
    run_context_t                        NONNULL clientq,
    advertising_proxy_reply             NULLABLE callback,
    const uint8_t                       *NONNULL prefix_buf,
    size_t                                       buf_len
);

/* advertising_proxy_remove_nat64_prefix
 *
 * For testing, remove nat64 prefix
 *
 * advertising_proxy_remove_nat64_prefix() Parameters:
 *
 * conn_ref:                  A pointer to advertising_proxy_conn_ref that is initialized to NULL.
 *                            If the call succeeds it will be initialized to a non-NULL value.
 *                            The same conn_ref can be used for more than one call.
 *
 * clientq:                   Queue the client wants to schedule the callback on (Note: Must not be NULL).
 *
 * callback:                  Callback function for the client that indicates success or failure.
 *                            Callback is not called until either the command has failed, or has completed.
 *
 * prefix_buf:                Prefix to be removed.
 *
 * buf_len:                   Length of the prefixbuf.
 *
 * return value:              Returns kDNSSDAdvertisingProxy_NoError when no error otherwise returns an
 *                            error code indicating the error that occurred. Note: A return value of
 *                            kDNSSDAdvertisingProxy_NoError does not mean that the service advertising
 *                            was stopped successfully. The callback may asynchronously return an
 *                            error (such as kDNSSDAdvertisingProxy_DaemonNotRunning).
 *
 */
DNS_SERVICES_EXPORT
advertising_proxy_error_type advertising_proxy_remove_nat64_prefix
(
    advertising_proxy_conn_ref NONNULL *NULLABLE conn_ref,
    run_context_t                        NONNULL clientq,
    advertising_proxy_reply             NULLABLE callback,
    const uint8_t                       *NONNULL prefix_buf,
    size_t                                       buf_len
);

DNS_SERVICES_EXPORT
advertising_proxy_error_type advertising_proxy_remove_prefix
(
    advertising_proxy_conn_ref NONNULL *NULLABLE conn_ref,
    run_context_t                        NONNULL clientq,
    advertising_proxy_reply             NULLABLE callback,
    const uint8_t                       *NONNULL prefix_buf,
    size_t                                       buf_len
);

/* advertising_proxy_stop
 *
 * For testing, stop advertising service
 *
 * advertising_proxy_stop() Parameters:
 *
 * conn_ref:                  A pointer to advertising_proxy_conn_ref that is initialized to NULL.
 *                            If the call succeeds it will be initialized to a non-NULL value.
 *                            The same conn_ref can be used for more than one call.
 *
 * clientq:                   Queue the client wants to schedule the callback on (Note: Must not be NULL).
 *
 * callback:                  Callback function for the client that indicates success or failure.
 *                            Callback is not called until either the command has failed, or has completed.
 *
 * return value:              Returns kDNSSDAdvertisingProxy_NoError when no error otherwise returns an
 *                            error code indicating the error that occurred. Note: A return value of
 *                            kDNSSDAdvertisingProxy_NoError does not mean that the service advertising
 *                            was stopped successfully. The callback may asynchronously return an
 *                            error (such as kDNSSDAdvertisingProxy_DaemonNotRunning).
 *
 */

DNS_SERVICES_EXPORT
advertising_proxy_error_type advertising_proxy_stop
(
    advertising_proxy_conn_ref NONNULL *NULLABLE conn_ref,
    run_context_t                        NONNULL clientq,
    advertising_proxy_reply             NULLABLE callback
);

/* advertising_proxy_get_ula
 *
 * For testing, advertise it's own prefix to thread network
 *
 * advertising_proxy_get_ula() Parameters:
 *
 * conn_ref:                  A pointer to advertising_proxy_conn_ref that is initialized to NULL.
 *                            If the call succeeds it will be initialized to a non-NULL value.
 *                            The same conn_ref can be used for more than one call.
 *
 * clientq:                   Queue the client wants to schedule the callback on (Note: Must not be NULL)
 *
 * callback:                  Callback function for the client that indicates success or failure.
 *                            Callback is not called until either the command has failed, or has completed.
 *                            The response object in the callback is a pointer to a uint64_t containing the
 *                            prefix in host byte order.
 *
 * return value:              Returns kDNSSDAdvertisingProxy_NoError when no error otherwise returns an
 *                            error code indicating the error that occurred. Note: A return value of
 *                            kDNSSDAdvertisingProxy_NoError does not mean that the prefix advertising
 *                            was successful. The callback may asynchronously return an
 *                            error (such as kDNSSDAdvertisingProxy_DaemonNotRunning)
 *
 */

DNS_SERVICES_EXPORT
advertising_proxy_error_type advertising_proxy_get_ula
(
    advertising_proxy_conn_ref NONNULL *NULLABLE conn_ref,
    run_context_t                        NONNULL clientq,
    advertising_proxy_reply             NULLABLE callback
);

/* advertising_proxy_disable_srp_replication
 *
 * For testing, disable SRP replication.
 *
 * advertising_proxy_disable_srp_replication() Parameters:
 *
 * conn_ref:                  A pointer to advertising_proxy_conn_ref that is initialized to NULL.
 *                            If the call succeeds it will be initialized to a non-NULL value.
 *                            The same conn_ref can be used for more than one call.
 *
 * clientq:                   Queue the client wants to schedule the callback on (Note: Must not be NULL)
 *
 * callback:                  Callback function for the client that indicates success or failure.
 *                            Callback is not called until either the command has failed, or has completed.
 *
 * return value:              Returns kDNSSDAdvertisingProxy_NoError when no error otherwise returns an
 *                            error code indicating the error that occurred. Note: A return value of
 *                            kDNSSDAdvertisingProxy_NoError does not mean that srp replication was disabled
 *                            successfully. The callback may asynchronously return an
 *                            error (such as kDNSSDAdvertisingProxy_DaemonNotRunning)
 *
 */

DNS_SERVICES_EXPORT
advertising_proxy_error_type advertising_proxy_disable_srp_replication
(
    advertising_proxy_conn_ref NONNULL *NULLABLE conn_ref,
    run_context_t                        NONNULL clientq,
    advertising_proxy_reply             NULLABLE callback
);

/* advertising_proxy_undrop_srpl_advertisement
 *
 * For testing, undrop_srpl_advertisement
 *
 * advertising_proxy_undrop_srpl_advertisement() Parameters:
 *
 * conn_ref:                  A pointer to advertising_proxy_conn_ref that is initialized to NULL.
 *                            If the call succeeds it will be initialized to a non-NULL value.
 *                            The same conn_ref can be used for more than one call.
 *
 * clientq:                   Queue the client wants to schedule the callback on (Note: Must not be NULL)
 *
 * callback:                  Callback function for the client that indicates success or failure.
 *                            Callback is not called until either the command has failed, or has completed.
 *
 * return value:              Returns kDNSSDAdvertisingProxy_NoError when no error otherwise returns an
 *                            error code indicating the error that occurred. Note: A return value of
 *                            kDNSSDAdvertisingProxy_NoError does not mean that srp replication advertisements
 *                            were successfully resumed. The callback may asynchronously return an
 *                            error (such as kDNSSDAdvertisingProxy_DaemonNotRunning)
 *
 */

DNS_SERVICES_EXPORT
advertising_proxy_error_type advertising_proxy_undrop_srpl_advertisement
(
    advertising_proxy_conn_ref NONNULL *NULLABLE conn_ref,
    run_context_t                        NONNULL clientq,
    advertising_proxy_reply             NULLABLE callback
);

/* advertising_proxy_undrop_srpl_connection
 *
 * For testing, restart all dropped srpl connections
 *
 * advertising_proxy_undrop_srpl_connection() Parameters:
 *
 * conn_ref:                  A pointer to advertising_proxy_conn_ref that is initialized to NULL.
 *                            If the call succeeds it will be initialized to a non-NULL value.
 *                            The same conn_ref can be used for more than one call.
 *
 * clientq:                   Queue the client wants to schedule the callback on (Note: Must not be NULL)
 *
 * callback:                  Callback function for the client that indicates success or failure.
 *                            Callback is not called until either the command has failed, or has completed.
 *
 * return value:              Returns kDNSSDAdvertisingProxy_NoError when no error otherwise returns an
 *                            error code indicating the error that occurred. Note: A return value of
 *                            kDNSSDAdvertisingProxy_NoError does not mean that srp replication connections
 *                            were successfully resumed. The callback may asynchronously return an
 *                            error (such as kDNSSDAdvertisingProxy_DaemonNotRunning)
 *
 */

DNS_SERVICES_EXPORT
advertising_proxy_error_type advertising_proxy_undrop_srpl_connection
(
    advertising_proxy_conn_ref NONNULL *NULLABLE conn_ref,
    run_context_t                        NONNULL clientq,
    advertising_proxy_reply             NULLABLE callback
);

/* advertising_proxy_unblock_anycast_service
 *
 * For testing, unblock_anycast_service
 *
 * advertising_proxy_unblock_anycast_service() Parameters:
 *
 * conn_ref:                  A pointer to advertising_proxy_conn_ref that is initialized to NULL.
 *                            If the call succeeds it will be initialized to a non-NULL value.
 *                            The same conn_ref can be used for more than one call.
 *
 * clientq:                   Queue the client wants to schedule the callback on (Note: Must not be NULL)
 *
 * callback:                  Callback function for the client that indicates success or failure.
 *                            Callback is not called until either the command has failed, or has completed.
 *
 * return value:              Returns kDNSSDAdvertisingProxy_NoError when no error otherwise returns an
 *                            error code indicating the error that occurred. Note: A return value of
 *                            kDNSSDAdvertisingProxy_NoError does not mean that anycast service advertisements
 *                            were successfully unblocked. The callback may asynchronously return an
 *                            error (such as kDNSSDAdvertisingProxy_DaemonNotRunning)
 *
 */

DNS_SERVICES_EXPORT
advertising_proxy_error_type advertising_proxy_unblock_anycast_service
(
    advertising_proxy_conn_ref NONNULL *NULLABLE conn_ref,
    run_context_t                        NONNULL clientq,
    advertising_proxy_reply             NULLABLE callback
);

/* advertising_proxy_drop_srpl_advertisement
 *
 * For testing, drop all srpl advertisements
 *
 * advertising_proxy_drop_srpl_advertisement() Parameters:
 *
 * conn_ref:                  A pointer to advertising_proxy_conn_ref that is initialized to NULL.
 *                            If the call succeeds it will be initialized to a non-NULL value.
 *                            The same conn_ref can be used for more than one call.
 *
 * clientq:                   Queue the client wants to schedule the callback on (Note: Must not be NULL)
 *
 * callback:                  Callback function for the client that indicates success or failure.
 *                            Callback is not called until either the command has failed, or has completed.
 *
 * return value:              Returns kDNSSDAdvertisingProxy_NoError when no error otherwise returns an
 *                            error code indicating the error that occurred. Note: A return value of
 *                            kDNSSDAdvertisingProxy_NoError does not mean that srp replication advertisements
 *                            were successfully discontinued. The callback may asynchronously return an
 *                            error (such as kDNSSDAdvertisingProxy_DaemonNotRunning)
 *
 */

DNS_SERVICES_EXPORT
advertising_proxy_error_type advertising_proxy_drop_srpl_advertisement
(
    advertising_proxy_conn_ref NONNULL *NULLABLE conn_ref,
    run_context_t                        NONNULL clientq,
    advertising_proxy_reply             NULLABLE callback
);

/* advertising_proxy_drop_srpl_connection
 *
 * For testing, drop all srpl connections
 *
 * advertising_proxy_drop_srpl_connection() Parameters:
 *
 * conn_ref:                  A pointer to advertising_proxy_conn_ref that is initialized to NULL.
 *                            If the call succeeds it will be initialized to a non-NULL value.
 *                            The same conn_ref can be used for more than one call.
 *
 * clientq:                   Queue the client wants to schedule the callback on (Note: Must not be NULL)
 *
 * callback:                  Callback function for the client that indicates success or failure.
 *                            Callback is not called until either the command has failed, or has completed.
 *
 * return value:              Returns kDNSSDAdvertisingProxy_NoError when no error otherwise returns an
 *                            error code indicating the error that occurred. Note: A return value of
 *                            kDNSSDAdvertisingProxy_NoError does not mean that srp replication connections
 *                            were successfully dropped. The callback may asynchronously return an
 *                            error (such as kDNSSDAdvertisingProxy_DaemonNotRunning)
 *
 */

DNS_SERVICES_EXPORT
advertising_proxy_error_type advertising_proxy_drop_srpl_connection
(
    advertising_proxy_conn_ref NONNULL *NULLABLE conn_ref,
    run_context_t                        NONNULL clientq,
    advertising_proxy_reply             NULLABLE callback
);

/* advertising_proxy_start_dropping_push_connections
 *
 * For testing, start dropping DNS Push connections every 90 seconds.
 *
 * advertising_proxy_start_dropping_push_connections() Parameters:
 *
 * conn_ref:                  A pointer to advertising_proxy_conn_ref that is initialized to NULL.
 *                            If the call succeeds it will be initialized to a non-NULL value.
 *                            The same conn_ref can be used for more than one call.
 *
 * clientq:                   Queue the client wants to schedule the callback on (Note: Must not be NULL)
 *
 * callback:                  Callback function for the client that indicates success or failure.
 *                            Callback is not called until either the command has failed, or has completed.
 *
 * return value:              Returns kDNSSDAdvertisingProxy_NoError when no error otherwise returns an
 *                            error code indicating the error that occurred. Note: A return value of
 *                            kDNSSDAdvertisingProxy_NoError does not mean that srp replication connections
 *                            were successfully dropped. The callback may asynchronously return an
 *                            error (such as kDNSSDAdvertisingProxy_DaemonNotRunning)
 *
 */

DNS_SERVICES_EXPORT
advertising_proxy_error_type advertising_proxy_start_dropping_push_connections
(
    advertising_proxy_conn_ref NONNULL *NULLABLE conn_ref,
    run_context_t                        NONNULL clientq,
    advertising_proxy_reply             NULLABLE callback
);

/* advertising_proxy_block_anycast service
 *
 * For testing, block anycast service
 *
 * advertising_proxy_block_anycast_service() Parameters:
 *
 * conn_ref:                  A pointer to advertising_proxy_conn_ref that is initialized to NULL.
 *                            If the call succeeds it will be initialized to a non-NULL value.
 *                            The same conn_ref can be used for more than one call.
 *
 * clientq:                   Queue the client wants to schedule the callback on (Note: Must not be NULL)
 *
 * callback:                  Callback function for the client that indicates success or failure.
 *                            Callback is not called until either the command has failed, or has completed.
 *
 * return value:              Returns kDNSSDAdvertisingProxy_NoError when no error otherwise returns an
 *                            error code indicating the error that occurred. Note: A return value of
 *                            kDNSSDAdvertisingProxy_NoError does not mean that anycast service advertisements
 *                            were successfully blocked. The callback may asynchronously return an
 *                            error (such as kDNSSDAdvertisingProxy_DaemonNotRunning)
 *
 */

DNS_SERVICES_EXPORT
advertising_proxy_error_type advertising_proxy_block_anycast_service
(
    advertising_proxy_conn_ref NONNULL *NULLABLE conn_ref,
    run_context_t                        NONNULL clientq,
    advertising_proxy_reply             NULLABLE callback
);

/* advertising_proxy_start_breaking_time_validation
 *
 * For testing, start breaking SIG(0) validation on replicated host messages. This tests that we correctly
 * handle such failures in the SRP replication protocol.
 *
 * advertising_proxy_start_dropping_push_connections() Parameters:
 *
 * conn_ref:                  A pointer to advertising_proxy_conn_ref that is initialized to NULL.
 *                            If the call succeeds it will be initialized to a non-NULL value.
 *                            The same conn_ref can be used for more than one call.
 *
 * clientq:                   Queue the client wants to schedule the callback on (Note: Must not be NULL)
 *
 * callback:                  Callback function for the client that indicates success or failure.
 *                            Callback is not called until either the command has failed, or has completed.
 *
 * return value:              Returns kDNSSDAdvertisingProxy_NoError when no error otherwise returns an
 *                            error code indicating the error that occurred. Note: A return value of
 *                            kDNSSDAdvertisingProxy_NoError does not mean that srp replication connections
 *                            were successfully dropped. The callback may asynchronously return an
 *                            error (such as kDNSSDAdvertisingProxy_DaemonNotRunning)
 *
 */

DNS_SERVICES_EXPORT
advertising_proxy_error_type advertising_proxy_start_breaking_time_validation
(
    advertising_proxy_conn_ref NONNULL *NULLABLE conn_ref,
    run_context_t                        NONNULL clientq,
    advertising_proxy_reply             NULLABLE callback
);

/* advertising_proxy_start_start_thread_shutdown
 *
 * For testing, start breaking SIG(0) validation on replicated host messages. This tests that we correctly
 * handle such failures in the SRP replication protocol.
 *
 * advertising_proxy_start_dropping_push_connections() Parameters:
 *
 * conn_ref:                  A pointer to advertising_proxy_conn_ref that is initialized to NULL.
 *                            If the call succeeds it will be initialized to a non-NULL value.
 *                            The same conn_ref can be used for more than one call.
 *
 * clientq:                   Queue the client wants to schedule the callback on (Note: Must not be NULL)
 *
 * callback:                  Callback function for the client that indicates success or failure.
 *                            Callback is not called until either the command has failed, or has completed.
 *                            Completion in this case means that all Thread services and prefixes were
 *                            successfully removed from the Thread network data and a network data update
 *                            was seen with this information removed, or else two seconds passed without
 *                            seeing the update (in which case we give up).
 *
 * return value:              Returns kDNSSDAdvertisingProxy_NoError when no error otherwise returns an
 *                            error code indicating the error that occurred.
 *
 */

DNS_SERVICES_EXPORT
advertising_proxy_error_type advertising_proxy_start_thread_shutdown
(
    advertising_proxy_conn_ref NONNULL *NULLABLE conn_ref,
    run_context_t                        NONNULL clientq,
    advertising_proxy_reply             NULLABLE callback
);

/* advertising_proxy_set-variable
 *
 * Set the specified variable to the specified value on the advertising proxy
 *
 * advertising_proxy_set_variable() Parameters:
 *
 * conn_ref:                  A pointer to advertising_proxy_conn_ref that is initialized to NULL.
 *                            If the call succeeds it will be initialized to a non-NULL value.
 *                            The same conn_ref can be used for more than one call.
 *
 * clientq:                   Queue the client wants to schedule the callback on (Note: Must not be NULL)
 *
 * callback:                  Callback function for the client that indicates success or failure.
 *                            Callback is not called until either the command has failed, or has completed.
 *
 * context:                   Context to return in callback
 *
 * variable:                  Name of variable to set
 *
 * value:                     Value to set. Value is a string which will be interpreted by the advertising proxy,
 *                            and can be a quoted string ("\"foo\""), "NULL", a number ("100"), or a boolean
 *                            ("true" or "false") depending on the variable's type.
 *
 * return value:              Returns kDNSSDAdvertisingProxy_NoError when no error otherwise returns an
 *                            error code indicating the error that occurred. Note: A return value of
 *                            kDNSSDAdvertisingProxy_NoError does not mean that the variable was set, just
 *                            that the variable set command was successfully created and sent.
 *
 */

DNS_SERVICES_EXPORT
advertising_proxy_error_type advertising_proxy_set_variable
(
    advertising_proxy_conn_ref NONNULL *NULLABLE conn_ref,
    run_context_t                        NONNULL clientq,
    advertising_proxy_response_reply    NULLABLE callback,
    void *                              NULLABLE context,
    const char *                         NONNULL name,
    const char *                         NONNULL value
);


/* advertising_proxy_ref_dealloc()
 *
 * Terminate a connection with the daemon and free memory associated with the advertising_proxy_conn_ref.
 * When used on a advertising_proxy_conn_ref returned by advertising_proxy_enable, terminates the advertising
 * proxy.  When used on a call that subscribes to notifications about objects managed by the advertising proxy,
 * discontinues those notifications.
 *
 * conn_ref:        A advertising_proxy_conn_ref initialized by any of the advertising_proxy_*() calls.
 *
 */
DNS_SERVICES_EXPORT void
advertising_proxy_ref_dealloc(advertising_proxy_conn_ref NONNULL conn_ref);

DNS_SERVICES_EXPORT advertising_proxy_error_type
advertising_proxy_resolver_init(os_log_t NULLABLE log_thingy);

DNS_SERVICES_EXPORT advertising_proxy_error_type
advertising_proxy_browse_create(advertising_proxy_subscription_t *NULLABLE *NONNULL aref,
                                run_context_t NONNULL clientq, const char *NONNULL regtype,
                                advertising_proxy_browse_reply NONNULL callBack, void *NULLABLE context);

DNS_SERVICES_EXPORT advertising_proxy_error_type
advertising_proxy_resolve_create(advertising_proxy_subscription_t *NULLABLE *NONNULL subscription_ret,
                                 run_context_t NONNULL clientq,
                                 const char *NONNULL name, const char *NONNULL regtype,
                                 const char *NONNULL domain,
                                 advertising_proxy_resolve_reply NONNULL callback, void *NULLABLE context);
DNS_SERVICES_EXPORT advertising_proxy_error_type
advertising_proxy_registrar_create(advertising_proxy_subscription_t *NULLABLE *NONNULL subscription_ret,
                                   run_context_t NONNULL clientq,
                                   advertising_proxy_registrar_reply NONNULL callback,
                                   void *NULLABLE context);
DNS_SERVICES_EXPORT advertising_proxy_error_type
advertising_proxy_get_addresses(advertising_proxy_subscription_t *NONNULL *NULLABLE subscription_ret, run_context_t NONNULL clientq,
                                const char *NULLABLE name, advertising_proxy_address_reply NONNULL callback, void *NULLABLEcontext);

#define advertising_proxy_subscription_retain(subscription) advertising_proxy_subscription_retain_(subscription, __FILE__, __LINE__)
DNS_SERVICES_EXPORT advertising_proxy_error_type
advertising_proxy_subscription_retain_(advertising_proxy_subscription_t *NONNULL subscription, const char *NONNULL file, int line);

#define advertising_proxy_subscription_release(subscription) advertising_proxy_subscription_release_(subscription, __FILE__, __LINE__)
DNS_SERVICES_EXPORT advertising_proxy_error_type
advertising_proxy_subscription_release_(advertising_proxy_subscription_t *NONNULL subscription, const char *NONNULL file, int line);

DNS_SERVICES_EXPORT advertising_proxy_error_type
advertising_proxy_subscription_cancel(advertising_proxy_subscription_t *NONNULL subscription);
__END_DECLS
#endif /* DNSSD_PROXY_SERVICES_H */

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 108
// indent-tabs-mode: nil
// End:
