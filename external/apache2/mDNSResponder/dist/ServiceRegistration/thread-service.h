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

#ifndef __THREAD_SERVICE_H__
#define __THREAD_SERVICE_H__ 1

typedef struct thread_service thread_service_t;
typedef struct probe_state probe_state_t;

typedef enum {
    add_complete,
    delete_complete,
    add_failed,
    delete_failed,
    add_pending,
    delete_pending,
    want_add,
    want_delete,
} thread_service_publication_state_t;

struct thread_pref_id {
    uint8_t partition_id[4]; // Partition id on which this prefix is claimed
    uint8_t prefix[5];       // 40-bit ULA prefix identifier (no need to keep the whole prefix)
};

struct thread_unicast_service {
    struct in6_addr address;   // IPv6 address on which service is offered
    uint8_t port[2];           // Port (network byte order)
    bool anycast_also_present; // True if the RLOC16 advertising this service is also advertising anycast
};

struct thread_anycast_service {
    struct in6_addr address; // Anycast IPv6 address constructed from the service identifier and mesh-local prefix
    uint8_t sequence_number;
};

typedef enum { any_service, pref_id, unicast_service, anycast_service } thread_service_type_t;

struct thread_service {
    int ref_count;
    thread_service_t *NULLABLE next;
    uint16_t rloc16;
    uint8_t service_id;
    thread_service_type_t service_type;
    bool user, ncp, stable, ignore, checking;
    bool previous_user, previous_ncp, previous_stable;
	thread_service_publication_state_t publication_state;
    time_t last_probe_time;
    bool checked;    // true if we've checked that this service is responding
    bool responding; // true if we've checked, and it's responding; last_probe_time is when it responded
    probe_state_t *NULLABLE probe_state;
    union {
        struct thread_pref_id pref_id;
        struct thread_anycast_service anycast;
        struct thread_unicast_service unicast;
    } u;
};

RELEASE_RETAIN_DECLS(thread_service);
#define thread_service_retain(watcher) thread_service_retain_(watcher, __FILE__, __LINE__)
#define thread_service_release(watcher) thread_service_release_(watcher, __FILE__, __LINE__)
void thread_service_list_release(thread_service_t *NONNULL *NULLABLE list_pointer);
#define thread_service_unicast_create(rloc16, address, port, service_id) \
	thread_service_unicast_create_(rloc16, address, port, service_id, __FILE__, __LINE__)
thread_service_t *NULLABLE thread_service_unicast_create_(uint16_t rloc16, uint8_t *NONNULL address,
                                                          uint8_t *NONNULL port, uint8_t service_id,
                                                          const char *NONNULL file, int line);
#define thread_service_anycast_create(rloc16, sequence_number, service_id) \
	thread_service_anycast_create_(rloc16, sequence_number, service_id,  __FILE__, __LINE__)
thread_service_t *NULLABLE thread_service_anycast_create_(uint16_t rloc16, uint8_t sequence_number,
                                                          uint8_t service_id, const char *NONNULL file, int line);
#define thread_service_pref_id_create(rloc16, partition_id, prefix, service_id) \
	thread_service_pref_id_create_(rloc16, partition_id, prefix, service_id, __FILE__, __LINE__)
thread_service_t *NULLABLE thread_service_pref_id_create_(uint16_t rloc16, uint8_t *NONNULL partition_id,
                                                          uint8_t *NONNULL prefix, uint8_t service_id,
												 const char *NONNULL file, int line);
void thread_service_note(const char *NONNULL owner_id, thread_service_t *NONNULL service,
						 const char *NONNULL event_description);
const char *NONNULL thread_service_publication_state_name_get(thread_service_publication_state_t publication_state);
bool thread_service_equal(thread_service_t *NULLABLE a, thread_service_t *NULLABLE b);
#endif // __THREAD_SERVICE_H__

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 120
// indent-tabs-mode: nil
// End:
