/* 
 * dhcpcd - DHCP client daemon
 * Copyright (c) 2006-2009 Roy Marples <roy@marples.name>
 * All rights reserved

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/types.h>

#include "config.h"
#include "dhcpcd.h"
#include "if-pref.h"
#include "net.h"

/* Interface comparer for working out ordering. */
static int
ifcmp(struct interface *si, struct interface *ti)
{
	int sill, till;

	if (si->state && !ti->state)
		return -1;
	if (!si->state && ti->state)
		return 1;
	if (!si->state && !ti->state)
		return 0;
	/* If one has a lease and the other not, it takes precedence. */
	if (si->state->new && !ti->state->new)
		return -1;
	if (!si->state->new && ti->state->new)
		return 1;
	/* If we are either, they neither have a lease, or they both have.
	 * We need to check for IPv4LL and make it non-preferred. */
	if (si->state->new) {
		sill = IN_LINKLOCAL(htonl(si->state->new->yiaddr));
		till = IN_LINKLOCAL(htonl(ti->state->new->yiaddr));
		if (!sill && till)
			return -1;
		if (sill && !till)
			return 1;
	}
	/* Then carrier status. */
	if (si->carrier > ti->carrier)
		return -1;
	if (si->carrier < ti->carrier)
		return 1;
	/* Finally, metric */
	if (si->metric < ti->metric)
		return -1;
	if (si->metric > ti->metric)
		return 1;
	return 0;
}

/* Sort the interfaces into a preferred order - best first, worst last. */
void
sort_interfaces(void)
{
	struct interface *sorted, *ifp, *ifn, *ift;

	if (!ifaces || !ifaces->next)
		return;
	sorted = ifaces;
	ifaces = ifaces->next;
	sorted->next = NULL;
	for (ifp = ifaces; ifp && (ifn = ifp->next, 1); ifp = ifn) {
		/* Are we the new head? */
		if (ifcmp(ifp, sorted) == -1) {
			ifp->next = sorted;
			sorted = ifp;
			continue;
		}
		/* Do we fit in the middle? */
		for (ift = sorted; ift->next; ift = ift->next) {
			if (ifcmp(ifp, ift->next) == -1) {
				ifp->next = ift->next;
				ift->next = ifp;
				break;
			}
		}
		/* We must be at the end */
		if (!ift->next) {
			ift->next = ifp;
			ifp->next = NULL;
		}
	}
	ifaces = sorted;
}
