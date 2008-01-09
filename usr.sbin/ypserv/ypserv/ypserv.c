/*	$NetBSD: ypserv.c,v 1.20.10.1 2008/01/09 02:02:37 matt Exp $	*/

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
__RCSID("$NetBSD: ypserv.c,v 1.20.10.1 2008/01/09 02:02:37 matt Exp $");
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
#include <stdarg.h>
#include <errno.h>

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

int	usedns;
#ifdef DEBUG
static int	foreground = 1;
#else
static int	foreground;
#endif

#ifdef LIBWRAP
int	lflag;
#endif

static struct bindsock {
	sa_family_t family;
	int type;
	int proto;
	const char *name;
} socklist[] = {
	{ AF_INET, SOCK_DGRAM, IPPROTO_UDP, "udp" },
	{ AF_INET, SOCK_STREAM, IPPROTO_TCP, "tcp" },
	{ AF_INET6, SOCK_DGRAM, IPPROTO_UDP, "udp6" },
	{ AF_INET6, SOCK_STREAM, IPPROTO_TCP, "tcp6" },
};

static void	usage(void) __dead;
static int	bind_resv_port(int, sa_family_t, in_port_t);

static void
_msgout(int level, const char *msg, ...)
{
	va_list ap;
	va_start(ap, msg);
	if (foreground)
                vwarnx(msg, ap);
        else
		vsyslog(level, msg, ap);
	va_end(ap);
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
	void *argp = &argument;
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
	(void)request_init(&req, RQ_DAEMON, getprogname(), RQ_CLIENT_SIN,
	    caller, NULL);
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

	(void)memset(&argument, 0, sizeof (argument));
	if (!svc_getargs(transp, xdr_argument, argp)) {
		svcerr_decode(transp);
		return;
	}
	result = (*local)(&argument, rqstp);
	if (result != NULL && !svc_sendreply(transp, xdr_result, result)) {
		svcerr_systemerr(transp);
	}
	if (!svc_freeargs(transp, xdr_argument, argp)) {
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
main(int argc, char *argv[])
{
	SVCXPRT *xprt;
	struct netconfig *cfg = NULL;
	int s;
	struct sigaction sa;
	struct bindsock *bs;
	in_port_t port = 0;
	int ch, xcreated = 0, one = 1;

	setprogname(argv[0]);

#ifdef LIBWRAP
#define	GETOPTSTR	"dflp:"
#else
#define	GETOPTSTR	"dfp:"
#endif
	while ((ch = getopt(argc, argv, GETOPTSTR)) != -1) {
		switch (ch) {
		case 'd':
			usedns = 1;
			break;
		case 'f':
			foreground = 1;
			break;
		case 'p':
			port = atoi(optarg);
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
	(void)pidfile(NULL);

	(void) rpcb_unset((u_int)YPPROG, (u_int)YPVERS, NULL);
	(void) rpcb_unset((u_int)YPPROG, (u_int)YPVERS_ORIG, NULL);


	ypdb_init();	/* init db stuff */

	sa.sa_handler = SIG_IGN;
	sa.sa_flags = SA_NOCLDWAIT;
	if (sigemptyset(&sa.sa_mask)) {
		_msgout(LOG_ERR, "sigemptyset: %s", strerror(errno));
		exit(1);
	}
	if (sigaction(SIGCHLD, &sa, NULL)) {
		_msgout(LOG_ERR, "sigaction: %s", strerror(errno));
		exit(1);
	}

	for (bs = socklist;
	    bs < &socklist[sizeof(socklist) / sizeof(socklist[0])]; bs++) {

		if ((s = socket(bs->family, bs->type, bs->proto)) == -1)
			continue;

		if (bs->family == AF_INET6) {
			/*
			 * We're doing host-based access checks here, so don't
			 * allow v4-in-v6 to confuse things.
			 */
			if (setsockopt(s, IPPROTO_IPV6,
			    IPV6_V6ONLY, &one, sizeof(one)) == -1) {
				_msgout(LOG_ERR, 
				    "can't disable v4-in-v6 on %s socket",
				    bs->name);
				exit(1);
			}
		}

		if ((cfg = getnetconfigent(bs->name)) == NULL) {
			_msgout(LOG_ERR,
			    "unable to get network configuration for %s port",
			    bs->name);
			goto out;
		}

		if (bind_resv_port(s, bs->family, port) != 0)
			goto out;

		if (bs->type == SOCK_STREAM) {
			(void)listen(s, SOMAXCONN);
			xprt = svc_vc_create(s, 0, 0);
		} else {
			xprt = svc_dg_create(s, 0, 0);
		}

		if (xprt == NULL) {
			_msgout(LOG_WARNING, "unable to create %s service",
			    bs->name);
			goto out;
		}
		if (svc_reg(xprt, (u_int)YPPROG, (u_int)YPVERS_ORIG, ypprog_1,
		    cfg) == 0 ||
		    svc_reg(xprt, (u_int)YPPROG, (u_int)YPVERS, ypprog_2,
		    cfg) == 0) {
			_msgout(LOG_WARNING, "unable to register %s service",
			    bs->name);
			goto out;
		}
		xcreated++;
		freenetconfigent(cfg);
		continue;
out:
		if (s != -1)
			(void)close(s);
		if (cfg) {
			freenetconfigent(cfg);
			cfg = NULL;
		}
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

static void
usage(void)
{

#ifdef LIBWRAP
#define	USAGESTR	"Usage: %s [-d] [-l] [-p <port>]\n"
#else
#define	USAGESTR	"Usage: %s [-d] [-p <port>]\n"
#endif

	(void)fprintf(stderr, USAGESTR, getprogname());
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
_yp_invalid_map(const char *map)
{
	if (map == NULL || *map == '\0')
		return 1;

	if (strlen(map) > YPMAXMAP)
		return 1;

	if (strchr(map, '/') != NULL)
		return 1;

	return 0;
}

static int
bind_resv_port(int sock, sa_family_t family, in_port_t port)
{
	struct sockaddr *sa;
	struct sockaddr_in sasin;
	struct sockaddr_in6 sasin6;

	switch (family) {
	case AF_INET:
		(void)memset(&sasin, 0, sizeof(sasin));
		sasin.sin_len = sizeof(sasin);
		sasin.sin_family = family;
		sasin.sin_port = htons(port);
		sa = (struct sockaddr *)(void *)&sasin;
		break;
	case AF_INET6:
		(void)memset(&sasin6, 0, sizeof(sasin6));
		sasin6.sin6_len = sizeof(sasin6);
		sasin6.sin6_family = family;
		sasin6.sin6_port = htons(port);
		sa = (struct sockaddr *)(void *)&sasin6;
		break;
	default:
		_msgout(LOG_ERR, "Unsupported address family %d", family);
		return -1;
	}
	if (bindresvport_sa(sock, sa) == -1) {
		_msgout(LOG_ERR, "Cannot bind to reserved port %d (%s)", port,
		    strerror(errno));
		return -1;
	}
	return 0;
}
