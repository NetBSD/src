/*	$NetBSD: yplib_host.c,v 1.7.34.1 2009/05/13 19:20:44 jym Exp $	*/

/*
 * Copyright (c) 1992, 1993 Theo de Raadt <deraadt@theos.com>
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
__RCSID("$NetBSD: yplib_host.c,v 1.7.34.1 2009/05/13 19:20:44 jym Exp $");
#endif

#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <sys/uio.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>

#include <rpc/rpc.h>
#include <rpc/xdr.h>
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypclnt.h>

#include "yplib_host.h"

struct timeval _yplib_host_timeout = { 10, 0 };

CLIENT *
yp_bind_host(char *server, u_int program, u_int version, u_short port,
	     int usetcp)
{
	struct sockaddr_in rsrv_sin;
	int rsrv_sock;
	struct hostent *h;
	static CLIENT *client;

	memset(&rsrv_sin, 0, sizeof rsrv_sin);
	rsrv_sin.sin_len = sizeof rsrv_sin;
	rsrv_sin.sin_family = AF_INET;
	rsrv_sock = RPC_ANYSOCK;
	if (port != 0) {
		rsrv_sin.sin_port = htons(port);
	}

	if (isdigit((unsigned char)*server)) {
		if (inet_aton(server,&rsrv_sin.sin_addr) == 0) {
			errx(1, "invalid IP address `%s'", server);
		}
	} else {
		h = gethostbyname(server);
		if(h == NULL) {
			errx(1, "unknown host `%s'", server);
		}
		memcpy(&rsrv_sin.sin_addr.s_addr, h->h_addr_list[0],
		    h->h_length);
	}

	if (usetcp)
		client = clnttcp_create(&rsrv_sin, program, version,
		    &rsrv_sock, 0, 0);
	else
		client = clntudp_create(&rsrv_sin, program, version,
		    _yplib_host_timeout, &rsrv_sock);

	if (client == NULL)
		errx(1, "%s: no contact with host `%s'",
		    usetcp ? "clnttcp_create" : "clntudp_create", server);

	return(client);
}

CLIENT *
yp_bind_local(u_int program, u_int version)
{
	struct sockaddr_in rsrv_sin;
	int rsrv_sock;
	static CLIENT *client;

	memset(&rsrv_sin, 0, sizeof rsrv_sin);
	rsrv_sin.sin_len = sizeof rsrv_sin;
	rsrv_sin.sin_family = AF_INET;
	rsrv_sin.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	rsrv_sock = RPC_ANYSOCK;

	client = clntudp_create(&rsrv_sin, program, version,
	    _yplib_host_timeout, &rsrv_sock);
	if (client == NULL)
		errx(1, "clntudp_create: no contact with localhost");

	return(client);
}

int
yp_match_host(CLIENT *client, char *indomain, char *inmap, const char *inkey,
	      int inkeylen, char **outval, int *outvallen)
{
	struct ypresp_val yprv;
	struct ypreq_key yprk;
	int r;

	*outval = NULL;
	*outvallen = 0;

	yprk.domain = indomain;
	yprk.map = inmap;
	yprk.keydat.dptr = __UNCONST(inkey);
	yprk.keydat.dsize = inkeylen;

	memset(&yprv, 0, sizeof yprv);

	r = clnt_call(client, YPPROC_MATCH, xdr_ypreq_key, &yprk,
	    xdr_ypresp_val, &yprv, _yplib_host_timeout);
	if(r != RPC_SUCCESS)
		clnt_perror(client, "yp_match_host: clnt_call");

	if(!(r=ypprot_err(yprv.status)) ) {
		*outvallen = yprv.valdat.dsize;
		*outval = (char *)malloc(*outvallen+1);
		memcpy(*outval, yprv.valdat.dptr, *outvallen);
		(*outval)[*outvallen] = '\0';
	}
	xdr_free(xdr_ypresp_val, (char *)&yprv);
	return r;
}

int
yp_first_host(CLIENT *client, char *indomain, char *inmap, char **outkey,
	      int *outkeylen, char **outval, int *outvallen)
{
	struct ypresp_key_val yprkv;
	struct ypreq_nokey yprnk;
	int r;

	*outkey = *outval = NULL;
	*outkeylen = *outvallen = 0;

	yprnk.domain = indomain;
	yprnk.map = inmap;
	memset(&yprkv, 0, sizeof yprkv);

	r = clnt_call(client, YPPROC_FIRST, xdr_ypreq_nokey, &yprnk,
	    xdr_ypresp_key_val, &yprkv, _yplib_host_timeout);
	if (r != RPC_SUCCESS)
		clnt_perror(client, "yp_first_host: clnt_call");

	if(!(r=ypprot_err(yprkv.status)) ) {
		*outkeylen = yprkv.keydat.dsize;
		*outkey = (char *)malloc(*outkeylen+1);
		memcpy(*outkey, yprkv.keydat.dptr, *outkeylen);
		(*outkey)[*outkeylen] = '\0';
		*outvallen = yprkv.valdat.dsize;
		*outval = (char *)malloc(*outvallen+1);
		memcpy(*outval, yprkv.valdat.dptr, *outvallen);
		(*outval)[*outvallen] = '\0';
	}
	xdr_free(xdr_ypresp_key_val, (char *)&yprkv);
	return r;
}

int
yp_next_host(CLIENT *client, char *indomain, char *inmap, char *inkey,
	     int inkeylen, char **outkey, int *outkeylen, char **outval,
	     int *outvallen)
{
	struct ypresp_key_val yprkv;
	struct ypreq_key yprk;
	int r;

	*outkey = *outval = NULL;
	*outkeylen = *outvallen = 0;

	yprk.domain = indomain;
	yprk.map = inmap;
	yprk.keydat.dptr = inkey;
	yprk.keydat.dsize = inkeylen;
	memset(&yprkv, 0, sizeof yprkv);

	r = clnt_call(client, YPPROC_NEXT, xdr_ypreq_key, &yprk,
	    xdr_ypresp_key_val, &yprkv, _yplib_host_timeout);
	if (r != RPC_SUCCESS)
		clnt_perror(client, "yp_next_host: clnt_call");

	if(!(r=ypprot_err(yprkv.status)) ) {
		*outkeylen = yprkv.keydat.dsize;
		*outkey = (char *)malloc(*outkeylen+1);
		memcpy(*outkey, yprkv.keydat.dptr, *outkeylen);
		(*outkey)[*outkeylen] = '\0';
		*outvallen = yprkv.valdat.dsize;
		*outval = (char *)malloc(*outvallen+1);
		memcpy(*outval, yprkv.valdat.dptr, *outvallen);
		(*outval)[*outvallen] = '\0';
	}
	xdr_free(xdr_ypresp_key_val, (char *)&yprkv);
	return r;
}

int
yp_all_host(CLIENT *client, const char *indomain, const char *inmap,
	    struct ypall_callback *incallback)
{
	struct ypreq_nokey yprnk;
	int status;

	yprnk.domain = indomain;
	yprnk.map = inmap;

	status = clnt_call(client, YPPROC_ALL, xdr_ypreq_nokey, &yprnk,
	    xdr_ypall, (char *)incallback, _yplib_host_timeout);

	if (status != RPC_SUCCESS)
		return YPERR_RPC;

	return 0;
}

int
yp_order_host(CLIENT *client, char *indomain, char *inmap, int *outorder)
{
	struct ypresp_order ypro;
	struct ypreq_nokey yprnk;
	int r;

	yprnk.domain = indomain;
	yprnk.map = inmap;

	memset(&ypro, 0, sizeof ypro);

	r = clnt_call(client, YPPROC_ORDER, xdr_ypreq_nokey, &yprnk,
	    xdr_ypresp_order, &ypro, _yplib_host_timeout);
	if (r != RPC_SUCCESS)
		clnt_perror(client, "yp_order_host: clnt_call");

	*outorder = ypro.ordernum;
	xdr_free(xdr_ypresp_order, (char *)&ypro);
	return ypprot_err(ypro.status);
}

int
yp_master_host(CLIENT *client, char *indomain, char *inmap, char **outname)
{
	struct ypresp_master yprm;
	struct ypreq_nokey yprnk;
	int r;

	yprnk.domain = indomain;
	yprnk.map = inmap;

	memset(&yprm, 0, sizeof yprm);

	r = clnt_call(client, YPPROC_MASTER, xdr_ypreq_nokey, &yprnk,
	    xdr_ypresp_master, &yprm, _yplib_host_timeout);
	if (r != RPC_SUCCESS)
		clnt_perror(client, "yp_master: clnt_call");

	if (!(r = ypprot_err(yprm.status))) {
		*outname = (char *)strdup(yprm.master);
	}
	xdr_free(xdr_ypresp_master, (char *)&yprm);
	return r;
}

int
yp_maplist_host(CLIENT *client, char *indomain, struct ypmaplist **outmaplist)
{
	struct ypresp_maplist ypml;
	int r;

	memset(&ypml, 0, sizeof ypml);

	r = clnt_call(client, YPPROC_MAPLIST, xdr_ypdomain_wrap_string,
	    indomain, xdr_ypresp_maplist, &ypml, _yplib_host_timeout);
	if (r != RPC_SUCCESS)
		clnt_perror(client, "yp_maplist: clnt_call");

	*outmaplist = ypml.list;
	/* NO: xdr_free(xdr_ypresp_maplist, &ypml);*/
	return ypprot_err(ypml.status);
}
