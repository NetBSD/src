/* omr-watcher.h
 *
 * Copyright (c) 2023-2024 Apple Inc. All rights reserved.
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
 * This code adds border router support to 3rd party HomeKit Routers as part of Apple’s commitment to the CHIP project.
 *
 * This file contains the interface for the omr_watcher_t object, which tracks off-mesh-routable prefixes on the
 * Thread network.
 */

#ifndef __OMR_WATCHER_H__
#define __OMR_WATCHER_H__ 1

typedef struct omr_watcher omr_watcher_t;
typedef struct omr_watcher_callback omr_watcher_callback_t;
typedef enum {
    omr_watcher_event_prefix_withdrawn,
    omr_watcher_event_prefix_flags_changed,
    omr_watcher_event_prefix_added,
    omr_watcher_event_prefix_update_finished
} omr_watcher_event_type_t;

typedef enum {
    omr_prefix_priority_invalid,
    omr_prefix_priority_low,
    omr_prefix_priority_medium,
    omr_prefix_priority_high,
} omr_prefix_priority_t;

struct omr_prefix {
    int ref_count;
    omr_prefix_t *NULLABLE next;
    struct in6_addr prefix;
    int prefix_length;
    int metric;
    int rloc;
    int flags;
    omr_prefix_priority_t priority;
    bool user, ncp, stable, onmesh, slaac, dhcp, preferred;
    bool previous_user, previous_ncp, previous_stable;
    bool added, removed, ignore;
    thread_service_publication_state_t publication_state;
};

typedef void (*omr_watcher_event_callback_t)(route_state_t *NONNULL route_state, void *NULLABLE context, omr_watcher_event_type_t event_type,
                                             omr_prefix_t *NULLABLE prefixes, omr_prefix_t *NULLABLE prefix);
typedef void (*omr_watcher_context_release_callback_t)(route_state_t *NONNULL route_state, void *NULLABLE context);

// Release/retain functions for omr_watcher_t:
RELEASE_RETAIN_DECLS(omr_watcher);
#define omr_watcher_retain(watcher) omr_watcher_retain_(watcher, __FILE__, __LINE__)
#define omr_watcher_release(watcher) omr_watcher_release_(watcher, __FILE__, __LINE__)
RELEASE_RETAIN_DECLS(omr_prefix);
#define omr_prefix_retain(prefix) omr_prefix_retain_(prefix, __FILE__, __LINE__)
#define omr_prefix_release(prefix) omr_prefix_release_(prefix, __FILE__, __LINE__)

// omr_prefix_create
//
// Allocate an omr_prefix_t and initialize it with the specified settings.

omr_prefix_t *NULLABLE omr_prefix_create(struct in6_addr *NONNULL prefix, int prefix_length, int metric, int flags,
                                         int rloc, bool stable, bool ncp);

// omr_prefix_flags_generate
//
// Given various common parameters, generate a flags word in the format expected by OpenThread for prefixes.

int omr_prefix_flags_generate(bool on_mesh, bool preferred, bool slaac, omr_prefix_priority_t priority);

// omr_prefix_priority_to_bits
//
// Given an omr_priority_t, return the bits that represent it in the prefix flag word.
//
int omr_prefix_priority_to_bits(omr_prefix_priority_t priority);

// omr_prefix_priority_to_int
//
// Given an omr_priority_t, return the human-readable integer that represents it. Invalid priority is
// returned as -1 (low).
//
int omr_prefix_priority_to_int(omr_prefix_priority_t priority);

// omr_watcher_callback_add
//
// Adds a callback on the omr_watcher object.
//
// watcher: the omr_watcher_t to which to add the callback

#define omr_watcher_callback_add(omw, callback, context_release, context) \
	omr_watcher_callback_add_(omw, callback, context_release, context, __FILE__, __LINE__)
omr_watcher_callback_t *NULLABLE
omr_watcher_callback_add_(omr_watcher_t *NONNULL omw, omr_watcher_event_callback_t NONNULL callback,
                          omr_watcher_context_release_callback_t NULLABLE context_release,
                          void *NULLABLE context, const char *NONNULL file, int line);

// omr_watcher_callback_cancel
//
// Cancel a callback that was returned by omr_watcher_add_callback().
//
// watcher:  the watcher that returned the callback object.
// callback: the callback to free
//
// The object passed in callback should not be retained by the caller after calling omr_watcher_callback_cancel().

void omr_watcher_callback_cancel(omr_watcher_t *NONNULL omw, omr_watcher_callback_t *NONNULL callback);

// omr_watcher_create
//
// Creates and starts an omr_watcher_t object. The object starts an ongoing query with wpantund/threadradiod to watch the Thread:OnMeshPrefixes
// property. Changes are reported to callbacks, which can be registered with
//
// route_state: pointer to a route state object to reference in callbacks, must be non-NULL.
//
// returns value: NULL on failure, or a pointer to an omr_watcher_t object.

#define omr_watcher_create(route_state, disconnect_callback) \
    omr_watcher_create_(route_state, disconnect_callback, __FILE__, __LINE__)
omr_watcher_t *NULLABLE
omr_watcher_create_(route_state_t *NONNULL route_state, void (*NULLABLE disconnect_callback)(void *NONNULL),
                    const char *NONNULL file, int line);

// omr_watcher_start
//
// Starts the omr watcher object. Prior to calling omr_start, no events will be delivered; after calling omr_start, events may be delivered.
//
// watcher: pointer to an omr_watcher_t object to start.
bool omr_watcher_start(omr_watcher_t *NONNULL watcher);


// omr_watcher_cancel
//
// Cancels the omr watcher object. No callbacks can occur after omr_watcher_cancel has been called.
//
// watcher: pointer to an omr_watcher_t object to cancel.
void omr_watcher_cancel(omr_watcher_t *NONNULL watcher);

// omr_watcher_prefix_present
//
// Returns true if there is a prefix in the watcher's prefix list that has the specified priority.
//
// watcher: watcher we use for the search
// ignore_prefix: the prefix we are currently publishing, and hence should ignore
// ignore_prefix_len: length of the prefix we should ignore

bool omr_watcher_prefix_present(omr_watcher_t *NONNULL watcher, omr_prefix_priority_t priority,
                                struct in6_addr *NONNULL ignore_prefix, int ignore_prefix_len);

// omr_watcher_prefix_exists
//
// Returns true if the specified prefix is in the watcher's current prefix list.
//
// watcher: watcher we use for the search
// address: address on prefix to search for
// prefix_len: length of prefix (host bits in address are ignored)

bool omr_watcher_prefix_exists(omr_watcher_t *NONNULL watcher,
                               const struct in6_addr *NONNULL address, int prefix_len);

// omr_watcher_prefix_present
//
// Returns true if there is a prefix in the provided list that has the specified priority.
//
// prefixes: pointer to an omr_prefix_t object to check
// preference: low, medium or high

bool
omr_watcher_prefix_wins(omr_watcher_t *NONNULL watcher, omr_prefix_priority_t priority,
                        struct in6_addr *NONNULL ignore_prefix, int ignore_prefix_length);

// omr_watcher_prefixes_get
//
// Returns the list of prefixes the omr_watcher has most recently seen, or NULL if none.
//

omr_prefix_t *NULLABLE
omr_watcher_prefixes_get(omr_watcher_t *NONNULL watcher);

// omr_watcher_add_prefix
//
// Adds the specified prefix at the specified priority. Returns true if prefix was added, false otherwise.
//

bool
omr_watcher_prefix_add(omr_watcher_t *NONNULL watcher, const void *NONNULL data, int prefix_length, omr_prefix_priority_t priority);

// omr_watcher_prefixes
//
// Deletes the specified prefix. Returns true of it was deleted, false otherwise.
//

bool
omr_watcher_prefix_remove(omr_watcher_t *NONNULL watcher, const void *NONNULL data, int prefix_length);

// omw_watcher_non_ula_prefix_present
//
// Returns true if there is an on-mesh prefix in the thread network data that is not a ULA prefix (implicitly a global prefix)
//

bool omr_watcher_non_ula_prefix_present(omr_watcher_t *NONNULL watcher);


#define omr_watcher_prefix_is_non_ula_prefix(omr_prefix) ((((uint8_t *)&(omr_prefix)->prefix)[0] & 0xfc) != 0xfc)

#endif // _OMR_WATCHER_H__

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 120
// indent-tabs-mode: nil
// End:
