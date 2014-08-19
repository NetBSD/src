/*	$NetBSD: rquotad.c,v 1.32.6.1 2014/08/20 00:02:23 tls Exp $	*/

/*
 * by Manuel Bouyer (bouyer@ensta.fr). Public domain.
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: rquotad.c,v 1.32.6.1 2014/08/20 00:02:23 tls Exp $");
#endif

#include <sys/param.h>
#include <sys/types.h>
#include <sys/mount.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <signal.h>

#include <stdio.h>
#include <fstab.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>

#include <rpc/rpc.h>
#include <rpcsvc/rquota.h>
#include <arpa/inet.h>

#include <quota.h>

static void rquota_service(struct svc_req *request, SVCXPRT *transp);
static void ext_rquota_service(struct svc_req *request, SVCXPRT *transp);
static void sendquota(struct svc_req *request, int vers, SVCXPRT *transp);
__dead static void cleanup(int);

static int from_inetd = 1;

static void 
cleanup(int dummy)
{

	(void)rpcb_unset(RQUOTAPROG, RQUOTAVERS, NULL);
	(void)rpcb_unset(RQUOTAPROG, EXT_RQUOTAVERS, NULL);
	exit(0);
}

int
main(int argc, char *argv[])
{
	SVCXPRT *transp;
	struct sockaddr_storage from;
	socklen_t fromlen;

	fromlen = sizeof(from);
	if (getsockname(0, (struct sockaddr *)&from, &fromlen) < 0)
		from_inetd = 0;

	if (!from_inetd) {
		(void) rpcb_unset(RQUOTAPROG, RQUOTAVERS, NULL);
		(void) rpcb_unset(RQUOTAPROG, EXT_RQUOTAVERS, NULL);
	}

	openlog("rpc.rquotad", LOG_PID, LOG_DAEMON);

	/* create and register the service */
	if (from_inetd) {
		transp = svc_dg_create(0, 0, 0);
		if (transp == NULL) {
			syslog(LOG_ERR, "couldn't create udp service.");
			exit(1);
		}
		if (!svc_reg(transp, RQUOTAPROG, RQUOTAVERS, rquota_service,
		    NULL)) {
			syslog(LOG_ERR,
			    "unable to register (RQUOTAPROG, RQUOTAVERS).");
			exit(1);
		}
		if (!svc_reg(transp, RQUOTAPROG, EXT_RQUOTAVERS,
		    ext_rquota_service, NULL)) {
			syslog(LOG_ERR,
			    "unable to register (RQUOTAPROG, EXT_RQUOTAVERS).");
			exit(1);
		}
	} else {
		if (!svc_create(rquota_service, RQUOTAPROG, RQUOTAVERS, "udp")){
			syslog(LOG_ERR,
			    "unable to create (RQUOTAPROG, RQUOTAVERS).");
			exit(1);
		}
		if (!svc_create(ext_rquota_service, RQUOTAPROG,
		    EXT_RQUOTAVERS, "udp")){
			syslog(LOG_ERR,
			    "unable to create (RQUOTAPROG, EXT_RQUOTAVERS).");
			exit(1);
		}
	}

	if (!from_inetd) {
		daemon(0, 0);
		(void) signal(SIGINT, cleanup);
		(void) signal(SIGTERM, cleanup);
		(void) signal(SIGHUP, cleanup);
	}
	svc_run();
	syslog(LOG_ERR, "svc_run returned");
	exit(1);
}

static void
rquota_service(struct svc_req *request, SVCXPRT *transp)
{
	switch (request->rq_proc) {
	case NULLPROC:
		(void)svc_sendreply(transp, (xdrproc_t)xdr_void, NULL);
		break;

	case RQUOTAPROC_GETQUOTA:
	case RQUOTAPROC_GETACTIVEQUOTA:
		sendquota(request, RQUOTAVERS, transp);
		break;

	default:
		svcerr_noproc(transp);
		break;
	}
	if (from_inetd)
		exit(0);
}

static void
ext_rquota_service(struct svc_req *request, SVCXPRT *transp)
{
	switch (request->rq_proc) {
	case NULLPROC:
		(void)svc_sendreply(transp, (xdrproc_t)xdr_void, NULL);
		break;

	case RQUOTAPROC_GETQUOTA:
	case RQUOTAPROC_GETACTIVEQUOTA:
		sendquota(request, EXT_RQUOTAVERS, transp);
		break;

	default:
		svcerr_noproc(transp);
		break;
	}
	if (from_inetd)
		exit(0);
}

/*
 * Convert a limit to rquota representation (where 0 == unlimited).
 * Clamp the result into a uint32_t.
 */
static uint32_t
limit_to_rquota(uint64_t lim)
{
	if (lim == QUOTA_NOLIMIT || lim > 0xfffffffeUL)
		return 0;
	else
		return (lim + 1);
}

/*
 * Convert a time to rquota representation.
 */
static uint32_t
time_to_rquota(time_t when, time_t now)
{
	if (when == QUOTA_NOTIME) {
		return 0;
	} else {
		return when - now;
	}
}

/*
 * Convert to rquota representation.
 */
static void
quotavals_to_rquota(const struct quotaval *blocks,
		    const struct quotaval *files,
		    struct rquota *rq)
{
	struct timeval now;

	gettimeofday(&now, NULL);

	rq->rq_active = TRUE;
	rq->rq_bsize = DEV_BSIZE;

	rq->rq_bhardlimit = limit_to_rquota(blocks->qv_hardlimit);
	rq->rq_bsoftlimit = limit_to_rquota(blocks->qv_softlimit);
	rq->rq_curblocks = blocks->qv_usage;
	rq->rq_btimeleft = time_to_rquota(blocks->qv_expiretime, now.tv_sec);

	rq->rq_fhardlimit = limit_to_rquota(files->qv_hardlimit);
	rq->rq_fsoftlimit = limit_to_rquota(files->qv_softlimit);
	rq->rq_curfiles = files->qv_usage;
	rq->rq_ftimeleft = time_to_rquota(files->qv_expiretime, now.tv_sec);
}

/* read quota for the specified id, and send it */
static void
sendquota(struct svc_req *request, int vers, SVCXPRT *transp)
{
	struct getquota_args getq_args;
	struct ext_getquota_args ext_getq_args;
	struct getquota_rslt getq_rslt;
	struct quotahandle *qh;
	struct quotakey qk;
	struct quotaval blocks, files;
	int idtype;

	memset((char *)&getq_args, 0, sizeof(getq_args));
	memset((char *)&ext_getq_args, 0, sizeof(ext_getq_args));
	switch (vers) {
	case RQUOTAVERS:
		if (!svc_getargs(transp, xdr_getquota_args,
		    (caddr_t)&getq_args)) {
			svcerr_decode(transp);
			return;
		}
		ext_getq_args.gqa_pathp = getq_args.gqa_pathp;
		ext_getq_args.gqa_id = getq_args.gqa_uid;
		ext_getq_args.gqa_type = RQUOTA_USRQUOTA;
		break;
	case EXT_RQUOTAVERS:
		if (!svc_getargs(transp, xdr_ext_getquota_args,
		    (caddr_t)&ext_getq_args)) {
			svcerr_decode(transp);
			return;
		}
		break;
	}
	switch (ext_getq_args.gqa_type) {
	case RQUOTA_USRQUOTA:
		idtype = QUOTA_IDTYPE_USER;
		break;
	case RQUOTA_GRPQUOTA:
		idtype = QUOTA_IDTYPE_GROUP;
		break;
	default:
		getq_rslt.status = Q_NOQUOTA;
		goto out;
	}
	if (request->rq_cred.oa_flavor != AUTH_UNIX) {
		/* bad auth */
		getq_rslt.status = Q_EPERM;
		goto out;
	}

	/*
	 * XXX validate the path...
	 */

	qh = quota_open(ext_getq_args.gqa_pathp);
	if (qh == NULL) {
		/*
		 * There are only three possible responses: success,
		 * permission denied, and "no quota", so we return
		 * the last for essentially all errors.
		 */
		if (errno == EPERM || errno == EACCES) {
			getq_rslt.status = Q_EPERM;
			goto out;
		}
		getq_rslt.status = Q_NOQUOTA;
		goto out;
	}

	qk.qk_id = ext_getq_args.gqa_id;
	qk.qk_idtype = idtype;
	qk.qk_objtype = QUOTA_OBJTYPE_BLOCKS;
	if (quota_get(qh, &qk, &blocks) < 0) {
		/* failed, return noquota */
		quota_close(qh);
		getq_rslt.status = Q_NOQUOTA;
		goto out;
	}

	qk.qk_objtype = QUOTA_OBJTYPE_FILES;
	if (quota_get(qh, &qk, &files) < 0) {
		/* failed, return noquota */
		quota_close(qh);
		getq_rslt.status = Q_NOQUOTA;
		goto out;
	}

	quota_close(qh);

	quotavals_to_rquota(&blocks, &files,
			    &getq_rslt.getquota_rslt_u.gqr_rquota);
	getq_rslt.status = Q_OK;

out:
	if (!svc_sendreply(transp, (xdrproc_t)xdr_getquota_rslt, (char *)&getq_rslt))
		svcerr_systemerr(transp);
	if (!svc_freeargs(transp, xdr_getquota_args, (caddr_t)&getq_args)) {
		syslog(LOG_ERR, "unable to free arguments");
		exit(1);
	}
}
