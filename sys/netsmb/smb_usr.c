/*	$NetBSD: smb_usr.c,v 1.1.4.1 2001/09/21 22:36:55 nathanw Exp $	*/

/*
 * Copyright (c) 2000, Boris Popov
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
 *    This product includes software developed by Boris Popov.
 * 4. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/proc.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <sys/socketvar.h>

#include "iconv.h"

#include <sys/tree.h>

#include <netsmb/smb.h>
#include <netsmb/smb_conn.h>
#include <netsmb/smb_rq.h>
#include <netsmb/smb_subr.h>
#include <netsmb/smb_dev.h>

/*
 * helpers for nsmb device. Can be moved to the smb_dev.c file.
 */
static int
smb_usr_conn2spec(struct smbioc_oconn *dp, struct smb_connspec *spec)
{
	int error;

	if (dp->ioc_server == NULL)
		return EINVAL;
	dp->ioc_server = smb_memdupin(dp->ioc_server, dp->ioc_svlen);
	if (dp->ioc_server == NULL)
		return ENOMEM;
	error = ENOMEM;
	if (dp->ioc_local) {
		dp->ioc_local = smb_memdupin(dp->ioc_local, dp->ioc_lolen);
		if (dp->ioc_server == NULL) {
			smb_memfree(dp->ioc_server);
			return ENOMEM;
		}
	}
	bzero(spec, sizeof(*spec));
	spec->sap = dp->ioc_server;
	spec->name = dp->ioc_srvname;
	return 0;
}

static int
smb_usr_vc2spec(struct smbioc_ossn *dp, struct smb_vcspec *spec)
{
	int flags = 0;

	if (dp->ioc_user[0] == 0)
		return EINVAL;
	bzero(spec, sizeof(*spec));
	spec->user = dp->ioc_user;
	spec->mode = dp->ioc_mode;
	spec->rights = dp->ioc_rights;
	spec->owner = dp->ioc_owner;
	spec->group = dp->ioc_group;
	if (dp->ioc_opt & SMBVOPT_PRIVATE)
		flags |= SMBV_PRIVATE;
	if (dp->ioc_opt & SMBVOPT_SINGLESHARE)
		flags |= SMBV_PRIVATE | SMBV_SINGLESHARE;
	return 0;
}

static int
smb_usr_share2spec(struct smbioc_oshare *dp, struct smb_sharespec *spec)
{
	bzero(spec, sizeof(*spec));
	spec->mode = dp->ioc_mode;
	spec->rights = dp->ioc_rights;
	spec->owner = dp->ioc_owner;
	spec->group = dp->ioc_group;
	spec->name = dp->ioc_share;
	spec->stype = dp->ioc_stype;
	return 0;
}

static int
smb_usr_createconn(struct smb_connspec *cspec, struct smbioc_oconn *dp,
	struct smb_cred *scred,	struct smb_conn **scpp)
{
	struct smb_conn *scp;
	int error;

	if (dp->ioc_localcs[0] == 0) {
		SMBERROR("no local charset ?\n");
		return EINVAL;
	}
	error = smb_ce_createconn(cspec, scred, &scp);
	if (error)
		return error;
	itry {
		scp->sc_laddr = dup_sockaddr(dp->ioc_local, 1);
		ierror(scp->sc_laddr == NULL, ENOMEM);
		ithrow(iconv_open("tolower", dp->ioc_localcs, &scp->sc_tolower));
		ithrow(iconv_open("toupper", dp->ioc_localcs, &scp->sc_toupper));
		if (dp->ioc_servercs[0]) {
			ithrow(iconv_open(dp->ioc_servercs,
			    dp->ioc_localcs, &scp->sc_toserver));
			ithrow(iconv_open(dp->ioc_localcs,
			    dp->ioc_servercs, &scp->sc_tolocal));
		}
		*scpp = scp;
	} icatch(error) {
		smb_conn_put(scp, scred);
	} ifinally {
	} iendtry;
	return error;
}

int
smb_usr_lookup(struct smbioc_lookup *dp, struct smb_cred *scred,
	struct smb_conn **scpp,	struct smb_vc **vcpp, struct smb_share **sspp)
{
	struct smb_conn *scp = NULL;
	struct smb_vc *vcp = NULL;
	struct smb_share *ssp = NULL;
	struct smb_connspec cspec;
	struct smb_vcspec vspec;
	struct smb_sharespec sspec, *sspecp = NULL;
	int error;

	if (dp->ioc_level < SMBL_VC || dp->ioc_level > SMBL_SHARE)
		return EINVAL;
	error = smb_usr_conn2spec(&dp->ioc_conn, &cspec);
	if (error)
		return error;
	error = smb_usr_vc2spec(&dp->ioc_ssn, &vspec);
	if (error)
		goto out;
	if (dp->ioc_level >= SMBL_SHARE) {
		error = smb_usr_share2spec(&dp->ioc_sh, &sspec);
		if (error)
			goto out;
		sspecp = &sspec;
	}
	error = smb_sm_lookup(&cspec, &vspec, sspecp, scred, &scp);
	if (error == 0) {
		/*
		 * Required connection already exists, so return it
		 */
		*scpp = scp;
		*vcpp = cspec.vcp;
		*sspp = vspec.ssp;
		return 0;
	}
	if ((dp->ioc_flags & SMBLK_CREATE) == 0)
		goto out;
	error = smb_sm_lookup(&cspec, NULL, NULL, scred, &scp);
	if (error) {
		/*
		 * This is a first connection to server
		 */
		error = smb_usr_createconn(&cspec, &dp->ioc_conn, scred, &scp);
		if (error)
			goto out;
		iconv_convstr(scp->sc_toupper, dp->ioc_ssn.ioc_user, dp->ioc_ssn.ioc_user);
	}
	/*
	 * Server is already known...
	 */
	error = smb_conn_lookupvc(scp, &vspec, scred, &vcp);
	if (error) {
		/*
		 * ...but VC isn't
		 */
		error = smb_vc_create(&vspec, dp->ioc_ssn.ioc_password, dp->ioc_ssn.ioc_workgroup, scp, scred, &vcp);
		if (error)
			goto out;
		error = smb_vc_connect(vcp, scred);
		if (error)
			goto out;
	}
	*scpp = scp;
	*vcpp = vcp;
	/*
	 * Now look if we need connection to a share
	 */
	if (dp->ioc_level < SMBL_SHARE)
		goto out;
	error = smb_share_create(vcp, &sspec, scred, &ssp);
	if (error)
		goto out;
	if (dp->ioc_sh.ioc_password[0])
		ssp->ss_pass = smb_strdup(dp->ioc_sh.ioc_password);
	error = smb_share_connect(ssp, scred);
	if (error == 0)
		*sspp = ssp;
out:
	if (error) {
		if (scp)
			smb_conn_put(scp, scred);
		if (vcp)
			smb_vc_put(vcp, scred);
		if (ssp)
			smb_share_put(ssp, scred->scr_p);
	}
	if (dp->ioc_conn.ioc_server)
		smb_memfree(dp->ioc_conn.ioc_server);
	if (dp->ioc_conn.ioc_local)
		smb_memfree(dp->ioc_conn.ioc_local);
	return error;
}

int
smb_usr_openconn(struct smbioc_oconn *dp, struct smb_cred *scred,
	struct smb_conn **scpp)
{
	struct smb_conn *scp = NULL;
	struct smb_connspec cs;
	int error;

	error = smb_usr_conn2spec(dp, &cs);
	if (error)
		return error;
	error = smb_sm_lookup(&cs, NULL, NULL, scred, &scp);
	if (error == 0) {
		*scpp = scp;
		goto out;
	}
	if ((dp->ioc_opt & SMBCOPT_CREATE) == 0)
		goto out;
	error = smb_usr_createconn(&cs, dp, scred, &scp);
out:
	if (dp->ioc_server)
		smb_memfree(dp->ioc_server);
	if (dp->ioc_local)
		smb_memfree(dp->ioc_local);
	return error;
}

/*
 * Connect to the resource specified by smbioc_connect structure.
 * It may either find an existing connection or try to establish a new one.
 * If no errors occurred smb_vc returned locked and referenced.
 */
int
smb_usr_opensession(struct smbioc_ossn *dp, struct smb_conn *scp,
	struct smb_cred *scred,	struct smb_vc **vcpp)
{
	struct smb_vc *vcp = NULL;
	struct smb_vcspec vspec;
	int error;

	error = smb_usr_vc2spec(dp, &vspec);
	if (error)
		return error;
	iconv_convstr(scp->sc_toupper, dp->ioc_user, dp->ioc_user);
	error = smb_conn_lookupvc(scp, &vspec, scred, &vcp);
	if (!error) {
		*vcpp = vcp;
		return 0;
	}
	if ((dp->ioc_opt & SMBVOPT_CREATE) == 0)
		return error;
	error = smb_vc_create(&vspec, dp->ioc_password, dp->ioc_workgroup, scp, scred, &vcp);
	if (error)
		return error;
	error = smb_vc_connect(vcp, scred);
	if (error) {
		smb_vc_put(vcp, scred);
	} else
		*vcpp = vcp;
	return error;
}

int
smb_usr_openshare(struct smb_vc *vcp, struct smbioc_oshare *dp,
	struct smb_cred *scred, struct smb_share **sspp)
{
	struct smb_share *ssp;
	struct smb_sharespec shspec;
	int error;

	error = smb_usr_share2spec(dp, &shspec);
	if (error)
		return error;
	error = smb_vc_lookupshare(vcp, &shspec, scred, &ssp);
	if (error == 0) {
		*sspp = ssp;
		return 0;
	}
	if ((dp->ioc_opt & SMBSOPT_CREATE) == 0)
		return error;
	error = smb_share_create(vcp, &shspec, scred, &ssp);
	if (error)
		return error;
	if (dp->ioc_password[0])
		ssp->ss_pass = smb_strdup(dp->ioc_password);
	error = smb_share_connect(ssp, scred);
	if (error) {
		smb_share_put(ssp, scred->scr_p);
	} else
		*sspp = ssp;
	return error;
}

int
smb_usr_simplerequest(struct smb_share *ssp, struct smbioc_rq *dp,
	struct smb_cred *scred)
{
	struct smb_rq rq, *rqp = &rq;
	struct mbdata *mbp;
	u_int8_t wc;
	u_int16_t bc;
	int error;

	switch (dp->ioc_cmd) {
	    case SMB_COM_TRANSACTION2:
	    case SMB_COM_TRANSACTION2_SECONDARY:
	    case SMB_COM_CLOSE_AND_TREE_DISC:
	    case SMB_COM_TREE_CONNECT:
	    case SMB_COM_TREE_DISCONNECT:
	    case SMB_COM_NEGOTIATE:
	    case SMB_COM_SESSION_SETUP_ANDX:
	    case SMB_COM_LOGOFF_ANDX:
	    case SMB_COM_TREE_CONNECT_ANDX:
		return EPERM;
	}
	error = smb_rq_init(rqp, SSTOTN(ssp), dp->ioc_cmd, scred);
	if (error)
		return error;
	mbp = &rqp->sr_rq;
	smb_rq_wstart(rqp);
	error = mb_put_mem(mbp, dp->ioc_twords, dp->ioc_twc * 2, MB_MUSER);
	if (error)
		goto bad;
	smb_rq_wend(rqp);
	smb_rq_bstart(rqp);
	error = mb_put_mem(mbp, dp->ioc_tbytes, dp->ioc_tbc, MB_MUSER);
	if (error)
		goto bad;
	smb_rq_bend(rqp);
	error = smb_rq_simple(rqp);
	if (error)
		goto bad;
	mbp = &rqp->sr_rp;
	mb_get_byte(mbp, &wc);
	dp->ioc_rwc = wc;
	wc *= 2;
	if (wc > dp->ioc_rpbufsz) {
		error = EBADRPC;
		goto bad;
	}
	error = mb_get_mem(mbp, dp->ioc_rpbuf, wc, MB_MUSER);
	if (error)
		goto bad;
	mb_get_wordle(mbp, &bc);
	if ((wc + bc) > dp->ioc_rpbufsz) {
		error = EBADRPC;
		goto bad;
	}
	dp->ioc_rbc = bc;
	error = mb_get_mem(mbp, dp->ioc_rpbuf + wc, bc, MB_MUSER);
bad:
	dp->ioc_errclass = rqp->sr_errclass;
	dp->ioc_serror = rqp->sr_serror;
	dp->ioc_error = rqp->sr_error;
	smb_rq_done(rqp);
	return error;

}

static int
smb_cpdatain(struct mbdata *mbp, int len, caddr_t data)
{
	int error;

	if (len == 0)
		return 0;
	error = mb_init(mbp);
	if (error)
		return error;
	return mb_put_mem(mbp, data, len, MB_MUSER);
}

int
smb_usr_t2request(struct smb_share *ssp, struct smbioc_t2rq *dp,
	struct smb_cred *scred)
{
	struct smb_t2rq t2, *t2p = &t2;
	struct mbdata *mbp;
	int error, len;

	if (dp->ioc_tparamcnt > 0xffff || dp->ioc_tdatacnt > 0xffff ||
	    dp->ioc_setupcnt > 3)
		return EINVAL;
	error = smb_t2_init(t2p, SSTOTN(ssp), dp->ioc_setup[0], scred);
	if (error)
		return error;
	len = t2p->t2_setupcount = dp->ioc_setupcnt;
	if (len > 1)
		t2p->t2_setupdata = dp->ioc_setup; 
	if (dp->ioc_name) {
		t2p->t_name = smb_strdupin(dp->ioc_name, 128);
		if (t2p->t_name == NULL) {
			error = ENOMEM;
			goto bad;
		}
	}
	t2p->t2_maxscount = 0;
	t2p->t2_maxpcount = dp->ioc_rparamcnt;
	t2p->t2_maxdcount = dp->ioc_rdatacnt;
	error = smb_cpdatain(&t2p->t2_tparam, dp->ioc_tparamcnt, dp->ioc_tparam);
	if (error)
		goto bad;
	error = smb_cpdatain(&t2p->t2_tdata, dp->ioc_tdatacnt, dp->ioc_tdata);
	if (error)
		goto bad;
	error = smb_t2_request(t2p);
	if (error)
		goto bad;
	mbp = &t2p->t2_rparam;
	if (mbp->mb_top) {
		len = mb_fixhdr(mbp);
		if (len > dp->ioc_rparamcnt) {
			error = EMSGSIZE;
			goto bad;
		}
		dp->ioc_rparamcnt = len;
		error = mb_get_mem(mbp, dp->ioc_rparam, len, MB_MUSER);
		if (error)
			goto bad;
	} else
		dp->ioc_rparamcnt = 0;
	mbp = &t2p->t2_rdata;
	if (mbp->mb_top) {
		len = mb_fixhdr(mbp);
		if (len > dp->ioc_rdatacnt) {
			error = EMSGSIZE;
			goto bad;
		}
		dp->ioc_rdatacnt = len;
		error = mb_get_mem(mbp, dp->ioc_rdata, len, MB_MUSER);
	} else
		dp->ioc_rdatacnt = 0;
bad:
	if (t2p->t_name)
		smb_strfree(t2p->t_name);
	smb_t2_done(t2p);
	return error;
}
