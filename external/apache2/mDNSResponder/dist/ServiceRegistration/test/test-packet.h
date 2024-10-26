/* test-packet.c
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
 * This file contains external definitions for test-packet.c.
 */

test_packet_state_t *NULLABLE
test_packet_state_create(test_state_t *NONNULL test_state,
                         void (*NONNULL advertise_finished_callback)(test_state_t *NONNULL test_state));
void test_packet_generate(test_state_t *NONNULL test_state, uint32_t lease_time, uint32_t key_lease_time,
                          bool removing, bool prepend);
void test_packet_reset_key(test_state_t *NONNULL test_state);
void test_packet_start(test_state_t *NONNULL state, bool expect_fail);
bool test_packet_srpl_intercept(srpl_connection_t *NONNULL srpl_connection, srpl_event_t *NULLABLE event);
void test_packet_message_delete(test_state_t *NONNULL state, int index);

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 108
// indent-tabs-mode: nil
// End:
