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
 * Definitions for a permitted interface list object, which maintains a list of
 * interfaces on which we are permitted to provide some service.
 */

#ifndef __IFPERMIT_H__
#define __IFPERMIT_H__ 1

typedef struct ifpermit_list ifpermit_list_t;
typedef struct srp_server_state srp_server_t;
#define ifpermit_list_create() ifpermit_list_create_(__FILE__, __LINE__)
ifpermit_list_t *NULLABLE ifpermit_list_create_(const char *NONNULL file, int line);
void ifpermit_list_add(ifpermit_list_t *NONNULL list, const char *NONNULL name);
void ifpermit_list_remove(ifpermit_list_t *NONNULL list, const char *NONNULL name);
#define ifpermit_list_retain(list) ifpermit_list_retain_(list, __FILE__, __LINE__)
void ifpermit_list_retain_(ifpermit_list_t *NULLABLE list, const char *NONNULL file, int line);
#define ifpermit_list_release(list) ifpermit_list_release_(list, __FILE__, __LINE__)
void ifpermit_list_release_(ifpermit_list_t *NULLABLE list, const char *NONNULL file, int line);
bool ifpermit_interface_is_permitted(ifpermit_list_t *NULLABLE permits, uint32_t ifindex);
#define ifpermit_add_permitted_interface_to_server(server_state, name) \
    ifpermit_add_permitted_interface_to_server_(server_state, name, __FILE__, __LINE__)
void ifpermit_add_permitted_interface_to_server_(srp_server_t *NONNULL server_state, const char *NONNULL name,
                                                 const char *NONNULL file, int line);
#endif // __IFPERMIT_H__

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 120
// indent-tabs-mode: nil
// End:
