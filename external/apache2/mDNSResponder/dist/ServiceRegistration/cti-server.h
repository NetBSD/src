/* cti-server.h
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
 * External function signatures for cti-server
 */

#ifndef __CTI_SERVER_H__
#define __CTI_SERVER_H__

#include <stdbool.h>

#define CTI_EVENT_SERVICE      (1 << 1)
#define CTI_EVENT_PREFIX       (1 << 2)
#define CTI_EVENT_ROLE         (1 << 3)
#define CTI_EVENT_PARTITION_ID (1 << 4)
#define CTI_EVENT_STATE        (1 << 5)
#define CTI_EVENT_XPANID       (1 << 6)

#ifndef NO_IOLOOP
#define NO_IOLOOP 1
#endif

#ifndef NOT_HAVE_SA_LEN
#define NOT_HAVE_SA_LEN 1
#endif

#include "cti-common.h"

int cti_init(void);
void cti_fd_init(int *NONNULL p_nfds, fd_set *NONNULL r) GCCATTR((nonnull (1,2)));
void cti_fd_process(fd_set *NONNULL r) GCCATTR((nonnull (1)));

typedef int (*send_event_t)(cti_connection_t NONNULL connection, int event) GCCATTR((nonnull (1)));
void cti_notify_event(unsigned int evt, send_event_t NONNULL evt_handler) GCCATTR((nonnull (2)));

int ctiAddService(uint32_t enterprise_number, const uint8_t *NONNULL service_data,
				  size_t service_data_length, const uint8_t *NONNULL server_data, size_t server_data_length)
    GCCATTR((nonnull (2, 4)));
int ctiRemoveService(uint32_t enterprise_number,
					 const uint8_t *service_data, size_t service_data_length) GCCATTR((nonnull (2)));
int ctiRetrieveServiceList(cti_connection_t NONNULL connection, int event) GCCATTR((nonnull (1)));
int ctiAddMeshPrefix(struct in6_addr *NONNULL prefix,
					 size_t prefix_length, bool on_mesh, bool preferred, bool slaac, bool stable) GCCATTR((nonnull (1)));
int ctiRemoveMeshPrefix(struct in6_addr *NONNULL prefix, size_t prefix_length) GCCATTR((nonnull (1)));
int ctiRetrievePrefixList(cti_connection_t NONNULL connection, int event) GCCATTR((nonnull (1)));
int ctiRetrievePartitionId(cti_connection_t NONNULL connection, int event) GCCATTR((nonnull (1)));
int ctiRetrieveXPANID(cti_connection_t NONNULL connection, int event) GCCATTR((nonnull (1)));
int ctiRetrieveTunnel(cti_connection_t NONNULL connection) GCCATTR((nonnull (1)));
int ctiRetrieveNodeType(cti_connection_t NONNULL connection, int event) GCCATTR((nonnull (1)));

#endif // __CTI_SERVER_H__

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 108
// indent-tabs-mode: nil
// End:
