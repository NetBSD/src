/*	$NetBSD: ypwhich.c,v 1.8 1997/07/18 07:40:43 thorpej Exp $	*/

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
__RCSID("$NetBSD: ypwhich.c,v 1.8 1997/07/18 07:40:43 thorpej Exp $");
#endif

#include <sys/param.h>
#include <sys/socket.h>
#include <ctype.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <rpc/rpc.h>
#include <rpc/xdr.h>
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypclnt.h>

const struct ypalias {
	char *alias, *name;
} ypaliases[] = {
	{ "passwd", "passwd.byname" },
	{ "group", "group.byname" },
	{ "networks", "networks.byaddr" },
	{ "hosts", "hosts.byaddr" },
	{ "protocols", "protocols.bynumber" },
	{ "services", "services.byname" },
	{ "aliases", "mail.aliases" },
	{ "ethers", "ethers.byname" },
};

int	main __P((int, char *[]));
int	bind_host __P((char *dom, struct sockaddr_in *));
void	usage __P((void));

extern	char *__progname;

int
main(argc, argv)
	int argc;
	char *argv[];
{
	char *domainname, *master, *map;
	struct ypmaplist *ypml, *y;
	struct hostent *hent;
	struct sockaddr_in sin;
	int notrans, mode, getmap;
	int c, r, i;

	yp_get_default_domain(&domainname);

	map = NULL;
	getmap = notrans = mode = 0;
	while((c = getopt(argc, argv, "xd:mt")) != -1) {
		switch (c) {
		case 'x':
			for(i = 0;
			    i < sizeof(ypaliases)/sizeof(ypaliases[0]); i++)
				printf("Use \"%s\" for \"%s\"\n",
					ypaliases[i].alias,
					ypaliases[i].name);
			exit(0);

		case 'd':
			domainname = optarg;
			break;

		case 't':
			notrans++;
			break;

		case 'm':
			mode++;
			break;

		default:
			usage();
		}
	}

	argc -= optind;
	argv += optind;

	if (domainname == NULL)
		errx(1, "YP domain name not set");

	if (mode == 0) {
		switch (argc) {
		case 0:
			memset(&sin, 0, sizeof(sin));
			sin.sin_family = AF_INET;
			sin.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

			if (bind_host(domainname, &sin))
				exit(1);
			break;

		case 1:
			memset(&sin, 0, sizeof(sin));
			sin.sin_family = AF_INET;
			if (inet_aton(argv[0], &sin.sin_addr) == 0) {
				hent = gethostbyname(argv[0]);
				if (hent == NULL)
					errx(1, "host %s unknown", argv[0]);
				memcpy(&sin.sin_addr, hent->h_addr,
				    sizeof(sin.sin_addr));
			}
			if (bind_host(domainname, &sin))
				exit(1);
			break;

		default:
			usage();
		}
		exit(0);
	}

	if (argc > 1)
		usage();

	if (argc == 1) {
		map = argv[0];
		for (i = 0; (notrans == 0) &&
		    i < sizeof(ypaliases)/sizeof(ypaliases[0]); i++)
			if (strcmp(map, ypaliases[i].alias) == 0)
				map = ypaliases[i].name;
		r = yp_master(domainname, map, &master);
		switch (r) {
		case 0:
			printf("%s\n", master);
			free(master);
			break;

		case YPERR_YPBIND:
			errx(1, "not running upbind");

		default:
			errx(1, "can't find master for map %s.  Reason: %s",
			    map, yperr_string(r));
		}
		exit(0);
	}

	ypml = NULL;
	r = yp_maplist(domainname, &ypml);
	switch (r) {
	case 0:
		for (y = ypml; y != NULL; ) {
			ypml = y;
			r = yp_master(domainname, ypml->ypml_name, &master);
			switch (r) {
			case 0:
				printf("%s %s\n", ypml->ypml_name, master);
				free(master);
				break;

			default:
				warnx("can't find master for map %s.  "
				    "Reason: %s", ypml->ypml_name,
				   yperr_string(r));
				break;
			}
			y = ypml->ypml_next;
			free(ypml);
		}
		break;

	case YPERR_YPBIND:
		errx(1, "not running ypbind");

	default:
		errx(1, "can't get map list for domain %s.  Reason: %s",
		    domainname, yperr_string(r));
	}
	exit(0);
}

/*
 * Like yp_bind except can query a specific host
 */
int
bind_host(dom, sin)
	char *dom;
	struct sockaddr_in *sin;
{
	struct hostent *hent = NULL;
	struct ypbind_resp ypbr;
	struct timeval tv;
	CLIENT *client;
	int sock, r;
	struct in_addr ina;

	sock = RPC_ANYSOCK;
	tv.tv_sec = 15;
	tv.tv_usec = 0;
	client = clntudp_create(sin, YPBINDPROG, YPBINDVERS, tv, &sock);
	if (client == NULL) {
		warnx("can't clntudp_create: %s", yperr_string(YPERR_YPBIND));
		return YPERR_YPBIND;
	}

	tv.tv_sec = 5;
	tv.tv_usec = 0;
	r = clnt_call(client, YPBINDPROC_DOMAIN, xdr_ypdomain_wrap_string,
	    &dom, xdr_ypbind_resp, &ypbr, tv);
	if (r != RPC_SUCCESS) {
		warnx("can't cnlt_call: %s", yperr_string(YPERR_YPBIND));
		clnt_destroy(client);
		return YPERR_YPBIND;
	} else {
		if (ypbr.ypbind_status != YPBIND_SUCC_VAL) {
			warnx("can't bind.  Reason: %s\n",
			    yperr_string(ypbr.ypbind_status));
			clnt_destroy(client);
			return r;
		}
	}
	clnt_destroy(client);

	ina.s_addr =
	    ypbr.ypbind_respbody.ypbind_bindinfo.ypbind_binding_addr.s_addr;
	hent = gethostbyaddr((const char *)&ina.s_addr, sizeof(ina.s_addr),
	    AF_INET);
	if (hent != NULL)
		printf("%s\n", hent->h_name);
	else
		printf("%s\n", inet_ntoa(ina));
	return 0;
}

void
usage()
{

	fprintf(stderr, "usage: %s [-d domain] [[-t] -m [mapname] | host]\n",
	    __progname);
	fprintf(stderr, "       %s -x\n", __progname);
	exit(1);
}
