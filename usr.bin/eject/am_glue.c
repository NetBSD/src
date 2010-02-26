/*	$NetBSD: am_glue.c,v 1.2 2010/02/26 20:18:37 christos Exp $	*/

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: am_glue.c,v 1.2 2010/02/26 20:18:37 christos Exp $");
#endif /* not lint */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* HAVE_CONFIG_H */
#include <am_defs.h>
#include <amu.h>
#include <rpc/pmap_prot.h>
#include <rpc/pmap_clnt.h>

#include "am_glue.h"

static CLIENT *clnt;

static struct timeval tv = { 5, 0 };
/*
 * Appease lint: Properly typecast some numbers defined in
 * src/extern/bsd/am-utils/dist/include/amq_defs.h.
 */
#define	xAMQ_PROGRAM		(rpcprog_t)AMQ_PROGRAM
#define xAMQ_VERSION		(rpcvers_t)AMQ_VERSION
#define xAMQPROC_SYNC_UMNT	(rpcproc_t)AMQPROC_SYNC_UMNT

static int
ping_pmap(void)
{
	u_short port = 0;
	CLIENT *cl;
	struct pmap parms;
	struct sockaddr_in si;
	int s = -1, rv;

	(void)memset(&si, 0, sizeof(si));
	si.sin_family = AF_INET;
	si.sin_len = sizeof(si);
	si.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	si.sin_port = htons(PMAPPORT);

	if ((cl = clntudp_bufcreate(&si, PMAPPROG, PMAPVERS, tv,
	    &s, RPCSMALLMSGSIZE, RPCSMALLMSGSIZE)) == NULL)
		return -1;

	parms.pm_prog = PMAPPROG;
	parms.pm_vers = PMAPVERS;
	parms.pm_prot = IPPROTO_UDP;
	parms.pm_port = 0;  /* not needed or used */

	rv = CLNT_CALL(cl, (rpcproc_t)PMAPPROC_GETPORT,
	    (xdrproc_t)xdr_pmap, &parms,
	    (xdrproc_t)xdr_u_short, &port, tv) == RPC_SUCCESS ? 0 : -1;

	CLNT_DESTROY(cl);
	return rv;
}

void
am_init(void)
{
	static const char *server = "localhost";

	if (ping_pmap() == -1)
		return;
	/*
	 * Create RPC endpoint
	 */
	/* try tcp first */
	clnt = clnt_create(server, xAMQ_PROGRAM, xAMQ_VERSION, "tcp");
	if (clnt != NULL)
		return;

	/* try udp next */
	clnt = clnt_create(server, xAMQ_PROGRAM, xAMQ_VERSION, "udp");
	if (clnt != NULL)	/* set udp timeout */
		(void)clnt_control(clnt, CLSET_RETRY_TIMEOUT, (void *)&tv);
}

int
am_unmount(const char *dirname)
{
	static struct timeval timeout = { ALLOWED_MOUNT_TIME, 0 };
	amq_sync_umnt result;

	if (clnt == NULL)
		return AMQ_UMNT_FAILED;

	if (clnt_call(clnt, xAMQPROC_SYNC_UMNT, xdr_amq_string, &dirname,
	    xdr_amq_sync_umnt, (void *)&result, timeout) != RPC_SUCCESS)
		return AMQ_UMNT_SERVER;

	return result.au_etype;
}
