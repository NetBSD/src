/* dnssd-client.h
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

#ifndef __DNSSD_CLIENT_H__
typedef struct dnssd_client dnssd_client_t;
RELEASE_RETAIN_DECLS(dnssd_client);
#define dnssd_client_retain(watcher) dnssd_client_retain_(watcher, __FILE__, __LINE__)
#define dnssd_client_release(watcher) dnssd_client_release_(watcher, __FILE__, __LINE__)
void dnssd_client_cancel(dnssd_client_t *NONNULL client);
dnssd_client_t *NULLABLE dnssd_client_create(srp_server_t *NONNULL server_state);
void dnssd_client_start(dnssd_client_t *NONNULL client);
#endif // __DNSSD_CLIENT_H__

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 120
// indent-tabs-mode: nil
// End:
