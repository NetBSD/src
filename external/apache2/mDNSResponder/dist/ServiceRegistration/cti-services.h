/* cti-services.h
 *
 * Copyright (c) 2020-2024 Apple Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * This code adds border router support to 3rd party HomeKit Routers as part of Apple’s commitment to the CHIP project.
 *
 * Concise Thread Interface for Thread Border router control.
 */

#ifndef __CTI_SERVICES_H__
#define __CTI_SERVICES_H__

#include <netinet/in.h>
#include <arpa/inet.h>
typedef void *run_context_t;
#include "srp.h"

#if (defined(__GNUC__) && (__GNUC__ >= 4))
#define DNS_SERVICES_EXPORT __attribute__((visibility("default")))
#else
#define DNS_SERVICES_EXPORT
#endif

#include "cti-common.h"

typedef enum _offmesh_route_origin {
    offmesh_route_origin_user,
    offmesh_route_origin_ncp,
} offmesh_route_origin_t;

typedef enum _offmesh_route_preference {
    offmesh_route_preference_low = -1,
    offmesh_route_preference_medium,
    offmesh_route_preference_high,
} offmesh_route_preference_t;

typedef struct _cti_service {
    uint64_t enterprise_number;
    uint16_t service_type;
    uint16_t service_version;
    uint16_t rloc16;
    uint16_t service_id;
    uint8_t *NONNULL service;
    uint8_t *NONNULL server;
    size_t service_length;
    size_t server_length;
    int ref_count;
    int flags;      // E.g., kCTIFlag_NCP
} cti_service_t;

typedef struct _cti_service_vec {
    size_t num;
    int ref_count;
    cti_service_t *NULLABLE *NONNULL services;
} cti_service_vec_t;

typedef struct _cti_prefix {
    struct in6_addr prefix;
    int prefix_length;
    int metric;
    int flags;
    int rloc;
    bool stable;
    bool ncp;
    int ref_count;
} cti_prefix_t;

// Bits in the prefix flags
#define kCTIPriorityShift      14
#define kCTIPreferredShift     13
#define kCTISLAACShift         12
#define kCTIDHCPShift          11
#define kCTIConfigureShift     10
#define kCTIDefaultRouteShift  9
#define kCTIOnMeshShift        8
#define kCTIDNSShift           7
#define kCTIDPShift            6

// Priority values
#define kCTIPriorityMedium    0
#define kCTIPriorityHigh      1
#define kCTIPriorityReserved  2
#define kCTIPriorityLow       3

// Macros to fetch values from the prefix flags
#define CTI_PREFIX_FLAGS_PRIORITY(flags)       (((flags) >> kCTIPriorityShift) & 3)
#define CTI_PREFIX_FLAGS_PREFERRED(flags)      (((flags) >> kCTIPreferredShift) & 1)
#define CTI_PREFIX_FLAGS_SLAAC(flags)          (((flags) >> kCTISLAACShift) & 1)
#define CTI_PREFIX_FLAGS_DHCP(flags)           (((flags) >> kCTIDHCPShift) & 1)
#define CTI_PREFIX_FLAGS_CONFIGURE(flags)      (((flags) >> kCTIConfigureShift) & 1)
#define CTI_PREFIX_FLAGS_DEFAULT_ROUTE(flags)  (((flags) >> kCTIDefaultRouteShift) & 1)
#define CTI_PREFIX_FLAGS_ON_MESH(flags)        (((flags) >> kCTIOnMeshShift) & 1)
#define CTI_PREFIX_FLAGS_DNS(flags)            (((flags) >> kCTIDNSShift) & 1)
#define CTI_PREFIX_DLAGS_DP(flags)             (((flags) >> kCTIDPShift) & 1)

// Macros to set values in the prefix flags
#define CTI_PREFIX_FLAGS_PRIORITY_SET(flags, value)       ((flags) = \
                                                            (((flags) & ~(3 << kCTIPriorityShift)) | \
                                                             (((value) & 3) << kCTIPriorityShift)))
#define CTI_PREFIX_FLAGS_PREFERRED_SET(flags, value)      ((flags) = \
                                                            (((flags) & ~(1 << kCTIPreferredShift)) | \
                                                             (((value) & 1) << kCTIPreferredShift)))
#define CTI_PREFIX_FLAGS_SLAAC_SET(flags, value)          ((flags) = \
                                                            (((flags) & ~(1 << kCTISLAACShift)) | \
                                                             (((value) & 1) << kCTISLAACShift)))
#define CTI_PREFIX_FLAGS_DHCP_SET(flags, value)           ((flags) = \
                                                            (((flags) & ~(1 << kCTIDHCPShift)) | \
                                                             (((value) & 1) << kCTIDHCPShift)))
#define CTI_PREFIX_FLAGS_CONFIGURE_SET(flags, value)      ((flags) = \
                                                            (((flags) & ~(1 << kCTIConfigureShift)) | \
                                                             (((value) & 1) << kCTIConfigureShift)))
#define CTI_PREFIX_FLAGS_DEFAULT_ROUTE_SET(flags, value)  ((flags) = \
                                                            (((flags) & ~(1 << kCTIDefaultRouteShift)) | \
                                                             (((value) & 1) << kCTIDefaultRouteShift)))
#define CTI_PREFIX_FLAGS_ON_MESH_SET(flags, value)        ((flags) = \
                                                            (((flags) & ~(1 << kCTIOnMeshShift)) | \
                                                             (((value) & 1) << kCTIOnMeshShift)))
#define CTI_PREFIX_FLAGS_DNS_SET(flags, value)            ((flags) = \
                                                            (((flags) & ~(1 << kCTIDNSShift)) | \
                                                             (((value) & 1) << kCTIDNSShift)))
#define CTI_PREFIX_DLAGS_DP_SET(flags, value)             ((flags) = \
                                                            (((flags) & ~(1 << kCTIDPShift)) | \
                                                             (((value) & 1) << kCTIDPShift)))

typedef struct _cti_prefix_vec {
    size_t num;
    int ref_count;
    cti_prefix_t *NULLABLE *NONNULL prefixes;
} cti_prefix_vec_t;

typedef struct _cti_route {
    struct in6_addr prefix;
    int prefix_length;
    offmesh_route_origin_t origin;
    bool nat64;
    bool stable;
    offmesh_route_preference_t preference;
    int rloc;
    bool next_hop_is_host;
    int ref_count;
} cti_route_t;

typedef struct _cti_route_vec {
    size_t num;
    int ref_count;
    cti_route_t *NULLABLE *NONNULL routes;
} cti_route_vec_t;

typedef struct srp_server_state srp_server_t;

/* cti_reply:
 *
 * A general reply mechanism to indicate success or failure for a cti call that doesn't
 * return any data.
 *
 * cti_reply parameters:
 *
 * context:    The context that was passed to the cti service call to which this is a callback.
 *
 * status:	   Will be kCTIStatus_NoError on success, otherwise will indicate the
 * 			   failure that occurred.
 */

typedef void
(*cti_reply_t)(void *NULLABLE context, cti_status_t status);

/* cti_string_property_reply: Callback for get calls that fetch a string property
 *
 * Called exactly once in response to (for example) a cti_get_tunnel_name() or cti_get_on_link_prefix() call, either
 * with an error or with a string containing the response.
 *
 * cti_reply parameters:
 *
 * context:       The context that was passed to the cti service call to which this is a callback.
 *
 * tunnel_name:   If error is kCTIStatus_NoError, a string containing the name of the Thread
 * 			      interface.
 *
 * status:	      Will be kCTIStatus_NoError on success, otherwise will indicate the
 * 			      failure that occurred.
 */

typedef void
(*cti_string_property_reply_t)(void *NULLABLE context, const char *NONNULL string,
                               cti_status_t status);

/* cti_get_tunnel_name
 *
 * Get the name of the tunnel that wpantund is presenting as the Thread network interface.
 * The tunnel name is passed to the reply callback if the request succeeds; otherwise an error
 * is either returned immediately or returned to the callback.
 *
 * context:        An anonymous pointer that will be passed along to the callback when
 *                 an event occurs.
 *
 * callback:       CallBack function for the client that indicates success or failure.
 *
 * client_queue:   Queue the client wants to schedule the callback on
 *
 * return value:   Returns kCTIStatus_NoError when no error otherwise returns an error code indicating the
 *                 error that occurred. Note: A return value of kCTIStatus_NoError does not mean that the
 *                 request succeeded, merely that it was successfully started.
 */

#define cti_get_tunnel_name(server, context, callback, client_queue) \
    cti_get_tunnel_name_(server, context, callback, client_queue, __FILE__, __LINE__)
DNS_SERVICES_EXPORT cti_status_t
cti_get_tunnel_name_(srp_server_t *NULLABLE server, void *NULLABLE context, cti_string_property_reply_t NONNULL callback,
                     run_context_t NULLABLE client_queue, const char *NONNULL file, int line);

/* cti_track_neighbor_ml_eid
 *
 * Track changes in our neighbor's mesh-local address (ML-EID). This makes sense when acting as an end device, not as a
 * router.  The ML-EID tunnel is passed to the reply callback if the request succeeds; otherwise an error is either
 * returned immediately or returned to the callback.
 *
 * context:        An anonymous pointer that will be passed along to the callback when
 *                 an event occurs.
 *
 * callback:       CallBack function for the client that indicates success or failure.
 *
 * client_queue:   Queue the client wants to schedule the callback on
 *
 * return value:   Returns kCTIStatus_NoError when no error otherwise returns an error code indicating the
 *                 error that occurred. Note: A return value of kCTIStatus_NoError does not mean that the
 *                 request succeeded, merely that it was successfully started.
 */

#define cti_track_neighbor_ml_eid(server, ref, context, callback, client_queue) \
    cti_track_neighbor_ml_eid_(server, ref, context, callback, client_queue, __FILE__, __LINE__)
DNS_SERVICES_EXPORT cti_status_t
cti_track_neighbor_ml_eid_(srp_server_t *NULLABLE server, cti_connection_t NULLABLE *NULLABLE ref,
                           void *NULLABLE context, cti_string_property_reply_t NONNULL callback,
                           run_context_t NULLABLE client_queue, const char *NONNULL file, int line);

/* cti_get_mesh_local_prefix
 *
 * Get the mesh_local IPv6 prefix that is in use on the Thread mesh. The prefix is passed to the reply callback if the
 * request succeeds; otherwise an error is either returned immediately or returned to the callback.
 *
 * context:        An anonymous pointer that will be passed along to the callback when
 *                 an event occurs.
 *
 * callback:       CallBack function for the client that indicates success or failure.
 *
 * client_queue:   Queue the client wants to schedule the callback on
 *
 * return value:   Returns kCTIStatus_NoError when no error otherwise returns an error code indicating the
 *                 error that occurred. Note: A return value of kCTIStatus_NoError does not mean that the
 *                 request succeeded, merely that it was successfully started.
 */

#define cti_get_mesh_local_prefix(server, context, callback, client_queue) \
    cti_get_mesh_local_prefix_(server, context, callback, client_queue, __FILE__, __LINE__)
DNS_SERVICES_EXPORT cti_status_t
cti_get_mesh_local_prefix_(srp_server_t *NULLABLE server,
                           void *NULLABLE context, cti_string_property_reply_t NONNULL callback,
                           run_context_t NULLABLE client_queue, const char *NONNULL file, int line);

/* cti_get_mesh_local_address
 *
 * Get the mesh_local IPv6 address that is in use on this device on the Thread mesh. The address is passed to the reply
 * callback if the request succeeds; otherwise an error is either returned immediately or returned to the callback.
 *
 * context:        An anonymous pointer that will be passed along to the callback when
 *                 an event occurs.
 *
 * callback:       CallBack function for the client that indicates success or failure.
 *
 * client_queue:   Queue the client wants to schedule the callback on
 *
 * return value:   Returns kCTIStatus_NoError when no error otherwise returns an error code indicating the
 *                 error that occurred. Note: A return value of kCTIStatus_NoError does not mean that the
 *                 request succeeded, merely that it was successfully started.
 */

#define cti_get_mesh_local_address(server, context, callback, client_queue) \
    cti_get_mesh_local_address_(server, context, callback, client_queue, __FILE__, __LINE__)
DNS_SERVICES_EXPORT cti_status_t
cti_get_mesh_local_address_(srp_server_t *NULLABLE server,
                            void *NULLABLE context, cti_string_property_reply_t NONNULL callback,
                            run_context_t NULLABLE client_queue, const char *NONNULL file, int line);

/*
 * cti_service_vec_create
 *
 * creates a service array vector of specified length
 *
 * num_services:     Number of service slots available in the service vector.
 *
 * return value:     NULL, if the call failed; otherwise a service vector capable of containing the requested number of
 *                   services.
 */
cti_service_vec_t *NULLABLE
cti_service_vec_create_(size_t num_services, const char *NONNULL file, int line);
#define cti_service_vec_create(num_services) cti_service_vec_create_(num_services, __FILE__, __LINE__)

/*
 * cti_service_vec_release
 *
 * decrements the reference count on the provided service vector and, if it reaches zero, finalizes the service vector,
 * which calls cti_service_release on each service in the vector, potentially also finalizing them.
 *
 * num_services:     Number of service slots available in the service vector.
 *
 * return value:     NULL, if the call failed; otherwise a service vector capable of containing the requested number of
 *                   services.
 */

void
cti_service_vec_release_(cti_service_vec_t *NONNULL services, const char *NONNULL file, int line);
#define cti_service_vec_release(services) cti_service_vec_release_(services, __FILE__, __LINE__)

/*
 * cti_service_create
 *
 * Creates a service containing the specified information.   service and server are retained, and will be
 * freed using free() when the service object is finalized.   Caller must not retain or free these values, and
 * they must be allocated on the malloc heap.
 *
 * enterprise_number: The enterprise number for this service.
 *
 * service_type:      The service type, from the service data.
 *
 * service_version:   The service version, from the service data.
 *
 * server:            Server information for this service, stored in network byte order.   Format depends on service type.
 *
 * server_length:     Length of server information in bytes.
 *
 * flags:             Thread network status flags, e.g. NCP versue User
 *
 * return value:     NULL, if the call failed; otherwise a service object containing the specified state.
 */

cti_service_t *NULLABLE
cti_service_create_(uint64_t enterprise_number, uint16_t rloc16, uint16_t service_type, uint16_t service_version,
                    uint8_t *NONNULL service, size_t service_length, uint8_t *NONNULL server, size_t server_length,
                    uint16_t service_id, int flags, const char *NONNULL file, int line);
#define cti_service_create(enterprise_number, rloc16, service_type, service_version, service, service_length, \
                           server, server_length, service_id, flags) \
    cti_service_create_(enterprise_number, rloc16, service_type, service_version, service, service_length, \
                        server, server_length, service_id, flags, __FILE__, __LINE__)

/*
 * cti_service_vec_release
 *
 * decrements the reference count on the provided service vector and, if it reaches zero, finalizes the service vector,
 * which calls cti_service_release on each service in the vector, potentially also finalizing them.
 *
 * services:         The service vector to release
 *
 * return value:     NULL, if the call failed; otherwise a service vector capable of containing the requested number of
 *                   services.
 */

void
cti_service_release_(cti_service_t *NONNULL service, const char *NONNULL file, int line);
#define cti_service_release(services) cti_service_release(service, __FILE__, __LINE__)

/* cti_service_reply: Callback from cti_get_service_list()
 *
 * Called when an error occurs during processing of the cti_get_service_list call, or when data
 * is available in response to the call.
 *
 * In the case of an error, the callback will not be called again, and the caller is responsible for
 * releasing the connection state and restarting if needed.
 *
 * The callback will be called once immediately upon success, and once again each time the service list
 * is updated.   If the callback wishes to retain either the cti_service_vec_t or any of the elements
 * of that vector, the appropriate retain function should be called; when the object is no longer needed,
 * the corresponding release call must be called.
 *
 * cti_reply parameters:
 *
 * context:           The context that was passed to the cti service call to which this is a callback.
 *
 * services:          If status is kCTIStatus_NoError, a cti_service_vec_t containing the list of services
 *                    provided in the update.
 *
 * status:	          Will be kCTIStatus_NoError if the service list request is successful, or
 *                    will indicate the failure that occurred.
 */

typedef void
(*cti_service_reply_t)(void *NULLABLE context, cti_service_vec_t *NULLABLE services, cti_status_t status);

/* cti_get_service_list
 *
 * Requests wpantund to immediately send the current list of services published in the Thread network data.
 * Whenever the service list is updated, the callback will be called again with the new information.  A
 * return value of kCTIStatus_NoError means that the caller can expect the reply callback to be called at least
 * once.  Any other error status means that the request could not be sent, and the callback will never be
 * called.
 *
 * To discontinue receiving service add/remove callbacks, the calling program should call
 * cti_connection_ref_deallocate on the conn_ref returned by a successful call to get_service_list();
 *
 * ref:            A pointer to a reference to the connection is stored through ref if ref is not NULL.
 *                 When events are no longer needed, call cti_discontinue_events() on the returned pointer.
 *
 * context:        An anonymous pointer that will be passed along to the callback when
 *                 an event occurs.
 *
 * callback:       CallBack function for the client that indicates success or failure.
 *                 If the get_services call fails, response will be NULL and status
 *                 will indicate what went wrong.  No further callbacks can be expected
 *                 after this.   If the request succeeds, then the callback will be called
 *                 once immediately with the current service list, and then again whenever
 *                 the service list is updated.
 *
 * client_queue:   Queue the client wants to schedule the callback on
 *
 * return value:   Returns kCTIStatus_NoError when no error otherwise returns an error code indicating
 *                 the error that occurred. Note: A return value of kCTIStatus_NoError does not mean
 *                 that the request succeeded, merely that it was successfully started.
 */

#define cti_get_service_list(server, ref, context, callback, client_queue) \
    cti_get_service_list_(server, ref, context, callback, client_queue, __FILE__, __LINE__)
DNS_SERVICES_EXPORT cti_status_t
cti_get_service_list_(srp_server_t *NULLABLE server, cti_connection_t NULLABLE *NULLABLE ref, void *NULLABLE context,
                      cti_service_reply_t NONNULL callback, run_context_t NULLABLE client_queue, const char *NONNULL file, int line);

/*
 * cti_service_vec_create
 *
 * creates a service array vector of specified length
 *
 * num_prefixes:     Number of prefix slots available in the prefix vector.
 *
 * return value:     NULL, if the call failed; otherwise a prefix vector capable of containing the requested number of
 *                   prefixes.
 */
cti_prefix_vec_t *NULLABLE
cti_prefix_vec_create_(size_t num_prefixes, const char *NONNULL file, int line);
#define cti_prefix_vec_create(num_prefixes) cti_prefix_vec_create_(num_prefixes, __FILE__, __LINE__)

/*
 * cti_prefix_vec_release
 *
 * decrements the reference count on the provided prefix vector and, if it reaches zero, finalizes the prefix vector,
 * which calls cti_prefix_release on each prefix in the vector, potentially also finalizing them.
 *
 * num_prefixes:     Number of prefix slots available in the prefix vector.
 *
 * return value:     NULL, if the call failed; otherwise a prefix vector capable of containing the requested number of
 *                   prefixes.
 */

void
cti_prefix_vec_release_(cti_prefix_vec_t *NONNULL prefixes, const char *NONNULL file, int line);
#define cti_prefix_vec_release(prefixes) cti_prefix_vec_release_(prefixes, __FILE__, __LINE__)

/*
 * cti_prefix_create
 *
 * Creates a prefix containing the specified information.   prefix and server are retained, and will be
 * freed using free() when the prefix object is finalized.   Caller must not retain or free these values, and
 * they must be allocated on the malloc heap.
 *
 * enterprise_number: The enterprise number for this prefix.
 *
 * prefix_type:      The prefix type, from the prefix data.
 *
 * prefix_version:   The prefix version, from the prefix data.
 *
 * server:            Server information for this prefix, stored in network byte order.   Format depends on prefix type.
 *
 * server_length:     Length of server information in bytes.
 *
 * flags:             Thread network status flags, e.g. NCP versue User
 *
 * return value:     NULL, if the call failed; otherwise a prefix object containing the specified state.
 */

cti_prefix_t *NULLABLE
cti_prefix_create_(struct in6_addr *NONNULL prefix, int prefix_length, int metric, int flags, int rloc, bool stable, bool ncp,
                   const char *NONNULL file, int line);
#define cti_prefix_create(prefix, prefix_length, metric, flags, rloc, stable, ncp) \
    cti_prefix_create_(prefix, prefix_length, metric, flags, rloc, stable, ncp, __FILE__, __LINE__)

/*
 * cti_prefix_vec_release
 *
 * decrements the reference count on the provided prefix vector and, if it reaches zero, finalizes the prefix vector,
 * which calls cti_prefix_release on each prefix in the vector, potentially also finalizing them.
 *
 * prefixes:         The prefix vector to release.
 *
 * return value:     NULL, if the call failed; otherwise a prefix vector capable of containing the requested number of
 *                   prefixes.
 */

void
cti_prefix_release_(cti_prefix_t *NONNULL prefix, const char *NONNULL file, int line);
#define cti_prefix_release(prefixes) cti_prefix_release(prefix, __FILE__, __LINE__)

/* cti_prefix_reply: Callback from cti_get_prefix_list()
 *
 * Called when an error occurs during processing of the cti_get_prefix_list call, or when a prefix
 * is added or removed.
 *
 * In the case of an error, the callback will not be called again, and the caller is responsible for
 * releasing the connection state and restarting if needed.
 *
 * The callback will be called once for each prefix present on the Thread network at the time
 * get_prefix_list() is first called, and then again whenever a prefix is added or removed.
 *
 * cti_reply parameters:
 *
 * context:           The context that was passed to the cti prefix call to which this is a callback.
 *
 * prefix_vec:        If status is kCTIStatus_NoError, a vector containing all of the prefixes that were reported in
 *                    this event.
 *
 * status:	          Will be kCTIStatus_NoError if the prefix list request is successful, or
 *                    will indicate the failure that occurred.
 */

typedef void
(*cti_prefix_reply_t)(void *NULLABLE context, cti_prefix_vec_t *NULLABLE prefixes, cti_status_t status);

/* cti_get_prefix_list
 *
 * Requests wpantund to immediately send the current list of off-mesh prefixes configured in the Thread
 * network data.  Whenever the prefix list is updated, the callback will be called again with the new
 * information.  A return value of kCTIStatus_NoError means that the caller can expect the reply callback to be
 * called at least once.  Any other error means that the request could not be sent, and the callback will
 * never be called.
 *
 * To discontinue receiving prefix add/remove callbacks, the calling program should call
 * cti_connection_ref_deallocate on the conn_ref returned by a successful call to get_prefix_list();
 *
 * ref:            A pointer to a reference to the connection is stored through ref if ref is not NULL.
 *                 When events are no longer needed, call cti_discontinue_events() on the returned pointer.
 *
 * context:        An anonymous pointer that will be passed along to the callback when
 *                 an event occurs.
 *
 * callback:       CallBack function for the client that indicates success or failure.
 *
 * client_queue:   Queue the client wants to schedule the callback on
 *
 * return value:   Returns kCTIStatus_NoError when no error otherwise returns an error code indicating
 *                 the error that occurred. Note: A return value of kCTIStatus_NoError does not mean
 *                 that the request succeeded, merely that it was successfully started.
 */

#define cti_get_prefix_list(server, ref, context, callback, client_queue) \
    cti_get_prefix_list_(server, ref, context, callback, client_queue, __FILE__, __LINE__)
DNS_SERVICES_EXPORT cti_status_t
cti_get_prefix_list_(srp_server_t *NULLABLE server, cti_connection_t NULLABLE *NULLABLE ref, void *NULLABLE context,
                     cti_prefix_reply_t NONNULL callback, run_context_t NULLABLE client_queue, const char *NONNULL file, int line);

/* cti_state_reply: Callback from cti_get_state()
 *
 * Called when an error occurs during processing of the cti_get_state call, or when network state
 * information is available.
 *
 * In the case of an error, the callback will not be called again, and the caller is responsible for
 * releasing the connection state and restarting if needed.
 *
 * The callback will be called initially to report the network state, and subsequently whenever the
 * network state changes.
 *
 * cti_reply parameters:
 *
 * context:           The context that was passed to the cti state call to which this is a callback.
 *
 * state:             The network state.
 *
 * status:	          Will be kCTIStatus_NoError if the prefix list request is successful, or
 *                    will indicate the failure that occurred.
 */

typedef void
(*cti_state_reply_t)(void *NULLABLE context, cti_network_state_t state, cti_status_t status);

/* cti_get_state
 *
 * Requests wpantund to immediately send the current state of the thread network.  Whenever the thread
 * network state changes, the callback will be called again with the new state.  A return value of
 * kCTIStatus_NoError means that the caller can expect the reply callback to be called at least once.  Any
 * other error means that the request could not be sent, and the callback will never be called.
 *
 * To discontinue receiving state change callbacks, the calling program should call
 * cti_connection_ref_deallocate on the conn_ref returned by a successful call to cti_get_state();
 *
 * ref:            A pointer to a reference to the connection is stored through ref if ref is not NULL.
 *                 When events are no longer needed, call cti_discontinue_events() on the returned pointer.
 *
 * context:        An anonymous pointer that will be passed along to the callback when
 *                 an event occurs.
 *
 * callback:       CallBack function for the client that indicates success or failure.
 *
 * client_queue:   Queue the client wants to schedule the callback on
 *
 * return value:   Returns kCTIStatus_NoError when no error otherwise returns an error code indicating
 *                 the error that occurred. Note: A return value of kCTIStatus_NoError does not mean
 *                 that the request succeeded, merely that it was successfully started.
 */

#define cti_get_state(server, ref, context, callback, client_queue) \
    cti_get_state_(server, ref, context, callback, client_queue, __FILE__, __LINE__)
DNS_SERVICES_EXPORT cti_status_t
cti_get_state_(srp_server_t *NULLABLE server, cti_connection_t NULLABLE *NULLABLE ref, void *NULLABLE context,
               cti_state_reply_t NONNULL callback, run_context_t NULLABLE client_queue, const char *NONNULL file, int line);

/* cti_uint64_property_reply: Callback from cti_get_partition_id() or cti_get_xpanid()
 *
 * Called when an error occurs during processing of the cti_get_* call, or when a new value for the requested property
 * is available.
 *
 * In the case of an error, the callback will not be called again, and the caller is responsible for
 * releasing the connection and restarting if needed.
 *
 * The callback will be called initially to report the current value for the property, and subsequently whenever the
 * property changes.
 *
 * cti_uint64_property_reply parameters:
 *
 * context:           The context that was passed to the cti prefix call to which this is a callback.
 *
 * property_value     The value of the property (only valid if status is kCTIStatus_NoError).
 *
 * status:	          Will be kCTIStatus_NoError if the partition ID request is successful, or will indicate the failure
 *                    that occurred.
 */

typedef void
(*cti_uint64_property_reply_t)(void *NULLABLE context, uint64_t property_value, cti_status_t status);

/* cti_get_partition_id
 *
 * Requests wpantund to immediately send the current partition_id of the thread network.  Whenever the thread
 * network partition_id changes, the callback will be called again with the new partition_id.  A return value of
 * kCTIStatus_NoError means that the caller can expect the reply callback to be called at least once.  Any
 * other error means that the request could not be sent, and the callback will never be called.
 *
 * To discontinue receiving partition_id change callbacks, the calling program should call
 * cti_connection_ref_deallocate on the conn_ref returned by a successful call to cti_get_partition_id();
 *
 * ref:            A pointer to a reference to the connection is stored through ref if ref is not NULL.
 *                 When events are no longer needed, call cti_discontinue_events() on the returned pointer.
 *
 * context:        An anonymous pointer that will be passed along to the callback when
 *                 an event occurs.
 * callback:       CallBack function for the client that indicates success or failure.
 *
 * client_queue:   Queue the client wants to schedule the callback on
 *
 * return value:   Returns kCTIStatus_NoError when no error otherwise returns an error code indicating
 *                 the error that occurred. Note: A return value of kCTIStatus_NoError does not mean
 *                 that the request succeeded, merely that it was successfully started.
 */

#define cti_get_partition_id(server, ref, context, callback, client_queue) \
    cti_get_partition_id_(server, ref, context, callback, client_queue, __FILE__, __LINE__)
DNS_SERVICES_EXPORT cti_status_t
cti_get_partition_id_(srp_server_t *NULLABLE server, cti_connection_t NULLABLE *NULLABLE ref, void *NULLABLE context,
                      cti_uint64_property_reply_t NONNULL callback, run_context_t NULLABLE client_queue,
                      const char *NONNULL file, int line);

/* cti_get_extended_pan_id
 *
 * Requests wpantund to immediately send the current extended_pan_id of the thread network.  Whenever the thread
 * network extended_pan_id changes, the callback will be called again with the new extended_pan_id.  A return value of
 * kCTIStatus_NoError means that the caller can expect the reply callback to be called at least once.  Any
 * other error means that the request could not be sent, and the callback will never be called.
 *
 * To discontinue receiving extended_pan_id change callbacks, the calling program should call
 * cti_connection_ref_deallocate on the conn_ref returned by a successful call to cti_get_extended_pan_id();
 *
 * ref:            A pointer to a reference to the connection is stored through ref if ref is not NULL.
 *                 When events are no longer needed, call cti_discontinue_events() on the returned pointer.
 *
 * context:        An anonymous pointer that will be passed along to the callback when
 *                 an event occurs.
 * callback:       CallBack function for the client that indicates success or failure.
 *
 * client_queue:   Queue the client wants to schedule the callback on
 *
 * return value:   Returns kCTIStatus_NoError when no error otherwise returns an error code indicating
 *                 the error that occurred. Note: A return value of kCTIStatus_NoError does not mean
 *                 that the request succeeded, merely that it was successfully started.
 */

#define cti_get_extended_pan_id(server, ref, context, callback, client_queue) \
    cti_get_extended_pan_id_(server, ref, context, callback, client_queue, __FILE__, __LINE__)
DNS_SERVICES_EXPORT cti_status_t
cti_get_extended_pan_id_(srp_server_t *NULLABLE server, cti_connection_t NULLABLE *NULLABLE ref, void *NULLABLE context,
                         cti_uint64_property_reply_t NONNULL callback, run_context_t NULLABLE client_queue,
                         const char *NONNULL file, int line);

/* cti_network_node_type_reply: Callback from cti_get_network_node_type()
 *
 * Called when an error occurs during processing of the cti_get_network_node_type call, or when network partition ID
 * information is available.
 *
 * In the case of an error, the callback will not be called again, and the caller is responsible for releasing the
 * connection network_node_type and restarting if needed.
 *
 * The callback will be called initially to report the network partition ID, and subsequently whenever the network
 * partition ID changes.
 *
 * cti_reply parameters:
 *
 * context:           The context that was passed to the cti prefix call to which this is a callback.
 *
 * network_node_type: The network node type, kCTI_NetworkNodeType_Unknown if it is not known.
 *
 * status:	          Will be kCTIStatus_NoError if the partition ID request is successful, or will indicate the failure
 *                    that occurred.
 */

typedef void
(*cti_network_node_type_reply_t)(void *NULLABLE context, cti_network_node_type_t network_node_type, cti_status_t status);

/* cti_get_network_node_type
 *
 * Requests wpantund to immediately send the current network_node_type of the thread network.  Whenever the thread
 * network network_node_type changes, the callback will be called again with the new network_node_type.  A return value of
 * kCTIStatus_NoError means that the caller can expect the reply callback to be called at least once.  Any
 * other error means that the request could not be sent, and the callback will never be called.
 *
 * To discontinue receiving network_node_type change callbacks, the calling program should call
 * cti_connection_ref_deallocate on the conn_ref returned by a successful call to cti_get_network_node_type();
 *
 * ref:            A pointer to a reference to the connection is stored through ref if ref is not NULL.
 *                 When events are no longer needed, call cti_discontinue_events() on the returned pointer.
 *
 * context:        An anonymous pointer that will be passed along to the callback when
 *                 an event occurs.
 * callback:       CallBack function for the client that indicates success or failure.
 *
 * client_queue:   Queue the client wants to schedule the callback on
 *
 * return value:   Returns kCTIStatus_NoError when no error otherwise returns an error code indicating
 *                 the error that occurred. Note: A return value of kCTIStatus_NoError does not mean
 *                 that the request succeeded, merely that it was successfully started.
 */

#define cti_get_network_node_type(server, ref, context, callback, client_queue) \
    cti_get_network_node_type_(server, ref, context, callback, client_queue, __FILE__, __LINE__)
DNS_SERVICES_EXPORT cti_status_t
cti_get_network_node_type_(srp_server_t *NULLABLE server, cti_connection_t NULLABLE *NULLABLE ref, void *NULLABLE context,
                           cti_network_node_type_reply_t NONNULL callback, run_context_t NULLABLE client_queue,
                           const char *NONNULL file, int line);

/* cti_add_service
 *
 * Requests wpantund to add the specified service to the thread network data.  A return value of
 * kCTIStatus_NoError means that the caller can expect the reply callback to be called with a success or fail
 * status exactly one time.  Any other error means that the request could not be sent, and the callback will
 * never be called.
 *
 * context:           An anonymous pointer that will be passed along to the callback when
 *                    an event occurs.
 *
 * callback:          CallBack function for the client that indicates success or failure.
 *
 * client_queue:      Queue the client wants to schedule the callback on
 *
 * enterprise_number: Contains the enterprise number of the service.
 *
 * service_data:      Typically four bytes, in network byte order, the first two bytes indicate
 *                    the type of service within the enterprise' number space, and the second
 *                    two bytes indicate the version number.
 *
 * service_data_len:  The length of the service data in bytes.
 *
 * server_data:       Typically four bytes, in network byte order, the first two bytes indicate
 *                    the type of service within the enterprise' number space, and the second
 *                    two bytes indicate the version number.
 *
 * server_data_len:   The length of the service data in bytes.
 *
 * return value:      Returns kCTIStatus_NoError when no error otherwise returns an error code indicating
 *                    the error that occurred. Note: A return value of kCTIStatus_NoError does not mean
 *                    that the request succeeded, merely that it was successfully started.
 */

#define cti_add_service(server, context, callback, client_queue, \
                        enterprise_number, service_data, service_data_length, server_data, server_data_length) \
    cti_add_service_(server, context, callback, client_queue, enterprise_number, \
                     service_data, service_data_length, server_data, server_data_length, __FILE__, __LINE__)
DNS_SERVICES_EXPORT cti_status_t
cti_add_service_(srp_server_t *NULLABLE server, void *NULLABLE context, cti_reply_t NONNULL callback,
                 run_context_t NULLABLE client_queue, uint32_t enterprise_number, const uint8_t *NONNULL service_data,
                 size_t service_data_length, const uint8_t *NULLABLE server_data, size_t server_data_length,
                 const char *NONNULL file, int line);

/* cti_remove_service
 *
 * Requests wpantund to remove the specified service from the thread network data.  A return value of
 * kCTIStatus_NoError means that the caller can expect the reply callback to be called with a success or fail
 * status exactly one time.  Any other error means that the request could not be sent, and the callback will
 * never be called.
 *
 * context:           An anonymous pointer that will be passed along to the callback when
 *                    an event occurs.
 *
 * callback:          callback function for the client that indicates success or failure.
 *
 * client_queue:      Queue the client wants to schedule the callback on
 *
 * enterprise_number: Contains the enterprise number of the service.
 *
 * service_data:      Typically four bytes, in network byte order, the first two bytes indicate
 *                    the type of service within the enterprise' number space, and the second
 *                    two bytes indicate the version number.
 *
 * service_data_len:  The length of the service data in bytes.
 *
 * return value:      Returns kCTIStatus_NoError when no error otherwise returns an error code indicating
 *                    the error that occurred. Note: A return value of kCTIStatus_NoError does not mean
 *                    that the request succeeded, merely that it was successfully started.
 */

#define cti_remove_service(server, context, callback, client_queue, \
                           enterprise_number, service_data, service_data_length) \
    cti_remove_service_(server, context, callback, client_queue,            \
                        enterprise_number, service_data, service_data_length, __FILE__, __LINE__)
DNS_SERVICES_EXPORT cti_status_t
cti_remove_service_(srp_server_t *NULLABLE server, void *NULLABLE context, cti_reply_t NONNULL callback,
                    run_context_t NULLABLE client_queue, uint32_t enterprise_number, const uint8_t *NONNULL service_data,
                    size_t service_data_length, const char *NONNULL file, int line);

/* cti_add_prefix
 *
 * Requests wpantund to add the specified prefix to the set of off-mesh prefixes configured on the thread
 * network.  A return value of kCTIStatus_NoError means that the caller can expect the reply callback to be
 * called with a success or fail status exactly one time.  Any other error means that the request could not
 * be sent, and the callback will never be called.
 *
 * context:        An anonymous pointer that will be passed along to the callback when
 *                 an event occurs.
 *
 * callback:       CallBack function for the client that indicates success or failure.
 *
 * client_queue:   Queue the client wants to schedule the callback on
 *
 * prefix:         A pointer to a struct in6_addr.  Must not be reatained by the callback.
 *
 * prefix_len:     The length of the prefix in bits.
 *
 * return value:   Returns kCTIStatus_NoError when no error otherwise returns an error code indicating
 *                 the error that occurred. Note: A return value of kCTIStatus_NoError does not mean
 *                 that the request succeeded, merely that it was successfully started.
 */

#define cti_add_prefix(server, context, callback, client_queue, \
                       prefix, prefix_length, on_mesh, preferred, slaac, stable, priority) \
    cti_add_prefix_(server, context, callback, client_queue, prefix, prefix_length, \
                    on_mesh, preferred, slaac, stable, priority, __FILE__, __LINE__)
DNS_SERVICES_EXPORT cti_status_t
cti_add_prefix_(srp_server_t *NULLABLE server, void *NULLABLE context, cti_reply_t NONNULL callback, run_context_t NULLABLE client_queue,
                struct in6_addr *NONNULL prefix, int prefix_length, bool on_mesh, bool preferred, bool slaac,
                bool stable, int priority, const char *NONNULL file, int line);

/* cti_remove_prefix
 *
 * Requests wpantund to remove the specified prefix from the set of off-mesh prefixes configured on the thread network.
 * A return value of kCTIStatus_NoError means that the caller can expect the reply callback to be called with a success
 * or fail status exactly one time.  Any other error means that the request could not be sent, and the callback will
 * never be called.
 *
 * context:        An anonymous pointer that will be passed along to the callback when
 *                 an event occurs.
 *
 * callback:       CallBack function for the client that indicates success or failure.
 *
 * client_queue:   Queue the client wants to schedule the callback on
 *
 * prefix:         A pointer to a struct in6_addr.  Must not be reatained by the callback.
 *
 * prefix_len:     The length of the prefix in bits.
 *
 * return value:   Returns kCTIStatus_NoError when no error otherwise returns an error code indicating
 *                 the error that occurred. Note: A return value of kCTIStatus_NoError does not mean
 *                 that the request succeeded, merely that it was successfully started.
 */

#define cti_remove_prefix(server, context, callback, client_queue, prefix, prefix_length) \
    cti_remove_prefix_(server, context, callback, client_queue, prefix, prefix_length, __FILE__, __LINE__)
DNS_SERVICES_EXPORT cti_status_t
cti_remove_prefix_(srp_server_t *NULLABLE server, void *NULLABLE context, cti_reply_t NONNULL callback,
                   run_context_t NULLABLE client_queue, struct in6_addr *NONNULL prefix, int prefix_length,
                   const char *NONNULL file, int line);

/* cti_add_route
 *
 * Requests wpantund to add the specified route on the thread network.
 * A return value of kCTIStatus_NoError means that the caller can expect the reply callback to be
 * called with a success or fail status exactly one time.  Any other error means that the request could not
 * be sent, and the callback will never be called.
 *
 * context:        An anonymous pointer that will be passed along to the callback when
 *                 an event occurs.
 *
 * callback:       CallBack function for the client that indicates success or failure.
 *
 * client_queue:   Queue the client wants to schedule the callback on
 *
 * prefix:         A pointer to a struct in6_addr.
 *
 * prefix_len:     The length of the prefix in bits.
 *
 * priority:       Route priority (>0 for high, 0 for medium, <0 for low).
 *
 * domain_id:      Domain id for the route (default is zero).
 *
 * stable:         True if the route is part of stable network data.
 *
 * nat64:          True if this is NAT64 prefix.
 *
 * return value:   Returns kCTIStatus_NoError when no error otherwise returns an error code indicating
 *                 the error that occurred. Note: A return value of kCTIStatus_NoError does not mean
 *                 that the request succeeded, merely that it was successfully started.
 */

#define cti_add_route(server, context, callback, client_queue, \
                       prefix, prefix_length, priority, domain_id, stable, nat64) \
    cti_add_route_(server, context, callback, client_queue, prefix, prefix_length, \
                    priority, domain_id, stable, nat64, __FILE__, __LINE__)
DNS_SERVICES_EXPORT cti_status_t
cti_add_route_(srp_server_t *NULLABLE server, void *NULLABLE context, cti_reply_t NONNULL callback,
               run_context_t NULLABLE client_queue, struct in6_addr *NONNULL prefix, int prefix_length,
               int priority, int domain_id, bool stable, bool nat64, const char *NONNULL file, int line);

/* cti_remove_route
 *
 * Requests wpantund to remove the specified route on the thread network.
 * A return value of kCTIStatus_NoError means that the caller can expect the reply callback to be called with a success
 * or fail status exactly one time.  Any other error means that the request could not be sent, and the callback will
 * never be called.
 *
 * context:        An anonymous pointer that will be passed along to the callback when
 *                 an event occurs.
 *
 * callback:       CallBack function for the client that indicates success or failure.
 *
 * client_queue:   Queue the client wants to schedule the callback on
 *
 * prefix:         A pointer to a struct in6_addr.
 *
 * prefix_len:     The length of the prefix in bits.
 *
 * domain_id:      Domain id for the route (default is zero).
 *
 * return value:   Returns kCTIStatus_NoError when no error otherwise returns an error code indicating
 *                 the error that occurred. Note: A return value of kCTIStatus_NoError does not mean
 *                 that the request succeeded, merely that it was successfully started.
 */

#define cti_remove_route(server, context, callback, client_queue, prefix, prefix_length, domain_id) \
    cti_remove_route_(server, context, callback, client_queue, prefix, prefix_length, domain_id, __FILE__, __LINE__)
DNS_SERVICES_EXPORT cti_status_t
cti_remove_route_(srp_server_t *NULLABLE server, void *NULLABLE context, cti_reply_t NONNULL callback,
                  run_context_t NULLABLE client_queue, struct in6_addr *NONNULL prefix, int prefix_length,
                  int domain_id, const char *NONNULL file, int line);

/*
 * cti_route_vec_create
 *
 * creates a route array vector of specified length
 *
 * num_routes:    Number of route slots available in the route vector.
 *
 * return value:  NULL, if the call failed; otherwise a prefix vector capable of containing the
 *                requested number of routes.
 */
cti_route_vec_t *NULLABLE
cti_route_vec_create_(size_t num_routes, const char *NONNULL file, int line);
#define cti_route_vec_create(num_routes) cti_route_vec_create_(num_routes, __FILE__, __LINE__)

/*
 * cti_route_vec_release
 *
 * decrements the reference count on the provided route vector and, if it reaches zero, finalizes the route vector,
 * which calls cti_route_release on each route in the vector, potentially also finalizing them.
 *
 * num_routes:     Number of route slots available in the route vector.
 *
 * return value:   NULL, if the call failed; otherwise a route vector capable of containing the
 *                 requested number of routes.
 */

void
cti_route_vec_release_(cti_route_vec_t *NONNULL routes, const char *NONNULL file, int line);
#define cti_route_vec_release(routes) cti_route_vec_release_(routes, __FILE__, __LINE__)

/*
 * cti_route_create
 *
 * Creates a cti_route_t containing the specified information. The route is retained, and will be
 * freed using free() when the route object is finalized. Caller must not retain or free these values, and
 * they must be allocated on the malloc heap.
 *
 * prefix:            A pointer to a struct in6_addr.
 *
 * prefix_len:        The length of the prefix in bits.
 *
 * origin:            User or ncp.
 *
 * nat64:             True if this is NAT64 prefix.
 *
 * stable:            True if the route is part of stable network data.
 *
 * preference:        Route priority.
 *
 * rloc:              Routing locator.
 *
 * next_hop_is_host:  True if next hop is host.
 *
 * return value:      NULL, if the call failed; otherwise a route object containing the specified state.
 */

cti_route_t *NULLABLE
cti_route_create_(struct in6_addr *NONNULL prefix, int prefix_length, offmesh_route_origin_t origin,
                  bool nat64, bool stable, offmesh_route_preference_t preference, int rloc,
                  bool next_hop_is_host, const char *NONNULL file, int line);
#define cti_route_create(prefix, prefix_length, origin, nat64, stable, preference, rloc, next_hop_is_host) \
    cti_route_create_(prefix, prefix_length, origin, nat64, stable, preference, rloc, next_hop_is_host, __FILE__, __LINE__)

/*
 * cti_route_release
 *
 * Decrements the reference count on the provided route vector and, if it reaches zero, finalizes the route vector,
 * which calls cti_route_release on each route in the vector, potentially also finalizing them.
 *
 * routes:           The route vector to release.
 *
 * return value:     NULL, if the call failed; otherwise a route vector capable of containing the requested number of
 *                   routes.
 */

void
cti_route_release_(cti_route_t *NONNULL route, const char *NONNULL file, int line);
#define cti_route_release(routes) cti_route_release(route, __FILE__, __LINE__)

/* cti_offmesh_route_reply: Callback from cti_get_offmesh_route_list()
 *
 * Called when an error occurs during processing of the cti_get_offmesh_route_list call, or when a route
 * is added or removed.
 *
 * In the case of an error, the callback will not be called again, and the caller is responsible for
 * releasing the connection state and restarting if needed.
 *
 * The callback will be called once for each offmesh route present on the Thread network at the time
 * cti_get_offmesh_prefix_list() is first called, and then again whenever a route is added or removed.
 *
 * cti_offmesh_route_reply parameters:
 *
 * context:           The context that was passed to the cti_get_offmesh_route_list call to which this is a callback.
 *
 * route_vec:         If status is kCTIStatus_NoError, a vector containing all of the routes that were reported in
 *                    this event.
 *
 * status:	          Will be kCTIStatus_NoError if the offmesh route list request is successful, or
 *                    will indicate the failure that occurred.
 */

typedef void
(*cti_offmesh_route_reply_t)(void *NULLABLE context, cti_route_vec_t *NULLABLE routes, cti_status_t status);

/* cti_get_offmesh_route_list
 *
 * Requests wpantund to immediately send the current list of off-mesh routes configured in the Thread
 * network data.  Whenever the route list is updated, the callback will be called again with the new
 * information.  A return value of kCTIStatus_NoError means that the caller can expect the reply callback to be
 * called at least once.  Any other error means that the request could not be sent, and the callback will
 * never be called.
 *
 * ref:            A pointer to a reference to the connection is stored through ref if ref is not NULL.
 *                 When events are no longer needed, call cti_discontinue_events() on the returned pointer.
 *
 * context:        An anonymous pointer that will be passed along to the callback when
 *                 an event occurs.
 *
 * callback:       CallBack function for the client that indicates success or failure.
 *
 * client_queue:   Queue the client wants to schedule the callback on
 *
 * return value:   Returns kCTIStatus_NoError when no error otherwise returns an error code indicating
 *                 the error that occurred. Note: A return value of kCTIStatus_NoError does not mean
 *                 that the request succeeded, merely that it was successfully started.
 */

#define cti_get_offmesh_route_list(server, ref, context, callback, client_queue) \
    cti_get_offmesh_route_list_(server, ref, context, callback, client_queue, __FILE__, __LINE__)
DNS_SERVICES_EXPORT cti_status_t
cti_get_offmesh_route_list_(srp_server_t *NULLABLE server, cti_connection_t NULLABLE *NULLABLE ref,
                            void *NULLABLE context, cti_offmesh_route_reply_t NONNULL callback,
                            run_context_t NULLABLE client_queue, const char *NONNULL file, int line);

/* cti_onmesh_prefix_reply: Callback from cti_get_onmesh_prefix_list()
 *
 * Called when an error occurs during processing of the cti_get_onmesh_prefix_list call, or when a route
 * is added or removed.
 *
 * In the case of an error, the callback will not be called again, and the caller is responsible for
 * releasing the connection state and restarting if needed.
 *
 * The callback will be called once for each offmesh route present on the Thread network at the time
 * cti_get_offmesh_prefix_list() is first called, and then again whenever a route is added or removed.
 *
 * cti_onmesh_prefix_reply parameters:
 *
 * context:           The context that was passed to the cti_get_onmesh_prefix_list call to which this is a callback.
 *
 * prefix_vec:        If status is kCTIStatus_NoError, a vector containing all of the prefixes that were reported in
 *                    this event.
 *
 * status:	          Will be kCTIStatus_NoError if the onmesh prefix list request is successful, or
 *                    will indicate the failure that occurred.
 */

typedef void
(*cti_onmesh_prefix_reply_t)(void *NULLABLE context, cti_prefix_vec_t *NULLABLE routes, cti_status_t status);

/* cti_get_onmesh_prefix_list
 *
 * Requests wpantund to immediately send the current list of on-mesh prefixes configured in the Thread
 * network data.  Whenever the prefix list is updated, the callback will be called again with the new
 * information.  A return value of kCTIStatus_NoError means that the caller can expect the reply callback to be
 * called at least once.  Any other error means that the request could not be sent, and the callback will
 * never be called.
 *
 * ref:            A pointer to a reference to the connection is stored through ref if ref is not NULL.
 *                 When events are no longer needed, call cti_discontinue_events() on the returned pointer.
 *
 * context:        An anonymous pointer that will be passed along to the callback when
 *                 an event occurs.
 *
 * callback:       CallBack function for the client that indicates success or failure.
 *
 * client_queue:   Queue the client wants to schedule the callback on
 *
 * return value:   Returns kCTIStatus_NoError when no error otherwise returns an error code indicating
 *                 the error that occurred. Note: A return value of kCTIStatus_NoError does not mean
 *                 that the request succeeded, merely that it was successfully started.
 */

#define cti_get_onmesh_prefix_list(server, ref, context, callback, client_queue) \
    cti_get_onmesh_prefix_list_(server, ref, context, callback, client_queue, __FILE__, __LINE__)
DNS_SERVICES_EXPORT cti_status_t
cti_get_onmesh_prefix_list_(srp_server_t *NULLABLE server, cti_connection_t NULLABLE *NULLABLE ref,
                            void *NULLABLE context, cti_onmesh_prefix_reply_t NONNULL callback,
                            run_context_t NULLABLE client_queue, const char *NONNULL file, int line);

/* cti_rloc16_reply: Callback from cti_get_rloc16()
 *
 * Called when an error occurs during processing of the cti_get_rloc16 call, or when rloc16
 * is updated.
 *
 * In the case of an error, the callback will not be called again, and the caller is responsible for
 * releasing the connection state and restarting if needed.
 *
 * The callback will be called initially to report the current value for the property, and subsequently
 * whenever the property changes.
 *
 * cti_rloc16_reply parameters:
 *
 * context:           The context that was passed to the cti call to which this is a callback.
 *
 * rloc16:            The rloc16 value(only valid if status is kCTIStatus_NoError).
 *
 * status:            Will be kCTIStatus_NoError if the rloc16 request is successful, or will indicate the failure
 *                    that occurred.
 */
typedef void
(*cti_rloc16_reply_t)(void *NULLABLE context, uint16_t rloc16, cti_status_t status);

/* cti_get_rloc16
 *
 * Requests wpantund to immediately send the rloc16 of the local device. Whenever the RLOC16
 * changes, the callback will be called again with the new RLOC16.  A return value of
 * kCTIStatus_NoError means that the caller can expect the reply callback to be called at least once.  Any
 * other error means that the request could not be sent, and the callback will never be called.
 *
 * ref:            A pointer to a reference to the connection is stored through ref if ref is not NULL.
 *                 When events are no longer needed, call cti_discontinue_events() on the returned pointer.
 *
 * context:        An anonymous pointer that will be passed along to the callback when
 *                 an event occurs.
 * callback:       CallBack function for the client that indicates success or failure.
 *
 * client_queue:   Queue the client wants to schedule the callback on
 *
 * return value:   Returns kCTIStatus_NoError when no error otherwise returns an error code indicating
 *                 the error that occurred. Note: A return value of kCTIStatus_NoError does not mean
 *                 that the request succeeded, merely that it was successfully started.
 */
#define cti_get_rloc16(server, ref, context, callback, client_queue) \
    cti_get_rloc16_(server, ref, context, callback, client_queue, __FILE__, __LINE__)

DNS_SERVICES_EXPORT cti_status_t
cti_get_rloc16_(srp_server_t *NULLABLE server, cti_connection_t NULLABLE *NULLABLE ref,
                void *NULLABLE context, cti_rloc16_reply_t NONNULL callback,
                run_context_t NULLABLE client_queue, const char *NONNULL file, int line);

/* cti_active_data_set_change_reply: Callback from cti_get_active_data_set_change()
 *
 * Called when an error occurs during processing of the cti_get_active_data_set_change call, or when the active data set
 * is updated.
 *
 * In the case of an error, the callback will not be called again, and the caller is responsible for
 * releasing the connection state and restarting if needed.
 *
 * The callback will be called whenever the active data set changes.
 *
 * cti_active_data_set_change_reply parameters:
 *
 * context:           The context that was passed to the cti call to which this is a callback.
 *
 * status:            Will be kCTIStatus_NoError if the active_data_set_change request is successful, or will indicate the failure
 *                    that occurred.
 */
