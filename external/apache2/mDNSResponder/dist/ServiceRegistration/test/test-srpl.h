/* test-srpl.h
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
 * This file contains external definitions for test-srpl.c.
 */

srp_server_t *NULLABLE test_srpl_add_server(test_state_t *NONNULL state);
void test_srpl_start_replication(srp_server_t *NONNULL server, int16_t port);
srpl_connection_t *NULLABLE test_srpl_connection_create(test_state_t *NONNULL state, srp_server_t *NONNULL server, srp_server_t *NONNULL client);
void test_srpl_finished_evaluate(srpl_connection_t *NONNULL srpl_connection);
void test_srpl_set_finished_checkpoint(srpl_connection_t *NONNULL srpl_connection,
                                       srpl_state_t srpl_state,
                                       void (*NULLABLE test_finished_callback)(test_state_t *NONNULL state, srp_server_t *NONNULL server));
// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 108
// indent-tabs-mode: nil
// End:
