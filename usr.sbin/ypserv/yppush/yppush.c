/*	$NetBSD: yppush.c,v 1.4 1997/07/18 21:57:12 thorpej Exp $	*/

/*
 * Copyright (c) 1995 Mats O Jansson <moj@stacken.kth.se>
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
 *	This product includes software developed by Mats O Jansson
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <sys/wait.h>

#include <ctype.h>
#include <errno.h>
#include <err.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#include <rpc/rpc.h>
#include <rpc/xdr.h>
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypclnt.h>

#include "protos.h"
#include "yplib_host.h"
#include "ypdef.h"
#include "ypdb.h"
#include "yppush.h"

extern	char *__progname;		/* from crt0.o */

int	main __P((int, char *[]));
int	pushit __P((int, char *, int, char *, int, char *));
void	push __P((int, char *));
void	req_xfr __P((pid_t, u_int, SVCXPRT *, char *, CLIENT *));
void	_svc_run __P((void));
void	cleanup __P((void));
void	usage __P((void));

int	Verbose = 0;
u_int	OrderNum;

char	Domain[YPMAXDOMAIN];
char	Map[YPMAXMAP];
char	LocalHost[YPMAXPEER];

static	DBM *yp_databas;

int
main(argc, argv)
	int  argc;
	char *argv[];
{
	struct ypall_callback ypcb;
	char *master, *domain, *map, *hostname;
	int c, r, i;
	datum o;
	char *ypmap = "ypservers";
	CLIENT *client;
	struct stat finfo;
	char order_key[YP_LAST_LEN] = YP_LAST_KEY;
	static char map_path[MAXPATHLEN];

	if (atexit(cleanup))
		err(1, "can't register cleanup function");

	domain = NULL;
	hostname = NULL;

	while ((c = getopt(argc, argv, "d:h:v")) != -1) {
		switch(c) {
		case 'd':
			domain = optarg;
			break;

		case 'h':
			hostname = optarg;
			break;

                case 'v':
			Verbose = 1;
			break;

		default:
                        usage();
                        /*NOTREACHED*/
		}
	}
	argv += optind; argc -= optind;

	if (argc != 1)
		usage();

	map = argv[0];

	if (domain == NULL)
		if ((c = yp_get_default_domain(&domain)))
			errx(1, "can't get YP domain name.  Reason: %s",
			    yperr_string(c));

	memset(Domain, 0, sizeof(Domain));
	snprintf(Domain, sizeof(Domain), "%s", domain);
	memset(Map, 0, sizeof(Map));
	snprintf(Map, sizeof(Map), "%s", map);

	/* Set up the hostname to be used for the xfr */
	localhostname(LocalHost, sizeof(LocalHost));

	/* Check domain */
	sprintf(map_path,"%s/%s",YP_DB_PATH,domain);
	if (!((stat(map_path, &finfo) == 0) &&
	      ((finfo.st_mode & S_IFMT) == S_IFDIR))) {
	  	fprintf(stderr,"yppush: Map does not exists.\n");
		exit(1);
	}
		
	/* Check map */
	memset(map_path, 0, sizeof(map_path));
	snprintf(map_path, sizeof(map_path), "%s/%s/%s%s", YP_DB_PATH,
	    Domain, Map, YPDB_SUFFIX);
	if (stat(map_path, &finfo) != 0)
		errx(1, "map does not exist");

	/* Open the database */
	memset(map_path, 0, sizeof(map_path));
	snprintf(map_path, sizeof(map_path), "%s/%s/%s", YP_DB_PATH,
	    Domain, Map);
	yp_databas = ypdb_open(map_path, 0, O_RDONLY);
	OrderNum=0xffffffff;
	if (yp_databas == NULL)
		errx(1, "%s%s: cannot open database", map_path, YPDB_SUFFIX);
	else {
		o.dptr = (char *) &order_key;
		o.dsize = YP_LAST_LEN;
		o = ypdb_fetch(yp_databas, o);
		if (o.dptr == NULL)
			errx(1, "%s: cannot determine order number", Map);
		else {
			OrderNum = 0;
			for (i = 0; i < o.dsize - 1; i++)
				if (!isdigit(o.dptr[i]))
					OrderNum=0xffffffff;

			if (OrderNum != 0)
				errx(1, "%s: invalid order number `%s'",
				    Map, o.dptr);
			else
				OrderNum = atoi(o.dptr);
		}
        }

	/* No longer need YP map. */
	ypdb_close(yp_databas);
	yp_databas = NULL;

	r = yp_bind(Domain);
	if (r != 0)
		errx(1, "%s", yperr_string(r));

	if (hostname != NULL)
		push(strlen(hostname), hostname);
	else {
		r = yp_master(Domain, ypmap, &master);
		if (r != 0)
			errx(1, "%s (map = %s)", yperr_string(r), ypmap);

		if (Verbose)
			printf("Contacting master for ypservers (%s).\n",
			    master);

		client = yp_bind_host(master, YPPROG, YPVERS, 0, 1);

		ypcb.foreach = pushit;
		ypcb.data = NULL;

		r = yp_all_host(client, Domain, ypmap, &ypcb);
		if (r != 0)
			errx(1, "%s (map = %s)", yperr_string(r), ypmap);
	}
	exit (0);
}

void
usage()
{

	fprintf(stderr, "usage: %s [-d domainname] [-h host] [-v] mapname\n",
	    __progname);
	exit(1);
}

void
_svc_run()
{
	fd_set readfds;
	struct timeval timeout;

	timeout.tv_sec = 60;
	timeout.tv_usec = 0;

	for(;;) {
		readfds = svc_fdset;

		switch (select(sysconf(_SC_OPEN_MAX), &readfds, NULL, NULL,
		    &timeout)) {

		case -1:
			if (errno == EINTR)
				continue;

			warnx("_svc_run: select failed");
			return;

		case 0:
			errx(0, "Callback timed out.");

		default:
			svc_getreqset(&readfds);
		}
	}
}

void
req_xfr(pid, prog, transp, host, client)
	pid_t pid;
	u_int prog;
	SVCXPRT *transp;
	char *host;
	CLIENT *client;
{
	struct ypreq_xfr request;
	struct timeval tv;

	tv.tv_sec = 0;
	tv.tv_usec = 0;

	request.map_parms.domain = &Domain[0];
	request.map_parms.map = &Map[0];
	request.map_parms.owner = &LocalHost[0];
	request.map_parms.ordernum = OrderNum;
	request.transid = (u_int)pid;
	request.proto = prog;
	request.port = transp->xp_port;

	if (Verbose)
		printf("%d: %s(%d@%s) -> %s@%s\n", request.transid,
		    request.map_parms.map, request.map_parms.ordernum,
		    host, request.map_parms.owner, request.map_parms.domain);

	switch (clnt_call(client, YPPROC_XFR, xdr_ypreq_xfr, &request,
	    xdr_void, NULL, tv)) {
	case RPC_SUCCESS:
	case RPC_TIMEDOUT:
		break;

	default:
		clnt_perror(client, "yppush: Cannot call YPPROC_XFR");
		kill(pid, SIGTERM);
	}
}

void
push(inlen, indata)
	int inlen;
	char *indata;
{
	char host[YPMAXPEER];
	CLIENT *client;
	SVCXPRT *transp;
	int sock = RPC_ANYSOCK;
	u_int prog;
	bool_t sts;
	pid_t pid;
	int status;
	struct rusage res;

	memset(host, 0, sizeof(host));
	snprintf(host, sizeof(host), "%*.*s", inlen, inlen, indata);

	client = clnt_create(host, YPPROG, YPVERS, "tcp");
	if (client == NULL) {
		if (Verbose)
			fprintf(stderr, "Target host: %s\n", host);
		clnt_pcreateerror("yppush: cannot create client");
		return;
	}

	transp = svcudp_create(sock);
	if (transp == NULL) {
		warnx("cannot create callback transport");
		return;
	}

	for (prog = 0x40000000; prog < 0x5fffffff; prog++) {
		if ((sts = svc_register(transp, prog, 1,
		    yppush_xfrrespprog_1, IPPROTO_UDP)))
			break;
	}

	if (sts == FALSE) {
		warnx("cannot register callback");
		return;
	}

	switch ((pid = fork())) {
	case -1:
		err(1, "fork failed");

	case 0:
		_svc_run();
		exit(0);

	default:
		close(transp->xp_sock);
		req_xfr(pid, prog, transp, host, client);
		wait4(pid, &status, 0, &res);
		svc_unregister(prog, 1);
		if (client != NULL)
		  	clnt_destroy(client);
	}
}

int
pushit(instatus, inkey, inkeylen, inval, invallen, indata)
	int instatus;
	char *inkey;
	int inkeylen;
	char *inval;
	int invallen;
	char *indata;
{

	if (instatus != YP_TRUE)
		return instatus;

	push(inkeylen, inkey);
	return 0;
}

void
cleanup()
{

	/* Make sure the map file is closed. */
	if (yp_databas != NULL)
		ypdb_close(yp_databas);
}
