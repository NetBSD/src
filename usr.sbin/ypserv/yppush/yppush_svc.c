/*	$NetBSD: yppush_svc.c,v 1.6 2002/07/06 00:46:12 wiz Exp $	*/

/*
 * Copyright (c) 1996 Mats O Jansson <moj@stacken.kth.se>
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
__RCSID("$NetBSD: yppush_svc.c,v 1.6 2002/07/06 00:46:12 wiz Exp $");
#endif

/*
 * Originally from an rpcgen-generated file, then cleaned up
 * by Jason R. Thorpe <thorpej@NetBSD.ORG>.
 */

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <signal.h>
#include <string.h>
#include <syslog.h>

#include <rpc/rpc.h>
#include <rpc/xdr.h>
#include <rpcsvc/yp_prot.h>

#include "yppush.h"

#ifdef DEBUG
#define RPC_SVC_FG
#endif

#define _RPCSVC_CLOSEDOWN 120
int _rpcpmstart;		/* Started by a port monitor ? */
int _rpcfdtype;			/* Whether Stream or Datagram ? */
int _rpcsvcdirty;		/* Still serving ? */

static	void _msgout(char *);

static
void _msgout(char *msg)
{
#ifdef RPC_SVC_FG
	if (_rpcpmstart)
		syslog(LOG_ERR, "%s", msg);
	else
		warnx("%s", msg);
#else
	syslog(LOG_ERR, "%s", msg);
#endif
}

void
yppush_xfrrespprog_1(struct svc_req *rqstp, SVCXPRT *transp)
{
	union {
		struct yppushresp_xfr yppushproc_xfrresp_1_arg;
	} argument;
	void *result;
	xdrproc_t xdr_argument, xdr_result;
	void *(*local)(void *, struct svc_req *);

	_rpcsvcdirty = 1;
	switch (rqstp->rq_proc) {
	case YPPUSHPROC_NULL:
		xdr_argument = xdr_void;
		xdr_result = xdr_void;
		local = yppushproc_null_1_svc;
		break;

	case YPPUSHPROC_XFRRESP:
		xdr_argument = xdr_yppushresp_xfr;
		xdr_result = xdr_void;
		local = yppushproc_xfrresp_1_svc;
		break;

	default:
		svcerr_noproc(transp);
		_rpcsvcdirty = 0;
		exit(1);
		return;
	}

	(void) memset((char *)&argument, 0, sizeof (argument));
	if (!svc_getargs(transp, xdr_argument, (caddr_t) &argument)) {
		svcerr_decode(transp);
		_rpcsvcdirty = 0;
		exit(1);
		return;
	}

	result = (*local)(&argument, rqstp);
	if (result != NULL && !svc_sendreply(transp, xdr_result, result))
		svcerr_systemerr(transp);

	if (!svc_freeargs(transp, xdr_argument, (caddr_t) &argument)) {
		_msgout("unable to free arguments");
		exit(1);
	}
	_rpcsvcdirty = 0;
	if (rqstp->rq_proc != YPPUSHPROC_NULL)
		exit(0);
	return;
}
