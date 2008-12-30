/*	$NetBSD: session.c,v 1.22 2008/12/30 15:50:25 tteras Exp $	*/

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

#include "config.h"

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

#include PATH_IPSEC_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <signal.h>
#include <sys/stat.h>
#include <paths.h>
#include <err.h>

#include <netinet/in.h>
#include <resolv.h>

#include "libpfkey.h"

#include "var.h"
#include "misc.h"
#include "vmbuf.h"
#include "plog.h"
#include "debug.h"

#include "schedule.h"
#include "session.h"
#include "grabmyaddr.h"
#include "evt.h"
#include "cfparse_proto.h"
#include "isakmp_var.h"
#include "isakmp.h"
#include "isakmp_var.h"
#include "isakmp_xauth.h"
#include "isakmp_cfg.h"
#include "admin_var.h"
#include "admin.h"
#include "privsep.h"
#include "oakley.h"
#include "pfkey.h"
#include "handler.h"
#include "localconf.h"
#include "remoteconf.h"
#include "backupsa.h"
#include "remoteconf.h"
#ifdef ENABLE_NATT
#include "nattraversal.h"
#endif

#include "algorithm.h" /* XXX ??? */

#include "sainfo.h"

struct fd_monitor {
	int (*callback)(void *ctx, int fd);
	void *ctx;
};

static void close_session __P((void));
static void initfds __P((void));
static void init_signal __P((void));
static int set_signal __P((int sig, RETSIGTYPE (*func) __P((int))));
static void check_sigreq __P((void));
static void check_flushsa_stub __P((struct sched *));
static void check_flushsa __P((void));
static int close_sockets __P((void));

static fd_set mask0;
static fd_set maskdying;
static struct fd_monitor fd_monitors[FD_SETSIZE];
static int nfds = 0;

static volatile sig_atomic_t sigreq[NSIG + 1];
static int dying = 0;
static struct sched scflushsa = SCHED_INITIALIZER();

void
monitor_fd(int fd, int when_dying, int (*callback)(void *, int), void *ctx)
{
	if (fd < 0 || fd >= FD_SETSIZE) {
		plog(LLV_ERROR, LOCATION, NULL, "fd_set overrun");
		exit(1);
	}

	FD_SET(fd, &mask0);
	if (when_dying)
		FD_SET(fd, &maskdying);

	if (fd > nfds)
		nfds = fd;

	fd_monitors[fd].callback = callback;
	fd_monitors[fd].ctx = ctx;
}

void
unmonitor_fd(int fd)
{
	if (fd < 0 || fd >= FD_SETSIZE) {
		plog(LLV_ERROR, LOCATION, NULL, "fd_set overrun");
		exit(1);
	}

	FD_CLR(fd, &mask0);
	FD_CLR(fd, &maskdying);
	fd_monitors[fd].callback = NULL;
	fd_monitors[fd].ctx = NULL;
}

int
session(void)
{
	fd_set rfds;
	struct timeval *timeout;
	int error;
	char pid_file[MAXPATHLEN];
	FILE *fp;
	pid_t racoon_pid = 0;
	int i;

	nfds = 0;
	FD_ZERO(&mask0);
	FD_ZERO(&maskdying);

	/* initialize schedular */
	sched_init();
	init_signal();

	if (pfkey_init() < 0)
		errx(1, "failed to initialize pfkey socket");

#ifdef ENABLE_ADMINPORT
	if (admin_init() < 0)
		errx(1, "failed to initialize admin port socket");
#endif

	if (isakmp_init() < 0)
		errx(1, "failed to initialize ISAKMP structures");

#ifdef ENABLE_HYBRID
	if (isakmp_cfg_init(ISAKMP_CFG_INIT_COLD))
		errx(1, "could not initialize ISAKMP mode config structures");
#endif

#ifdef HAVE_LIBLDAP
	if (xauth_ldap_init_conf() != 0)
		errx(1, "could not initialize ldap config");
#endif

#ifdef HAVE_LIBRADIUS
	if (xauth_radius_init_conf(0) != 0)
		errx(1, "could not initialize radius config");
#endif

	/*
	 * in order to prefer the parameters by command line,
	 * saving some parameters before parsing configuration file.
	 */
	save_params();
	if (cfparse() != 0)
		errx(1, "failed to parse configuration file.");
	restore_params();

#ifdef ENABLE_HYBRID
	if(isakmp_cfg_config.network4 && isakmp_cfg_config.pool_size == 0)
		if ((error = isakmp_cfg_resize_pool(ISAKMP_CFG_MAX_CNX)) != 0)
			return error;
#endif

	if (dump_config)
		dumprmconf();

#ifdef HAVE_LIBRADIUS
	if (xauth_radius_init() != 0)
		errx(1, "could not initialize libradius");
#endif

	if (myaddr_init() != 0)
		errx(1, "failed to listen to configured addresses");
	myaddr_sync();

#ifdef ENABLE_NATT
	natt_keepalive_init ();
#endif

	/* write .pid file */
	if (lcconf->pathinfo[LC_PATHTYPE_PIDFILE] == NULL) 
		strlcpy(pid_file, _PATH_VARRUN "racoon.pid", MAXPATHLEN);
	else if (lcconf->pathinfo[LC_PATHTYPE_PIDFILE][0] == '/') 
		strlcpy(pid_file, lcconf->pathinfo[LC_PATHTYPE_PIDFILE], MAXPATHLEN);
	else {
		strlcat(pid_file, _PATH_VARRUN, MAXPATHLEN);
		strlcat(pid_file, lcconf->pathinfo[LC_PATHTYPE_PIDFILE], MAXPATHLEN);
	} 
	fp = fopen(pid_file, "w");
	if (fp) {
		if (fchmod(fileno(fp),
			S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) == -1) {
			syslog(LOG_ERR, "%s", strerror(errno));
			fclose(fp);
			exit(1);
		}
	} else {
		plog(LLV_ERROR, LOCATION, NULL,
			"cannot open %s", pid_file);
	}

	if (privsep_init() != 0)
		exit(1);

	/*
	 * The fork()'ed privileged side will close its copy of fp.  We wait
	 * until here to get the correct child pid.
	 */
	racoon_pid = getpid();
	fprintf(fp, "%ld\n", (long)racoon_pid);
	fclose(fp);

	for (i = 0; i <= NSIG; i++)
		sigreq[i] = 0;

	while (1) {
		/*
		 * asynchronous requests via signal.
		 * make sure to reset sigreq to 0.
		 */
		check_sigreq();

		/* scheduling */
		timeout = schedular();

		/* schedular can change select() mask, so we reset
		 * the working copy here */
		if (dying)
			rfds = maskdying;
		else
			rfds = mask0;

		error = select(nfds + 1, &rfds, NULL, NULL, timeout);
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

		for (i = 0; i <= nfds; i++) {
			if (FD_ISSET(i, &rfds))
				fd_monitors[i].callback(fd_monitors[i].ctx, i);
		}
	}
}

/* clear all status and exit program. */
static void
close_session()
{
#ifdef ENABLE_FASTQUIT
	flushph2();
#endif
	flushph1();
	close_sockets();
	backupsa_clean();

	plog(LLV_INFO, LOCATION, NULL, "racoon process %d shutdown\n", getpid());

	exit(0);
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
	/* Do not just set it to 1, because we may miss some signals by just setting
	 * values to 0/1
	 */
	sigreq[sig]++;
}


/* XXX possible mem leaks and no way to go back for now !!!
 */
static void reload_conf(){
	int error;

#ifdef ENABLE_HYBRID
	if ((isakmp_cfg_init(ISAKMP_CFG_INIT_WARM)) != 0) {
		plog(LLV_ERROR, LOCATION, NULL, 
		    "ISAKMP mode config structure reset failed, "
		    "not reloading\n");
		return;
	}
#endif

	save_sainfotree();

	/* TODO: save / restore / flush old lcconf (?) / rmtree
	 */
	save_rmconf();
	initrmconf();

#ifdef HAVE_LIBRADIUS
	/* free and init radius configuration */
	xauth_radius_init_conf(1);
#endif

	pfkey_reload();

	save_params();
	flushlcconf();
	error = cfparse();
	if (error != 0){
		plog(LLV_ERROR, LOCATION, NULL, "config reload failed\n");
		/* We are probably in an inconsistant state... */
		return;
	}
	restore_params();

#if 0	
	if (dump_config)
		dumprmconf ();
#endif

	myaddr_sync();

#ifdef HAVE_LIBRADIUS
	/* re-initialize radius state */
	xauth_radius_init();
#endif

	/* Revalidate ph1 / ph2tree !!!
	 * update ctdtree if removing some ph1 !
	 */
	revalidate_ph12();
	/* Update ctdtree ?
	 */

	save_sainfotree_flush();
	save_rmconf_flush();
}

static void
check_sigreq()
{
	int sig;

	/* 
	 * XXX We are not able to tell if we got 
	 * several time the same signal. This is
	 * not a problem for the current code, 
	 * but we shall remember this limitation.
	 */
	for (sig = 0; sig <= NSIG; sig++) {
		if (sigreq[sig] == 0)
			continue;

		sigreq[sig]--;
		switch(sig) {
		case 0:
			return;
			
			/* Catch up childs, mainly scripts.
			 */
		case SIGCHLD:
	    {
			pid_t pid;
			int s;
			
			pid = wait(&s);
	    }
		break;

#ifdef DEBUG_RECORD_MALLOCATION
		/* 
		 * XXX This operation is signal handler unsafe and may lead to 
		 * crashes and security breaches: See Henning Brauer talk at
		 * EuroBSDCon 2005. Do not run in production with this option
		 * enabled.
		 */
		case SIGUSR2:
			DRM_dump();
			break;
#endif

		case SIGHUP:
			/* Save old configuration, load new one...  */
			reload_conf();
			break;

		case SIGINT:
		case SIGTERM:			
			plog(LLV_INFO, LOCATION, NULL, 
			    "caught signal %d\n", sig);
			evt_generic(EVT_RACOON_QUIT, NULL);
			pfkey_send_flush(lcconf->sock_pfkey, 
			    SADB_SATYPE_UNSPEC);
#ifdef ENABLE_FASTQUIT
			close_session();
#else
			sched_schedule(&scflushsa, 1, check_flushsa_stub);
#endif
			dying = 1;
			break;

		default:
			plog(LLV_INFO, LOCATION, NULL, 
			    "caught signal %d\n", sig);
			break;
		}
	}
}

/*
 * waiting the termination of processing until sending DELETE message
 * for all inbound SA will complete.
 */
static void
check_flushsa_stub(p)
	struct sched *p;
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

	if (buf != NULL)
		vfree(buf);

	if (n) {
		sched_schedule(&scflushsa, 1, check_flushsa_stub);
		return;
	}

	close_session();
}

static void
init_signal()
{
	int i;

	/*
	 * Ignore SIGPIPE as we check the return value of system calls
	 * that write to pipe-like fds.
	 */
	signal(SIGPIPE, SIG_IGN);

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
	myaddr_close();
	pfkey_close(lcconf->sock_pfkey);
#ifdef ENABLE_ADMINPORT
	(void)admin_close();
#endif
	return 0;
}

