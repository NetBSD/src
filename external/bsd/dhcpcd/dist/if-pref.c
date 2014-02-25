#include <sys/cdefs.h>
 __RCSID("$NetBSD: if-pref.c,v 1.1.1.8 2014/02/25 13:14:28 roy Exp $");

/*
 * dhcpcd - DHCP client daemon
 * Copyright (c) 2006-2014 Roy Marples <roy@marples.name>
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
#include "dhcp.h"
#include "if-pref.h"
#include "net.h"

/* Interface comparer for working out ordering. */
static int
ifcmp(const struct interface *si, const struct interface *ti)
{
	int sill, till;
	const struct dhcp_state *sis, *tis;

	sis = D_CSTATE(si);
	tis = D_CSTATE(ti);
	if (sis && !tis)
		return -1;
	if (!sis && tis)
		return 1;
	if (!sis && !tis)
		return 0;
	/* If one has a lease and the other not, it takes precedence. */
	if (sis->new && !tis->new)
		return -1;
	if (!sis->new && tis->new)
		return 1;
	/* If we are either, they neither have a lease, or they both have.
	 * We need to check for IPv4LL and make it non-preferred. */
	if (sis->new && tis->new) {
		sill = (sis->new->cookie == htonl(MAGIC_COOKIE));
		till = (tis->new->cookie == htonl(MAGIC_COOKIE));
		if (!sill && till)
			return 1;
		if (sill && !till)
			return -1;
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
sort_interfaces(struct dhcpcd_ctx *ctx)
{
	struct if_head sorted;
	struct interface *ifp, *ift;

	if (ctx->ifaces == NULL ||
	    (ifp = TAILQ_FIRST(ctx->ifaces)) == NULL ||
	    TAILQ_NEXT(ifp, next) == NULL)
		return;

	TAILQ_INIT(&sorted);
	TAILQ_REMOVE(ctx->ifaces, ifp, next);
	TAILQ_INSERT_HEAD(&sorted, ifp, next);
	while ((ifp = TAILQ_FIRST(ctx->ifaces))) {
		TAILQ_REMOVE(ctx->ifaces, ifp, next);
		TAILQ_FOREACH(ift, &sorted, next) {
			if (ifcmp(ifp, ift) == -1) {
				TAILQ_INSERT_BEFORE(ift, ifp, next);
				break;
			}
		}
		if (ift == NULL)
			TAILQ_INSERT_TAIL(&sorted, ifp, next);
	}
	TAILQ_CONCAT(ctx->ifaces, &sorted, next);
}
