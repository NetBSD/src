/*	$KAME: remoteconf.c,v 1.30 2003/06/27 07:32:39 sakane Exp $	*/

/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: remoteconf.c,v 1.2 2003/07/12 09:37:12 itojun Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/queue.h>

#include <netinet/in.h>
#include <netkey/key_var.h>

#ifdef IPV6_INRIA_VERSION
#include <netinet/ipsec.h>
#else
#include <netinet6/ipsec.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "var.h"
#include "misc.h"
#include "vmbuf.h"
#include "plog.h"
#include "sockmisc.h"
#include "debug.h"

#include "isakmp_var.h"
#include "isakmp.h"
#include "ipsec_doi.h"
#include "oakley.h"
#include "remoteconf.h"
#include "localconf.h"
#include "grabmyaddr.h"
#include "proposal.h"
#include "vendorid.h"
#include "gcmalloc.h"

static LIST_HEAD(_rmtree, remoteconf) rmtree;

/*%%%*/
/*
 * search remote configuration.
 * don't use port number to search if its value is either IPSEC_PORT_ANY.
 * If matching anonymous entry, then new entry is copied from anonymous entry.
 * If no anonymous entry found, then return NULL.
 * OUT:	NULL:	NG
 *	Other:	remote configuration entry.
 */
struct remoteconf *
getrmconf(remote)
	struct sockaddr *remote;
{
	struct remoteconf *p;
	struct remoteconf *anon = NULL;
	int withport;
	char buf[NI_MAXHOST + NI_MAXSERV + 10];
	char addr[NI_MAXHOST], port[NI_MAXSERV];

	withport = 0;

	switch (remote->sa_family) {
	case AF_INET:
		if (((struct sockaddr_in *)remote)->sin_port != IPSEC_PORT_ANY)
			withport = 1;
		break;
#ifdef INET6
	case AF_INET6:
		if (((struct sockaddr_in6 *)remote)->sin6_port != IPSEC_PORT_ANY)
			withport = 1;
		break;
#endif
	default:
		plog(LLV_ERROR, LOCATION, NULL,
			"invalid family: %d\n", remote->sa_family);
		exit(1);
	}

	GETNAMEINFO(remote, addr, port);
	snprintf(buf, sizeof(buf), "%s%s%s%s", addr,
		withport ? "[" : "",
		withport ? port : "",
		withport ? "]" : "");

	LIST_FOREACH(p, &rmtree, chain) {
		if ((!withport && cmpsaddrwop(remote, p->remote) == 0)
		 || (withport && cmpsaddrstrict(remote, p->remote) == 0)) {
			plog(LLV_DEBUG, LOCATION, NULL,
				"configuration found for %s.\n", buf);
			return p;
		}

		/* save the pointer to the anonymous configuration */
		if (p->remote->sa_family == AF_UNSPEC)
			anon = p;
	}

	if (anon != NULL) {
		plog(LLV_DEBUG, LOCATION, NULL,
			"anonymous configuration selected for %s.\n", buf);
		return anon;
	}

	plog(LLV_DEBUG, LOCATION, NULL,
		"no remote configuration found.\n");
	return NULL;
}

struct remoteconf *
newrmconf()
{
	struct remoteconf *new;

	new = racoon_calloc(1, sizeof(*new));
	if (new == NULL)
		return NULL;

	new->proposal = NULL;

	/* set default */
	new->doitype = IPSEC_DOI;
	new->sittype = IPSECDOI_SIT_IDENTITY_ONLY;
	new->idvtype = IDTYPE_ADDRESS;
	new->idvtype_p = IDTYPE_ADDRESS;
	new->nonce_size = DEFAULT_NONCE_SIZE;
	new->keepalive = FALSE;
	new->passive = FALSE;
	new->ini_contact = TRUE;
	new->pcheck_level = PROP_CHECK_STRICT;
	new->verify_identifier = FALSE;
	new->verify_cert = TRUE;
	new->getcert_method = ISAKMP_GETCERT_PAYLOAD;
	new->send_cert = TRUE;
	new->send_cr = TRUE;
	new->support_proxy = FALSE;
	new->gen_policy = FALSE;
	new->retry_counter = lcconf->retry_counter;
	new->retry_interval = lcconf->retry_interval;

	return new;
}

void
delrmconf(rmconf)
	struct remoteconf *rmconf;
{
	if (rmconf->etypes)
		deletypes(rmconf->etypes);
	if (rmconf->dhgrp)
		oakley_dhgrp_free(rmconf->dhgrp);
	if (rmconf->proposal)
		delisakmpsa(rmconf->proposal);
	racoon_free(rmconf);
}

void
delisakmpsa(sa)
	struct isakmpsa *sa;
{
	if (sa->dhgrp)
		oakley_dhgrp_free(sa->dhgrp);
	if (sa->next)
		delisakmpsa(sa->next);
#ifdef HAVE_GSSAPI
	if (sa->gssid)
		vfree(sa->gssid);
#endif
	racoon_free(sa);
}

void
deletypes(e)
	struct etypes *e;
{
	if (e->next)
		deletypes(e->next);
	racoon_free(e);
}

/*
 * insert into head of list.
 */
void
insrmconf(new)
	struct remoteconf *new;
{
	LIST_INSERT_HEAD(&rmtree, new, chain);
}

void
remrmconf(rmconf)
	struct remoteconf *rmconf;
{
	LIST_REMOVE(rmconf, chain);
}

void
flushrmconf()
{
	struct remoteconf *p, *next;

	for (p = LIST_FIRST(&rmtree); p; p = next) {
		next = LIST_NEXT(p, chain);
		remrmconf(p);
		delrmconf(p);
	}
}

void
initrmconf()
{
	LIST_INIT(&rmtree);
}

/* check exchange type to be acceptable */
struct etypes *
check_etypeok(rmconf, etype)
	struct remoteconf *rmconf;
	u_int8_t etype;
{
	struct etypes *e;

	for (e = rmconf->etypes; e != NULL; e = e->next) {
		if (e->type == etype)
			break;
	}

	return e;
}

/*%%%*/
struct isakmpsa *
newisakmpsa()
{
	struct isakmpsa *new;

	new = racoon_calloc(1, sizeof(*new));
	if (new == NULL)
		return NULL;

	/*
	 * Just for sanity, make sure this is initialized.  This is
	 * filled in for real when the ISAKMP proposal is configured.
	 */
	new->vendorid = VENDORID_UNKNOWN;

	new->next = NULL;
	new->rmconf = NULL;
#ifdef HAVE_GSSAPI
	new->gssid = NULL;
#endif

	return new;
}

/*
 * insert into tail of list.
 */
void
insisakmpsa(new, rmconf)
	struct isakmpsa *new;
	struct remoteconf *rmconf;
{
	struct isakmpsa *p;

	new->rmconf = rmconf;

	if (rmconf->proposal == NULL) {
		rmconf->proposal = new;
		return;
	}

	for (p = rmconf->proposal; p->next != NULL; p = p->next)
		;
	p->next = new;

	return;
}

const char *
rm2str(rmconf)
	const struct remoteconf *rmconf;
{
	if (rmconf->remote->sa_family == AF_UNSPEC)
		return "anonymous";
	return saddr2str(rmconf->remote);
}
