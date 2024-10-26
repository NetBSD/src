/* omr-publisher.h
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
 * This file contains general support definitions for the Off-Mesh Routable
 * (OMR) prefix publisher state machine.
 */

#ifndef __OMR_PUBLISHER_H__
#define __OMR_PUBLISHER_H__ 1

typedef struct dhcpv6_client dhcpv6_client_t;
typedef struct br_event br_event_t;
typedef struct omr_publisher omr_publisher_t;

#define OMR_PUBLISHER_START_WAIT	    15 * 1000 // Start wait interval is random(0..15) seconds
#define OMR_PUBLISHER_DHCP_SUCCESS_WAIT      1500 // one and a half seconds (no need for jitter since we already jittered)
#define OMR_PUBLISHER_MIN_START              3000 // three seconds (minimum, for new router coming to existing network)

RELEASE_RETAIN_DECLS(omr_publisher);
#define omr_publisher_retain(publisher) omr_publisher_retain_(publisher, __FILE__, __LINE__)
#define omr_publisher_release(publisher) omr_publisher_release_(publisher, __FILE__, __LINE__)
void omr_publisher_cancel(omr_publisher_t *NONNULL publisher);
omr_publisher_t *NULLABLE omr_publisher_create(route_state_t *NONNULL route_state, const char *NONNULL name);
void omr_publisher_set_omr_watcher(omr_publisher_t *NONNULL omr_publisher, omr_watcher_t *NONNULL omr_watcher);
void omr_publisher_set_reconnect_callback(omr_publisher_t *NONNULL omr_publisher,
										  void (*NULLABLE reconnect_callback)(void *NULLABLE context));
void omr_publisher_start(omr_publisher_t *NONNULL publisher);
omr_prefix_t *NULLABLE omr_publisher_published_prefix_get(omr_publisher_t *NONNULL publisher);
void omr_publisher_force_publication(omr_publisher_t *NONNULL publisher, omr_prefix_priority_t priority);
void omr_publisher_interface_configuration_changed(omr_publisher_t *NONNULL publisher);
bool omr_publisher_publishing_dhcp(omr_publisher_t *NONNULL publisher);
bool omr_publisher_publishing_ula(omr_publisher_t *NONNULL publisher);
bool omr_publisher_publishing_prefix(omr_publisher_t *NONNULL publisher);

// The OMR publisher knows whether the prefix being published can be used for routing, even if it's not publishing it itself.
// If there is a medium- or high-priority prefix published by some other router, that prefix can be assumed to be routable.
// If the OMR publisher is publishing a DHCP route, that prefix can be assumed to be routable. If another router is publishing
// a low-priority prefix, or OMR publisher is, that prefix can't be used for routing off of the adjacent infrastructure link.
// Of course it still works for routing between Thread and the adjacent infrastructure link--what it can't be used for is
// routing across a multi-link infrastructure network or to the internet.
bool omr_publisher_have_routable_prefix(omr_publisher_t *NONNULL publisher);

// Check that the prefix that we saw in an RA is not also the one we are publishing on Thread. This can happen with broken DHCP
// PD servers, and we have seen it in some home routers.
void omr_publisher_check_prefix(omr_publisher_t *NULLABLE publisher, struct in6_addr *NONNULL prefix, int len);

void omr_publisher_unpublish_prefix(omr_publisher_t *NONNULL publisher);
#endif // __OMR_PUBLISHER_H__

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 120
// indent-tabs-mode: nil
// End:
