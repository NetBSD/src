/* adv-ctl-server.h
 *
 * Copyright (c) 2020 Apple Computer, Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * This code adds border router support to 3rd party HomeKit Routers as part of Apple’s commitment to the CHIP project.
 *
 * Concise Thread Interface for Thread Border router control.
 */

#ifndef __ADV_CTL_SERVER_H__
#define __ADV_CTL_SERVER_H__
typedef struct missing_service missing_service_t;

void adv_ctl_thread_shutdown_status_check(srp_server_t *NONNULL server_state);
bool adv_ctl_init(void *NULLABLE context);
bool adv_ctl_service_types_compare(const char *NONNULL service_type, const char *NONNULL registered_type);
#endif /* __ADV_CTL_SERVER_H__ */

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 120
// indent-tabs-mode: nil
// End:
