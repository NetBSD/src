/*	$NetBSD: ypserv.c,v 1.16 2002/07/06 00:18:48 wiz Exp $	*/

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

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: ypserv.c,v 1.16 2002/07/06 00:18:48 wiz Exp $");
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>

#include <err.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <util.h>

#include <rpc/rpc.h>
#include <rpc/xdr.h>
#include <rpc/pmap_clnt.h>

#include <rpcsvc/yp_prot.h>

#include "ypdef.h"
#include "ypserv.h"

#ifdef LIBWRAP
#include <tcpd.h>

int allow_severity = LOG_DAEMON | LOG_INFO;
int deny_severity = LOG_DAEMON | LOG_WARNING;

/* XXX For ypserv_proc.c -- NOT THREAD SAFE!  (like any of this code is) */
const char *clientstr;
const char *svcname;
#endif /* LIBWRAP */

#define SIG_PF void(*)(int)

int	usedns;
#ifdef DEBUG
int	foreground = 1;
#else
int	foreground;
#endif

#ifdef LIBWRAP
int	lflag;
#endif

int	main(int, char *[]);
void	usage(void);

void	sighandler(int);


static
void _msgout(int level, const char *msg)
{
	if (foreground)
                warnx("%s", msg);
        else
                syslog(level, "%s", msg);
}

static void
ypprog_2(struct svc_req *rqstp, SVCXPRT *transp)
{
	union {
		char * ypproc_domain_2_arg;
		char * ypproc_domain_nonack_2_arg;
		struct ypreq_key ypproc_match_2_arg;
		struct ypreq_nokey ypproc_first_2_arg;
		struct ypreq_key ypproc_next_2_arg;
		struct ypreq_xfr ypproc_xfr_2_arg;
		struct ypreq_nokey ypproc_all_2_arg;
		struct ypreq_nokey ypproc_master_2_arg;
		struct ypreq_nokey ypproc_order_2_arg;
		char * ypproc_maplist_2_arg;
	} argument;
	char *result;
	xdrproc_t xdr_argument, xdr_result;
	void *(*local)(void *, struct svc_req *);
#ifdef LIBWRAP
	struct request_info req;
	struct sockaddr *caller;
#define	SVCNAME(x)	svcname = x
#else
#define	SVCNAME(x)	/* nothing */
#endif

#ifdef LIBWRAP
	caller = svc_getrpccaller(transp)->buf;
	request_init(&req, RQ_DAEMON, getprogname(), RQ_CLIENT_SIN, caller,
	    NULL);
	sock_methods(&req);
#endif

	switch (rqstp->rq_proc) {
	case YPPROC_NULL:
		xdr_argument = xdr_void;
		xdr_result = xdr_void;
		local = ypproc_null_2_svc;
		SVCNAME("null_2");
		break;

	case YPPROC_DOMAIN:
		xdr_argument = xdr_ypdomain_wrap_string;
		xdr_result = xdr_bool;
		local = ypproc_domain_2_svc;
		SVCNAME("domain_2");
		break;

	case YPPROC_DOMAIN_NONACK:
		xdr_argument = xdr_ypdomain_wrap_string;
		xdr_result = xdr_bool;
		local = ypproc_domain_nonack_2_svc;
		SVCNAME("domain_nonack_2");
		break;

	case YPPROC_MATCH:
		xdr_argument = xdr_ypreq_key;
		xdr_result = xdr_ypresp_val;
		local = ypproc_match_2_svc;
		SVCNAME("match_2");
		break;

	case YPPROC_FIRST:
		xdr_argument = xdr_ypreq_nokey;
		xdr_result = xdr_ypresp_key_val;
		local = ypproc_first_2_svc;
		SVCNAME("first_2");
		break;

	case YPPROC_NEXT:
		xdr_argument = xdr_ypreq_key;
		xdr_result = xdr_ypresp_key_val;
		local = ypproc_next_2_svc;
		SVCNAME("next_2");
		break;

	case YPPROC_XFR:
		xdr_argument = xdr_ypreq_xfr;
		xdr_result = xdr_ypresp_xfr;
		local = ypproc_xfr_2_svc;
		SVCNAME("xfer_2");
		break;

	case YPPROC_CLEAR:
		xdr_argument = xdr_void;
		xdr_result = xdr_void;
		local = ypproc_clear_2_svc;
		SVCNAME("clear_2");
		break;

	case YPPROC_ALL:
		xdr_argument = xdr_ypreq_nokey;
		xdr_result = xdr_ypresp_all;
		local = ypproc_all_2_svc;
		SVCNAME("all_2");
		break;

	case YPPROC_MASTER:
		xdr_argument = xdr_ypreq_nokey;
		xdr_result = xdr_ypresp_master;
		local = ypproc_master_2_svc;
		SVCNAME("master_2");
		break;

	case YPPROC_ORDER:
		xdr_argument = xdr_ypreq_nokey;
		xdr_result = xdr_ypresp_order;
		local = ypproc_order_2_svc;
		SVCNAME("order_2");
		break;

	case YPPROC_MAPLIST:
		xdr_argument = xdr_ypdomain_wrap_string;
		xdr_result = xdr_ypresp_maplist;
		local = ypproc_maplist_2_svc;
		SVCNAME("maplist_2");
		break;

	default:
		svcerr_noproc(transp);
		return;
	}

#ifdef LIBWRAP
	clientstr = eval_client(&req);

	if (hosts_access(&req) == 0) {
		syslog(deny_severity,
		    "%s: refused request from %.500s", svcname, clientstr);
		svcerr_auth(transp, AUTH_FAILED);
		return;
	}
#endif

	(void) memset((char *)&argument, 0, sizeof (argument));
	if (!svc_getargs(transp, xdr_argument, (caddr_t) &argument)) {
		svcerr_decode(transp);
		return;
	}
	result = (*local)(&argument, rqstp);
	if (result != NULL && !svc_sendreply(transp, xdr_result, result)) {
		svcerr_systemerr(transp);
	}
	if (!svc_freeargs(transp, xdr_argument, (caddr_t) &argument)) {
		_msgout(LOG_ERR, "unable to free arguments");
		exit(1);
	}
	return;
}

/*
 * limited NIS version 1 support: the null, domain, and domain_nonack
 * request/reply format is identical between v1 and v2.  SunOS4's ypbind
 * makes v1 domain_nonack calls.
 */
static void
ypprog_1(struct svc_req *rqstp, SVCXPRT *transp)
{
	switch (rqstp->rq_proc) {
	case YPPROC_NULL:
	case YPPROC_DOMAIN:
	case YPPROC_DOMAIN_NONACK:
		ypprog_2(rqstp, transp);
		return;

	default:
		svcerr_noproc(transp);
		return;
	}
}

int
main(argc, argv)
	int argc;
	char *argv[];
{
	SVCXPRT *udptransp, *tcptransp, *udp6transp, *tcp6transp;
	struct netconfig *udpconf, *tcpconf, *udp6conf, *tcp6conf;
	int udpsock, tcpsock, udp6sock, tcp6sock;
	struct sigaction sa;
	int ch, xcreated = 0, one = 1;

#ifdef LIBWRAP
#define	GETOPTSTR	"dfl"
#else
#define	GETOPTSTR	"df"
#endif

	while ((ch = getopt(argc, argv, GETOPTSTR)) != -1) {
		switch (ch) {
		case 'd':
			usedns = 1;
			break;
		case 'f':
			foreground = 1;
			break;

#ifdef LIBWRAP
		case 'l':
			lflag = 1;
			break;
#endif
		default:
			usage();
		}
	}

#undef GETOPTSTR

	/* This program must be run by root. */
	if (geteuid() != 0)
		errx(1, "must run as root");

	if (foreground == 0 && daemon(0, 0))
		err(1, "can't detach");

	openlog("ypserv", LOG_PID, LOG_DAEMON);
	syslog(LOG_INFO, "starting");
	pidfile(NULL);

	(void) rpcb_unset(YPPROG, YPVERS, NULL);
	(void) rpcb_unset(YPPROG, YPVERS_ORIG, NULL);

	udpsock  = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	tcpsock  = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	udp6sock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
	tcp6sock = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);

	/*
	 * We're doing host-based access checks here, so don't allow
	 * v4-in-v6 to confuse things.
	 */
	if (udp6sock != -1 && setsockopt(udp6sock, IPPROTO_IPV6,
	    IPV6_V6ONLY, &one, sizeof(one)) < 0) {
		_msgout(LOG_ERR, "can't disable v4-in-v6 on UDP socket");
		exit(1);
	}
	if (tcp6sock != -1 && setsockopt(tcp6sock, IPPROTO_IPV6,
	    IPV6_V6ONLY, &one, sizeof(one)) < 0) {
		_msgout(LOG_ERR, "can't disable v4-in-v6 on TCP socket");
		exit(1);
	}

	ypdb_init();	/* init db stuff */

	sa.sa_handler = sighandler;
	sa.sa_flags = 0;
	if (sigemptyset(&sa.sa_mask)) {
		_msgout(LOG_ERR, "sigemptyset: %m");
		exit(1);
	}
	if (sigaction(SIGCHLD, &sa, NULL)) {
		_msgout(LOG_ERR, "sigaction: %m");
		exit(1);
	}

	udpconf  = getnetconfigent("udp");
	tcpconf  = getnetconfigent("tcp");
	udp6conf = getnetconfigent("udp6");
	tcp6conf = getnetconfigent("tcp6");

	if (udpsock != -1 && udpconf != NULL) {
		if (bindresvport(udpsock, NULL) == 0) {
			udptransp = svc_dg_create(udpsock, 0, 0);
			if (udptransp != NULL) {
				if (svc_reg(udptransp, YPPROG, YPVERS_ORIG,
					    ypprog_1, udpconf) == 0 ||
				    svc_reg(udptransp, YPPROG, YPVERS,
					    ypprog_2, udpconf) == 0)
					_msgout(LOG_WARNING,
					    "unable to register UDP service");
				else
					xcreated++;
			} else
				_msgout(LOG_WARNING,
				    "unable to create UDP service");
		} else
			_msgout(LOG_ERR, "unable to bind reserved UDP port");
		freenetconfigent(udpconf);
	}

	if (tcpsock != -1 && tcpconf != NULL) {
		if (bindresvport(tcpsock, NULL) == 0) {
			listen(tcpsock, SOMAXCONN);
			tcptransp = svc_vc_create(tcpsock, 0, 0);
			if (tcptransp != NULL) {
				if (svc_reg(tcptransp, YPPROG, YPVERS_ORIG,
					    ypprog_1, tcpconf) == 0 ||
				    svc_reg(tcptransp, YPPROG, YPVERS,
					    ypprog_2, tcpconf) == 0)
					_msgout(LOG_WARNING,
					    "unable to register TCP service");
				else
					xcreated++;
			} else
				_msgout(LOG_WARNING,
				    "unable to create TCP service");
		} else
			_msgout(LOG_ERR, "unable to bind reserved TCP port");
		freenetconfigent(tcpconf);
	}

	if (udp6sock != -1 && udp6conf != NULL) {
		if (bindresvport(udp6sock, NULL) == 0) {
			udp6transp = svc_dg_create(udp6sock, 0, 0);
			if (udp6transp != NULL) {
				if (svc_reg(udp6transp, YPPROG, YPVERS_ORIG,
					    ypprog_1, udp6conf) == 0 ||
				    svc_reg(udp6transp, YPPROG, YPVERS,
					    ypprog_2, udp6conf) == 0)
					_msgout(LOG_WARNING,
					    "unable to register UDP6 service");
				else
					xcreated++;
			} else
				_msgout(LOG_WARNING,
				    "unable to create UDP6 service");
		} else
			_msgout(LOG_ERR, "unable to bind reserved UDP6 port");
		freenetconfigent(udp6conf);
	}

	if (tcp6sock != -1 && tcp6conf != NULL) {
		if (bindresvport(tcp6sock, NULL) == 0) {
			listen(tcp6sock, SOMAXCONN);
			tcp6transp = svc_vc_create(tcp6sock, 0, 0);
			if (tcp6transp != NULL) {
				if (svc_reg(tcp6transp, YPPROG, YPVERS_ORIG,
					    ypprog_1, tcp6conf) == 0 ||
				    svc_reg(tcp6transp,  YPPROG, YPVERS,
					    ypprog_2, tcp6conf) == 0)
					_msgout(LOG_WARNING,
					    "unable to register TCP6 service");
				else
					xcreated++;
			} else
				_msgout(LOG_WARNING,
				    "unable to create TCP6 service");
		} else
			_msgout(LOG_ERR, "unable to bind reserved TCP6 port");
		freenetconfigent(tcp6conf);
	}

	if (xcreated == 0) {
		_msgout(LOG_ERR, "unable to create any services");
		exit(1);
	}

	svc_run();
	_msgout(LOG_ERR, "svc_run returned");
	exit(1);
	/* NOTREACHED */
}

void
sighandler(sig)
	int sig;
{

	/* SIGCHLD */
	while (wait3((int *)NULL, WNOHANG, (struct rusage *)NULL) > 0);
}

void
usage()
{

#ifdef LIBWRAP
#define	USAGESTR	"usage: %s [-d] [-l]\n"
#else
#define	USAGESTR	"usage: %s [-d]\n"
#endif

	fprintf(stderr, USAGESTR, getprogname());
	exit(1);

#undef USAGESTR
}

/*
 * _yp_invalid_map: check if given map name isn't legal.
 * returns non-zero if invalid
 *
 * XXX: this probably should be in libc/yp/yplib.c
 */
int
_yp_invalid_map(map)
	const char *map;
{
	if (map == NULL || *map == '\0')
		return 1;

	if (strlen(map) > YPMAXMAP)
		return 1;

	if (strchr(map, '/') != NULL)
		return 1;

	return 0;
}
