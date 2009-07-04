/*	$NetBSD: ctl_transact.c,v 1.7 2009/07/04 01:01:18 dholland Exp $	*/
/*
 * Copyright (c) 1983-2003, Regents of the University of California.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are 
 * met:
 * 
 * + Redistributions of source code must retain the above copyright 
 *   notice, this list of conditions and the following disclaimer.
 * + Redistributions in binary form must reproduce the above copyright 
 *   notice, this list of conditions and the following disclaimer in the 
 *   documentation and/or other materials provided with the distribution.
 * + Neither the name of the University of California, San Francisco nor 
 *   the names of its contributors may be used to endorse or promote 
 *   products derived from this software without specific prior written 
 *   permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS 
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED 
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A 
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "bsd.h"

#if	defined(TALK_43) || defined(TALK_42)

#include <sys/cdefs.h>
#ifndef lint
#if 0
static char sccsid[] = "@(#)ctl_transact.c	5.2 (Berkeley) 3/13/86";
#else
__RCSID("$NetBSD: ctl_transact.c,v 1.7 2009/07/04 01:01:18 dholland Exp $");
#endif
#endif /* not lint */

#include <sys/time.h>
#include <unistd.h>
#include "hunt.h"
#include "talk_ctl.h"

#define CTL_WAIT 2	/* time to wait for a response, in seconds */
#define MAX_RETRY 5

/*
 * SOCKDGRAM is unreliable, so we must repeat messages if we have
 * not received an acknowledgement within a reasonable amount
 * of time
 */
void
ctl_transact(target, msg, type, rp)
	struct in_addr target;
	CTL_MSG msg;
	int type;
	CTL_RESPONSE *rp;
{
	struct pollfd set[1];
	int nready, cc, retries;

	nready = 0;
	msg.type = type;
	daemon_addr.sin_addr = target;
	daemon_addr.sin_port = daemon_port;
	set[0].fd = ctl_sockt;
	set[0].events = POLLIN;

	/*
	 * Keep sending the message until a response of
	 * the proper type is obtained.
	 */
	do {
		/* resend message until a response is obtained */
		for (retries = MAX_RETRY; retries > 0; retries -= 1) {
			cc = sendto(ctl_sockt, &msg, sizeof (msg), 0,
				&daemon_addr, sizeof (daemon_addr));
			if (cc != sizeof (msg)) {
				if (errno == EINTR)
					continue;
				p_error("Error on write to talk daemon");
			}
			nready = poll(set, 1, CTL_WAIT * 1000);
			if (nready < 0) {
				if (errno == EINTR)
					continue;
				p_error("Error waiting for daemon response");
			}
			if (nready != 0)
				break;
		}
		if (retries <= 0)
			break;
		/*
		 * Keep reading while there are queued messages 
		 * (this is not necessary, it just saves extra
		 * request/acknowledgements being sent)
		 */
		do {
			cc = recv(ctl_sockt, rp, sizeof (*rp), 0);
			if (cc < 0) {
				if (errno == EINTR)
					continue;
				p_error("Error on read from talk daemon");
			}
			/* an immediate poll */
			nready = poll(set, 1, 0);
		} while (nready > 0 && (
#ifdef	TALK_43
		    rp->vers != TALK_VERSION ||
#endif
		    rp->type != type));
	} while (
#ifdef	TALK_43
	    rp->vers != TALK_VERSION ||
#endif
	    rp->type != type);
	rp->id_num = ntohl(rp->id_num);
#ifdef	TALK_43
	rp->addr.sa_family = ntohs(rp->addr.sa_family);
# else
	rp->addr.sin_family = ntohs(rp->addr.sin_family);
# endif
}
#endif
