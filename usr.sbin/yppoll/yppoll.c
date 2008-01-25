/*	$NetBSD: yppoll.c,v 1.14 2008/01/25 19:58:54 christos Exp $	*/

/*
 * Copyright (c) 1992, 1993 Theo de Raadt <deraadt@fsa.ca>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Copyright (c) 1992, 1993 John Brezak
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
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: yppoll.c,v 1.14 2008/01/25 19:58:54 christos Exp $");
#endif /* not lint */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <rpc/rpc.h>
#include <rpc/xdr.h>
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypclnt.h>

static CLIENT *mkclient(struct sockaddr_in *, unsigned long, unsigned long,
    int);
static int get_remote_info(const char *, const char *, const char *, int *,
    char **, int);
static void usage(void) __attribute__((__noreturn__));

int
main(int  argc, char *argv[])
{
	char *domainname;
	char *hostname = NULL;
	char *inmap, *master;
	int order;
	int c, r, tcp = 0;

	(void)yp_get_default_domain(&domainname);

	while ((c = getopt(argc, argv, "Th:d:")) != -1) {
		switch (c) {
		case 'T':
			tcp = 1;
			break;
		case 'd':
			domainname = optarg;
			break;

		case 'h':
			hostname = optarg;
			break;

		default:
			usage();
			/*NOTREACHED*/
		}
	}

	if (domainname == NULL)
		errx(1, "YP domain name not set");

	argc -= optind;
	argv += optind;

	if (argc != 1)
		usage();

	inmap = argv[0];

	if (hostname != NULL)
		r = get_remote_info(domainname, inmap, hostname,
		    &order, &master, tcp);
	else {
		r = yp_order(domainname, inmap, &order);
		if (r == 0)
			r = yp_master(domainname, inmap, &master);
	}

	if (r != 0)
		errx(1, "no such map %s. Reason: %s",
		    inmap, yperr_string(r));

	(void)printf("Map %s has order number %d. %s", inmap, order,
	    ctime((void *)&order));
	(void)printf("The master server is %s.\n", master);
	return 0;
}

static CLIENT *
mkclient(struct sockaddr_in *sin, unsigned long prog, unsigned long vers,
    int tcp)
{
	static struct timeval tv = { 10, 0 };
	int fd = RPC_ANYSOCK;

	if (tcp)
		return clnttcp_create(sin, prog, vers, &fd, 0, 0);
	else
		return clntudp_create(sin, prog, vers, tv, &fd);
}

static int
get_remote_info(const char *indomain, const char *inmap, const char *server,
    int *outorder, char **outname, int tcp)
{
	struct ypresp_order ypro;
	struct ypresp_master yprm;
	struct ypreq_nokey yprnk;
	struct timeval tv;
	int r;
	struct sockaddr_in rsrv_sin;
	CLIENT *client;
	struct hostent *h;

	(void)memset(&rsrv_sin, 0, sizeof(rsrv_sin));
	rsrv_sin.sin_len = sizeof rsrv_sin;
	rsrv_sin.sin_family = AF_INET;

	h = gethostbyname(server);
	if (h == NULL) {
		if (inet_aton(server, &rsrv_sin.sin_addr) == 0)
			errx(1, "unknown host %s", server);
	} else
		(void)memcpy(&rsrv_sin.sin_addr.s_addr, h->h_addr,
		    (size_t)h->h_length);

	client = mkclient(&rsrv_sin, YPPROG, YPVERS, tcp);
	if (client == NULL)
		errx(1, "clnt%s_create: no contact with host %s.",
		    tcp ? "tcp" : "udp", server);
	
	yprnk.domain = indomain;
	yprnk.map = inmap;

	(void)memset(&ypro, 0, sizeof(ypro));

	tv.tv_sec = 10;
	tv.tv_usec = 0;

	r = clnt_call(client, (unsigned int)YPPROC_ORDER, xdr_ypreq_nokey,
	    &yprnk, xdr_ypresp_order, &ypro, tv);
	if (r != RPC_SUCCESS)
		clnt_perror(client, "yp_order: clnt_call");

	*outorder = ypro.ordernum;
	xdr_free(xdr_ypresp_order, (void *)&ypro);

	r = ypprot_err(ypro.status);
	if (r == RPC_SUCCESS) {
		(void)memset(&yprm, 0, sizeof(yprm));

		r = clnt_call(client, (unsigned int)YPPROC_MASTER,
		    xdr_ypreq_nokey, &yprnk, xdr_ypresp_master, &yprm, tv);
		if (r != RPC_SUCCESS)
			clnt_perror(client, "yp_master: clnt_call");
		r = ypprot_err(yprm.status);
		if (r == 0)
			*outname = (char *)strdup(yprm.master);
		xdr_free(xdr_ypresp_master, (void *)&yprm);
	}
	clnt_destroy(client);
	return r;
}

static void
usage(void)
{

	(void)fprintf(stderr,
	    "Usage: %s [-T] [-h host] [-d domainname] mapname\n",
	    getprogname());
	exit(1);
}
