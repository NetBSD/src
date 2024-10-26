/* dso-utils.c
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
 */

#define DNSMessageHeader dns_wire_t

void dso_simple_response(comm_t *NONNULL comm, message_t *NULLABLE message, const dns_wire_t *NONNULL wire, int rcode);
void dso_retry_delay_response(comm_t *NONNULL comm, message_t *NONNULL message, const dns_wire_t *NONNULL wire,
                              int rcode, uint32_t milliseconds);

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 108
// indent-tabs-mode: nil
// End:
