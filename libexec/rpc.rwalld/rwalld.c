/* $NetBSD: rwalld.c,v 1.18 2000/06/14 17:25:18 cgd Exp $ */

/*
 * Copyright (c) 1993 Christopher G. Demetriou
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
 *          This product includes software developed for the
 *          NetBSD Project.  See http://www.netbsd.org/ for
 *          information about NetBSD.
 * 4. The name of the author may not be used to endorse or promote products
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
 * 
 * <<Id: LICENSE,v 1.2 2000/06/14 15:57:33 cgd Exp>>
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: rwalld.c,v 1.18 2000/06/14 17:25:18 cgd Exp $");
#endif /* not lint */

#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <pwd.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <errno.h>
#include <sys/socket.h>
#include <signal.h>
#include <sys/wait.h>
#include <rpc/rpc.h>
#include <rpcsvc/rwall.h>

#ifdef OSF
#define WALL_CMD "/usr/sbin/wall"
#else
#define WALL_CMD "/usr/bin/wall -n"
#endif

static int from_inetd = 1;

static void cleanup(int);
static void wallprog_1(struct svc_req *, SVCXPRT *);

int main(int, char *[]);

static void
cleanup(int n)
{

	(void)rpcb_unset(WALLPROG, WALLVERS, NULL);
	exit(0);
}

int
main(int argc, char *argv[])
{
	SVCXPRT *transp;
	struct sockaddr_storage from;
	int fromlen;

	if (geteuid() == 0) {
		struct passwd *pep = getpwnam("nobody");
		if (pep)
			setuid(pep->pw_uid);
		else
			setuid(getuid());
	}

	/*
	 * See if inetd started us
	 */
	fromlen = sizeof(from);
	if (getsockname(0, (struct sockaddr *)&from, &fromlen) < 0)
		from_inetd = 0;

	if (!from_inetd) {
		daemon(0, 0);

		(void) rpcb_unset(WALLPROG, WALLVERS, NULL);

		(void) signal(SIGINT, cleanup);
		(void) signal(SIGTERM, cleanup);
		(void) signal(SIGHUP, cleanup);
	}

	openlog("rpc.rwalld", LOG_PID, LOG_DAEMON);

	if (from_inetd) {
		transp = svc_dg_create(0, 0, 0);
		if (transp == NULL) {
			syslog(LOG_ERR, "cannot create udp service.");
			exit(1);
		}
		if (!svc_reg(transp, WALLPROG, WALLVERS, wallprog_1, NULL)) {
			syslog(LOG_ERR, "unable to register "
			    "(WALLPROG, WALLVERS).");
			exit(1);
		}
	} else {
		if (!svc_create(wallprog_1, WALLPROG, WALLVERS, "udp")) {
			syslog(LOG_ERR, "unable to create "
			    "(WALLPROG, WALLVERS.)");
			exit(1);
		}
	}

	svc_run();
	syslog(LOG_ERR, "svc_run returned");
	exit(1);

}

void *
wallproc_wall_1_svc(char **s, struct svc_req *rqstp)
{
	FILE *pfp;

	pfp = popen(WALL_CMD, "w");
	if (pfp != NULL) {
		fprintf(pfp, "\007\007%s", *s);
		pclose(pfp);
	}

	return (*s);
}

static void
wallprog_1(struct svc_req *rqstp, SVCXPRT *transp)
{
	union {
		char *wallproc_wall_1_arg;
	} argument;
	char *result;
	xdrproc_t xdr_argument, xdr_result;
	char *(*local) __P((char **, struct svc_req *));

	switch (rqstp->rq_proc) {
	case NULLPROC:
		(void)svc_sendreply(transp, xdr_void, (char *)NULL);
		goto leave;

	case WALLPROC_WALL:
		xdr_argument = (xdrproc_t)xdr_wrapstring;
		xdr_result = (xdrproc_t)xdr_void;
		local = (char *(*) __P((char **, struct svc_req *)))
			wallproc_wall_1_svc;
		break;

	default:
		svcerr_noproc(transp);
		goto leave;
	}
	memset((char *)&argument, 0, sizeof(argument));
	if (!svc_getargs(transp, xdr_argument, (caddr_t)&argument)) {
		svcerr_decode(transp);
		goto leave;
	}
	result = (*local)((char **)&argument, rqstp);
	if (result != NULL && !svc_sendreply(transp, xdr_result, result)) {
		svcerr_systemerr(transp);
	}
	if (!svc_freeargs(transp, xdr_argument, (caddr_t)&argument)) {
		syslog(LOG_ERR, "unable to free arguments");
		exit(1);
	}
leave:
	if (from_inetd)
		exit(0);
}
