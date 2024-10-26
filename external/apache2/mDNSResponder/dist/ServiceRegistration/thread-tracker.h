/* node-type-tracker.h
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
 * This file contains general support definitions for the thread network state tracker.
 */

#ifndef __THREAD_TRACKER_H__
#define __THREAD_TRACKER_H__ 1

typedef struct thread_tracker thread_tracker_t;

typedef enum thread_thread {
    thread_network_state_uninitialized,
    thread_network_state_fault,
    thread_network_state_upgrading,
    thread_network_state_deep_sleep,
    thread_network_state_offline,
    thread_network_state_commissioned,
    thread_network_state_associating,
    thread_network_state_credentials_needed,
    thread_network_state_associated,
    thread_network_state_isolated,
    thread_network_state_asleep,
    thread_network_state_waking,
    thread_network_state_unknown
} thread_network_state_t;

RELEASE_RETAIN_DECLS(thread_tracker);
#define thread_tracker_retain(watcher) thread_tracker_retain_(watcher, __FILE__, __LINE__)
#define thread_tracker_release(watcher) thread_tracker_release_(watcher, __FILE__, __LINE__)
const char *NONNULL thread_tracker_network_state_to_string(thread_network_state_t state);
void thread_tracker_cancel(thread_tracker_t *NONNULL publisher);
thread_tracker_t *NULLABLE thread_tracker_create(srp_server_t *NONNULL route_state);
void thread_tracker_set_reconnect_callback(thread_tracker_t *NONNULL tracker,
										  void (*NULLABLE reconnect_callback)(void *NULLABLE context));
void thread_tracker_start(thread_tracker_t *NONNULL tracker);
bool thread_tracker_callback_add(thread_tracker_t *NONNULL tracker, void (*NONNULL callback)(void *NULLABLE context),
								  void (*NULLABLE context_release)(void *NULLABLE context), void *NULLABLE context);
void thread_tracker_callback_cancel(thread_tracker_t *NONNULL tracker, void *NONNULL context);
thread_network_state_t thread_tracker_state_get(thread_tracker_t *NULLABLE tracker, bool previous);
bool thread_tracker_associated_get(thread_tracker_t *NULLABLE tracker, bool previous);
#endif // __THREAD_TRACKER_H__

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 120
// indent-tabs-mode: nil
// End:
