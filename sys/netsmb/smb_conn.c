/*	$NetBSD: smb_conn.c,v 1.1.2.2 2001/01/08 14:58:06 bouyer Exp $	*/

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
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/lock.h>
#include <uvm/uvm_extern.h>
#include <sys/sysctl.h>
#include <sys/socket.h>
#include <sys/socketvar.h>

#include <sys/tree.h>

#include "iconv.h"

#include <netsmb/smb.h>
#include <netsmb/smb_conn.h>
#include <netsmb/smb_rq.h>
#include <netsmb/smb_subr.h>
#include <netsmb/smb_tran.h>
#include <netsmb/smb_trantcp.h>

static struct tree cetree;

extern struct linker_set sysctl_net_smb;

#ifdef SYSCTL_DECL
SYSCTL_DECL(_net_smb);
#endif

#ifndef NetBSD
SYSCTL_NODE(_net, OID_AUTO, smb, CTLFLAG_RW, NULL, "SMB protocol");

MALLOC_DEFINE(M_SMBCONN, "SMB conn", "SMB connection");
#endif

#ifdef NetBSD
static struct callout smb_timer_ch;
#endif

#ifndef NetBSD
static struct callout_handle smb_timer_handle;
#endif
static int smb_timer_tick;

static void smb_conn_free(struct smb_conn *scp);
static void smb_vc_free(struct smb_vc *vcp);
static void smb_share_free(struct smb_share *ssp);
static void smb_conn_gone(struct smb_conn *scp, struct smb_cred *scred);
static void smb_vc_gone(struct smb_vc *vcp, struct smb_cred *scred);
static void smb_vc_timo(struct smb_vc *vcp);
static void smb_share_gone(struct smb_share *ssp, struct smb_cred *scred);
static void smb_timer(void *arg);
#ifndef NetBSD
#if __FreeBSD_version > 500000
static int  smb_sysctl_treedump(SYSCTL_HANDLER_ARGS);
#else
static int  smb_sysctl_treedump SYSCTL_HANDLER_ARGS;
#endif
#endif

#ifndef NetBSD
SYSCTL_PROC(_net_smb, OID_AUTO, treedump, CTLFLAG_RD | CTLTYPE_OPAQUE,
	    NULL, 0, smb_sysctl_treedump, "S,treedump", "Requester tree");
#endif

static int
smb_sm_node_alloc(struct tnode *tp, void *data)
{
	struct smb_ce *cep;
	struct smb_conn *scp;
	struct smb_share *ssp;
	struct smb_vc *vcp;
	int error, ntype;

	error = 0;
	ntype = TTYPE(tp);
	switch(ntype) {
	    case TSMB_CE:
		MALLOC(cep, struct smb_ce *, sizeof(*cep), M_SMBCONN, M_WAITOK);
		if (cep == NULL)
			return ENOMEM;
		bzero(cep, sizeof(*cep));
		cep->ce_nextconn = 1;
		TNTOCE(tp) = cep;
		CETOTN(cep) = tp;
		break;
	    case TSMB_CONN:
		if (tp->t_tree->tr_root == NULL) {
			SMBERROR("no root");
			return EINVAL;
		}
		MALLOC(scp, struct smb_conn*, sizeof(*scp), M_SMBCONN, M_WAITOK);
		if (scp == NULL)
			return ENOMEM;
		bzero(scp, sizeof(*scp));
		TNTOCN(tp) = scp;
		CNTOTN(scp) = tp;
		break;
	    case TSMB_VC:
		MALLOC(vcp, struct smb_vc*, sizeof(*vcp), M_SMBCONN, M_WAITOK);
		if (vcp == NULL)
			return ENOMEM;
		bzero(vcp, sizeof(*vcp));
		TNTOVC(tp) = vcp;
		VCTOTN(vcp) = tp;
		break;
	    case TSMB_SHARE:
		MALLOC(ssp, struct smb_share*, sizeof(*ssp), M_SMBCONN, M_WAITOK);
		if (ssp == NULL)
			return ENOMEM;
		bzero(ssp, sizeof(*ssp));
		TNTOSS(tp) = ssp;
		SSTOTN(ssp) = tp;
		break;
	    default:
		return EINVAL;
	}
	return error;
}

static void
smb_sm_node_free(struct tnode *tp)
{
	struct smb_ce *cep;

	if (tp->t_mdata == NULL)
		return;
	switch (TTYPE(tp)) {
	    case TSMB_CE:
		cep = TNTOCE(tp);
		free(cep, M_SMBCONN);
		break;
	    case TSMB_CONN:
		smb_conn_free(TNTOCN(tp));
		break;
	    case TSMB_VC:
		smb_vc_free(TNTOVC(tp));
		break;
	    case TSMB_SHARE:
		smb_share_free(TNTOSS(tp));
		break;
	    default:
		return;
	}
	return;
}

static void
smb_sm_node_gone(struct tnode *tp, struct proc *p)
{
	struct smb_cred scred;

	if (!TISGONE(tp)) {
		SMBERROR("bug in the tree.c code: gone\n");
		tunlock(tp, 0, p);
		return;
	}
	smb_makescred(&scred, p, NULL);
	switch (TTYPE(tp)) {
	    case TSMB_CE:
		tunlock(tp, 0, p);
		break;
	    case TSMB_CONN:
		smb_conn_gone(TNTOCN(tp), &scred);
		break;
	    case TSMB_VC:
		smb_vc_gone(TNTOVC(tp), &scred);
		break;
	    case TSMB_SHARE:
		smb_share_gone(TNTOSS(tp), &scred);
		break;
	    default:
		return;
	}
	return;
}


struct tnode_ops smb_connops = {
	smb_sm_node_alloc,
	smb_sm_node_free,
	NULL,
	smb_sm_node_gone
};

int
smb_sm_init(void)
{
	struct compname cn;
	struct tnode *tp;
	struct timeval tv;
	int error;

	tree_init(&cetree, &smb_connops);
	compname_init(&cn, "\\\\", 0, curproc);
	error = tree_node_alloc(&cetree, &cn, TFL_DIR | TSMB_CE, NULL, &tp);
	if (error)
		return error;
	cetree.tr_root = tp;
	tunlock(tp, 0, curproc);
	tv.tv_sec = 1;
	tv.tv_usec = 0;
#ifndef NetBSD
	smb_timer_tick = tvtohz(&tv);
	smb_timer_handle = timeout(smb_timer, NULL, smb_timer_tick);
#else
	callout_init(&smb_timer_ch);
	smb_timer_tick = hzto(&tv);
	callout_reset(&smb_timer_ch, smb_timer_tick, smb_timer, NULL);
#endif
	return 0;
}

int
smb_sm_done(void)
{
	struct tnode *tp;

#ifndef NetBSD
	untimeout(smb_timer, NULL, smb_timer_handle);
#else
	callout_stop(&smb_timer_ch);
#endif
	tp = cetree.tr_root;
	trele(tp);
	if (!TISGONE(tp)) {
		SMBERROR("%d connections were active\n", tp->t_usecount);
		return EBUSY;
	}
	tree_node_free(tp);
	return 0;
}

static int
smb_sm_getce(struct smb_cred *scred, int flags, struct smb_ce **cepp)
{
	struct tnode *tp;
	struct smb_ce *cep;

	tp = cetree.tr_root;
	cep = TNTOCE(tp);
	*cepp = cep;
	if (flags == 0)
		return 0;
	return smb_ce_lock(cep, flags, scred->scr_p);
}

/*
 * Connection engine implementation
 */
static int
smb_ce_cmpconn(struct tnode *tp, void *data)
{
	struct smb_conn *scp = TNTOCN(tp);
	struct smb_connspec *dp = data;
	int error;

	if (TTYPE(tp) != TSMB_CONN || !TISDIR(tp)) {
		SMBERROR("wrong type for node %s\n", tp->t_name);
		return 1;
	}
	if (!CONNADDREQ(scp->sc_paddr, dp->sap))
		return 1;
	if (dp->vcspec == NULL)
		return 0;
	error = smb_conn_lookupvc(scp, dp->vcspec, dp->scred, &dp->vcp);
	return error == 0 ? 0 : 1;
}

static int
smb_ce_lookupconn(smb_rc_t cep, struct smb_connspec *csp,
	struct smb_cred *scred, struct smb_conn **scpp)
{
	struct tnode *dtp, *tp;
	int error;

	dtp = CETOTN(cep);
	csp->scred = scred;
	csp->vcp = NULL;
	error = tree_node_lookupbin(dtp, smb_ce_cmpconn, csp, &tp);
	if (error == 0)
		*scpp = TNTOCN(tp);
	return error;
}

/*
 * Lock entire conn/share tree
 */
int
smb_ce_lock(smb_rc_t cep, int flags, struct proc *p)
{
#if 0
	struct tnode *tp = CETOTN(cep);

	return tlock(tp, flags | LK_CANRECURSE, p);
#endif
	return 0;
}

void
smb_ce_unlock(smb_rc_t cep, struct proc *p)
{
#if 0
	struct tnode *tp = CETOTN(cep);

	tunlock(tp, 0, p);
#endif
	return;
}

/*
 * Allocate new connection with given name(addr).
 */
int
smb_ce_createconn(struct smb_connspec *csp,
	struct smb_cred *scred, struct smb_conn **scpp)
{
	smb_rc_t cep;
	struct smb_conn *scp;
	struct tnode *dtp, *tp;
	struct compname cn;
	struct proc *p = scred->scr_p;
	int error;

	error = smb_sm_getce(scred, LK_EXCLUSIVE, &cep);
	if (error)
		return error;
	csp->vcspec = NULL;
	error = smb_ce_lookupconn(cep, csp, scred, &scp);
	if (!error) {
		smb_conn_put(scp, scred);
		smb_ce_unlock(cep, p);
		return EEXIST;
	}
	dtp = CETOTN(cep);
	compname_init(&cn, csp->name, 0, p);
	error = tree_node_alloc(dtp->t_tree, &cn, TSMB_CONN | TFL_DIR, NULL, &tp);
	if (error) {
		smb_ce_unlock(cep, p);
		return error;
	}
	scp = TNTOCN(tp);
	cep->ce_conncnt++;
	scp->sc_index = cep->ce_nextconn++;
	scp->sc_paddr = dup_sockaddr(csp->sap, 1);
	scp->sc_timo = SMB_DEFRQTIMO;
	tree_node_addchild(dtp, tp);
	*scpp = scp;
	smb_ce_unlock(cep, p);
	return 0;
}

/*
 * General lookup in the tree.
 */
int
smb_sm_lookup(struct smb_connspec *cnspec, struct smb_vcspec *vcspec,
	struct smb_sharespec *shspec, struct smb_cred *scred,
	struct smb_conn **scpp)
{
	struct smb_conn *scp;
	struct smb_ce *cep;
	int error;

	*scpp = NULL;
	error = smb_sm_getce(scred, LK_SHARED, &cep);
	if (error)
		return error;
	cnspec->vcspec = vcspec;
	if (vcspec)
		vcspec->shspec = shspec;
	error = smb_ce_lookupconn(cep, cnspec, scred, &scp);
	if (error == 0) {
		if (smb_conn_get(scp, scred) != 0) {
			if (cnspec->vcp)
				smb_vc_put(cnspec->vcp, scred);
			if (vcspec && vcspec->ssp)
				smb_share_put(vcspec->ssp, scred->scr_p);
			error = ENOENT;
		} else
			*scpp = scp;
	} else
		error = ENOENT;
	smb_ce_unlock(cep, scred->scr_p);
	return error;
}

/*
 * Connection implementation
 */
static void
smb_conn_free(struct smb_conn *scp)
{
	if (scp == NULL)
		return;
	if (scp->sc_paddr)
		free(scp->sc_paddr, M_SONAME);
	if (scp->sc_laddr)
		free(scp->sc_laddr, M_SONAME);
	if (scp->sc_tolower)
		iconv_close(scp->sc_tolower);
	if (scp->sc_toupper)
		iconv_close(scp->sc_toupper);
	if (scp->sc_tolocal)
		iconv_close(scp->sc_tolocal);
	if (scp->sc_toserver)
		iconv_close(scp->sc_toserver);
	free(scp, M_SMBCONN);
}

int
smb_conn_get(struct smb_conn *scp, struct smb_cred *scred)
{
	struct tnode *tp;
	int error;

	tp = CNTOTN(scp);
	if (TISGONE(tp))
		return EINVAL;
	error = tget(tp, LK_EXCLUSIVE | LK_CANRECURSE, scred->scr_p);
	if (error == ERESTART)
		return EINTR;
	return error;
}

static void
smb_conn_gone(struct smb_conn *scp, struct smb_cred *scred)
{
	struct tnode *tp, *dtp;
	struct smb_ce *cep;
	struct proc *p = scred->scr_p;
	int error;

	tp = CNTOTN(scp);
	dtp = tp->t_parent;
	cep = TNTOCE(dtp);
	error = tget(dtp, LK_EXCLUSIVE, p);
	if (error) {
		SMBERROR("can't lock parent %s", dtp->t_name);
		tunlock(tp, 0, p);
		return;
	}
	tree_node_rmchild(dtp, tp);
	tput(dtp, p);
	tunlock(tp, 0, p);
	tree_node_free(tp);
}

void
smb_conn_put(struct smb_conn *scp, struct smb_cred *scred)
{
	struct tnode *tp;

	tp = CNTOTN(scp);
	if (TISGONE(tp)) {
		SMBERROR("too bad, node %s is already gone\n", tp->t_name);
		return;
	}
	tput(tp, scred->scr_p);
}


void
smb_conn_rele(struct smb_conn *scp, struct smb_cred *scred)
{
	struct tnode *tp;

	tp = CNTOTN(scp);
	if (!TISGONE(tp)) {
		trele(tp);
		if (!TISGONE(tp))
			return;
	}
	return;
}

static int
smb_conn_cmpvc(struct tnode *tp, void *data)
{
	struct smb_vc *vcp = TNTOVC(tp);
	struct smb_vcspec *dp = data;
	int exact = 1;

	if (strcmp(tp->t_name, dp->user) != 0)
		return 1;
	if ((vcp->vc_flags & SMBV_PRIVATE) &&
	    vcp->vc_uid != dp->scred->scr_cred->cr_uid)
		return 1;
	if (dp->owner != SMBM_ANY_OWNER) {
		if (vcp->vc_uid != dp->owner)
			return 1;
	} else
		exact = 0;
	if (dp->group != SMBM_ANY_GROUP) {
		if (vcp->vc_grp != dp->group)
			return 1;
	} else
		exact = 0;

	if (dp->mode & SMBM_EXACT) {
		if (!exact)
			return 1;
		return (dp->mode & SMBM_MASK) == vcp->vc_mode ? 0 : 1;
	}
	if (smb_vc_access(vcp, dp->scred, dp->mode) != 0)
		return 1;
	if (dp->shspec == NULL)
		return 0;
	dp->ssp = NULL;
	return smb_vc_lookupshare(vcp, dp->shspec, dp->scred, &dp->ssp) ? 1 : 0;
}

/*
 * Lookup VC in the connection. VC referenced and locked on return.
 * Connection expected to be locked on entry and will be left locked
 * on exit.
 */
int
smb_conn_lookupvc(struct smb_conn *scp, struct smb_vcspec *vcsp,
	struct smb_cred *scred,	struct smb_vc **vcpp)
{
	struct smb_vc *vcp;
	struct tnode *dtp, *tp;
	int error;

	*vcpp = NULL;
	dtp = CNTOTN(scp);
	vcsp->scred = scred;
	error = tree_node_lookupbin(dtp, smb_conn_cmpvc, vcsp, &tp);
	if (error == 0) {
		vcp = TNTOVC(tp);
		if (smb_vc_get(vcp, scred) != 0)
			error = ENOENT;
		else
			*vcpp = vcp;
	} else
		error = ENOENT;
	return error;
}

int
smb_conn_lock(struct smb_conn *scp, struct proc *p)
{
/*	return tlock(CNTOTN(scp), LK_EXCLUSIVE, p); */
	return 0;
}


int
smb_conn_unlock(struct smb_conn *scp, struct proc *p)
{
	/* return tunlock(CNTOTN(scp), 0, p); */
	return 0;
}


void
smb_conn_timo(struct smb_conn *scp)
{
	struct tnode *tp, *parent;

	parent = CNTOTN(scp);
	SLIST_FOREACH(tp, &parent->t_ddata->tm_children, t_link)
		smb_vc_timo(TNTOVC(tp));
}

#ifndef NetBSD
static int
smb_conn_getinfo(struct smb_conn *scp, struct smb_conn_info *cip)
{
	bzero(cip, sizeof(struct smb_conn_info));
	cip->itype = SMB_INFO_CONN;
	cip->usecount = CNTOTN(scp)->t_usecount;
	cip->flags = scp->sc_flags;
	snprintf(cip->srvname, sizeof(cip->srvname), "%s", CNTOTN(scp)->t_name);
	return 0;
}
#endif

static void
smb_timer(void *arg)
{
	struct smb_conn *scp;
	struct tnode *parent, *tp;
	struct smb_ce *cep;
	struct smb_cred scred;

	smb_makescred(&scred, NULL, NULL);
	if (smb_sm_getce(&scred, LK_SHARED | LK_NOWAIT, &cep) == 0) {
		parent = CETOTN(cep);
		SLIST_FOREACH(tp, &parent->t_ddata->tm_children, t_link) {
			scp = TNTOCN(tp);
			smb_conn_timo(scp);
		}
		smb_ce_unlock(cep, NULL);
	}
#ifndef NetBSD
	smb_timer_handle = timeout(smb_timer, NULL, smb_timer_tick);
#else
	callout_reset(&smb_timer_ch, smb_timer_tick, smb_timer, NULL);
#endif
}

/*
 * Session implementation
 */

/*
 * Allocate VC structure and attach it to the connection.
 * Connection expected to be locked on entry. VC will be returned
 * in locked state.
 */
int
smb_vc_create(struct smb_vcspec *vcspec, char *pass, const char *domain,
	struct smb_conn *scp, struct smb_cred *scred, struct smb_vc **vcpp)
{
	struct tnode *tp, *dtp;
	struct smb_vc *vcp;
	struct compname cn;
	struct proc *p = scred->scr_p;
	struct ucred *cred = scred->scr_cred;
	uid_t uid = vcspec->owner;
	gid_t gid = vcspec->group;
	uid_t realuid = cred->cr_uid;
	int error, isroot;

	isroot = smb_suser(cred) == 0;
	/*
	 * Only superuser can create VCs with different uid and gid
	 */
	if (uid != SMBM_ANY_OWNER && uid != realuid && !isroot)
		return EPERM;
	if (gid != SMBM_ANY_GROUP && !groupmember(gid, cred) && !isroot)
		return EPERM;
	error = smb_conn_lookupvc(scp, vcspec, scred, &vcp);
	if (!error) {
		smb_vc_put(vcp, scred);
		return EEXIST;
	}
	if (uid == SMBM_ANY_OWNER)
		uid = realuid;
	if (gid == SMBM_ANY_GROUP)
		gid = cred->cr_groups[0];
	dtp = CNTOTN(scp);
	compname_init(&cn, vcspec->user, 0, p);
	error = tree_node_alloc(dtp->t_tree, &cn, TSMB_VC | TFL_DIR, NULL, &tp);
	if (error) {
		return error;
	}
	vcp = TNTOVC(tp);
	vcp->vc_id = SMB_UID_UNKNOWN;
	scp->sc_vcnt++;
	vcp->vc_domain = smb_strdup((domain && domain[0]) ? domain : "NODOMAIN");
	if (vcp->vc_domain == NULL) {
		tree_node_free(tp);
		return ENOMEM;
	}
	vcp->vc_uid = uid;
	vcp->vc_grp = gid;
	vcp->vc_pass = smb_strdup(pass);
	vcp->vc_mode = vcspec->rights & SMBM_MASK;
	vcp->vc_flags = vcspec->flags & (SMBV_PRIVATE | SMBV_SINGLESHARE);
	TAILQ_INIT(&vcp->vc_rqlist);
	tree_node_addchild(dtp, tp);
	*vcpp = vcp;
	return error;
}

static void
smb_vc_free(struct smb_vc *vcp)
{
	if (vcp == NULL)
		return;
	if (vcp->vc_pass)
		smb_strfree(vcp->vc_pass);
	if (vcp->vc_domain)
		smb_strfree(vcp->vc_domain);
	free(vcp, M_SMBCONN);
}

int
smb_vc_access(struct smb_vc *vcp, struct smb_cred *scred, mode_t mode)
{
	struct ucred *cred = scred->scr_cred;

	if (smb_suser(cred) == 0 || cred->cr_uid == vcp->vc_uid)
		return 0;
	mode >>= 3;
	if (!groupmember(vcp->vc_grp, cred))
		mode >>= 3;
	return (vcp->vc_mode & mode) == mode ? 0 : EACCES;
}


int
smb_vc_get(struct smb_vc *vcp, struct smb_cred *scred)
{
	struct tnode *tp;
	int error;

	tp = VCTOTN(vcp);
	if (TISGONE(tp))
		return EINVAL;
	error = tget(tp, LK_EXCLUSIVE | LK_CANRECURSE, scred->scr_p);
	if (error == ERESTART)
		return EINTR;
	return error;
}

static void
smb_vc_gone(struct smb_vc *vcp, struct smb_cred *scred)
{
	struct tnode *tp, *dtp;
	struct smb_conn *scp;
	struct proc *p = scred->scr_p;
	int error;

	tp = VCTOTN(vcp);
	dtp = tp->t_parent;
	scp = TNTOCN(dtp);
	smb_vc_disconnect(vcp, scred);
	error = tget(dtp, LK_EXCLUSIVE | LK_CANRECURSE, p);
	if (error) {
		SMBERROR("can't lock to parent %s", dtp->t_name);
		tunlock(tp, 0, p);
		return;
	}
	scp->sc_vcnt--;
	tree_node_rmchild(dtp, tp);
	tput(dtp, p);
	tunlock(tp, 0, p);
	tree_node_free(tp);
}

void
smb_vc_put(struct smb_vc *vcp, struct smb_cred *scred)
{
	tput(VCTOTN(vcp), scred->scr_p);
}

void
smb_vc_rele(struct smb_vc *vcp, struct smb_cred *scred)
{
	struct tnode *tp;

	tp = VCTOTN(vcp);
	if (!TISGONE(tp)) {
		trele(tp);
		if (!TISGONE(tp))
			return;
	}
	return;
}

void
smb_vc_unlock(struct smb_vc *vcp, int flags, struct smb_cred *scred)
{
	tunlock(VCTOTN(vcp), flags, scred->scr_p);
}

static int
smb_vc_cmpshare(struct tnode *tp, void *data)
{
	struct smb_share *ssp = TNTOSS(tp);
	struct smb_sharespec *dp = data;
	int exact = 1;

	if (strcmp(tp->t_name, dp->name) != 0)
		return 1;
	if (dp->owner != SMBM_ANY_OWNER) {
		if (ssp->ss_uid != dp->owner)
			return 1;
	} else
		exact = 0;
	if (dp->group != SMBM_ANY_GROUP) {
		if (ssp->ss_grp != dp->group)
			return 1;
	} else
		exact = 0;

	if (dp->mode & SMBM_EXACT) {
		if (!exact)
			return 1;
		return (dp->mode & SMBM_MASK) == ssp->ss_mode ? 0 : 1;
	}
	if (smb_share_access(ssp, dp->scred, dp->mode) != 0)
		return 1;
	return 0;
}

/*
 * Lookup share in the given VC. Share referenced and locked on return.
 * VC expected to be locked on entry and will be left locked on exit.
 */
int
smb_vc_lookupshare(struct smb_vc *vcp, struct smb_sharespec *data,
	struct smb_cred *scred,	struct smb_share **sspp)
{
	struct smb_share *ssp;
	struct tnode *dtp, *tp;
	int error;

	*sspp = NULL;
	dtp = VCTOTN(vcp);
	data->scred = scred;
	error = tree_node_lookupbin(dtp, smb_vc_cmpshare, data, &tp);
	if (error == 0) {
		ssp = TNTOSS(tp);
		if (smb_share_get(ssp, scred) != 0)
			error = ENOENT;
		else
			*sspp = ssp;
	} else
		error = ENOENT;
	return error;
}

