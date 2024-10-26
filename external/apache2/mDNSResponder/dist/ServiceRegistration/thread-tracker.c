/* thread-tracker.c
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
 * Track the state of the thread mesh (connected/disconnected, basically)
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
#include <netinet/icmp6.h>
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
#include "thread-device.h"
#include "state-machine.h"
#include "thread-service.h"
#include "thread-tracker.h"

typedef struct thread_tracker_callback thread_tracker_callback_t;
struct thread_tracker_callback {
	thread_tracker_callback_t *next;
	void (*context_release)(void *NONNULL context);
	void (*callback)(void *context);
    void *context;
};

struct thread_tracker {
	int ref_count;
    uint64_t id;
    void (*reconnect_callback)(route_state_t *route_state);
    route_state_t *route_state;
    srp_server_t *server_state;
    cti_connection_t NULLABLE thread_context;
	thread_tracker_callback_t *callbacks;
	uint64_t last_thread_network_state_change;
	thread_network_state_t current_state, previous_state;
	bool associated, previous_associated;
};

static uint64_t thread_tracker_serial_number = 0;

static void
thread_tracker_finalize(thread_tracker_t *tracker)
{
    free(tracker);
}

RELEASE_RETAIN_FUNCS(thread_tracker);

const char *
thread_tracker_network_state_to_string(thread_network_state_t state)
{
#define NETWORK_STATE_TO_STRING(type) case thread_network_state_##type: return # type
	switch(state) {
        NETWORK_STATE_TO_STRING(uninitialized);
        NETWORK_STATE_TO_STRING(fault);
        NETWORK_STATE_TO_STRING(upgrading);
        NETWORK_STATE_TO_STRING(deep_sleep);
        NETWORK_STATE_TO_STRING(offline);
        NETWORK_STATE_TO_STRING(commissioned);
        NETWORK_STATE_TO_STRING(associating);
        NETWORK_STATE_TO_STRING(credentials_needed);
        NETWORK_STATE_TO_STRING(associated);
        NETWORK_STATE_TO_STRING(isolated);
        NETWORK_STATE_TO_STRING(asleep);
        NETWORK_STATE_TO_STRING(waking);
        NETWORK_STATE_TO_STRING(unknown);
	default:
		return "<invalid>";
	}
}

static void
thread_tracker_callback(void *context, cti_network_state_t cti_state, cti_status_t status)
{
    thread_tracker_t *tracker = context;
    bool associated = false;
    thread_network_state_t state;

	if (status != kCTIStatus_NoError) {
		if (status == kCTIStatus_Disconnected || status == kCTIStatus_DaemonNotRunning) {
			INFO("disconnected");
			if (tracker->route_state != NULL && tracker->reconnect_callback != NULL) {
				tracker->reconnect_callback(tracker->route_state);
			}
		} else {
			INFO("unexpected error %d", status);
		}
        cti_events_discontinue(tracker->thread_context);
        tracker->thread_context = NULL;
        RELEASE_HERE(tracker, thread_tracker);
        return;
    }

    tracker->last_thread_network_state_change = ioloop_timenow();

    switch(cti_state) {
    case kCTI_NCPState_Uninitialized:
        state = thread_network_state_uninitialized;
        break;
    case kCTI_NCPState_Fault:
        state = thread_network_state_fault;
        break;
    case kCTI_NCPState_Upgrading:
        state = thread_network_state_upgrading;
        break;
    case kCTI_NCPState_DeepSleep:
        state = thread_network_state_deep_sleep;
        break;
    case kCTI_NCPState_Offline:
        state = thread_network_state_offline;
        break;
    case kCTI_NCPState_Commissioned:
        state = thread_network_state_commissioned;
        break;
    case kCTI_NCPState_Associating:
        state = thread_network_state_associating;
        break;
    case kCTI_NCPState_CredentialsNeeded:
        state = thread_network_state_credentials_needed;
        break;
    case kCTI_NCPState_Associated:
        state = thread_network_state_associated;
        break;
    case kCTI_NCPState_Isolated:
        state = thread_network_state_isolated;
        break;
    case kCTI_NCPState_NetWake_Asleep:
        state = thread_network_state_asleep;
        break;
    case kCTI_NCPState_NetWake_Waking:
        state = thread_network_state_waking;
        break;
    case kCTI_NCPState_Unknown:
        state = thread_network_state_unknown;
        break;
    }

	if ((cti_state == kCTI_NCPState_Associated)     || (cti_state == kCTI_NCPState_Isolated) ||
		(cti_state == kCTI_NCPState_NetWake_Asleep) || (cti_state == kCTI_NCPState_NetWake_Waking))
	{
		associated = true;
	}

	INFO("state is: " PUB_S_SRP " (%d)\n ", thread_tracker_network_state_to_string(state), cti_state);

    if (tracker->current_state != state) {
		tracker->previous_state = tracker->current_state;
		tracker->previous_associated = tracker->associated;
		tracker->current_state = state;
		tracker->associated = associated;

        // Call any callbacks to trigger updates based on new information.
        for (thread_tracker_callback_t *callback = tracker->callbacks; callback != NULL; callback = callback->next) {
            callback->callback(callback->context);
        }
    }
}

thread_tracker_t *
thread_tracker_create(srp_server_t *server_state)
{
	thread_tracker_t *ret = NULL;
	thread_tracker_t *tracker = calloc(1, sizeof(*ret));
	if (tracker == NULL) {
		ERROR("[ST%lld] no memory", ++thread_tracker_serial_number);
		goto exit;
	}
	RETAIN_HERE(tracker, thread_tracker);
    tracker->id = ++thread_tracker_serial_number;
    tracker->server_state = server_state;
	tracker->associated = tracker->previous_associated = false;
	tracker->current_state = tracker->previous_state = thread_network_state_uninitialized;

	ret = tracker;
	tracker = NULL;
exit:
	if (tracker != NULL) {
		RELEASE_HERE(tracker, thread_tracker);
	}
	return ret;
}

void
thread_tracker_start(thread_tracker_t *tracker)
{
    int status = cti_get_state(tracker->server_state, &tracker->thread_context, tracker, thread_tracker_callback, NULL);
    if (status != kCTIStatus_NoError) {
        INFO("[TT%lld] service list get failed: %d", tracker->id, status);
    }
    RETAIN_HERE(tracker, thread_tracker); // for the callback
}

bool
thread_tracker_callback_add(thread_tracker_t *tracker,
							 void (*callback)(void *context), void (*context_release)(void *context), void *context)
{
	bool ret = false;
    thread_tracker_callback_t **tpp;

	// It's an error for two callbacks to have the same context
	for (tpp = &tracker->callbacks; *tpp != NULL; tpp = &(*tpp)->next) {
		if ((*tpp)->context == context) {
			FAULT("[TT%lld] duplicate context %p", tracker->id, context);
			goto exit;
		}
	}

	thread_tracker_callback_t *tracker_callback = calloc(1, sizeof(*tracker_callback));
	if (tracker_callback == NULL) {
		ERROR("[TT%lld] no memory", tracker->id);
		goto exit;
	}
	tracker_callback->callback = callback;
	tracker_callback->context_release = context_release;
	tracker_callback->context = context;

	// The callback list holds a reference to the tracker
	if (tracker->callbacks == NULL) {
		RETAIN_HERE(tracker, thread_tracker);
	}

	// Keep the callback on the list.
	*tpp = tracker_callback;

	ret = true;
exit:
	return ret;

}

static void
thread_tracker_callback_free(thread_tracker_callback_t *callback)
{
    if (callback->context_release != NULL) {
        callback->context_release(callback->context);
    }
    free(callback);
}

void
thread_tracker_cancel(thread_tracker_t *tracker)
{
    if (tracker == NULL) {
        return;
    }
	if (tracker->thread_context != NULL) {
		cti_events_discontinue(tracker->thread_context);
		tracker->thread_context = NULL;
		RELEASE_HERE(tracker, thread_tracker);
	}
	if (tracker->callbacks != NULL) {
        thread_tracker_callback_t *next;
		for (thread_tracker_callback_t *callback = tracker->callbacks; callback != NULL; callback = next) {
			next = callback->next;
            thread_tracker_callback_free(callback);
        }
		tracker->callbacks = NULL;
		// Release the reference held by the callback list.
		RELEASE_HERE(tracker, thread_tracker);
	}
}

void
thread_tracker_callback_cancel(thread_tracker_t *tracker, void *context)
{
    if (tracker == NULL) {
        return;
    }
	for (thread_tracker_callback_t **tpp = &tracker->callbacks; *tpp != NULL; tpp = &((*tpp)->next)) {
		thread_tracker_callback_t *callback = *tpp;
		if (callback->context == context) {
            *tpp = callback->next;
            thread_tracker_callback_free(callback);
            return;
		}
	}
}

thread_network_state_t
thread_tracker_state_get(thread_tracker_t *NULLABLE tracker, bool previous)
{
	if (tracker != NULL) {
		return previous ? tracker->previous_state : tracker->current_state;
	}
	return thread_network_state_uninitialized;
}

bool
thread_tracker_associated_get(thread_tracker_t *NULLABLE tracker, bool previous)
{
	if (tracker != NULL) {
		return previous ? tracker->previous_associated : tracker->associated;
	}
	return false;
}

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 120
// indent-tabs-mode: nil
// End:
