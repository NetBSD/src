/*	$NetBSD: ypserv.c,v 1.4 1997/04/17 17:46:17 christos Exp $	*/

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
#include <sys/socket.h>
#include <sys/wait.h>

#include <netinet/in.h>

#include <memory.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <rpc/rpc.h>
#include <rpc/xdr.h>
#include <rpc/pmap_clnt.h>

#include <rpcsvc/yp_prot.h>

#ifdef SYSLOG
#include <syslog.h>
#else
#define LOG_ERR 1
#define openlog(a, b, c)
#endif

#include "acl.h"
#include "yplog.h"
#include "ypdef.h"
#include "ypserv.h"

#ifdef __STDC__
#define SIG_PF void(*)(int)
#endif

#ifdef DEBUG
#define RPC_SVC_FG
#endif

#define _RPCSVC_CLOSEDOWN 120
static int _rpcpmstart;		/* Started by a port monitor ? */
static int _rpcfdtype;		/* Whether Stream or Datagram ? */
static int _rpcsvcdirty;	/* Still serving ? */

int	usedns;
char	*aclfile;

extern	char *__progname;	/* from crt0.s */
extern	char *optarg;
extern	int optind;

void	usage __P((void));

void sig_child();
void sig_hup();

static
void _msgout(char* msg)
{
#ifdef RPC_SVC_FG
	if (_rpcpmstart)
		syslog(LOG_ERR, msg);
	else
		warnx("%s", msg);
#else
	syslog(LOG_ERR, msg);
#endif
}

static void
closedown()
{
	if (_rpcsvcdirty == 0) {
		extern fd_set svc_fdset;
		static int size;
		int i, openfd;

		if (_rpcfdtype == SOCK_DGRAM)
			exit(0);
		if (size == 0) {
			size = getdtablesize();
		}
		for (i = 0, openfd = 0; i < size && openfd < 2; i++)
			if (FD_ISSET(i, &svc_fdset))
				openfd++;
		if (openfd <= (_rpcpmstart?0:1))
			exit(0);
	}
	(void) alarm(_RPCSVC_CLOSEDOWN);
}

static void
ypprog_2(struct svc_req *rqstp, register SVCXPRT *transp)
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
	void *(*local) __P((void *, struct svc_req *));

	_rpcsvcdirty = 1;
	switch (rqstp->rq_proc) {
	case YPPROC_NULL:
		xdr_argument = xdr_void;
		xdr_result = xdr_void;
		local = ypproc_null_2_svc;
		break;

	case YPPROC_DOMAIN:
		xdr_argument = xdr_ypdomain_wrap_string;
		xdr_result = xdr_bool;
		local = ypproc_domain_2_svc;
		break;

	case YPPROC_DOMAIN_NONACK:
		xdr_argument = xdr_ypdomain_wrap_string;
		xdr_result = xdr_bool;
		local = ypproc_domain_nonack_2_svc;
		break;

	case YPPROC_MATCH:
		xdr_argument = xdr_ypreq_key;
		xdr_result = xdr_ypresp_val;
		local = ypproc_match_2_svc;
		break;

	case YPPROC_FIRST:
		xdr_argument = xdr_ypreq_nokey;
		xdr_result = xdr_ypresp_key_val;
		local = ypproc_first_2_svc;
		break;

	case YPPROC_NEXT:
		xdr_argument = xdr_ypreq_key;
		xdr_result = xdr_ypresp_key_val;
		local = ypproc_next_2_svc;
		break;

	case YPPROC_XFR:
		xdr_argument = xdr_ypreq_xfr;
		xdr_result = xdr_ypresp_xfr;
		local = ypproc_xfr_2_svc;
		break;

	case YPPROC_CLEAR:
		xdr_argument = xdr_void;
		xdr_result = xdr_void;
		local = ypproc_clear_2_svc;
		break;

	case YPPROC_ALL:
		xdr_argument = xdr_ypreq_nokey;
		xdr_result = xdr_ypresp_all;
		local = ypproc_all_2_svc;
		break;

	case YPPROC_MASTER:
		xdr_argument = xdr_ypreq_nokey;
		xdr_result = xdr_ypresp_master;
		local = ypproc_master_2_svc;
		break;

	case YPPROC_ORDER:
		xdr_argument = xdr_ypreq_nokey;
		xdr_result = xdr_ypresp_order;
		local = ypproc_order_2_svc;
		break;

	case YPPROC_MAPLIST:
		xdr_argument = xdr_ypdomain_wrap_string;
		xdr_result = xdr_ypresp_maplist;
		local = ypproc_maplist_2_svc;
		break;

	default:
		svcerr_noproc(transp);
		_rpcsvcdirty = 0;
		return;
	}
	(void) memset((char *)&argument, 0, sizeof (argument));
	if (!svc_getargs(transp, xdr_argument, (caddr_t) &argument)) {
		svcerr_decode(transp);
		_rpcsvcdirty = 0;
		return;
	}
	result = (*local)(&argument, rqstp);
	if (result != NULL && !svc_sendreply(transp, xdr_result, result)) {
		svcerr_systemerr(transp);
	}
	if (!svc_freeargs(transp, xdr_argument, (caddr_t) &argument)) {
		_msgout("unable to free arguments");
		exit(1);
	}
	_rpcsvcdirty = 0;
	return;
}

/*
 * limited NIS version 1 support: the null, domain, and domain_nonack
 * request/reply format is identical between v1 and v2.  SunOS4's ypbind
 * makes v1 domain_nonack calls.
 */
static void
ypprog_1(struct svc_req *rqstp, register SVCXPRT *transp)
{
	switch (rqstp->rq_proc) {
	case YPPROC_NULL:
	case YPPROC_DOMAIN:
	case YPPROC_DOMAIN_NONACK:
		ypprog_2(rqstp, transp);
		return;

	default:
		svcerr_noproc(transp);
		_rpcsvcdirty = 0;
		return;
	}
}

int
main(argc, argv)
	int argc;
	char **argv;
{
	register SVCXPRT *transp;
	int sock, proto;
	struct sockaddr_in saddr;
	int asize = sizeof(saddr);
	int xflag = 0;
	int ch;
	
	while ((ch = getopt(argc, argv, "a:dx")) != -1) {
		switch (ch) {
		case 'a':
			aclfile = optarg;
			break;

		case 'd':
			usedns = 1;
			break;

		case 'x':
			xflag = 1;
			break;

		default:
			usage();
		}
	}

	/* This program must be run by root. */
	if (geteuid() != 0)
		errx(1, "must run as root");

	/* Deal with the acl file. */
	acl_parse(aclfile);

	if (xflag)
		exit(1);

#ifndef RPC_SVC_FG
	if (daemon(0, 0))
		err(1, "can't detach");
	openlog("ypserv", LOG_PID, LOG_DAEMON);
#endif

	{
		FILE *pidfile = fopen(YPSERV_PID_PATH, "w");

		if (pidfile != NULL) {
			fprintf(pidfile, "%d\n", getpid());
			fclose(pidfile);
		} else
			err(1, "can't write PID file");
	}

	sock = RPC_ANYSOCK;
	(void) pmap_unset(YPPROG, YPVERS);
	(void) pmap_unset(YPPROG, YPVERS_ORIG);

	ypopenlog();	/* open log file */
	ypdb_init();	/* init db stuff */

	(void)signal(SIGCHLD, sig_child);
	(void)signal(SIGHUP, sig_hup);

	if ((_rpcfdtype == 0) || (_rpcfdtype == SOCK_DGRAM)) {
		transp = svcudp_create(sock);
		if (transp == NULL) {
			_msgout("cannot create udp service.");
			exit(1);
		}
		if (!_rpcpmstart)
			proto = IPPROTO_UDP;
		if (!svc_register(transp, YPPROG, YPVERS_ORIG, ypprog_1, proto)) {
			_msgout("unable to register (YPPROG, YPVERS_ORIG, udp).");
			exit(1);
		}
		if (!svc_register(transp, YPPROG, YPVERS, ypprog_2, proto)) {
			_msgout("unable to register (YPPROG, YPVERS, udp).");
			exit(1);
		}
	}

	if ((_rpcfdtype == 0) || (_rpcfdtype == SOCK_STREAM)) {
		if (_rpcpmstart)
			transp = svcfd_create(sock, 0, 0);
		else
			transp = svctcp_create(sock, 0, 0);
		if (transp == NULL) {
			_msgout("cannot create tcp service.");
			exit(1);
		}
		if (!_rpcpmstart)
			proto = IPPROTO_TCP;
		if (!svc_register(transp, YPPROG, YPVERS_ORIG, ypprog_1, proto)) {
			_msgout("unable to register (YPPROG, YPVERS_ORIG, tcp).");
			exit(1);
		}
		if (!svc_register(transp, YPPROG, YPVERS, ypprog_2, proto)) {
			_msgout("unable to register (YPPROG, YPVERS, tcp).");
			exit(1);
		}
	}

	if (transp == (SVCXPRT *)NULL) {
		_msgout("could not create a handle");
		exit(1);
	}
	if (_rpcpmstart) {
		(void) signal(SIGALRM, (SIG_PF) closedown);
		(void) alarm(_RPCSVC_CLOSEDOWN);
	}
	svc_run();
	_msgout("svc_run returned");
	exit(1);
	/* NOTREACHED */
}

void
sig_child()
{

	while (wait3((int *)NULL, WNOHANG, (struct rusage *)NULL) > 0);
}

void
sig_hup()
{

	acl_reset();
	yplog("sig_hup: reread %s", aclfile ? aclfile : YP_SECURENET_FILE);
	acl_parse(aclfile);
}

void
usage()
{

	fprintf(stderr, "usage: %s [-a aclfile] [-d] [-x]\n", __progname);
	exit(1);
}
