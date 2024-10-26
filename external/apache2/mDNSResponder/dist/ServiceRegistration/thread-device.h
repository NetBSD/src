/* thread-device.h
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
 * Definitions for thread functionality that's present on border routers and non-border router Thread devices.
 */

#ifndef __THREAD_DEVICE_H__
#define __THREAD_DEVICE_H__ 1

void thread_device_stop(srp_server_t *NONNULL server_state);
void thread_device_startup(srp_server_t *NONNULL server_state);
void thread_device_shutdown(srp_server_t *NONNULL server_state);

#endif // __THREAD_DEVICE_H__

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 120
// indent-tabs-mode: nil
// End:
