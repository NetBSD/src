/* srp-gw.c
 *
 * Copyright (c) 2019-2022 Apple Computer, Inc. All rights reserved.
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
 * Structure definitions for the Service Registration Protocol gateway.
 */

#ifndef SRP_GW_H
#define SRP_GW_H
typedef struct subnet subnet_t;
struct subnet {
    subnet_t *NULLABLE next;
    uint8_t preflen;
    uint8_t family;
    char bytes[8];
};

typedef struct udp_validator udp_validator_t;
struct udp_validator {
    udp_validator_t *NULLABLE next;
    char *NONNULL ifname;
    int ifindex;
    subnet_t *NONNULL subnets;
};

typedef struct delete delete_t;
struct delete {
    delete_t *NULLABLE next;
    dns_name_t *NONNULL name;
    dns_name_t *NONNULL zone;
    bool consumed;
};

typedef struct host_addr host_addr_t;
struct host_addr {
    host_addr_t *NULLABLE next;
    dns_rr_t rr;
};
typedef struct dns_host_description dns_host_description_t;
struct dns_host_description {
    dns_name_t *NONNULL name;
    host_addr_t *NULLABLE addrs;
    dns_rr_t *NULLABLE key;
    delete_t *NULLABLE delete;
    int num_instances;
};

typedef struct service service_t;
struct service {
    service_t *NULLABLE next;
    service_t *NULLABLE base_type; // A pointer back in the list, or to self
    dns_rr_t *NONNULL rr; // The service name is rr->name.
    dns_name_t *NONNULL zone;
};

typedef struct service_instance service_instance_t;
struct service_instance {
    service_instance_t *NULLABLE next;
    dns_host_description_t *NULLABLE host;
    dns_name_t *NONNULL name;
    delete_t *NULLABLE delete;
    service_t *NONNULL service;
    int num_instances;
    dns_rr_t *NULLABLE srv, *NULLABLE txt;
    bool skip_update;
};

// The update_t structure is used to maintain the ongoing state of a particular DNS Update.

typedef enum update_state update_state_t;
enum update_state {
    connect_to_server,              // Establish a connection with the auth server.
    create_nonexistent,             // Update service instance assuming it's not already there (case 1).
    refresh_existing,               // Update service instance assuming it's already there and the same (case 2).
    create_nonexistent_instance,    // Update service instance assuming it's not there
    refresh_existing_instance,      // Update host assuming it's there and the same
    create_nonexistent_host,        // Update a host that's not present (and also the services)
    refresh_existing_host,          // Update a host that's present (and also the services)
    delete_failed_instance,         // The update failed, so delete service instances that were successfully added.
};

typedef enum update_event update_event_t;
enum update_event {
    update_event_connected,
    update_event_disconnected,
    update_event_response_received
};

typedef struct update update_t;
struct update {
    comm_t *NONNULL server;                       // Connection to authoritative server
    comm_t *NONNULL client;                       // Connection to SRP client (which might just be a UDP socket).
    update_state_t state;
    dns_host_description_t *NONNULL host;
    service_instance_t *NONNULL instances;
    service_instance_t *NULLABLE instance;        // If we are updating instances one at a time.
    service_instance_t *NULLABLE added_instances; // Instances we have successfully added.
    service_t *NONNULL services;
    dns_name_t *NULLABLE zone_name;               // If NULL, we are processing an update for services.arpa.
    message_t *NONNULL message;
    dns_message_t *NONNULL parsed_message;
    dns_wire_t *NONNULL update;                   // The current update...
    size_t update_length;
    size_t update_max;
    uint8_t fail_rcode;                           // rcode to return after deleting added service instances.
};
#endif  //SRP_GW_H
// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 108
// indent-tabs-mode: nil
// End:
