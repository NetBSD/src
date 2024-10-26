/* service-tracker.c
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
 * Track services on the Thread mesh.
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
#include "service-tracker.h"
#include "probe-srp.h"
#include "adv-ctl-server.h"

struct service_tracker_callback {
	service_tracker_callback_t *next;
	void (*context_release)(void *NONNULL context);
	void (*callback)(void *context);
    void *context;
};

struct service_tracker {
	int ref_count;
    uint64_t id;
    route_state_t *route_state;
    srp_server_t *server_state;
    cti_connection_t NULLABLE thread_service_context;
	service_tracker_callback_t *callbacks;
    thread_service_t *NULLABLE thread_services;
    uint16_t rloc16;
    bool user_service_seen;
};

static uint64_t service_tracker_serial_number = 0;

static void
service_tracker_finalize(service_tracker_t *tracker)
{
    thread_service_t *next;
    for (thread_service_t *service = tracker->thread_services; service != NULL; service = next) {
        next = service->next;
        thread_service_release(service);
    }
    free(tracker);
}

static void
service_tracker_context_release(void *context)
{
    service_tracker_t *tracker = context;
    if (tracker != NULL) {
        RELEASE_HERE(tracker, service_tracker);
    }
}

RELEASE_RETAIN_FUNCS(service_tracker);

void
service_tracker_thread_service_note(service_tracker_t *tracker, thread_service_t *tservice, const char *event_description)
{
    char owner_id[20];
    snprintf(owner_id, sizeof(owner_id), "[ST%lld]", tracker->id);
    thread_service_note(owner_id, tservice, event_description);
}

typedef struct state_debug_accumulator {
    char change[20]; // " +stable +user +ncp"
    char *p_change;
    size_t left;
    bool changed;
} accumulator_t;

static void
service_tracker_flags_accumulator_init(accumulator_t *accumulator)
{
    memset(accumulator, 0, sizeof(*accumulator));
    accumulator->p_change = accumulator->change;
    accumulator->left = sizeof(accumulator->change);
}

static void
service_tracker_flags_accumulate(accumulator_t *accumulator, bool previous, bool cur, const char *name)
{
    size_t len;
    if (previous != cur) {
        snprintf(accumulator->p_change, accumulator->left, "%s%s%s",
                 accumulator->p_change == accumulator->change ? "" : " ", cur ? "+" : "-", name);
        len = strlen(accumulator->p_change);
        accumulator->p_change += len;
        accumulator->left -= len;
        accumulator->changed = true;
    }
}

static void
service_tracker_callback(void *context, cti_service_vec_t *services, cti_status_t status)
{
    service_tracker_t *tracker = context;
    size_t i;
    thread_service_t **pservice = &tracker->thread_services, *service = NULL;
    tracker->user_service_seen = false;

    if (status == kCTIStatus_Disconnected || status == kCTIStatus_DaemonNotRunning) {
        INFO("[ST%lld] disconnected", tracker->id);
        cti_events_discontinue(tracker->thread_service_context);
        tracker->thread_service_context = NULL;
        RELEASE_HERE(tracker, service_tracker); // Not expecting any more callbacks.
        return;
    }

    if (status != kCTIStatus_NoError) {
        ERROR("[ST%lld] %d", tracker->id, status);
    } else {
        // Delete any SRP services that are not in the list provided by Thread.
        while (*pservice != NULL) {
            service = *pservice;
            if (service->service_type == unicast_service) {
                struct thread_unicast_service *uservice = &service->u.unicast;
                for (i = 0; i < services->num; i++) {
                    cti_service_t *cti_service = services->services[i];
                    // Is this a valid SRP service?
                    if (IS_SRP_SERVICE(cti_service)) {
                        // Is this service still present?
                        if (!memcmp(&uservice->address, cti_service->server, 16) &&
                            !memcmp(&uservice->port, &cti_service->server[16], 2)) {
                            break;
                        }
                    }
                }
            } else if (service->service_type == anycast_service) {
                struct thread_anycast_service *aservice = &service->u.anycast;
                for (i = 0; i < services->num; i++) {
                    cti_service_t *cti_service = services->services[i];
                    // Is this a valid SRP anycast service?
                    if (IS_SRP_ANYCAST_SERVICE(cti_service)) {
                        // Is this service still present?
                        if (service->rloc16 == cti_service->rloc16 &&
                            aservice->sequence_number == cti_service->service[1]) {
                            break;
                        }
                    }
                }
            } else if (service->service_type == pref_id) {
                struct thread_pref_id *pref_id = &service->u.pref_id;
                for (i = 0; i < services->num; i++) {
                    cti_service_t *cti_service = services->services[i];
                    // Is this an SRP service?
                    if (IS_PREF_ID_SERVICE(cti_service)) {
                        // Is this service still present?
                        if (!memcmp(&pref_id->partition_id, cti_service->server, 4) &&
                            !memcmp(pref_id->prefix, &cti_service->server[4], 5))
                        {
                            break;
                        }
                    }
                }
            } else {
                i = services->num;
            }

            if (i == services->num) {
                service_tracker_thread_service_note(tracker, service, "went away");
                *pservice = service->next;
                thread_service_release(service);
                service = NULL;
            } else {
                // We'll re-initialize these flags from the service list when we check for duplicates.
                service->previous_user = service->user;
                service->user = false;
                service->previous_stable = service->stable;
                service->stable = false;
                service->previous_ncp = service->ncp;
                service->ncp = false;
                pservice = &service->next;
                service->ignore = false;
            }
        }

        // Add any services that are not present.
        for (i = 0; i < services->num; i++) {
            cti_service_t *cti_service = services->services[i];
            for (service = tracker->thread_services; service != NULL; service = service->next) {
                if (IS_SRP_SERVICE(cti_service) && service->service_type == unicast_service) {
                    if (!memcmp(&service->u.unicast.address, cti_service->server, 16) &&
                        !memcmp(&service->u.unicast.port, &cti_service->server[16], 2)) {
                        break;
                    }
                } else if (IS_SRP_ANYCAST_SERVICE(cti_service) && service->service_type == anycast_service) {
                    uint8_t sequence_number = cti_service->service[1];
                    if (service->rloc16 == cti_service->rloc16 &&
                        service->u.anycast.sequence_number == sequence_number) {
                        break;
                    }
                } else if (IS_PREF_ID_SERVICE(cti_service) && service->service_type == pref_id) {

                    if (!memcmp(&service->u.pref_id.partition_id, cti_service->server, 4) &&
                        !memcmp(service->u.pref_id.prefix, &cti_service->server[4], 5))
                    {
                        break;
                    }
                }
            }
            if (service == NULL) {
                bool save = false;
                if (IS_SRP_SERVICE(cti_service)) {
                    service = thread_service_unicast_create(cti_service->rloc16, cti_service->server,
                                                            &cti_service->server[16], cti_service->service_id);
                    save = true;
                } else if (IS_SRP_ANYCAST_SERVICE(cti_service)) {
                    uint8_t sequence_number = cti_service->service[1];
                    service = thread_service_anycast_create(cti_service->rloc16, sequence_number,
                                                            cti_service->service_id);
                    save = true;
                } else if (IS_PREF_ID_SERVICE(cti_service)) {
                    save = true;
                    service = thread_service_pref_id_create(cti_service->rloc16, cti_service->server,
                                                            &cti_service->server[4], cti_service->service_id);
                }
                if (save) {
                    if (service == NULL) {
                        ERROR("[ST%lld] no memory for service.", tracker->id);
                    } else {
                        service_tracker_thread_service_note(tracker, service, "showed up");
                        *pservice = service;
                        pservice = &service->next;
                    }
                }
            }
            // Also, since we're combing the list, update ncp, user and stable flags.   Note that a service can
            // appear more than once in the thread service list.
            if (service != NULL) {
                if (cti_service->flags & kCTIFlag_NCP) {
                    service->ncp = true;
                } else {
                    service->user = true;
                    tracker->user_service_seen = true;
                }
                if (cti_service->flags & kCTIFlag_Stable) {
                    service->stable = true;
                }
            }
        }
        accumulator_t accumulator;
        for (service = tracker->thread_services; service != NULL; service = service->next) {
            // For unicast services, see if there's also an anycast service on the same RLOC16.
            if (service->service_type == unicast_service) {
                service->u.unicast.anycast_also_present = false;
                for (thread_service_t *aservice = tracker->thread_services; aservice != NULL; aservice = aservice->next)
                {
                    if (aservice->service_type == anycast_service && aservice->rloc16 == service->rloc16) {
                        service->u.unicast.anycast_also_present = true;
                    }
                }
            }
            service_tracker_flags_accumulator_init(&accumulator);
            service_tracker_flags_accumulate(&accumulator, service->previous_ncp, service->ncp, "ncp");
            service_tracker_flags_accumulate(&accumulator, service->previous_stable, service->ncp, "stable");
            service_tracker_flags_accumulate(&accumulator, service->previous_user, service->user, "user");
            if (accumulator.changed) {
                service_tracker_thread_service_note(tracker, service, accumulator.change);
            }
        }

        // At this point the thread prefix list contains the same information as what we just received.
        // Call any callbacks to trigger updates based on new information.
        for (service_tracker_callback_t *callback = tracker->callbacks; callback != NULL; callback = callback->next) {
            callback->callback(callback->context);
        }
        if (!tracker->user_service_seen && tracker->server_state != NULL &&
            tracker->server_state->awaiting_service_removal)
        {
            tracker->server_state->awaiting_service_removal = false;
            adv_ctl_thread_shutdown_status_check(tracker->server_state);
        }
    }
}

bool
service_tracker_local_service_seen(service_tracker_t *tracker)
{
    return tracker->user_service_seen;
}

service_tracker_t *
service_tracker_create(srp_server_t *server_state)
{
	service_tracker_t *ret = NULL;
	service_tracker_t *tracker = calloc(1, sizeof(*ret));
	if (tracker == NULL) {
		ERROR("[ST%lld] no memory", ++service_tracker_serial_number);
		goto exit;
	}
	RETAIN_HERE(tracker, service_tracker);
    tracker->id = ++service_tracker_serial_number;
    tracker->server_state = server_state;

	ret = tracker;
	tracker = NULL;
exit:
	if (tracker != NULL) {
		RELEASE_HERE(tracker, service_tracker);
	}
	return ret;
}

void
service_tracker_start(service_tracker_t *tracker)
{
    if (tracker->thread_service_context != NULL) {
        cti_events_discontinue(tracker->thread_service_context);
        tracker->thread_service_context = NULL;
        INFO("[ST%lld] restarting", tracker->id);
        if (tracker->ref_count != 1) {
            RELEASE_HERE(tracker, service_tracker); // Release the old retain for the callback.
        } else {
            FAULT("service tracker reference count should not be 1 here!");
        }
    }
    int status = cti_get_service_list(tracker->server_state, &tracker->thread_service_context,
                                      tracker, service_tracker_callback, NULL);
    if (status != kCTIStatus_NoError) {
        INFO("[ST%lld] service list get failed: %d", tracker->id, status);
        return;
    }
    INFO("[ST%lld] service list get started", tracker->id);
    RETAIN_HERE(tracker, service_tracker); // for the callback.
}

bool
service_tracker_callback_add(service_tracker_t *tracker,
							 void (*callback)(void *context), void (*context_release)(void *context), void *context)
{
	bool ret = false;
    service_tracker_callback_t **tpp;

	// It's an error for two callbacks to have the same context
	for (tpp = &tracker->callbacks; *tpp != NULL; tpp = &(*tpp)->next) {
		if ((*tpp)->context == context) {
			FAULT("[ST%lld] duplicate context %p", tracker->id, context);
			goto exit;
		}
	}

	service_tracker_callback_t *tracker_callback = calloc(1, sizeof(*tracker_callback));
	if (tracker_callback == NULL) {
		ERROR("[ST%lld] no memory", tracker->id);
		goto exit;
	}
	tracker_callback->callback = callback;
	tracker_callback->context_release = context_release;
	tracker_callback->context = context;

	// The callback list holds a reference to the tracker
	if (tracker->callbacks == NULL) {
		RETAIN_HERE(tracker, service_tracker);
	}

	// Keep the callback on the list.
	*tpp = tracker_callback;

	ret = true;
exit:
	return ret;

}

static void
service_tracker_callback_free(service_tracker_callback_t *callback)
{
    if (callback->context_release != NULL) {
        callback->context_release(callback->context);
    }
    free(callback);
}

void
service_tracker_stop(service_tracker_t *tracker)
{
    if (tracker == NULL) {
        return;
    }
	if (tracker->thread_service_context != NULL) {
		cti_events_discontinue(tracker->thread_service_context);
		tracker->thread_service_context = NULL;
		RELEASE_HERE(tracker, service_tracker);
	}
}

void
service_tracker_cancel(service_tracker_t *tracker)
{
    if (tracker == NULL) {
        return;
    }
    service_tracker_stop(tracker);

	if (tracker->callbacks != NULL) {
        service_tracker_callback_t *next;
		for (service_tracker_callback_t *callback = tracker->callbacks; callback != NULL; callback = next) {
			next = callback->next;
            service_tracker_callback_free(callback);
        }
		tracker->callbacks = NULL;
		// Release the reference held by the callback list.
		RELEASE_HERE(tracker, service_tracker);
	}
}

void
service_tracker_callback_cancel(service_tracker_t *tracker, void *context)
{
    if (tracker == NULL) {
        return;
    }
	for (service_tracker_callback_t **tpp = &tracker->callbacks; *tpp != NULL; tpp = &((*tpp)->next)) {
		service_tracker_callback_t *callback = *tpp;
		if (callback->context == context) {
            *tpp = callback->next;
            service_tracker_callback_free(callback);
            return;
		}
	}
}

static int
service_tracker_get_winning_anycast_sequence_number(service_tracker_t *NULLABLE tracker)
{
    if (tracker == NULL) {
        return -1;
    }
    int winning_sequence_number = -1;
    // Find the sequence number that would win.
    for (thread_service_t *service = tracker->thread_services; service != NULL; service = service->next)
    {
        if (service->ignore) {
            continue;
        }
        if ((int)service->u.anycast.sequence_number > winning_sequence_number) {
            winning_sequence_number = service->u.anycast.sequence_number;
        }
    }
    return winning_sequence_number;
}

thread_service_t *
service_tracker_services_get(service_tracker_t *NULLABLE tracker)
{
	if (tracker != NULL) {
		return tracker->thread_services;
	}
    return NULL;
}

// Check to see if a service exists that matches the service passed in as an argument, and if it is still validated.
// Service object might be a different object
bool
service_tracker_verified_service_still_exists(service_tracker_t *NULLABLE tracker, thread_service_t *old_service)
{
    if (tracker == NULL || old_service == NULL) {
        return false;
    }
    for (thread_service_t *service = tracker->thread_services; service != NULL; service = service->next) {
        if (service->ignore) {
            continue;
        }
        if (service->responding && old_service->service_type == service->service_type) {
            if (service->service_type == unicast_service) {
                if (service->u.unicast.port == old_service->u.unicast.port &&
                    !memcmp(&service->u.unicast.address, &old_service->u.unicast.address,
                            sizeof(service->u.unicast.address)))
                {
                    return true;
                }
            } else if (service->service_type == anycast_service) {
                if (service->u.anycast.sequence_number == old_service->u.anycast.sequence_number) {
                    return true;
                }
            }
            FAULT("old_service type is bogus: %d", old_service->service_type);
            return false;
        }
    }
    return false;
}

// If true, there is a service on the list that we've verified. Return it (caller must retain if saving pointer).
thread_service_t *
service_tracker_verified_service_get(service_tracker_t *NULLABLE tracker)
{
    if (tracker == NULL) {
        return false;
    }
    for (thread_service_t *service = tracker->thread_services; service != NULL; service = service->next) {
        if (service->ignore) {
            continue;
        }
        if (service->checked && service->responding) {
            return service;
        }
    }
    return false;
}

// Check for an unverified service on the list. If we are currently checking a service, return that service.
thread_service_t *
service_tracker_unverified_service_get(service_tracker_t *NULLABLE tracker, thread_service_type_t service_type)
{
    thread_service_t *ret = NULL;
    if (tracker != NULL) {
        for (thread_service_t *service = tracker->thread_services; service != NULL; service = service->next) {
            if (service_type != any_service && service_type != service->service_type) {
                continue;
            }
            if (service->ignore) {
                continue;
            }
            if (service->checking) {
                return service;
            }
            if (ret == NULL && !service->checked && !service->probe_state && !service->user) {
                ret = service;
            }
        }
    }
    if (tracker != NULL && ret != NULL) {
        char buf[128];
        snprintf(buf, sizeof(buf), "service_tracker_unverified_service_get returning %p", ret);
        service_tracker_thread_service_note(tracker, ret, buf);
    }
    return ret;
}

static void
service_tracker_probe_callback(thread_service_t *UNUSED service, void *context, bool UNUSED succeeded)
{
    service_tracker_t *tracker = context;
    // Notify consumers of service tracker callbacks that something has changed.
    for (service_tracker_callback_t *callback = tracker->callbacks; callback != NULL; callback = callback->next) {
        callback->callback(callback->context);
    }
}

// Find a service that is not currently being probed and has not been probed, and start probing it. If we are already probing a service,
// or if there are no services remaining to probe, do nothing.
void
service_tracker_verify_next_service(service_tracker_t *NULLABLE tracker)
{
    thread_service_t *service;
    if (tracker == NULL) {
        return;
    }
    int winning_sequence_number = service_tracker_get_winning_anycast_sequence_number(tracker);

    for (service = tracker->thread_services; service != NULL; service = service->next) {
        if (service->probe_state) {
            return;
        }
        // For anycast services, if not on the winning sequence number, don't check
        if (service->service_type == anycast_service && service->u.anycast.sequence_number != winning_sequence_number) {
            continue;
        }
        // If this is our service, we don't need to check it.
        if (service->user) {
            continue;
        }
        // If we've probed it recently, don't probe it again yet.
        if (srp_time() - service->last_probe_time < 300)
        {
            continue;
        }
        if (service->checked) {
            continue;
        }
        // If we didn't continue, yet, it's because we found a service we can probe, so probe it.
        RETAIN_HERE(tracker, service_tracker); // For the srp probe
        probe_srp_service(service, tracker, service_tracker_probe_callback, service_tracker_context_release);
        return;
    }
}

void
service_tracker_cancel_probes(service_tracker_t *NULLABLE tracker)
{
    if (tracker == NULL) {
        return;
    }
    for (thread_service_t *service = tracker->thread_services; service != NULL; service = service->next) {
        if (service->probe_state) {
            probe_srp_service_probe_cancel(service);
        }
    }
}

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 120
// indent-tabs-mode: nil
// End:
