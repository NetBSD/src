/* state-machine.h
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
 * This file contains general support definitions for state machines in the Thread Border Router
 * implementation.
 */

#ifndef __STATE_MACHINE_H__
#define __STATE_MACHINE_H__ 1

#define RELEASE_RETAIN_FUNCS(type)                                              \
void                                                                            \
type##_retain_(type##_t * omw, const char *file, int line)                      \
{                                                                               \
    RETAIN(omw, type);                                                          \
}                                                                               \
                                                                                \
void                                                                            \
type##_release_(type##_t *NONNULL omw, const char *file, int line)              \
{                                                                               \
    RELEASE(omw, type);                                                         \
}

#define RELEASE_RETAIN_DECLS(type)                                              \
void type##_retain_(type##_t *NONNULL omw, const char *NONNULL file, int line); \
void type##_release_(type##_t *NONNULL omw, const char *NONNULL file, int line);

// The assumptions below are that every object that holds a state that these macros can operate on has
// the following elements:
//
// name: (char *), NUL terminated, name of object instance
// state_name: (const char *), NUL terminated, name of the state the object is in
// state: the current state of the state machine, as an enum
//
// For macros that take events, the event is assumed to have the following elements:
//
// name: (char *), NUL terminated, name of event type (iow, not specific to an event instance)

// For states that never receive events.
#define BR_REQUIRE_STATE_OBJECT_EVENT_NULL(state_object, event)                                                        \
    do {                                                                                                               \
        if ((event) != NULL) {                                                                                         \
            ERROR(PUB_S_SRP "/" PRI_S_SRP ": received unexpected " PUB_S_SRP " event in state " PUB_S_SRP,             \
                state_object->state_header.state_machine_type_name,                                                    \
                state_object->state_header.name, event->name, state_object->state_header.state_name);                  \
            return state_machine_state_invalid;                                                       	               \
        }                                                                                    	                       \
    } while (false)

// Announce that we have entered a state that takes no events
#define BR_STATE_ANNOUNCE_NO_EVENTS(state_object)                                                                      \
    do {																							                   \
        INFO(PUB_S_SRP "/" PRI_S_SRP ": entering state " PUB_S_SRP, state_object->state_header.name,                   \
             state_object->state_header.state_machine_type_name,                                                       \
             state_object->state_header.state_name);                                                                   \
    } while (false)

// Announce that we have entered a state that takes no events, and include a domain name
#define BR_STATE_ANNOUNCE_NO_EVENTS_NAME(state_object, fqdn)                                                           \
    do {                                                                                                               \
        char hostname[kDNSServiceMaxDomainName];                                                                       \
        dns_name_print(fqdn, hostname, sizeof(hostname));                                                              \
        INFO(PUB_S_SRP "/" PRI_S_SRP ": entering state " PUB_S_SRP " with host " PRI_S_SRP,                            \
            state_object->state_header.state_machine_type_name,                                                        \
            state_object->state_header.name, state_object->state_header.state_name, hostname);                         \
    } while (false)

// Announce that we have entered a state that takes no events
#define BR_STATE_ANNOUNCE(state_object, event)                                                                         \
    do {							 															                       \
        if (event != NULL)  {                                                                                          \
            INFO(PUB_S_SRP "/" PRI_S_SRP ": event " PUB_S_SRP " received in state " PUB_S_SRP,                         \
                 state_object->state_header.state_machine_type_name,                                                   \
                 state_object->state_header.name, event->name, state_object->state_header.state_name);                 \
        } else {                                                                                                       \
            INFO(PUB_S_SRP "/" PRI_S_SRP ": entering state " PUB_S_SRP,                                                \
                 state_object->state_header.state_machine_type_name,                                                   \
                 state_object->state_header.name, state_object->state_header.state_name);                              \
        }                                                                                                              \
    } while (false)

#define BR_UNEXPECTED_EVENT_MAIN(state_object, event, bad, event_is_message)                                           \
    do {                                                                                                               \
        if (event_is_message(event)) {                                                                                 \
            INFO(PUB_S_SRP "/" PRI_S_SRP ": invalid event " PUB_S_SRP " in state " PUB_S_SRP,                          \
                 state_object->state_header.state_machine_type_name,                                                   \
                 (state_object)->state_header.name, (event)->name, state_object->state_header.state_name);             \
            return (int)bad;														                                   \
        }                                                                                                              \
        INFO(PUB_S_SRP "/" PRI_S_SRP ": unexpected event " PUB_S_SRP " in state " PUB_S_SRP,                           \
             state_object->state_header.state_machine_type_name,                                                       \
             (state_object)->state_header.name, (event)->name,                                                         \
             state_object->state_header.state_name);                                                                   \
        return (int)state_machine_state_invalid;                                                                       \
    } while (false)

// UNEXPECTED_EVENT flags the response as bad on a protocol level, triggering a retry delay
// UNEXPECTED_EVENT_NO_ERROR doesn't.
#define BR_UNEXPECTED_EVENT(state_object, event)                                                                       \
    BR_UNEXPECTED_EVENT_MAIN(state_object, event, state_machine_state_invalid,                                         \
                                                                          state_machine_event_is_message)
#define BR_UNEXPECTED_EVENT_NO_ERROR(state_object, event)                                                              \
    BR_UNEXPECTED_EVENT_MAIN(state_object, event, state_object_drop_state(state_object->instance, state_object),       \
                          state_machine_event_is_message)

// Generalized border router event object

#define state_machine_event_is_message(x) false

typedef enum {
    state_machine_event_type_invalid,
    state_machine_event_type_timeout,
    state_machine_event_type_prefix,
    state_machine_event_type_dhcp,
    state_machine_event_type_service_list_changed,
    state_machine_event_type_listener_ready,
    state_machine_event_type_listener_canceled,
    state_machine_event_type_ml_eid_changed,
    state_machine_event_type_rloc_changed,
    state_machine_event_type_thread_network_state_changed,
    state_machine_event_type_thread_node_type_changed,
    state_machine_event_type_probe_completed,
    state_machine_event_type_got_mesh_local_prefix,
    state_machine_event_type_daemon_disconnect,
    state_machine_event_type_stop,
    state_machine_event_type_dns_registration_invalidated,
    state_machine_event_type_thread_interface_changed,
    state_machine_event_type_wed_ml_eid_changed,
    state_machine_event_type_neighbor_ml_eid_changed,
    state_machine_event_type_srp_needed,
    state_machine_event_type_dns_registration_bad_service,
} state_machine_event_type_t;

typedef struct state_machine_event state_machine_event_t;
typedef struct state_machine_header state_machine_header_t;
typedef void (*state_machine_event_finalize_callback_t)(state_machine_event_t *NONNULL event);
typedef struct omr_prefix omr_prefix_t;
struct state_machine_event {
	int ref_count;
	const char *NULLABLE name;
    state_machine_event_type_t type;
    omr_prefix_t *NULLABLE thread_prefixes;
    state_machine_event_finalize_callback_t NULLABLE finalize;
};

#ifndef STATE_MACHINE_IMPLEMENTATION
typedef enum state_machine_state {
	state_machine_state_invalid = 0,
} state_machine_state_t;
#endif // STATE_MACHINE_IMPLEMENTATION

typedef state_machine_state_t (*state_machine_action_t)(state_machine_header_t *NONNULL state_header, state_machine_event_t *NULLABLE event);
typedef struct state_machine_state_decl {
    state_machine_state_t state;
    const char *NONNULL name;
    state_machine_action_t NONNULL action;
} state_machine_decl_t;

typedef enum {
    state_machine_type_invalid,
    state_machine_type_omr_publisher,
    state_machine_type_service_publisher,
    state_machine_type_dnssd_client,
} state_machine_type_t;

struct state_machine_header {
	char *NULLABLE name;
    void *NULLABLE state_object;
	const char *NULLABLE state_name;
    state_machine_decl_t *NULLABLE states;
    const char *NULLABLE state_machine_type_name;
    size_t num_states;
	state_machine_state_t state;
    state_machine_type_t state_machine_type;
    bool once;
};

void state_machine_next_state(state_machine_header_t *NONNULL state_header, state_machine_state_t state);
void state_machine_event_finalize(state_machine_event_t *NONNULL event);
RELEASE_RETAIN_DECLS(state_machine_event);
state_machine_event_t *NULLABLE
state_machine_event_create(state_machine_event_type_t type,
                           state_machine_event_finalize_callback_t NULLABLE finalize_callback);
void state_machine_event_deliver(state_machine_header_t *NONNULL state_header, state_machine_event_t *NONNULL event);
bool state_machine_header_setup(state_machine_header_t *NONNULL state_header, void *NONNULL state_object, const char *NULLABLE name,
                                state_machine_type_t type, state_machine_decl_t *NONNULL states, size_t num_states);
void state_machine_cancel(state_machine_header_t *NONNULL state_header);
#endif // __STATE_MACHINE_H__

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 120
// indent-tabs-mode: nil
// End:
