/*
 * Copyright (c) 2021 Apple Inc. All rights reserved.
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
 */

#include "mdns_addr_tailq.h"

#ifdef __APPLE__
#include <AssertMacros.h>
#endif // __APPLE__

#include "mdns_strict.h"

//======================================================================================================================
// MARK: - Functions

mdns_addr_tailq_t *
mdns_addr_tailq_create(void)
{
	mdns_addr_tailq_t * const me = mdns_calloc(1, sizeof(*me));
	require(me != NULL, exit);

	STAILQ_INIT(me);
exit:
	return me;
}
//======================================================================================================================

void
mdns_addr_tailq_dispose(mdns_addr_tailq_t * NONNULL me)
{
	mdns_addr_with_port_t * n1 = STAILQ_FIRST(me);
	mdns_addr_with_port_t * n2;
	while (n1 != NULL) {
		n2 = STAILQ_NEXT(n1, __entries);
		mdns_free(n1);
		n1 = n2;
	}
	mdns_addr_tailq_t * me_temp = me;
	mdns_free(me_temp);
}

//======================================================================================================================

bool
mdns_addr_tailq_empty(const mdns_addr_tailq_t * const NONNULL me)
{
	return STAILQ_EMPTY(me);
}

//======================================================================================================================

const mdns_addr_with_port_t * NULLABLE
mdns_addr_tailq_add_front(mdns_addr_tailq_t * const NONNULL me, const mDNSAddr * const NONNULL address,
						  const mDNSIPPort port)
{
	mdns_addr_with_port_t * const addr_with_port = mdns_calloc(1, sizeof(*addr_with_port));
	require(addr_with_port != NULL, exit);

	addr_with_port->address = *address;
	addr_with_port->port = port;

	STAILQ_INSERT_HEAD(me, addr_with_port, __entries);

exit:
	return addr_with_port;
}

//======================================================================================================================

const mdns_addr_with_port_t * NULLABLE
mdns_addr_tailq_add_back(mdns_addr_tailq_t * const NONNULL me, const mDNSAddr * const NONNULL address,
						 const mDNSIPPort port)
{
	mdns_addr_with_port_t * const addr_with_port = mdns_calloc(1, sizeof(*addr_with_port));
	require(addr_with_port != NULL, exit);

	addr_with_port->address = *address;
	addr_with_port->port = port;

	STAILQ_INSERT_TAIL(me, addr_with_port, __entries);

exit:
	return addr_with_port;
}

//======================================================================================================================

const mdns_addr_with_port_t * NULLABLE
mdns_addr_tailq_get_front(const mdns_addr_tailq_t * const NONNULL me)
{
	return STAILQ_FIRST(me);
}

//======================================================================================================================

void
mdns_addr_tailq_remove_front(mdns_addr_tailq_t * const NONNULL me)
{
	mdns_addr_with_port_t * n = STAILQ_FIRST(me);
	STAILQ_REMOVE_HEAD(me, __entries);
	mdns_free(n);
}

//======================================================================================================================

bool
mdns_addr_tailq_remove(mdns_addr_tailq_t * const NONNULL me, const mDNSAddr * const NONNULL address,
					   const mDNSIPPort port)
{
	bool found;
	require_action(!mdns_addr_tailq_empty(me), exit, found = false);

	mdns_addr_with_port_t * addr_with_port;
	mdns_addr_with_port_t * addr_with_port_temp;
	found = false;
	STAILQ_FOREACH_SAFE(addr_with_port, me, __entries, addr_with_port_temp) {
		if (mDNSVal16(port) != mDNSVal16(addr_with_port->port)) {
			continue;
		}
		if (!mDNSSameAddress(address, &addr_with_port->address)) {
			continue;
		}
		found = true;

		STAILQ_REMOVE(me, addr_with_port, mdns_addr_with_port, __entries);
		mdns_free(addr_with_port);
		break;
	}

exit:
	return found;
}

//======================================================================================================================

bool
mdns_addr_with_port_equal(const mdns_addr_with_port_t * const NONNULL addr_1,
						  const mdns_addr_with_port_t * const NONNULL addr_2)
{
	bool equal;

	if (addr_1 == addr_2) {
		equal = true;
		goto exit;
	}

	if (mDNSVal16(addr_1->port) != mDNSVal16(addr_2->port)) {
		equal = false;
		goto exit;
	}

	if (!mDNSSameAddress(&addr_1->address, &addr_2->address)) {
		equal = false;
		goto exit;
	}

	equal = true;
exit:
	return equal;
}
