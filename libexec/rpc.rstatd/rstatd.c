/*	$NetBSD: rstatd.c,v 1.14 2006/05/09 20:18:07 mrg Exp $	*/

/*-
 * Copyright (c) 1993, John Brezak
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: rstatd.c,v 1.14 2006/05/09 20:18:07 mrg Exp $");
#endif /* not lint */

#include <sys/types.h>
#include <sys/socket.h>

#include <stdio.h>
#include <rpc/rpc.h>
#include <signal.h>
#include <syslog.h>
#include <string.h>
#include <stdlib.h>
#include <rpcsvc/rstat.h>

extern void rstat_service(struct svc_req *, SVCXPRT *);
void cleanup(int);
int main(int, char *[]);

int from_inetd = 1;     /* started from inetd ? */
int closedown = 20;	/* how long to wait before going dormant */

void
cleanup(int dummy)
{
        (void) rpcb_unset(RSTATPROG, RSTATVERS_TIME, NULL);
        (void) rpcb_unset(RSTATPROG, RSTATVERS_SWTCH, NULL);
        (void) rpcb_unset(RSTATPROG, RSTATVERS_ORIG, NULL);
        exit(0);
}

int
main(int argc, char *argv[])
{
	SVCXPRT *transp;
	struct sockaddr_storage from;
	socklen_t fromlen;
        
        if (argc == 2)
                closedown = atoi(argv[1]);
        if (closedown <= 0)
                closedown = 20;

        /*
         * See if inetd started us
         */
	fromlen = sizeof(from);
        if (getsockname(0, (struct sockaddr *)&from, &fromlen) < 0)
                from_inetd = 0;

        if (!from_inetd) {
                /* daemon(0, 0); */

                (void)rpcb_unset(RSTATPROG, RSTATVERS_TIME, NULL);
                (void)rpcb_unset(RSTATPROG, RSTATVERS_SWTCH, NULL);
                (void)rpcb_unset(RSTATPROG, RSTATVERS_ORIG, NULL);

		(void) signal(SIGINT, cleanup);
		(void) signal(SIGTERM, cleanup);
		(void) signal(SIGHUP, cleanup);
        }
        
        openlog("rpc.rstatd", LOG_PID, LOG_DAEMON);

	if (from_inetd) {
		transp = svc_dg_create(0, 0, 0);
		if (transp == NULL) {
			syslog(LOG_ERR, "cannot create udp service.");
			exit(1);
		}

		if (!svc_reg(transp, RSTATPROG, RSTATVERS_TIME, rstat_service,
		    NULL)) {
			syslog(LOG_ERR, "unable to register (RSTATPROG,"
			    "RSTATVERS_TIME)");
			exit(1);
		}

		if (!svc_reg(transp, RSTATPROG, RSTATVERS_SWTCH, rstat_service,
		    NULL)) {
			syslog(LOG_ERR, "unable to register (RSTATPROG,"
			    "RSTATVERS_TIME)");
			exit(1);
		}

		if (!svc_reg(transp, RSTATPROG, RSTATVERS_ORIG, rstat_service,
		    NULL)) {
			syslog(LOG_ERR, "unable to register (RSTATPROG,"
			    "RSTATVERS_ORIG)");
			exit(1);
		}
	} else {
		if (!svc_create(rstat_service, RSTATPROG, RSTATVERS_TIME,
		    "udp")) {
			syslog(LOG_ERR,
			    "unable to create (RSTATPROG, RSTATVERS_TIME).");
			exit(1);
		}
		if (!svc_create(rstat_service, RSTATPROG, RSTATVERS_SWTCH,
		    "udp")) {
			syslog(LOG_ERR,
			    "unable to create (RSTATPROG, RSTATVERS_SWTCH).");
			exit(1);
		}
		if (!svc_create(rstat_service, RSTATPROG, RSTATVERS_ORIG,
		    "udp")) {
			syslog(LOG_ERR,
			    "unable to register (RSTATPROG, RSTATVERS_ORIG).");
			exit(1);
		}
	}

        svc_run();
	syslog(LOG_ERR, "svc_run returned");
	exit(1);
}
