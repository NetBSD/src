/* srp-proxy.h
 *
 * Copyright (c) 2018-2021 Apple Inc. All rights reserved.
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
 * Service Registration Protocol common definitions.
 */

#ifndef __SRP_PROXY_H
#define __SRP_PROXY_H

typedef struct srp_proxy_listener_state srp_proxy_listener_state_t;
typedef struct srp_server_state srp_server_t;
typedef struct srpl_connection srpl_connection_t;
typedef struct client_update client_update_t;

void srp_proxy_listener_cancel(srp_proxy_listener_state_t *NONNULL listener_state);
comm_t *NULLABLE srp_proxy_listen(uint16_t *NULLABLE avoid_ports, int num_avoid_ports, const char *NULLABLE interface_name,
                                  ready_callback_t NULLABLE ready, cancel_callback_t NULLABLE cancel,
                                  addr_t *NULLABLE address, finalize_callback_t NULLABLE context_callback, void *NONNULL context);
void srp_proxy_init(const char *NONNULL update_zone);
client_update_t *NULLABLE srp_evaluate(const char *NULLABLE remote_name,
                                       dns_message_t *NONNULL *NULLABLE in_parsed_message,
                                       message_t *NONNULL raw_message, int index);
bool srp_update_start(client_update_t *NONNULL client_update);
#define srp_parse_client_updates_free(messages) srp_parse_client_updates_free_(messages, __FILE__, __LINE__);
void srp_parse_client_updates_free_(client_update_t *NULLABLE messages, const char *NONNULL file, int line);

// Provided
void dns_input(comm_t *NONNULL comm, message_t *NONNULL message, void *NULLABLE context);
void srp_mdns_flush(srp_server_t *NONNULL server_state);
bool srp_dns_evaluate(comm_t *NONNULL connection, srp_server_t *NULLABLE server_state,
                      message_t *NONNULL message, dns_message_t *NONNULL *NULLABLE p_parsed_message);
bool srp_parse_host_messages_evaluate(srp_server_t *NONNULL server_state, srpl_connection_t *NONNULL srpl_connection,
                                      message_t *NONNULL *NONNULL messages, int num_messages);
#endif // __SRP_PROXY_H

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 108
// indent-tabs-mode: nil
// End:
