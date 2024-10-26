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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <pwd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <time.h>
#include <dns_sd.h>
#include <net/if.h>
#include <inttypes.h>
#include <sys/resource.h>

#include "srp.h"
#include "dns-msg.h"
#include "srp-crypto.h"
#include "ioloop.h"
#include "srp-gw.h"
#include "srp-proxy.h"
#include "srp-mdns-proxy.h"
#include "dnssd-proxy.h"
#include "config-parse.h"
#include "cti-services.h"
#include "route.h"
#include "state-machine.h"

#ifdef DEBUG
#define STATE_DEBUGGING_ABORT() abort();
#else
#define STATE_DEBUGGING_ABORT()
#endif

static state_machine_decl_t *
state_machine_state_get(state_machine_header_t *header, state_machine_state_t state)
{
    if (!header->once) {
        for (size_t i = 0; i < header->num_states; i++) {
            if (header->states[i].state != (state_machine_state_t)i) {
                ERROR(PUB_S_SRP "/" PRI_S_SRP " state %zu doesn't match " PUB_S_SRP,
					  header->state_machine_type_name, header->name, i, header->states[i].name);
                STATE_DEBUGGING_ABORT();
                return NULL;
            }
        }
        header->once = true;
    }
    if ((size_t)state < 0 || (size_t)state >= header->num_states) {
        STATE_DEBUGGING_ABORT();
        return NULL;
    }
    return &header->states[state];
}

void
state_machine_next_state(state_machine_header_t *state_header, state_machine_state_t state)
{
    state_machine_state_t next_state = state;

    do {
        state_machine_decl_t *new_state = state_machine_state_get(state_header, next_state);

        if (new_state == NULL) {
			ERROR(PUB_S_SRP "/" PRI_S_SRP " next state is invalid: %d",
				  state_header->state_machine_type_name, state_header->name, next_state);
            STATE_DEBUGGING_ABORT();
            return;
        }
        state_header->state = next_state;
        state_header->state_name = new_state->name;
        state_machine_action_t action = new_state->action;
        if (action != NULL) {
            next_state = action(state_header, NULL);
        }
    } while (next_state != state_machine_state_invalid);
}

void
state_machine_event_finalize(state_machine_event_t *event)
{
	if (event->finalize != NULL) {
		event->finalize(event);
	}
	free(event);
}

RELEASE_RETAIN_FUNCS(state_machine_event);

typedef struct {
    state_machine_event_type_t event_type;
    char *name;
} state_machine_event_configuration_t;

#define EVENT_NAME_DECL(name) { state_machine_event_type_##name, #name }

state_machine_event_configuration_t state_machine_event_configurations[] = {
    EVENT_NAME_DECL(invalid),
    EVENT_NAME_DECL(timeout),
    EVENT_NAME_DECL(prefix),
    EVENT_NAME_DECL(dhcp),
    EVENT_NAME_DECL(service_list_changed),
    EVENT_NAME_DECL(listener_ready),
    EVENT_NAME_DECL(listener_canceled),
    EVENT_NAME_DECL(ml_eid_changed),
    EVENT_NAME_DECL(rloc_changed),
    EVENT_NAME_DECL(thread_network_state_changed),
    EVENT_NAME_DECL(thread_node_type_changed),
    EVENT_NAME_DECL(probe_completed),
    EVENT_NAME_DECL(got_mesh_local_prefix),
    EVENT_NAME_DECL(daemon_disconnect),
    EVENT_NAME_DECL(stop),
    EVENT_NAME_DECL(dns_registration_invalidated),
    EVENT_NAME_DECL(thread_interface_changed),
    EVENT_NAME_DECL(wed_ml_eid_changed),
    EVENT_NAME_DECL(neighbor_ml_eid_changed),
    EVENT_NAME_DECL(srp_needed),
    EVENT_NAME_DECL(dns_registration_bad_service),
};
#define STATE_MACHINE_NUM_EVENT_TYPES (sizeof(state_machine_event_configurations) / sizeof(state_machine_event_configuration_t))

static state_machine_event_configuration_t *
state_machine_event_configuration_get(state_machine_event_type_t event)
{
    static bool once = false;
    if (!once) {
        for (unsigned i = 0; i < STATE_MACHINE_NUM_EVENT_TYPES; i++) {
            if (state_machine_event_configurations[i].event_type != (state_machine_event_type_t)i) {
                ERROR("event %d doesn't match " PUB_S_SRP, i, state_machine_event_configurations[i].name);
                STATE_DEBUGGING_ABORT();
                return NULL;
            }
        }
        once = true;
    }
    if (event < 0 || event >= STATE_MACHINE_NUM_EVENT_TYPES) {
        STATE_DEBUGGING_ABORT();
        return NULL;
    }
    return &state_machine_event_configurations[event];
}

#if 0
static const char *
state_machine_state_name(state_machine_header_t *header, state_machine_state_t state)
{
    for (size_t i = 0; i < header->num_states; i++) {
        if (header->states[i].state == state) {
            return header->states[i].name;
        }
    }
    return "unknown state";
}
#endif

void
state_machine_event_deliver(state_machine_header_t *state_header, state_machine_event_t *event)
{
    state_machine_decl_t *state = state_machine_state_get(state_header, state_header->state);
    if (state == NULL) {
        ERROR(PUB_S_SRP "/" PRI_S_SRP ": event " PUB_S_SRP " received in invalid state %d",
              state_header->state_machine_type_name, state_header->name, event->name, state_header->state);
        STATE_DEBUGGING_ABORT();
        return;
    }
    if (state->action == NULL) {
        FAULT(PUB_S_SRP "/" PRI_S_SRP ": event " PUB_S_SRP " received in state " PUB_S_SRP " with NULL action",
              state_header->state_machine_type_name, state_header->name, event->name, state->name);
        return;
    }
    state_machine_state_t next_state = state->action(state_header, event);
    if (next_state != state_machine_state_invalid) {
        state_machine_next_state(state_header, next_state);
    }
}

state_machine_event_t *
state_machine_event_create(state_machine_event_type_t type,
						   state_machine_event_finalize_callback_t finalize_callback)
{
    state_machine_event_configuration_t *event_config = state_machine_event_configuration_get(type);
    if (event_config == NULL) {
        ERROR("invalid event type %d", type);
        STATE_DEBUGGING_ABORT();
        return NULL;
    }
	state_machine_event_t *event = calloc(1, sizeof (*event));
    event->type = type;
    event->name = event_config->name;
	RETAIN_HERE(event, state_machine_event);
	event->finalize = finalize_callback;
	return event;
}

typedef struct state_machine_type_decl {
	state_machine_type_t state_machine_type;
	const char *state_machine_type_name;
} state_machine_type_decl_t;

#define STATE_NAME_DECL(name) { state_machine_type_##name, #name }
state_machine_type_decl_t state_machine_types[] = {
    STATE_NAME_DECL(invalid),
	STATE_NAME_DECL(omr_publisher),
    STATE_NAME_DECL(service_publisher),
    STATE_NAME_DECL(dnssd_client),
};
#define STATE_MACHINE_NUM_TYPES (sizeof(state_machine_types) / sizeof(state_machine_type_decl_t))

bool
state_machine_header_setup(state_machine_header_t *state_header, void *state_object, const char *name,
						   state_machine_type_t type, state_machine_decl_t *states, size_t num_states)
{
	memset(state_header, 0, sizeof(*state_header));
	for (unsigned i = 0; i < STATE_MACHINE_NUM_TYPES; i++) {
		if (state_machine_types[i].state_machine_type == type) {
			state_header->state_machine_type_name = state_machine_types[i].state_machine_type_name;
			break;
		}
	}
	if (state_header->state_machine_type_name == NULL) {
		return false;
	}
	state_header->state_object = state_object;
	state_header->name = strdup(name);
	if (state_header->name == NULL) {
		return false;
	}
	state_header->state_machine_type = type;
	state_header->states = states;
	state_header->num_states = num_states;
	return true;
}

void state_machine_cancel(state_machine_header_t *NONNULL state_header)
{
    INFO("canceling " PUB_S_SRP, state_header->name);
    state_header->state = state_machine_state_invalid;
}

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 120
// indent-tabs-mode: nil
// End:
