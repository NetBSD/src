/*	$NetBSD: yppush.c,v 1.10 1999/01/11 22:40:01 kleink Exp $	*/

/*
 *
 * Copyright (c) 1997 Charles D. Cranor
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * yppush
 * author: Chuck Cranor <chuck@ccrc.wustl.edu>
 * date: 05-Nov-97
 *
 * notes: this is a full rewrite of Mats O Jansson <moj@stacken.kth.se>'s
 * yppush.c.   i have restructured and cleaned up the entire file.
 */
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <rpc/rpc.h>
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypclnt.h>

#include "ypdb.h"
#include "ypdef.h"
#include "yplib_host.h"
#include "yppush.h"

/*
 * yppush: push a new YP map out YP servers
 *
 * usage:
 *   yppush [-d domain] [-h host] [-v] mapname
 *
 *   -d: the domainname the map lives in [if different from default]
 *   -h: push only to this host [otherwise, push to all hosts]
 *   -v: verbose
 */

/*
 * structures
 */

struct yppush_info {
	char   *ourdomain;	/* domain of interest */
	char   *map;		/* map we are pushing */
	char   *owner;		/* owner of map */
	int     order;		/* order number of map (version) */
};
/*
 * global vars
 */

extern char *__progname;	/* from crt0.o */
int     verbo = 0;		/* verbose */

/*
 * prototypes
 */

int	main __P((int, char *[]));
int	pushit __P((int, char *, int, char *, int, char *));
void	push __P((char *, int, struct yppush_info *));
void	_svc_run __P((void));
void	usage __P((void));


/*
 * main
 */

int
main(argc, argv)
	int     argc;
	char   *argv[];

{
	char   *targhost = NULL;
	struct yppush_info ypi = {NULL, NULL, NULL, 0};
	int     c, rv;
	const char *cp;
	char   *master;
	DBM    *ypdb;
	datum   datum;
	CLIENT *ypserv;
	struct timeval tv;
	enum clnt_stat retval;
	struct ypall_callback ypallcb;

	/*
         * parse command line
         */
	while ((c = getopt(argc, argv, "d:h:v")) != -1) {
		switch (c) {
		case 'd':
			ypi.ourdomain = optarg;
			break;
		case 'h':
			targhost = optarg;
			break;
		case 'v':
			verbo = 1;
			break;
		default:
			usage();
			/* NOTREACHED */
		}
	}
	argc -= optind;
	argv += optind;
	if (argc != 1)
		usage();
	ypi.map = argv[0];
	if (strlen(ypi.map) > YPMAXMAP)
		errx(1, "%s: map name too long (limit %d)", ypi.map, YPMAXMAP);

	/*
         * ensure we have a domain
         */
	if (ypi.ourdomain == NULL) {
		c = yp_get_default_domain(&ypi.ourdomain);
		if (ypi.ourdomain == NULL)
			errx(1, "unable to get default domain: %s",
			    yperr_string(c));
	}
	/*
         * verify that the domain and specified database exsists
         *
         * XXXCDC: this effectively prevents us from pushing from any
         * host but the master.   an alternate plan is to find the master
         * host for a map, clear it, ask for the order number, and then
         * send xfr requests.   if that was used we would not need local
         * file access.
         */
	if (chdir(YP_DB_PATH) < 0)
		err(1, "%s", YP_DB_PATH);
	if (chdir(ypi.ourdomain) < 0)
		err(1, "%s/%s", YP_DB_PATH, ypi.ourdomain);

	/*
         * now open the database so we can extract "order number"
         * (i.e. timestamp) of the map.
         */
	ypdb = ypdb_open(ypi.map, 0, O_RDONLY);
	if (ypdb == NULL)
		err(1, "ypdb_open %s/%s/%s", YP_DB_PATH, ypi.ourdomain,
		    ypi.map);
	datum.dptr = YP_LAST_KEY;
	datum.dsize = YP_LAST_LEN;
	datum = ypdb_fetch(ypdb, datum);
	ypdb_close(ypdb);
	if (datum.dptr == NULL)
		errx(1,
		    "unable to fetch %s key: check database with 'makedbm -u'",
		    YP_LAST_KEY);
	ypi.order = 0;
	cp = datum.dptr;
	while (cp < datum.dptr + datum.dsize) {
		if (!isdigit(*cp))
			errx(1,
		    "invalid order number: check database with 'makedbm -u'");
		ypi.order = (ypi.order * 10) + *cp - '0';
		cp++;
	}

	if (verbo)
		printf("pushing %s [order=%d] in domain %s\n", ypi.map,
		    ypi.order, ypi.ourdomain);

	/*
         * ok, we are ready to do it.   first we send a clear_2 request
         * to the local server [should be the master] to make sure it has
         * the correct database open.
         *
         * XXXCDC: note that yp_bind_local exits on failure so ypserv can't
         * be null.   this makes it difficult to print a useful error message.
         * [it will print "clntudp_create: no contact with localhost"]
         */
	tv.tv_sec = 10;
	tv.tv_usec = 0;
	ypserv = yp_bind_local(YPPROG, YPVERS);
	retval = clnt_call(ypserv, YPPROC_CLEAR, xdr_void, 0, xdr_void, 0, tv);
	if (retval != RPC_SUCCESS)
		errx(1, "clnt_call CLEAR to local ypserv: %s",
		    clnt_sperrno(retval));
	clnt_destroy(ypserv);

	/*
         * now use normal yplib functions to bind to the domain.
         */
	rv = yp_bind(ypi.ourdomain);
	if (rv)
		errx(1, "error binding to %s: %s", ypi.ourdomain,
		    yperr_string(rv));

	/*
         * find 'owner' of the map (see pushit for usage)
         */
	rv = yp_master(ypi.ourdomain, ypi.map, &ypi.owner);
	if (rv)
		errx(1, "error finding master for %s in %s: %s", ypi.map,
		    ypi.ourdomain, yperr_string(rv));

	/*
         * inform user of our progress
         */
	if (verbo) {
		printf("pushing map %s in %s: order=%d, owner=%s\n", ypi.map,
		    ypi.ourdomain, ypi.order, ypi.owner);
		printf("pushing to %s\n",
		    (targhost) ? targhost : "<all ypservs>");
	}

	/*
         * finally, do it.
         */
	if (targhost) {
		push(targhost, strlen(targhost), &ypi);
	} else {

		/*
	         * no host specified, do all hosts the master knows about via
	         * the ypservers map.
	         */
		rv = yp_master(ypi.ourdomain, "ypservers", &master);
		if (rv)
			errx(1, "error finding master for ypservers in %s: %s",
			    ypi.ourdomain, yperr_string(rv));

		if (verbo)
			printf(
		"contacting ypservers %s master on %s for list of ypservs...\n",
			    ypi.ourdomain, master);

		ypserv = yp_bind_host(master, YPPROG, YPVERS, 0, 1);

		ypallcb.foreach = pushit;	/* callback function */
		ypallcb.data = (char *) &ypi;	/* data to pass into callback */

		rv = yp_all_host(ypserv, ypi.ourdomain, "ypservers", &ypallcb);
		if (rv)
			errx(1, "pushing %s in %s failed: %s", ypi.map,
			    ypi.ourdomain, yperr_string(rv));
	}
	exit(0);
}

/*
 * usage: print usage and exit
 */
void
usage()
{
	fprintf(stderr, "usage: %s [-d domain] [-h host] [-v] map\n",
	    __progname);
	exit(1);
}

/*
 * pushit: called from yp_all_host to push a specific host.
 * the key/value pairs are from the ypservers map.
 */
int
pushit(instatus, inkey, inkeylen, inval, invallen, indata)
	int     instatus, inkeylen, invallen;
	char   *inkey, *inval, *indata;
{
	struct yppush_info *ypi = (struct yppush_info *) indata;

	if (instatus != YP_TRUE)		/* failure? */
		return (instatus);

	push(inkey, inkeylen, ypi);		/* do it! */
	return (0);
}

/*
 * push: push a specific map on a specific host
 */
void
push(host, hostlen, ypi)
	char   *host;
	int     hostlen;
	struct yppush_info *ypi;
{
	char    target[YPMAXPEER];
	CLIENT *ypserv;
	SVCXPRT *transp;
	int     prog, pid, rv;
	struct timeval tv;
	struct ypreq_xfr req;

	/*
         * get our target host in a null terminated string
         */
	snprintf(target, sizeof(target), "%*.*s", hostlen, hostlen, host);

	/*
         * XXXCDC: arg!  we would like to use yp_bind_host here, except that
         * it exits on failure and we don't want to give up just because
         * one host fails.  thus, we have to do it the hard way.
         */
	ypserv = clnt_create(target, YPPROG, YPVERS, "tcp");
	if (ypserv == NULL) {
		clnt_pcreateerror(target);
		return;
	}

	/*
         * our XFR rpc request to the client just starts the transfer.
         * when the client is done, it wants to call a procedure that
         * we are serving to tell us that it is done.   so we must create
         * and register a procedure for us for it to call.
         */
	transp = svcudp_create(RPC_ANYSOCK);
	if (transp == NULL) {
		warnx("callback svcudp_create failed");
		goto error;
	}

	/* register it with portmap */
	for (prog = 0x40000000; prog < 0x5fffffff; prog++) {
		if (svc_register(transp, prog, 1, yppush_xfrrespprog_1,
		    IPPROTO_UDP))
			break;
	}
	if (prog >= 0x5fffffff) {
		warnx("unable to register callback");
		goto error;
	}

	/*
         * now fork off a server to catch our reply
         */
	pid = fork();
	if (pid == -1) {
		svc_unregister(prog, 1);	/* drop our mapping with
						 * portmap */
		warn("fork failed");
		goto error;
	}

	/*
         * child process becomes the server
         */
	if (pid == 0) {
		_svc_run();
		exit(0);
	}

	/*
         * we are the parent process: send XFR request to server.
         * the "owner" field isn't used by ypserv (and shouldn't be, since
         * the ypserv has no idea if we are a legitimate yppush or not).
         * instead, the owner of the map is determined by the master value
         * currently cached on the slave server.
         */
	close(transp->xp_sock);	/* close child's socket, we don't need it */
	/* don't wait for anything here, we will wait for child's exit */
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	req.map_parms.domain = ypi->ourdomain;
	req.map_parms.map = ypi->map;
	req.map_parms.owner = ypi->owner;	/* NOT USED */
	req.map_parms.ordernum = ypi->order;
	req.transid = (u_int) pid;
	req.proto = prog;
	req.port = transp->xp_port;

	if (verbo)
		printf("asking host %s to transfer map (xid=%d)\n", target,
		    req.transid);

	rv = clnt_call(ypserv, YPPROC_XFR, xdr_ypreq_xfr, &req,
	    		xdr_void, NULL, tv);			/* do it! */

	if (rv != RPC_SUCCESS && rv != RPC_TIMEDOUT) {
		warnx("unable to xfr to host %s: %s", target, clnt_sperrno(rv));
		kill(pid, SIGTERM);
	}

	/*
         * now wait for child to get the reply and exit
         */
	wait4(pid, NULL, 0, NULL);
	svc_unregister(prog, 1);

	/*
         * ... and we are done.   fall through
         */

error:
	if (transp)
		svc_destroy(transp);
	clnt_destroy(ypserv);
	return;
}

/*
 * _svc_run: this is the main loop for the RPC server that we fork off
 * to await the reply from ypxfr.
 */
void
_svc_run()
{
	fd_set  readfds;
	struct timeval tv;
	int     rv, nfds;

	nfds = sysconf(_SC_OPEN_MAX);
	while (1) {

		readfds = svc_fdset;	/* structure copy from global var */
		tv.tv_sec = 60;
		tv.tv_usec = 0;

		rv = select(nfds, &readfds, NULL, NULL, &tv);

		if (rv < 0) {
			if (errno == EINTR)
				continue;
			warn("_svc_run: select failed");
			return;
		}
		if (rv == 0)
			errx(0, "_svc_run: callback timed out");

		/*
	         * got something
	         */
		svc_getreqset(&readfds);

	}
}
