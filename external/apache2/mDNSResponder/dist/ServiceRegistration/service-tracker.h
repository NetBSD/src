/* service-tracker.h
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

#ifndef __SERVICE_TRACKER_H__
#define __SERVICE_TRACKER_H__ 1

typedef struct service_tracker_callback service_tracker_callback_t;
typedef struct service_tracker service_tracker_t;
typedef struct srp_server_state srp_server_t;

RELEASE_RETAIN_DECLS(service_tracker);
#define service_tracker_retain(watcher) service_tracker_retain_(watcher, __FILE__, __LINE__)
#define service_tracker_release(watcher) service_tracker_release_(watcher, __FILE__, __LINE__)
void service_tracker_stop(service_tracker_t *NONNULL tracker);
void service_tracker_cancel(service_tracker_t *NONNULL tracker);
bool service_tracker_local_service_seen(service_tracker_t *NONNULL tracker);
service_tracker_t *NULLABLE service_tracker_create(srp_server_t *NONNULL route_state);
void service_tracker_set_reconnect_callback(service_tracker_t *NONNULL tracker,
										  void (*NULLABLE reconnect_callback)(void *NULLABLE context));
void service_tracker_start(service_tracker_t *NONNULL tracker);
bool service_tracker_callback_add(service_tracker_t *NONNULL tracker, void (*NONNULL callback)(void *NULLABLE context),
								  void (*NULLABLE context_release)(void *NULLABLE context), void *NULLABLE context);
void service_tracker_callback_cancel(service_tracker_t *NONNULL tracker, void *NONNULL context);
thread_service_t *NULLABLE service_tracker_services_get(service_tracker_t *NULLABLE tracker);
void service_tracker_thread_service_note(service_tracker_t *NONNULL tracker,
                                         thread_service_t *NONNULL service,
                                         const char *NONNULL event_description);
bool service_tracker_verified_service_still_exists(service_tracker_t *NULLABLE tracker,
                                                   thread_service_t *NULLABLE old_service);
thread_service_t *NULLABLE service_tracker_verified_service_get(service_tracker_t *NULLABLE tracker);
thread_service_t *NULLABLE service_tracker_unverified_service_get(service_tracker_t *NULLABLE tracker,
                                                                  thread_service_type_t service_type);
void service_tracker_verify_next_service(service_tracker_t *NULLABLE tracker);
void service_tracker_cancel_probes(service_tracker_t *NULLABLE tracker);
#endif // __SERVICE_TRACKER_H__

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 120
// indent-tabs-mode: nil
// End:
