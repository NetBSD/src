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

#ifndef __NODE_TYPE_TRACKER_H__
#define __NODE_TYPE_TRACKER_H__ 1

typedef struct node_type_tracker node_type_tracker_t;

typedef enum thread_node_type {
    node_type_unknown,
    node_type_router,
    node_type_end_device,
    node_type_sleepy_end_device,
    node_type_synchronized_sleepy_end_device,
    node_type_nest_lurker,
    node_type_commissioner,
    node_type_leader,
    node_type_sleepy_router,
} thread_node_type_t;

RELEASE_RETAIN_DECLS(node_type_tracker);
#define node_type_tracker_retain(watcher) node_type_tracker_retain_(watcher, __FILE__, __LINE__)
#define node_type_tracker_release(watcher) node_type_tracker_release_(watcher, __FILE__, __LINE__)
const char *NONNULL node_type_tracker_thread_node_type_to_string(thread_node_type_t node_type);
void node_type_tracker_cancel(node_type_tracker_t *NONNULL publisher);
node_type_tracker_t *NULLABLE node_type_tracker_create(srp_server_t *NONNULL route_state);
void node_type_tracker_set_reconnect_callback(node_type_tracker_t *NONNULL tracker,
										  void (*NULLABLE reconnect_callback)(void *NULLABLE context));
void node_type_tracker_start(node_type_tracker_t *NONNULL tracker);
bool node_type_tracker_callback_add(node_type_tracker_t *NONNULL tracker, void (*NONNULL callback)(void *NULLABLE context),
								  void (*NULLABLE context_release)(void *NULLABLE context), void *NULLABLE context);
void node_type_tracker_callback_cancel(node_type_tracker_t *NONNULL tracker, void *NONNULL context);
thread_node_type_t node_type_tracker_thread_node_type_get(node_type_tracker_t *NULLABLE tracker, bool previous);
#endif // __NODE_TYPE_TRACKER_H__

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 120
// indent-tabs-mode: nil
// End:
