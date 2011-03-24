/*	$NetBSD: getnfsquota.c,v 1.1 2011/03/24 17:05:43 bouyer Exp $	*/

/*
 * Copyright (c) 1980, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Robert Elz at The University of Melbourne.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
__COPYRIGHT("@(#) Copyright (c) 1980, 1990, 1993\
 The Regents of the University of California.  All rights reserved.");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)quota.c	8.4 (Berkeley) 4/28/95";
#else
__RCSID("$NetBSD: getnfsquota.c,v 1.1 2011/03/24 17:05:43 bouyer Exp $");
#endif
#endif /* not lint */

/*
 * Disk quota reporting program.
 */
#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <quota/quota.h>

#include <rpc/rpc.h>
#include <rpc/pmap_prot.h>
#include <rpcsvc/rquota.h>

/* convert a rquota limit to our semantic */
static uint64_t
rqlim2qlim(uint32_t lim)
{
	if (lim == 0)
		return UQUAD_MAX;
	else
		return (lim - 1);
}
 
static int
callaurpc(const char *host, rpcprog_t prognum, rpcvers_t versnum,
    rpcproc_t procnum, xdrproc_t inproc, void *in, xdrproc_t outproc, void *out)
{
	struct sockaddr_in server_addr;
	enum clnt_stat clnt_stat;
	struct hostent *hp;
	struct timeval timeout, tottimeout;
 
	CLIENT *client = NULL;
	int sock = RPC_ANYSOCK;
 
	if ((hp = gethostbyname(host)) == NULL)
		return (int) RPC_UNKNOWNHOST;
	timeout.tv_usec = 0;
	timeout.tv_sec = 6;
	memmove(&server_addr.sin_addr, hp->h_addr, hp->h_length);
	server_addr.sin_family = AF_INET;
	server_addr.sin_port =  0;

	if ((client = clntudp_create(&server_addr, prognum,
	    versnum, timeout, &sock)) == NULL)
		return (int) rpc_createerr.cf_stat;

	client->cl_auth = authunix_create_default();
	tottimeout.tv_sec = 25;
	tottimeout.tv_usec = 0;
	clnt_stat = clnt_call(client, procnum, inproc, in,
	    outproc, out, tottimeout);
 
	return (int) clnt_stat;
}

int
getnfsquota(const char *mp, struct ufs_quota_entry *qv,
    uint32_t id, const char *class)
{
	struct getquota_args gq_args;
	struct ext_getquota_args ext_gq_args;
	struct getquota_rslt gq_rslt;
	struct timeval tv;
	char *host, *path;
	int ret, rpcqtype;

	if (strcmp(class, QUOTADICT_CLASS_USER) == 0)
		rpcqtype = RQUOTA_USRQUOTA;
	else if (strcmp(class, QUOTADICT_CLASS_GROUP) == 0)
		rpcqtype = RQUOTA_GRPQUOTA;
	else {
		errno = EINVAL;
		return -1;
	}

	/*
	 * must be some form of "hostname:/path"
	 */
	path = strdup(mp);
	if (path == NULL) {
		errno = ENOMEM;
		return -1;
	}
	host = strsep(&path, ":");
	if (path == NULL) {
		free(host);
		errno = EINVAL;
		return -1;
	}

	ext_gq_args.gqa_pathp = path;
	ext_gq_args.gqa_id = id;
	ext_gq_args.gqa_type = rpcqtype;
	ret = callaurpc(host, RQUOTAPROG, EXT_RQUOTAVERS,
	    RQUOTAPROC_GETQUOTA, xdr_ext_getquota_args, &ext_gq_args,
	    xdr_getquota_rslt, &gq_rslt);
	if (ret == RPC_PROGVERSMISMATCH && rpcqtype == RQUOTA_USRQUOTA) {
		/* try RQUOTAVERS */
		gq_args.gqa_pathp = path;
		gq_args.gqa_uid = id;
		ret = callaurpc(host, RQUOTAPROG, RQUOTAVERS,
		    RQUOTAPROC_GETQUOTA, xdr_getquota_args, &gq_args,
		    xdr_getquota_rslt, &gq_rslt);
	}
	free(host);

	if (ret != RPC_SUCCESS) {
		return 0;
	}

	switch (gq_rslt.status) {
	case Q_NOQUOTA:
		break;
	case Q_EPERM:
		errno = EACCES;
		return -1;
	case Q_OK:
		gettimeofday(&tv, NULL);

		/* blocks*/
		qv[QUOTA_LIMIT_BLOCK].ufsqe_hardlimit = rqlim2qlim(
		    gq_rslt.getquota_rslt_u.gqr_rquota.rq_bhardlimit *
		    (gq_rslt.getquota_rslt_u.gqr_rquota.rq_bsize / DEV_BSIZE));
		qv[QUOTA_LIMIT_BLOCK].ufsqe_softlimit = rqlim2qlim(
		    gq_rslt.getquota_rslt_u.gqr_rquota.rq_bsoftlimit *
		    (gq_rslt.getquota_rslt_u.gqr_rquota.rq_bsize / DEV_BSIZE));
		qv[QUOTA_LIMIT_BLOCK].ufsqe_cur =
		    gq_rslt.getquota_rslt_u.gqr_rquota.rq_curblocks *
		    (gq_rslt.getquota_rslt_u.gqr_rquota.rq_bsize / DEV_BSIZE);
		qv[QUOTA_LIMIT_BLOCK].ufsqe_time = (tv.tv_sec +
		    gq_rslt.getquota_rslt_u.gqr_rquota.rq_btimeleft);

		/* inodes */
		qv[QUOTA_LIMIT_FILE].ufsqe_hardlimit = rqlim2qlim(
		    gq_rslt.getquota_rslt_u.gqr_rquota.rq_fhardlimit);
		qv[QUOTA_LIMIT_FILE].ufsqe_softlimit = rqlim2qlim(
		    gq_rslt.getquota_rslt_u.gqr_rquota.rq_fsoftlimit);
		qv[QUOTA_LIMIT_FILE].ufsqe_cur =
		    gq_rslt.getquota_rslt_u.gqr_rquota.rq_curfiles;
		qv[QUOTA_LIMIT_FILE].ufsqe_time = (int)(tv.tv_sec +
		    gq_rslt.getquota_rslt_u.gqr_rquota.rq_ftimeleft);
		qv[QUOTA_LIMIT_BLOCK].ufsqe_grace =
		    qv[QUOTA_LIMIT_FILE].ufsqe_grace = 0;
		return 1;
	default:
		/* XXX sert errno and return -1 ? */
		break;
	}
	return 0;
}
