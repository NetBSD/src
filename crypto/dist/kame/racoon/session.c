/*	$KAME: session.c,v 1.32 2003/09/24 02:01:17 jinmei Exp $	*/

/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
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
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: session.c,v 1.4 2004/04/12 03:34:07 itojun Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/socket.h>
#if HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif
#ifndef WEXITSTATUS
# define WEXITSTATUS(s)	((unsigned)(s) >> 8)
#endif
#ifndef WIFEXITED
# define WIFEXITED(s)	(((s) & 255) == 0)
#endif

#ifdef IPV6_INRIA_VERSION
#include <netinet/ipsec.h>
#else
#include <netinet6/ipsec.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <signal.h>

#include "libpfkey.h"

#include "var.h"
#include "misc.h"
#include "vmbuf.h"
#include "plog.h"
#include "debug.h"

#include "schedule.h"
#include "session.h"
#include "grabmyaddr.h"
#include "cfparse_proto.h"
#include "isakmp_var.h"
#include "admin_var.h"
#include "oakley.h"
#include "pfkey.h"
#include "handler.h"
#include "localconf.h"
#include "remoteconf.h"
#include "backupsa.h"

static void close_session __P((void));
static void check_rtsock __P((void *));
static void initfds __P((void));
static void init_signal __P((void));
static int set_signal __P((int sig, RETSIGTYPE (*func) __P((int))));
static void check_sigreq __P((void));
static void check_flushsa_stub __P((void *));
static void check_flushsa __P((void));
static int close_sockets __P((void));

static fd_set mask0;
static int nfds = 0;
static int sigreq = 0;

int
session(void)
{
	fd_set rfds;
	struct timeval *timeout;
	int error;
	struct myaddrs *p;

	/* initialize schedular */
	sched_init();

	init_signal();

#ifdef ENABLE_ADMINPORT
	/* debug port has no authentication, do not open it */
	if (admin_init() < 0)
		exit(1);
#endif

	initmyaddr();

	if (isakmp_init() < 0)
		exit(1);

	initfds();

	sigreq = 0;
	while (1) {
		rfds = mask0;

		/*
		 * asynchronous requests via signal.
		 * make sure to reset sigreq to 0.
		 */
		check_sigreq();

		/* scheduling */
		timeout = schedular();

		error = select(nfds, &rfds, (fd_set *)0, (fd_set *)0, timeout);
		if (error < 0) {
			switch (errno) {
			case EINTR:
				continue;
			default:
				plog(LLV_ERROR, LOCATION, NULL,
					"failed to select (%s)\n",
					strerror(errno));
				return -1;
			}
			/*NOTREACHED*/
		}

#ifdef ENABLE_ADMINPORT
		if (FD_ISSET(lcconf->sock_admin, &rfds))
			admin_handler();
#endif

		for (p = lcconf->myaddrs; p; p = p->next) {
			if (!p->addr)
				continue;
			if (FD_ISSET(p->sock, &rfds))
				isakmp_handler(p->sock);
		}

		if (FD_ISSET(lcconf->sock_pfkey, &rfds))
			pfkey_handler();

		if (lcconf->rtsock >= 0 && FD_ISSET(lcconf->rtsock, &rfds)) {
			if (update_myaddrs() && lcconf->autograbaddr)
				sched_new(5, check_rtsock, NULL);
			initfds();
		}
	}
}

/* clear all status and exit program. */
static void
close_session()
{
	flushph1();
	close_sockets();
	backupsa_clean();

	plog(LLV_INFO, LOCATION, NULL, "racoon shutdown\n");
	exit(0);
}

static void
check_rtsock(p)
	void *p;
{
	isakmp_close();
	grab_myaddrs();
	autoconf_myaddrsport();
	isakmp_open();

	/* initialize socket list again */
	initfds();
}

static void
initfds()
{
	struct myaddrs *p;

	nfds = 0;

	FD_ZERO(&mask0);

#ifdef ENABLE_ADMINPORT
	if (lcconf->sock_admin >= FD_SETSIZE) {
		plog(LLV_ERROR, LOCATION, NULL, "fd_set overrun\n");
		exit(1);
	}
	FD_SET(lcconf->sock_admin, &mask0);
	nfds = (nfds > lcconf->sock_admin ? nfds : lcconf->sock_admin);
#endif
	if (lcconf->sock_pfkey >= FD_SETSIZE) {
		plog(LLV_ERROR, LOCATION, NULL, "fd_set overrun\n");
		exit(1);
	}
	FD_SET(lcconf->sock_pfkey, &mask0);
	nfds = (nfds > lcconf->sock_pfkey ? nfds : lcconf->sock_pfkey);
	if (lcconf->rtsock >= 0) {
		if (lcconf->rtsock >= FD_SETSIZE) {
			plog(LLV_ERROR, LOCATION, NULL, "fd_set overrun\n");
			exit(1);
		}
		FD_SET(lcconf->rtsock, &mask0);
		nfds = (nfds > lcconf->rtsock ? nfds : lcconf->rtsock);
	}

	for (p = lcconf->myaddrs; p; p = p->next) {
		if (!p->addr)
			continue;
		if (p->sock >= FD_SETSIZE) {
			plog(LLV_ERROR, LOCATION, NULL, "fd_set overrun\n");
			exit(1);
		}
		FD_SET(p->sock, &mask0);
		nfds = (nfds > p->sock ? nfds : p->sock);
	}
	nfds++;
}

static int signals[] = {
	SIGHUP,
	SIGINT,
	SIGTERM,
	SIGUSR1,
	SIGUSR2,
	SIGCHLD,
	0
};

/*
 * asynchronous requests will actually dispatched in the
 * main loop in session().
 */
RETSIGTYPE
signal_handler(sig)
	int sig;
{
	switch (sig) {
	case SIGCHLD:
	    {
		pid_t pid;
		int s;

		pid = wait(&s);
	    }
		break;

#ifdef DEBUG_RECORD_MALLOCATION
	case SIGUSR2:
		DRM_dump();
		break;
#endif
	default:
		/* XXX should be blocked any signal ? */
		sigreq = sig;
		break;
	}
}

static void
check_sigreq()
{
	switch (sigreq) {
	case 0:
		return;

	case SIGHUP:
		if (cfreparse()) {
			plog(LLV_ERROR, LOCATION, NULL,
				"configuration read failed\n");
			exit(1);
		}
		sigreq = 0;
		break;

	default:
		plog(LLV_INFO, LOCATION, NULL, "caught signal %d\n", sigreq);
		pfkey_send_flush(lcconf->sock_pfkey, SADB_SATYPE_UNSPEC);
		sched_new(1, check_flushsa_stub, NULL);
		sigreq = 0;
		break;
	}
}

/*
 * waiting the termination of processing until sending DELETE message
 * for all inbound SA will complete.
 */
static void
check_flushsa_stub(p)
	void *p;
{

	check_flushsa();
}

static void
check_flushsa()
{
	vchar_t *buf;
	struct sadb_msg *msg, *end, *next;
	struct sadb_sa *sa;
	caddr_t mhp[SADB_EXT_MAX + 1];
	int n;

	buf = pfkey_dump_sadb(SADB_SATYPE_UNSPEC);
	if (buf == NULL) {
		plog(LLV_DEBUG, LOCATION, NULL,
		    "pfkey_dump_sadb: returned nothing.\n");
		return;
	}

	msg = (struct sadb_msg *)buf->v;
	end = (struct sadb_msg *)(buf->v + buf->l);

	/* counting SA except of dead one. */
	n = 0;
	while (msg < end) {
		if (PFKEY_UNUNIT64(msg->sadb_msg_len) < sizeof(*msg))
			break;
		next = (struct sadb_msg *)((caddr_t)msg + PFKEY_UNUNIT64(msg->sadb_msg_len));
		if (msg->sadb_msg_type != SADB_DUMP) {
			msg = next;
			continue;
		}

		if (pfkey_align(msg, mhp) || pfkey_check(mhp)) {
			plog(LLV_ERROR, LOCATION, NULL,
				"pfkey_check (%s)\n", ipsec_strerror());
			msg = next;
			continue;
		}

		sa = (struct sadb_sa *)(mhp[SADB_EXT_SA]);
		if (!sa) {
			msg = next;
			continue;
		}

		if (sa->sadb_sa_state != SADB_SASTATE_DEAD) {
			n++;
			msg = next;
			continue;
		}

		msg = next;
	}

	if (n) {
		sched_new(1, check_flushsa_stub, NULL);
		return;
	}

	close_session();
}

static void
init_signal()
{
	int i;

	for (i = 0; signals[i] != 0; i++)
		if (set_signal(signals[i], signal_handler) < 0) {
			plog(LLV_ERROR, LOCATION, NULL,
				"failed to set_signal (%s)\n",
				strerror(errno));
			exit(1);
		}
}

static int
set_signal(sig, func)
	int sig;
	RETSIGTYPE (*func) __P((int));
{
	struct sigaction sa;

	memset((caddr_t)&sa, 0, sizeof(sa));
	sa.sa_handler = func;
	sa.sa_flags = SA_RESTART;

	if (sigemptyset(&sa.sa_mask) < 0)
		return -1;

	if (sigaction(sig, &sa, (struct sigaction *)0) < 0)
		return(-1);

	return 0;
}

static int
close_sockets()
{
	isakmp_close();
	pfkey_close(lcconf->sock_pfkey);
#ifdef ENABLE_ADMINPORT
	(void)admin_close();
#endif
	return 0;
}

