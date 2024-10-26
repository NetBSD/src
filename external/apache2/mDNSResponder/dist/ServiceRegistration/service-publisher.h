/* service-publisher.h
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
 * This file contains definitions for the service publisher, which can be used to safely publish and track services
 * on the Thread network.
 */

#ifndef __SERVICE_PUBLISHER_H__
#define __SERVICE_PUBLISHER_H__ 1

typedef struct service_publisher service_publisher_t;

RELEASE_RETAIN_DECLS(service_publisher);
#define service_publisher_retain(watcher) service_publisher_retain_(watcher, __FILE__, __LINE__)
#define service_publisher_release(watcher) service_publisher_release_(watcher, __FILE__, __LINE__)

bool service_publisher_is_address_mesh_local(service_publisher_t *NONNULL publisher, addr_t *NONNULL address);
bool service_publisher_could_publish(service_publisher_t *NULLABLE publisher);
void service_publisher_cancel(service_publisher_t *NONNULL publisher);
service_publisher_t *NULLABLE service_publisher_create(srp_server_t *NONNULL server_state);
void service_publisher_start(service_publisher_t *NONNULL publisher);
void service_publisher_stop_publishing(service_publisher_t *NONNULL publisher);
bool service_publisher_get_ml_eid(service_publisher_t *NULLABLE publisher, struct in6_addr *NONNULL ml_eid);

void service_publisher_unadvertise_all(service_publisher_t *NONNULL publisher);
void service_publisher_re_advertise_matching(service_publisher_t *NONNULL publisher);
void service_publisher_wanted_service_added(service_publisher_t *NONNULL publisher);
#endif // _SERVICE_PUBLISHER_H__

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 120
// indent-tabs-mode: nil
// End:
