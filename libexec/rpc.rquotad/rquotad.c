/*	$NetBSD: rquotad.c,v 1.28 2011/08/27 15:46:59 joerg Exp $	*/

/*
 * by Manuel Bouyer (bouyer@ensta.fr). Public domain.
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: rquotad.c,v 1.28 2011/08/27 15:46:59 joerg Exp $");
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

#include <quota/quotaprop.h>
#include <quota/quota.h>
#include <rpc/rpc.h>
#include <rpcsvc/rquota.h>
#include <arpa/inet.h>

static void rquota_service(struct svc_req *request, SVCXPRT *transp);
static void ext_rquota_service(struct svc_req *request, SVCXPRT *transp);
static void sendquota(struct svc_req *request, int vers, SVCXPRT *transp);
__dead static void cleanup(int);

static int from_inetd = 1;

static uint32_t
qlim2rqlim(uint64_t lim)
{
	if (lim == UQUAD_MAX)
		return 0;
	else
		return (lim + 1);
}

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
		daemon(0, 0);

		(void) rpcb_unset(RQUOTAPROG, RQUOTAVERS, NULL);
		(void) rpcb_unset(RQUOTAPROG, EXT_RQUOTAVERS, NULL);
		(void) signal(SIGINT, cleanup);
		(void) signal(SIGTERM, cleanup);
		(void) signal(SIGHUP, cleanup);
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

	svc_run();
	syslog(LOG_ERR, "svc_run returned");
	exit(1);
}

static void
rquota_service(struct svc_req *request, SVCXPRT *transp)
{
	switch (request->rq_proc) {
	case NULLPROC:
		(void)svc_sendreply(transp, xdr_void, (char *)NULL);
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
		(void)svc_sendreply(transp, xdr_void, (char *)NULL);
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

/* read quota for the specified id, and send it */
static void
sendquota(struct svc_req *request, int vers, SVCXPRT *transp)
{
	struct getquota_args getq_args;
	struct ext_getquota_args ext_getq_args;
	struct getquota_rslt getq_rslt;
	struct ufs_quota_entry qe[QUOTA_NLIMITS];
	const char *class;
	struct timeval timev;

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
		class = QUOTADICT_CLASS_USER;
		break;
	case RQUOTA_GRPQUOTA:
		class = QUOTADICT_CLASS_GROUP;
		break;
	default:
		getq_rslt.status = Q_NOQUOTA;
		goto out;
	}
	if (request->rq_cred.oa_flavor != AUTH_UNIX) {
		/* bad auth */
		getq_rslt.status = Q_EPERM;
	} else if (!getufsquota(ext_getq_args.gqa_pathp, qe,
	    ext_getq_args.gqa_id, class)) {
		/* failed, return noquota */
		getq_rslt.status = Q_NOQUOTA;
	} else {
		gettimeofday(&timev, NULL);
		getq_rslt.status = Q_OK;
		getq_rslt.getquota_rslt_u.gqr_rquota.rq_active = TRUE;
		getq_rslt.getquota_rslt_u.gqr_rquota.rq_bsize = DEV_BSIZE;
		getq_rslt.getquota_rslt_u.gqr_rquota.rq_bhardlimit =
		    qlim2rqlim(qe[QUOTA_LIMIT_BLOCK].ufsqe_hardlimit);
		getq_rslt.getquota_rslt_u.gqr_rquota.rq_bsoftlimit =
		    qlim2rqlim(qe[QUOTA_LIMIT_BLOCK].ufsqe_softlimit);
		getq_rslt.getquota_rslt_u.gqr_rquota.rq_curblocks =
		    qe[QUOTA_LIMIT_BLOCK].ufsqe_cur;
		getq_rslt.getquota_rslt_u.gqr_rquota.rq_fhardlimit =
		    qlim2rqlim(qe[QUOTA_LIMIT_FILE].ufsqe_hardlimit);
		getq_rslt.getquota_rslt_u.gqr_rquota.rq_fsoftlimit =
		    qlim2rqlim(qe[QUOTA_LIMIT_FILE].ufsqe_softlimit);
		getq_rslt.getquota_rslt_u.gqr_rquota.rq_curfiles =
		    qe[QUOTA_LIMIT_FILE].ufsqe_cur;
		getq_rslt.getquota_rslt_u.gqr_rquota.rq_btimeleft =
		    qe[QUOTA_LIMIT_BLOCK].ufsqe_time - timev.tv_sec;
		getq_rslt.getquota_rslt_u.gqr_rquota.rq_ftimeleft =
		    qe[QUOTA_LIMIT_FILE].ufsqe_time - timev.tv_sec;
	}
out:
	if (!svc_sendreply(transp, xdr_getquota_rslt, (char *)&getq_rslt))
		svcerr_systemerr(transp);
	if (!svc_freeargs(transp, xdr_getquota_args, (caddr_t)&getq_args)) {
		syslog(LOG_ERR, "unable to free arguments");
		exit(1);
	}
}
