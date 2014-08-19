/*	$NetBSD: quota_nfs.c,v 1.2.8.1 2014/08/20 00:02:20 tls Exp $	*/
/*-
  * Copyright (c) 2011 Manuel Bouyer
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
  * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
  * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
  * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
  * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
  * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  * POSSIBILITY OF SUCH DAMAGE.
  */

#include <sys/cdefs.h>
__RCSID("$NetBSD: quota_nfs.c,v 1.2.8.1 2014/08/20 00:02:20 tls Exp $");

#include <sys/types.h>
#include <sys/param.h> /* XXX for DEV_BSIZE */
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <netdb.h>
#include <errno.h>

#include <rpc/rpc.h>
#include <rpc/pmap_prot.h>
#include <rpcsvc/rquota.h>

#include <quota.h>
#include "quotapvt.h"

static uint64_t
rq_scale(uint32_t rqblocks, uint32_t rqsize)
{
	return ((uint64_t)rqblocks * rqsize) / DEV_BSIZE;
}

static uint64_t
rq_scalelimit(uint32_t rqval, uint32_t rqsize)
{
	if (rqval == 0) {
		return QUOTA_NOLIMIT;
	} else {
		return rq_scale(rqval - 1, rqsize);
	}
}

static uint64_t
rq_plainlimit(uint32_t rqval)
{
	if (rqval == 0) {
		return QUOTA_NOLIMIT;
	} else {
		return rqval - 1;
	}
}

static void
rquota_to_quotavals(const struct rquota *rq,
		    struct quotaval *blocks, struct quotaval *files)
{
	struct timeval now;

	gettimeofday(&now, NULL);

	/* blocks*/
	blocks->qv_hardlimit = rq_scalelimit(rq->rq_bhardlimit, rq->rq_bsize);
	blocks->qv_softlimit = rq_scalelimit(rq->rq_bsoftlimit, rq->rq_bsize);
	blocks->qv_usage = rq_scale(rq->rq_curblocks, rq->rq_bsize);
	blocks->qv_expiretime = rq->rq_btimeleft + now.tv_sec;
	blocks->qv_grace = QUOTA_NOTIME;

	/* inodes */
	files->qv_hardlimit = rq_plainlimit(rq->rq_fhardlimit);
	files->qv_softlimit = rq_plainlimit(rq->rq_fsoftlimit);
	files->qv_usage = rq->rq_curfiles;
	files->qv_expiretime = rq->rq_ftimeleft + now.tv_sec;
	files->qv_grace = QUOTA_NOTIME;
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
__quota_nfs_get(struct quotahandle *qh, const struct quotakey *qk,
		struct quotaval *qv)
{
	struct getquota_args gq_args;
	struct ext_getquota_args ext_gq_args;
	struct getquota_rslt gq_rslt;
	struct quotaval blocks, inodes;
	char *host, *path;
	int ret, rpcqtype;
	int sverrno;

	switch (qk->qk_idtype) {
	    case QUOTA_IDTYPE_USER:
		rpcqtype = RQUOTA_USRQUOTA;
		break;
	    case QUOTA_IDTYPE_GROUP:
		rpcqtype = RQUOTA_GRPQUOTA;
		break;
	    default:
		errno = EINVAL;
		return -1;
	}

	switch (qk->qk_objtype) {
	    case QUOTA_OBJTYPE_BLOCKS:
	    case QUOTA_OBJTYPE_FILES:
		break;
	    default:
		errno = EINVAL;
		return -1;
	}

	/*
	 * must be some form of "hostname:/path"
	 */
	path = strdup(qh->qh_mountdevice);
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
	ext_gq_args.gqa_id = qk->qk_id;
	ext_gq_args.gqa_type = rpcqtype;
	ret = callaurpc(host, RQUOTAPROG, EXT_RQUOTAVERS,
	    RQUOTAPROC_GETQUOTA, (xdrproc_t)xdr_ext_getquota_args,
	    &ext_gq_args, (xdrproc_t)xdr_getquota_rslt, &gq_rslt);
	if (ret == RPC_PROGVERSMISMATCH && rpcqtype == RQUOTA_USRQUOTA) {
		/* try RQUOTAVERS */
		gq_args.gqa_pathp = path;
		gq_args.gqa_uid = qk->qk_id;
		ret = callaurpc(host, RQUOTAPROG, RQUOTAVERS,
		    RQUOTAPROC_GETQUOTA, (xdrproc_t)xdr_getquota_args,
		    &gq_args, (xdrproc_t)xdr_getquota_rslt, &gq_rslt);
	}
	sverrno = errno;
	free(host);

	if (ret != RPC_SUCCESS) {
		/*
		 * Remap some error codes for callers convenience:
		 *  - if the file server does not support any quotas at all,
		 *    return ENOENT
		 *  - if the server can not be reached something is very
		 *    wrong - or we are run inside a virtual rump network
		 *    but querying an NFS mount from the host. Make sure
		 *    to fail silently and return ENOENT as well.
		 */
		if (ret == RPC_SYSTEMERROR
		    && rpc_createerr.cf_error.re_errno == EHOSTUNREACH)
			sverrno = ENOENT;
		else if (sverrno == ENOTCONN)
			sverrno = ENOENT;
		errno = sverrno;
		return -1;
	}

	switch (gq_rslt.status) {
	case Q_NOQUOTA:
		quotaval_clear(qv);
		return 0;
	case Q_EPERM:
		errno = EACCES;
		return -1;
	case Q_OK:
		rquota_to_quotavals(&gq_rslt.getquota_rslt_u.gqr_rquota,
				    &blocks, &inodes);
		if (qk->qk_objtype == QUOTA_OBJTYPE_BLOCKS) {
			*qv = blocks;
		} else {
			*qv = inodes;
		}
		return 0;
	default:
		break;
	}
	/* XXX not exactly a good errno */
	errno = ERANGE;
	return -1;
}