int
smb_vc_connect(struct smb_vc *vcp, struct smb_cred *scred)
{
	struct smb_conn *scp = VCTOCN(vcp);
	struct sockaddr *sap = scp->sc_paddr;
	struct proc *p = scred->scr_p;
	struct smb_tran_desc *tdp;
	int error, type;

	if (vcp->vc_id != SMB_UID_UNKNOWN) {
		SMBERROR("called for already opened connection");
		return EISCONN;
	}
	if (sap == NULL)
		return EDESTADDRREQ;

	type = SMBT_NBTCP;	/* now the only known one */
	switch (type) {
	    case SMBT_NBTCP:
		tdp = &smb_tran_nbtcp_desc;
		break;
	    default:
		return EAFNOSUPPORT;
	}
	error = smb_vc_tranlock(vcp, p);
	if (error)
		return error;
	itry {
		ierror(vcp->vc_tdata != NULL, EINPROGRESS);
		vcp->vc_tdesc = tdp;
		ithrow(SMB_TRAN_CREATE(vcp, p));
		if (scp->sc_laddr) {
			ithrow(SMB_TRAN_BIND(vcp, scp->sc_laddr, p));
		}
		ithrow(SMB_TRAN_CONNECT(vcp, sap, p));
		vcp->vc_flags &= ~SMBV_INVALID;
		ithrow(smb_smb_negotiate(vcp, scred));
		ithrow(smb_smb_ssnsetup(vcp, scred));
	} icatch(error) {
		smb_vc_disconnect(vcp, scred);
		vcp->vc_flags |= SMBV_INVALID;
	} ifinally {
		smb_vc_tranunlock(vcp, p);
	} iendtry;
	return error;
}

/*
 * Destroy VC to server, invalidate shares linked with it.
 * Transport should be locked on entry.
 */
int
smb_vc_disconnect(struct smb_vc *vcp, struct smb_cred *scred)
{
	struct proc *p = scred->scr_p;

	smb_smb_ssnclose(vcp, scred);
	smb_vc_invalidate(vcp);
	if (vcp->vc_tdata != NULL) {
		SMB_TRAN_DISCONNECT(vcp, p);
		SMB_TRAN_DONE(vcp, p);
		vcp->vc_tdata = NULL;
	}
	return 0;
}

int
smb_vc_reconnect(struct smb_vc *vcp)
{
	struct smb_cred *scred = vcp->vc_scred;
	struct tnode *tp, *parent;
	int error;

	error = smb_vc_tranlock(vcp, scred->scr_p);
	if (error)
		return error;
	smb_vc_disconnect(vcp, scred);
	smb_vc_tranunlock(vcp, scred->scr_p);
	error = smb_vc_connect(vcp, scred);
	if (error)
		return error;
	parent = VCTOTN(vcp);
	SLIST_FOREACH(tp, &parent->t_ddata->tm_children, t_link)
		smb_share_connect(TNTOSS(tp), scred);
	return 0;
}

void
smb_vc_invalidate(struct smb_vc *vcp)
{
	struct smb_rq *rqp;
	struct tnode *tp, *parent;

	if (!smb_vc_valid(vcp))
		return;
	vcp->vc_flags |= SMBV_INVALID;
	vcp->vc_id = SMB_UID_UNKNOWN;
	/*
         * Invalidate all outstanding requests for this connection
	 */
	TAILQ_FOREACH(rqp, &vcp->vc_rqlist, sr_link) {
		if (rqp->sr_rp.mb_top)
			continue;
		rqp->sr_flags |= SMBR_RESTART;
	}
	/*
	 * Mark all dependant shares as dead
	 */
	parent = VCTOTN(vcp);
	SLIST_FOREACH(tp, &parent->t_ddata->tm_children, t_link)
		smb_share_invalidate(TNTOSS(tp));
	/*
	 * Interrupt all sleeping (in recv/send) processes
	 */
	SMB_TRAN_INTR(vcp);
}

int
smb_vc_valid(struct smb_vc *vcp)
{
	return (vcp->vc_flags & SMBV_INVALID) == 0 && vcp->vc_tdata != NULL;
}

static char smb_emptypass[] = "";

char*
smb_vc_pass(struct smb_vc *vcp)
{
	if (vcp->vc_pass)
		return vcp->vc_pass;
	return smb_emptypass;
}

#ifndef NetBSD
static int
smb_vc_getinfo(struct smb_vc *vcp, struct smb_vc_info *vip)
{
	bzero(vip, sizeof(struct smb_vc_info));
	vip->itype = SMB_INFO_VC;
	vip->usecount = VCTOTN(vcp)->t_usecount;
	vip->uid = vcp->vc_uid;
	vip->gid = vcp->vc_grp;
	vip->mode = vcp->vc_mode;
	vip->flags = vcp->vc_flags;
	vip->sopt = vcp->vc_sopt;
	bzero(&vip->sopt.sv_skey, sizeof(vip->sopt.sv_skey));
	snprintf(vip->vcname, sizeof(vip->vcname), "%s", VCTOTN(vcp)->t_name);
	return 0;
}
#endif

static void
smb_vc_timo(struct smb_vc *vcp)
{
	if (vcp->vc_locks & SMBV_LOCK)
		return;
	if (vcp->vc_tdata != NULL)
		SMB_TRAN_TIMO(vcp);
}

u_short
smb_vc_nextmid(struct smb_vc *vcp)
{
	u_short r;

	simple_lock(&vcp->vc_tnode->t_interlock);
	r = vcp->vc_mid++;
	simple_unlock(&vcp->vc_tnode->t_interlock);
	return r;
}

int
smb_vc_tranlock(struct smb_vc *vcp, struct proc *p)
{
	u_int *statep = &vcp->vc_locks;
	int slpflag = 0, slptimeo = 0;

	slpflag = PCATCH;
	while (*statep & SMBV_LOCK) {
		if (smb_proc_intr(p))
			return EINTR;
		*statep |= SMBV_WANT;
		(void)tsleep((caddr_t)statep, slpflag | (PZERO - 1),
			"smbtrlk", slptimeo);
		if (slpflag == PCATCH) {
			slpflag = 0;
			slptimeo = 2 * hz;
		}
	}
	*statep |= SMBV_LOCK;
	return 0;
}

int
smb_vc_tranunlock(struct smb_vc *vcp, struct proc *p)
{
	u_int *statep = &vcp->vc_locks;

	if ((*statep & SMBV_LOCK) == 0)
		SMBPANIC("not locked\n");
		*statep &= ~SMBV_LOCK;
	if (*statep & SMBV_WANT) {
		*statep &= ~SMBV_WANT;
		wakeup((caddr_t)statep);
	}
	return 0;
}

int
smb_vc_rqlock(struct smb_vc *vcp, struct proc *p)
{
	u_int *statep = &vcp->vc_locks;
	int slpflag = 0, slptimeo = 0;

	slpflag = PCATCH;
	while (*statep & SMBV_RQLISTLOCK) {
		if (smb_proc_intr(p))
			return EINTR;
		*statep |= SMBV_RQLISTWANT;
		(void)tsleep((caddr_t)statep, slpflag | (PZERO - 1),
			"smbrqlk", slptimeo);
		if (slpflag == PCATCH) {
			slpflag = 0;
			slptimeo = 2 * hz;
		}
	}
	*statep |= SMBV_RQLISTLOCK;
	return 0;
}

int
smb_vc_rqunlock(struct smb_vc *vcp, struct proc *p)
{
	u_int *statep = &vcp->vc_locks;

	if ((*statep & SMBV_RQLISTLOCK) == 0)
		SMBPANIC("not locked\n");
		*statep &= ~SMBV_RQLISTLOCK;
	if (*statep & SMBV_RQLISTWANT) {
		*statep &= ~SMBV_RQLISTWANT;
		wakeup((caddr_t)statep);
	}
	return 0;
}

/*
 * Share implementation
 */
/*
 * Allocate share structure and attach it to the VC
 * Connection expected to be locked on entry. Share will be returned
 * in locked state.
 */
int
smb_share_create(struct smb_vc *vcp, struct smb_sharespec *shspec,
	struct smb_cred *scred, struct smb_share **sspp)
{
	struct tnode *tp, *dtp;
	struct smb_share *ssp;
	struct compname cn;
	struct proc *p = scred->scr_p;
	struct ucred *cred = scred->scr_cred;
	uid_t realuid = cred->cr_uid;
	uid_t uid = shspec->owner;
	gid_t gid = shspec->group;
	int error, isroot;

	isroot = smb_suser(cred) == 0;
	/*
	 * Only superuser can create shares with different uid and gid
	 */
	if (uid != SMBM_ANY_OWNER && uid != realuid && !isroot)
		return EPERM;
	if (gid != SMBM_ANY_GROUP && !groupmember(gid, cred) && !isroot)
		return EPERM;
	error = smb_vc_lookupshare(vcp, shspec, scred, &ssp);
	if (!error) {
		smb_share_put(ssp, scred->scr_p);
		return EEXIST;
	}
	if (uid == SMBM_ANY_OWNER)
		uid = realuid;
	if (gid == SMBM_ANY_GROUP)
		gid = cred->cr_groups[0];
	dtp = VCTOTN(vcp);
	compname_init(&cn, shspec->name, 0, p);
	error = tree_node_alloc(dtp->t_tree, &cn, TSMB_SHARE, NULL, &tp);
	if (error) {
		return error;
	}
	ssp = TNTOSS(tp);
	ssp->ss_type = shspec->stype;
	ssp->ss_id = SMB_TID_UNKNOWN;
	ssp->ss_uid = uid;
	ssp->ss_grp = gid;
	ssp->ss_mode = shspec->rights & SMBM_MASK;
	tree_node_addchild(dtp, tp);
	*sspp = ssp;
	return error;
}

static void
smb_share_free(struct smb_share *ssp)
{
	if (ssp == NULL)
		return;
	if (ssp->ss_pass)
		smb_strfree(ssp->ss_pass);
	free(ssp, M_SMBCONN);
}

static void
smb_share_gone(struct smb_share *ssp, struct smb_cred *scred)
{
	struct tnode *tp, *dtp;
	struct smb_vc *vcp;
	struct proc *p = scred->scr_p;
	int error;

	tp = SSTOTN(ssp);
	dtp = tp->t_parent;
	vcp = TNTOVC(dtp);
	smb_share_detach(ssp, scred);
	error = tget(dtp, LK_EXCLUSIVE, p);
	if (error) {
		SMBERROR("can't lock parent %s\n", dtp->t_name);
		tunlock(tp, 0, p);
		return;
	}
	tree_node_rmchild(dtp, tp);
	tput(dtp, p);
	tunlock(tp, 0, p);
	tree_node_free(tp);
}

int
smb_share_access(struct smb_share *ssp, struct smb_cred *scred, mode_t mode)
{
	struct ucred *cred = scred->scr_cred;

	if (smb_suser(cred) == 0 || cred->cr_uid == ssp->ss_uid)
		return 0;
	mode >>= 3;
	if (!groupmember(ssp->ss_grp, cred))
		mode >>= 3;
	return (ssp->ss_mode & mode) == mode ? 0 : EACCES;
}

int
smb_share_get(struct smb_share *ssp, struct smb_cred *scred)
{
	struct tnode *tp;
	int error;

	tp = SSTOTN(ssp);
	if (TISGONE(tp))
		return EINVAL;
	error = tget(tp, LK_EXCLUSIVE | LK_CANRECURSE, scred->scr_p);
	if (error == ERESTART)
		return EINTR;
	return error;
}

void
smb_share_put(struct smb_share *ssp, struct proc *p)
{
	struct tnode *tp = SSTOTN(ssp);

	tput(tp, p);
}

void
smb_share_unlock(struct smb_share *ssp, int flags, struct smb_cred *scred)
{
	struct tnode *tp;

	tp = SSTOTN(ssp);
	tunlock(tp, flags, scred->scr_p);
}

/*
 * Release share and destroy it if usecount drops to zero.
 */
void
smb_share_rele(struct smb_share *ssp, struct smb_cred *scred)
{
	struct tnode *tp;

	tp = SSTOTN(ssp);
	if (!TISGONE(tp)) {
		trele(tp);
		if (!TISGONE(tp))
			return;
	}
	return;
}

int
smb_share_connect(struct smb_share *ssp, struct smb_cred *scred)
{
	int error;

	error = smb_smb_treeconnect(ssp, scred);
	return error;
}

int
smb_share_detach(struct smb_share *ssp, struct smb_cred *scred)
{
	int error;

	error = smb_smb_treedisconnect(ssp, scred);
	return error;
}

void
smb_share_invalidate(struct smb_share *ssp)
{
	if (!smb_share_valid(ssp))
		return;
	ssp->ss_id = SMB_TID_UNKNOWN;
}

int
smb_share_valid(struct smb_share *ssp)
{
	return ssp->ss_id != SMB_TID_UNKNOWN;
}

char*
smb_share_pass(struct smb_share *ssp)
{
	struct smb_vc *vcp;

	if (ssp->ss_pass)
		return ssp->ss_pass;
	vcp = TNTOVC(SSTOTN(ssp)->t_parent);
	if (vcp->vc_pass)
		return vcp->vc_pass;
	return smb_emptypass;
}

#ifndef NetBSD
static int
smb_share_getinfo(struct smb_share *ssp, struct smb_share_info *sip)
{
	bzero(sip, sizeof(struct smb_share_info));
	sip->itype = SMB_INFO_SHARE;
	sip->usecount = SSTOTN(ssp)->t_usecount;
	sip->id  = ssp->ss_id;
	sip->type= ssp->ss_type;
	sip->uid = ssp->ss_uid;
	sip->gid = ssp->ss_grp;
	sip->mode= ssp->ss_mode;
	sip->flags = ssp->ss_flags;
	snprintf(sip->sname, sizeof(sip->sname), "%s", SSTOTN(ssp)->t_name);
	return 0;
}
#endif

#ifndef NetBSD
/*
 * Dump an entire tree into sysctl call
 */
static int
#if __FreeBSD_version > 500000
smb_sysctl_treedump(SYSCTL_HANDLER_ARGS)
#else
smb_sysctl_treedump SYSCTL_HANDLER_ARGS
#endif
{
	struct proc *p = req->p;
	struct smb_cred scred;
	struct tnode *parent, *tscp, *tvcp, *tssp;
	struct smb_ce *cep;
	struct smb_conn *scp;
	struct smb_vc *vcp;
	struct smb_conn_info sci;
	int error, itype;

	smb_makescred(&scred, p, p->p_ucred);
	error = smb_sm_getce(&scred, LK_SHARED, &cep);
	if (error)
		return error;
	parent = CETOTN(cep);
	SLIST_FOREACH(tscp, &parent->t_ddata->tm_children, t_link) {
		scp = TNTOCN(tscp);
		smb_conn_getinfo(scp, &sci);
		error = SYSCTL_OUT(req, &sci, sizeof(struct smb_conn_info));
		if (error)
			break;
		SLIST_FOREACH(tvcp, &tscp->t_ddata->tm_children, t_link) {
			struct smb_vc_info vci;

			vcp = TNTOVC(tvcp);
			smb_vc_getinfo(vcp, &vci);
			error = SYSCTL_OUT(req, &vci, sizeof(struct smb_vc_info));
			if (error)
				break;
			SLIST_FOREACH(tssp, &tvcp->t_ddata->tm_children, t_link) {
				struct smb_share_info ssi;

				smb_share_getinfo(TNTOSS(tssp), &ssi);
				error = SYSCTL_OUT(req, &ssi, sizeof(struct smb_share_info));
				if (error)
					break;
			}
			if (error)
				break;
		}
		if (error)
			break;
	}
	if (!error) {
		itype = SMB_INFO_NONE;
		error = SYSCTL_OUT(req, &itype, sizeof(itype));
	}
	smb_ce_unlock(cep, p);
	return(error);
}
#endif
