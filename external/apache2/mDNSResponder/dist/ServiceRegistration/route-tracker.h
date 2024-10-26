/* route-tracker.h
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
 * This file contains interface definitions for a route tracker for tracking prefixes and routes on infrastructure so
 * that they can be published on the Thread network.
 */

#ifndef __ROUTE_TRACKER_H__
#define __ROUTE_TRACKER_H__ 1

typedef struct route_tracker route_tracker_t;

#ifndef BUILD_TEST_ENTRY_POINTS
RELEASE_RETAIN_DECLS(route_tracker);
#define route_tracker_retain(watcher) route_tracker_retain_(watcher, __FILE__, __LINE__)
#define route_tracker_release(watcher) route_tracker_release_(watcher, __FILE__, __LINE__)
#else
typedef struct route_state route_state_t;
typedef struct interface interface_t;
#endif // BUILD_TEST_ENTRY_POINTS

// Cancel the tracker.
void route_tracker_cancel(route_tracker_t *NONNULL tracker);

// Create a route tracker.
route_tracker_t *NULLABLE route_tracker_create(route_state_t *NONNULL route_state, const char *NONNULL name);
void route_tracker_set_reconnect_callback(route_tracker_t *NONNULL route_tracker,
                                          void (*NULLABLE reconnect_callback)(void *NULLABLE context));
void route_tracker_start(route_tracker_t *NONNULL tracker);
void route_tracker_shutdown(route_state_t *NULLABLE route_state);
bool route_tracker_check_for_gua_prefixes_on_infrastructure(route_tracker_t *NULLABLE tracker);
#ifndef BUILD_TEST_ENTRY_POINTS
void route_tracker_route_state_changed(route_tracker_t *NONNULL tracker, interface_t *NULLABLE interface);
void route_tracker_interface_configuration_changed(route_tracker_t *NONNULL tracker);
void route_tracker_monitor_mesh_routes(route_tracker_t *NONNULL tracker, cti_route_vec_t *NONNULL routes);
bool route_tracker_local_routes_seen(route_tracker_t *NULLABLE tracker);
#else //  BUILD_TEST_ENTRY_POINTS
void route_tracker_test_start(int iterations);
#endif //  BUILD_TEST_ENTRY_POINTS
#endif // __ROUTE_TRACKER_H__

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 120
// indent-tabs-mode: nil
// End:
