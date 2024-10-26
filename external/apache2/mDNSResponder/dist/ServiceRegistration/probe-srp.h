/* probe-srp.c
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
 * This file contains code to queue and send updates for Thread services.
 */

#ifndef __PROBE_SRP_H__
#define __PROBE_SRP_H__ 1
typedef struct probe_state probe_state_t;
void probe_srp_service(thread_service_t *NONNULL service, void *NULLABLE context,
                       void (*NONNULL callback)(thread_service_t *NONNULL service,
                                                void *NULLABLE context, bool succeeded),
                       void (*NULLABLE context_release_callback)(void *NONNULL context));
void probe_srp_service_probe_cancel(thread_service_t *NONNULL service);
#endif // __PROBE_SRP_H__

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 120
// indent-tabs-mode: nil
// End:
