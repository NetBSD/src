/*	$NetBSD: ypset.c,v 1.11 1997/07/18 08:16:58 thorpej Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Theo de Raadt.
 * 4. The name of the author may not be used to endorse or promote
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
__RCSID("$NetBSD: ypset.c,v 1.11 1997/07/18 08:16:58 thorpej Exp $");
#endif

#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <err.h>
#include <netdb.h>

#include <rpc/rpc.h>
#include <rpc/xdr.h>
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypclnt.h>
#include <arpa/inet.h>

extern char *__progname;

int	main __P((int, char *[]));
static void usage __P((void));
static void gethostaddr __P((const char *, struct in_addr *));
static int bind_tohost __P((struct sockaddr_in *, char *, char *));

int
main(argc, argv)
	int argc;
	char *argv[];
{
	struct sockaddr_in sin;
	extern char *optarg;
	extern int optind;
	char *domainname;
	int c;

	yp_get_default_domain(&domainname);

	(void) memset(&sin, 0, sizeof sin);
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	while ((c = getopt(argc, argv, "h:d:")) != -1) {
		switch(c) {
		case 'd':
			domainname = optarg;
			break;

		case 'h':
			gethostaddr(optarg, &sin.sin_addr);
			break;

		default:
			usage();
		}
	}

	if (domainname == NULL)
		errx(1, "YP domain name not set");

	argc -= optind;
	argv += optind;

	if (argc != 1)
		usage();

	return bind_tohost(&sin, domainname, argv[0]) != 0;
}

static void
gethostaddr(host, ia)
	const char *host;
	struct in_addr *ia;
{
	struct hostent *hp;

	if (inet_aton(host, ia) != 0)
		return;

	hp = gethostbyname(host);
	if (hp == NULL)
		errx(1, "Cannot get host address for %s: %s", host,
		    hstrerror(h_errno));
	(void) memcpy(ia, hp->h_addr, sizeof(*ia));
}

static int
bind_tohost(sin, dom, server)
	struct sockaddr_in *sin;
	char *dom, *server;
{
	struct ypbind_setdom ypsd;
	struct timeval tv;
	CLIENT *client;
	int sock, port;
	int r;
	
	port = htons(getrpcport(server, YPPROG, YPPROC_NULL, IPPROTO_UDP));
	if (port == 0)
		errx(1, "%s not running ypserv.", server);

	(void) memset(&ypsd, 0, sizeof ypsd);

	gethostaddr(server, &ypsd.ypsetdom_addr);

	(void) strncpy(ypsd.ypsetdom_domain, dom, sizeof ypsd.ypsetdom_domain);
	ypsd.ypsetdom_domain[sizeof(ypsd.ypsetdom_domain) - 1] = '\0';
	ypsd.ypsetdom_port = port;
	ypsd.ypsetdom_vers = YPVERS;
	
	tv.tv_sec = 15;
	tv.tv_usec = 0;
	sock = RPC_ANYSOCK;

	client = clntudp_create(sin, YPBINDPROG, YPBINDVERS, tv, &sock);
	if (client == NULL) {
		warnx("Can't yp_bind: Reason: %s", yperr_string(YPERR_YPBIND));
		return YPERR_YPBIND;
	}
	client->cl_auth = authunix_create_default();

	r = clnt_call(client, YPBINDPROC_SETDOM,
	    xdr_ypbind_setdom, &ypsd, xdr_void, NULL, tv);
	if (r) {
		warnx("Cannot ypset for domain %s on host %s: %s.\n",
		    dom, server, clnt_sperrno(r));
		clnt_destroy(client);
		return YPERR_YPBIND;
	}
	clnt_destroy(client);
	return 0;
}

static void
usage()
{
	(void) fprintf(stderr, "usage: %s [-h host ] [-d domain] server\n",
	    __progname);
	exit(1);
}
