/*	$NetBSD: rpc.yppasswdd.c,v 1.3 1999/06/06 02:44:52 thorpej Exp $	*/

/*
 * Copyright (c) 1994 Mats O Jansson <moj@stacken.kth.se>
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
#include <sys/wait.h>
#include <err.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <util.h>

#include <rpc/rpc.h>
#include <rpc/pmap_clnt.h>
#include <rpcsvc/yppasswd.h>

extern	char *__progname;		/* from crt0.s */

int	noshell, nogecos, nopw, domake;
char	make_arg[_POSIX2_LINE_MAX] = "make";

extern	void make_passwd __P((yppasswd *, struct svc_req *, SVCXPRT *));

int	main __P((int, char *[]));
void	yppasswddprog_1 __P((struct svc_req *, SVCXPRT *));
void	usage __P((void));

int
main(argc, argv)
	int     argc;
	char   *argv[];
{
	SVCXPRT *transp;
	int i;

	for (i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			if (strcmp("-noshell", argv[i]) == 0)
				noshell = 1;
			else if (strcmp("-nogecos", argv[i]) == 0)
				nogecos = 1;
			else if (strcmp("-nopw", argv[i]) == 0)
				nopw = 1;
			else if (strcmp("-m", argv[i]) == 0) {
				domake = 1;
				for (; i < argc; i++) {
					strcat(make_arg, " ");
					strcat(make_arg, argv[i]);
				}
			} else
				usage();
		} else
			usage();
	}

	if (daemon(0, 0))
		err(1, "can't detach");
	pidfile(NULL);

	(void) pmap_unset(YPPASSWDPROG, YPPASSWDVERS);

	transp = svcudp_create(RPC_ANYSOCK);
	if (transp == NULL)
		errx(1, "cannot create UDP service");

	if (!svc_register(transp, YPPASSWDPROG, YPPASSWDVERS, yppasswddprog_1,
	    IPPROTO_UDP))
		errx(1, "unable to register YPPASSWDPROG/YPPASSWDVERS/UDP");

	transp = svctcp_create(RPC_ANYSOCK, 0, 0);
	if (transp == NULL)
		errx(1, "cannot create TCP service");

	if (!svc_register(transp, YPPASSWDPROG, YPPASSWDVERS, yppasswddprog_1,
	    IPPROTO_TCP))
		errx(1, "unable to register YPPASSWDPROG/YPPASSWDVERS/TCP");

	svc_run();
	errx(1, "svc_run returned");
}

void
yppasswddprog_1(rqstp, transp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	union {
		yppasswd yppasswdproc_update_1_arg;
	} argument;

	switch (rqstp->rq_proc) {
	case NULLPROC:
		(void) svc_sendreply(transp, xdr_void, (char *) NULL);
		return;

	case YPPASSWDPROC_UPDATE:
		/*
		 * We'd like this to look like a regular RPC
		 * stub, but we have to send the reply in the
		 * handler in order to avoid both signal race
		 * conditions locally and timeouts on the
		 * client.
		 */
		(void) memset(&argument, 0, sizeof(argument));
		if (!svc_getargs(transp, xdr_yppasswd, (caddr_t) & argument)) {
			svcerr_decode(transp);
			return;
		}
		make_passwd((yppasswd *)&argument, rqstp, transp);
		if (!svc_freeargs(transp, xdr_yppasswd, (caddr_t) &argument))
			errx(1, "unable to free arguments");
		return;
	}

	svcerr_noproc(transp);
}

void
usage()
{

	fprintf(stderr, "usage: %s [-noshell] [-nogecos] [-nopw] "
	    "[-m arg1 arg2 ...]\n", __progname);
	exit(1);
}