typedef void
(*cti_reply_t)(void *NULLABLE context, cti_status_t status);

/* cti_get_active_data_set_change
 *
 * Requests wpantund to immediately send the active_data_set_change of the local device. Whenever the ACTIVE_DATA_SET_CHANGE
 * changes, the callback will be called again with the new ACTIVE_DATA_SET_CHANGE.  A return value of
 * kCTIStatus_NoError means that the caller can expect the reply callback to be called at least once.  Any
 * other error means that the request could not be sent, and the callback will never be called.
 *
 * ref:            A pointer to a reference to the connection is stored through ref if ref is not NULL.
 *                 When events are no longer needed, call cti_discontinue_events() on the returned pointer.
 *
 * context:        An anonymous pointer that will be passed along to the callback when
 *                 an event occurs.
 * callback:       CallBack function for the client that indicates success or failure.
 *
 * client_queue:   Queue the client wants to schedule the callback on
 *
 * return value:   Returns kCTIStatus_NoError when no error otherwise returns an error code indicating
 *                 the error that occurred. Note: A return value of kCTIStatus_NoError does not mean
 *                 that the request succeeded, merely that it was successfully started.
 */
#define cti_track_active_data_set(server, ref, context, callback, client_queue) \
    cti_track_active_data_set_(server, ref, context, callback, client_queue, __FILE__, __LINE__)

DNS_SERVICES_EXPORT cti_status_t
cti_track_active_data_set_(srp_server_t *NULLABLE server, cti_connection_t NULLABLE *NULLABLE ref,
                void *NULLABLE context, cti_reply_t NONNULL callback,
                run_context_t NULLABLE client_queue, const char *NONNULL file, int line);

/* cti_wed_change_reply: Callback from cti_get_wed_change()
 *
 * Called when an error occurs during processing of the cti_get_wed_change call, or when the active data set
 * is updated.
 *
 * In the case of an error, the callback will not be called again, and the caller is responsible for
 * releasing the connection state and restarting if needed.
 *
 * The callback will be called whenever the active data set changes.
 *
 * cti_wed_change_reply parameters:
 *
 * context:           The context that was passed to the cti call to which this is a callback.
 *
 * status:            Will be kCTIStatus_NoError if the wed_change request is successful, or will indicate the failure
 *                    that occurred.
 */
typedef void
(*cti_reply_t)(void *NULLABLE context, cti_status_t status);

/* cti_wed_reply: Callback for get calls that fetch the current WED status
 *
 * Called one or more times in response to a cti_track_wed_status() call, either with an error or with the current WED status.
 *
 * cti_wed_reply parameters:
 *
 * context:       The context that was passed to the cti service call to which this is a callback.
 *
 * ext_address:   If error is kCTIStatus_NoError, a string containing the name of the ext address.
 *
 * ml_eid:        If error is kCTIStatus_NoError, a string containing the ML-EID
 *
 * added:         If true, this was added; otherwise it was removed.
 *
 * status:	      Will be kCTIStatus_NoError on success, otherwise will indicate the
 * 			      failure that occurred.
 */

typedef void
(*cti_wed_reply_t)(void *NULLABLE context, const char *NULLABLE ext_address, const char *NULLABLE ml_eid, bool added,
                   cti_status_t status);

/* cti_get_wed_change
 *
 * Requests wpantund to immediately send the WED attachment status of the local device. Whenever the WED attachment
 * status changes, the callback will be called again with the new status.  A return value of kCTIStatus_NoError means
 * that the caller can expect the reply callback to be called at least once.  Any other error means that the request
 * could not be sent, and the callback will never be called.
 *
 * ref:            A pointer to a reference to the connection is stored through ref if ref is not NULL.
 *                 When events are no longer needed, call cti_discontinue_events() on the returned pointer.
 *
 * context:        An anonymous pointer that will be passed along to the callback when
 *                 an event occurs.
 *
 * callback:       CallBack function for the client that indicates success or failure, and on success
 *                 indicates attachment to a Wake-on End Device (WED).
 *
 * client_queue:   Queue the client wants to schedule the callback on
 *
 * return value:   Returns kCTIStatus_NoError when no error otherwise returns an error code indicating
 *                 the error that occurred. Note: A return value of kCTIStatus_NoError does not mean
 *                 that the request succeeded, merely that it was successfully started.
 */
#define cti_track_wed_status(server, ref, context, callback, client_queue) \
    cti_track_wed_status_(server, ref, context, callback, client_queue, __FILE__, __LINE__)

DNS_SERVICES_EXPORT cti_status_t
cti_track_wed_status_(srp_server_t *NULLABLE server, cti_connection_t NULLABLE *NULLABLE ref,
                      void *NULLABLE context, cti_wed_reply_t NONNULL callback,
                      run_context_t NULLABLE client_queue, const char *NONNULL file, int line);

/* cti_add_ml_eid_mapping
 *
 * Provides the thread daemon with a mapping between an IP address registered by an SRP client and
 * the source address from which the registration was received, which will be the client's ml-eid.
 * Additionally provides the client's hostname.
 *
 * context:        An anonymous pointer that will be passed along to the callback when
 *                 an event occurs.
 *
 * callback:       CallBack function for the client that indicates success or failure, and on success
 *                 indicates that the update was accepted by the thread daemon.
 *
 * omr_addr:       IP address on the OMR prefix registered by the client.
 *
 * ml_eid:         IP address on the mesh-local prefix of the client.
 *
 * client_queue:   Queue the client wants to schedule the callback on
 *
 *
 * return value:   Returns kCTIStatus_NoError when no error otherwise returns an error code indicating
 *                 the error that occurred. Note: A return value of kCTIStatus_NoError does not mean
 *                 that the request succeeded, merely that it was successfully started.
 */
#define cti_add_ml_eid_mapping(server, context, callback, client_queue, omr_addr, ml_eid, hostname) \
    cti_add_ml_eid_mapping_(server, context, callback, client_queue, omr_addr, ml_eid, hostname, __FILE__, __LINE__)

DNS_SERVICES_EXPORT cti_status_t
cti_add_ml_eid_mapping_(srp_server_t *NULLABLE server, void *NULLABLE context,
                        cti_reply_t NONNULL callback, run_context_t NULLABLE client_queue,
                        struct in6_addr *NONNULL omr_addr, struct in6_addr *NONNULL ml_eid,
                        const char *NONNULL hostname, const char *NONNULL file, int line);

/* cti_events_discontinue
 *
 * Requests that the CTI library stop delivering events on the specified connection.   The connection will have
 * been returned by a CTI library call that subscribes to events.
 */
DNS_SERVICES_EXPORT cti_status_t
cti_events_discontinue(cti_connection_t NONNULL ref);

typedef union cti_callback {
    cti_reply_t NULLABLE reply;
    cti_string_property_reply_t NONNULL string_property_reply;
    cti_service_reply_t NONNULL service_reply;
    cti_prefix_reply_t NONNULL prefix_reply;
    cti_state_reply_t NONNULL state_reply;
    cti_uint64_property_reply_t NONNULL uint64_property_reply;
    cti_network_node_type_reply_t NONNULL network_node_type_reply;
    cti_offmesh_route_reply_t NONNULL offmesh_route_reply;
    cti_onmesh_prefix_reply_t NONNULL onmesh_prefix_reply;
    cti_rloc16_reply_t NONNULL rloc16_reply;
    cti_wed_reply_t NONNULL wed_reply;
} cti_callback_t;

#endif /* __CTI_SERVICES_H__ */

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 120
// indent-tabs-mode: nil
// End:
