/*	$NetBSD: rusersd.c,v 1.12.8.1 2000/06/22 15:58:35 minoura Exp $	*/

/*-
 *  Copyright (c) 1993 John Brezak
 *  All rights reserved.
 * 
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. The name of the author may not be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR `AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: rusersd.c,v 1.12.8.1 2000/06/22 15:58:35 minoura Exp $");
#endif /* not lint */

#include <stdio.h>
#include <rpc/rpc.h>
#include <sys/socket.h>
#include <signal.h>
#include <syslog.h>
#include <stdlib.h>
#include <rpcsvc/rusers.h>	/* New version */
#include <rpcsvc/rnusers.h>	/* Old version */

#include "rusers_proc.h"


int from_inetd = 1;

static void cleanup(int);
int main(int, char *[]);

static void
cleanup(int n)
{

	(void) rpcb_unset(RUSERSPROG, RUSERSVERS_3, NULL);
	(void) rpcb_unset(RUSERSPROG, RUSERSVERS_IDLE, NULL);
	exit(0);
}

int
main(int argc, char *argv[])
{
	SVCXPRT *transp;
	struct sockaddr_storage from;
	int fromlen;

	/*
	 * See if inetd started us
	 */
	fromlen = sizeof(from);
	if (getsockname(0, (struct sockaddr *)&from, &fromlen) < 0)
		from_inetd = 0;
	
	if (!from_inetd) {
		daemon(0, 0);

		(void) rpcb_unset(RUSERSPROG, RUSERSVERS_3, NULL);
		(void) rpcb_unset(RUSERSPROG, RUSERSVERS_IDLE, NULL);

		(void) signal(SIGINT, cleanup);
		(void) signal(SIGTERM, cleanup);
		(void) signal(SIGHUP, cleanup);
	}

	openlog("rpc.rusersd", LOG_PID, LOG_DAEMON);

	if (from_inetd) {
		transp = svc_dg_create(0, 0, 0);
		if (transp == NULL) {
			syslog(LOG_ERR, "cannot create udp service.");
			exit(1);
		}
		if (!svc_reg(transp, RUSERSPROG, RUSERSVERS_3, rusers_service,
		    NULL)) {
			syslog(LOG_ERR, "unable to register "
			    "(RUSERSPROG, RUSERSVERS_3).");
			exit(1);
		}
		if (!svc_reg(transp, RUSERSPROG, RUSERSVERS_IDLE,
		    rusers_service, NULL)) {
			syslog(LOG_ERR, "unable to register "
			    "(RUSERSPROG, RUSERSVERS_IDLE).");
			exit(1);
		}
	} else {
		if (!svc_create(rusers_service, RUSERSPROG, RUSERSVERS_3,
		    "udp")) {
			syslog(LOG_ERR, "unable to create "
			    "(RUSERSPROG, RUSERSVERS_3).");
			exit(1);
		}
		if (!svc_create(rusers_service, RUSERSPROG, RUSERSVERS_IDLE,
		    "udp")) {
			syslog(LOG_ERR, "unable to create "
			    "(RUSERSPROG, RUSERSVERS_IDLE).");
			exit(1);
		}
	}

	svc_run();
	syslog(LOG_ERR, "svc_run returned");
	exit(1);
}
