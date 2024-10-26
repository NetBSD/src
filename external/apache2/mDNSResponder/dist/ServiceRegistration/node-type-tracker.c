/* node-type-tracker.c
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
#include "node-type-tracker.h"

typedef struct node_type_tracker_callback node_type_tracker_callback_t;
struct node_type_tracker_callback {
	node_type_tracker_callback_t *next;
	void (*context_release)(void *NONNULL context);
	void (*callback)(void *context);
    void *context;
};

struct node_type_tracker {
	int ref_count;
    uint64_t id;
    void (*reconnect_callback)(route_state_t *route_state);
    route_state_t *route_state;
    srp_server_t *server_state;
    cti_connection_t NULLABLE thread_context;
	node_type_tracker_callback_t *callbacks;
	uint64_t last_thread_network_node_type_change;
	thread_node_type_t current_node_type, previous_node_type;
};

static uint64_t node_type_tracker_serial_number = 0;

static void
node_type_tracker_finalize(node_type_tracker_t *tracker)
{
    free(tracker);
}

RELEASE_RETAIN_FUNCS(node_type_tracker);

const char *
node_type_tracker_thread_node_type_to_string(thread_node_type_t node_type)
{
#define NODE_TYPE_TO_STRING(type) case node_type_##type: return # type
	switch(node_type) {
		NODE_TYPE_TO_STRING(unknown);
		NODE_TYPE_TO_STRING(router);
		NODE_TYPE_TO_STRING(end_device);
		NODE_TYPE_TO_STRING(sleepy_end_device);
		NODE_TYPE_TO_STRING(nest_lurker);
		NODE_TYPE_TO_STRING(commissioner);
		NODE_TYPE_TO_STRING(leader);
		NODE_TYPE_TO_STRING(sleepy_router);
	default:
		return "<invalid>";
	}
}

static void
node_type_tracker_callback(void *context, cti_network_node_type_t cti_node_type, cti_status_t status)
{
    node_type_tracker_t *tracker = context;
	thread_node_type_t node_type;

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
        RELEASE_HERE(tracker, node_type_tracker);
        return;
    }

	switch(cti_node_type) {
	case kCTI_NetworkNodeType_Unknown:
		node_type = node_type_unknown;
		break;
	case kCTI_NetworkNodeType_Router:
		node_type = node_type_router;
		break;
	case kCTI_NetworkNodeType_EndDevice:
		node_type = node_type_end_device;
		break;
	case kCTI_NetworkNodeType_SleepyEndDevice:
		node_type = node_type_sleepy_end_device;
		break;
    case kCTI_NetworkNodeType_SynchronizedSleepyEndDevice:
        node_type = node_type_synchronized_sleepy_end_device;
        break;
	case kCTI_NetworkNodeType_NestLurker:
		node_type = node_type_nest_lurker;
		break;
	case kCTI_NetworkNodeType_Commissioner:
		node_type = node_type_commissioner;
		break;
	case kCTI_NetworkNodeType_Leader:
		node_type = node_type_leader;
		break;
    case kCTI_NetworkNodeType_SleepyRouter:
        node_type = node_type_sleepy_router;
        break;
	}

    tracker->last_thread_network_node_type_change = ioloop_timenow();

	INFO("node type is: " PUB_S_SRP " (%d)\n ", node_type_tracker_thread_node_type_to_string(node_type), cti_node_type);

    if (tracker->current_node_type != node_type) {
		tracker->previous_node_type = tracker->current_node_type;
		tracker->current_node_type = node_type;

        // Call any callbacks to trigger updates based on new information.
        for (node_type_tracker_callback_t *callback = tracker->callbacks; callback != NULL; callback = callback->next) {
            callback->callback(callback->context);
        }
    }
}

node_type_tracker_t *
node_type_tracker_create(srp_server_t *server_state)
{
	node_type_tracker_t *ret = NULL;
	node_type_tracker_t *tracker = calloc(1, sizeof(*ret));
	if (tracker == NULL) {
		ERROR("[ST%lld] no memory", ++node_type_tracker_serial_number);
		goto exit;
	}
	RETAIN_HERE(tracker, node_type_tracker);
    tracker->id = ++node_type_tracker_serial_number;
    tracker->server_state = server_state;
    tracker->current_node_type = tracker->previous_node_type = node_type_unknown;

	ret = tracker;
	tracker = NULL;
exit:
	if (tracker != NULL) {
		RELEASE_HERE(tracker, node_type_tracker);
	}
	return ret;
}

void
node_type_tracker_start(node_type_tracker_t *tracker)
{
    int status = cti_get_network_node_type(tracker->server_state, &tracker->thread_context,
                                           tracker, node_type_tracker_callback, NULL);
    if (status != kCTIStatus_NoError) {
        INFO("[TT%lld] service list get failed: %d", tracker->id, status);
    }
    RETAIN_HERE(tracker, node_type_tracker); // for the callback
}

bool
node_type_tracker_callback_add(node_type_tracker_t *tracker,
							 void (*callback)(void *context), void (*context_release)(void *context), void *context)
{
	bool ret = false;
    node_type_tracker_callback_t **tpp;

	// It's an error for two callbacks to have the same context
	for (tpp = &tracker->callbacks; *tpp != NULL; tpp = &(*tpp)->next) {
		if ((*tpp)->context == context) {
			FAULT("[TT%lld] duplicate context %p", tracker->id, context);
			goto exit;
		}
	}

	node_type_tracker_callback_t *tracker_callback = calloc(1, sizeof(*tracker_callback));
	if (tracker_callback == NULL) {
		ERROR("[TT%lld] no memory", tracker->id);
		goto exit;
	}
	tracker_callback->callback = callback;
	tracker_callback->context_release = context_release;
	tracker_callback->context = context;

	// The callback list holds a reference to the tracker
	if (tracker->callbacks == NULL) {
		RETAIN_HERE(tracker, node_type_tracker);
	}

	// Keep the callback on the list.
	*tpp = tracker_callback;

	ret = true;
exit:
	return ret;

}

static void
node_type_tracker_callback_free(node_type_tracker_callback_t *callback)
{
    if (callback->context_release != NULL) {
        callback->context_release(callback->context);
    }
    free(callback);
}

void
node_type_tracker_cancel(node_type_tracker_t *tracker)
{
    if (tracker == NULL) {
        return;
    }
	if (tracker->thread_context != NULL) {
		cti_events_discontinue(tracker->thread_context);
		tracker->thread_context = NULL;
		RELEASE_HERE(tracker, node_type_tracker);
	}
	if (tracker->callbacks != NULL) {
        node_type_tracker_callback_t *next;
		for (node_type_tracker_callback_t *callback = tracker->callbacks; callback != NULL; callback = next) {
			next = callback->next;
            node_type_tracker_callback_free(callback);
        }
		tracker->callbacks = NULL;
		// Release the reference held by the callback list.
		RELEASE_HERE(tracker, node_type_tracker);
	}
}

void
node_type_tracker_callback_cancel(node_type_tracker_t *tracker, void *context)
{
    if (tracker == NULL) {
        return;
    }
	for (node_type_tracker_callback_t **tpp = &tracker->callbacks; *tpp != NULL; tpp = &((*tpp)->next)) {
		node_type_tracker_callback_t *callback = *tpp;
		if (callback->context == context) {
            *tpp = callback->next;
            node_type_tracker_callback_free(callback);
            return;
		}
	}
}

thread_node_type_t
node_type_tracker_thread_node_type_get(node_type_tracker_t *NULLABLE tracker, bool previous)
{
	if (tracker != NULL) {
		return previous ? tracker->previous_node_type : tracker->current_node_type;
	}
    return node_type_unknown;
}

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 120
// indent-tabs-mode: nil
// End:
