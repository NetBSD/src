/*	$NetBSD: test.c,v 1.4 2011/08/30 20:29:41 joerg Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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
__RCSID("$NetBSD: test.c,v 1.4 2011/08/30 20:29:41 joerg Exp $");
#endif

#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <err.h>

#include <rpc/rpc.h>
#include <rpcsvc/bootparam_prot.h>

/* Default timeout can be changed using clnt_control() */
static struct timeval TIMEOUT = {25, 0};

bp_whoami_res *
bootparamproc_whoami_1(bp_whoami_arg *argp, CLIENT *clnt)
{
	static bp_whoami_res res;
	enum clnt_stat st;


	(void)memset(&res, 0, sizeof(res));
	if ((st = clnt_call(clnt, BOOTPARAMPROC_WHOAMI, xdr_bp_whoami_arg, argp,
	    xdr_bp_whoami_res, &res, TIMEOUT)) != RPC_SUCCESS) {
		warnx("clnt_call returned %s", clnt_sperrno(st));
		return NULL;
	}
	return &res;
}


int
main(int argc, char **argv)
{
	CLIENT *cli;
	bp_whoami_res *out;
	bp_whoami_arg bp;
	struct hostent *hp;
	uint32_t addr;

	if (argc < 2) {
		warnx("usage: test <hostname>");
		errx(1, "Always talks to bootparamd at localhost");
	}
	if ((hp = gethostbyname(argv[1])) == NULL)
		errx(1, "Cannot resolve `%s'", argv[1]);

	printf("Creating client for localhost\n");
	cli = clnt_create("localhost", BOOTPARAMPROG, BOOTPARAMVERS, "udp");
	if (!cli)
		errx(1, "Failed to create client");
	memcpy(&addr, hp->h_addr_list[0], sizeof(addr));
	addr = htonl(addr);
	bp.client_address.address_type = IP_ADDR_TYPE;
	bp.client_address.bp_address_u.ip_addr.net = (addr >> 24) & 0xff;
	bp.client_address.bp_address_u.ip_addr.host = (addr >> 16) & 0xff;
	bp.client_address.bp_address_u.ip_addr.lh = (addr >> 8) & 0xff;
	bp.client_address.bp_address_u.ip_addr.impno = (addr >> 0) & 0xff;
	
	if ((out = bootparamproc_whoami_1(&bp, cli)) != NULL) {
		printf("Success [host=%s, domain=%s, gw=%d.%d.%d.%d]\n",
		    out->client_name, out->domain_name, 
		    out->router_address.bp_address_u.ip_addr.net & 0xff,
		    out->router_address.bp_address_u.ip_addr.host & 0xff,
		    out->router_address.bp_address_u.ip_addr.lh & 0xff,
		    out->router_address.bp_address_u.ip_addr.impno & 0xff);
	} else
		printf("Fail\n");

	return 0;
}
