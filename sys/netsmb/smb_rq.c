/*	$NetBSD: smb_rq.c,v 1.1 2000/12/07 03:48:10 deberg Exp $	*/

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
#include <sys/mbuf.h>

#include <sys/subr_mbuf.h>
#include <sys/tree.h>

#include <netsmb/smb.h>
#include <netsmb/smb_conn.h>
#include <netsmb/smb_rq.h>
#include <netsmb/smb_subr.h>
#include <netsmb/smb_tran.h>

#ifndef NetBSD
MALLOC_DEFINE(M_SMBRQ, "SMBRQ", "SMB request");
#endif

static int  smb_rq_recv(struct smb_rq *arqp);
static int  smb_rq_send(struct smb_rq *rqp);
static int  smb_rq_reply(struct smb_rq *rqp);
static int  smb_rq_add(struct smb_rq *rqp);
static void smb_rq_remove(struct smb_rq *rqp);
static int  smb_rq_rcvlock(struct smb_rq *rqp);
static int  smb_rq_rcvunlock(struct smb_rq *rqp);

static int  smb_rq_getenv(struct tnode *layer, struct smb_conn **scpp,
		struct smb_vc **vcpp, struct smb_share **sspp);
static int  smb_rq_new(struct smb_rq *rqp, u_char cmd);
static int  smb_t2_reply(struct smb_t2rq *t2p);

int
smb_rq_alloc(struct tnode *layer, u_char cmd, struct smb_cred *scred,
	struct smb_rq **rqpp)
{
	struct smb_rq *rqp;
	int error;

	MALLOC(rqp, struct smb_rq *, sizeof(*rqp), M_SMBRQ, M_WAITOK);
	if (rqp == NULL)
		return ENOMEM;
	error = smb_rq_init(rqp, layer, cmd, scred);
	rqp->sr_flags |= SMBR_ALLOCED;
	if (error) {
		smb_rq_done(rqp);
		return error;
	}
	*rqpp = rqp;
	return 0;
}

static char tzero[12];

int
smb_rq_init(struct smb_rq *rqp, struct tnode *layer, u_char cmd,
	struct smb_cred *scred)
{
	int error;

	bzero(rqp, sizeof(*rqp));
	simple_lock_init(&rqp->sr_slock);
	error = smb_rq_getenv(layer, &rqp->sr_conn, &rqp->sr_vc, &rqp->sr_share);
	if (error)
		return error;
	error = smb_vc_access(rqp->sr_vc, scred, SMBM_EXEC);
	if (error)
		return error;
	if (rqp->sr_share) {
		error = smb_share_access(rqp->sr_share, scred, SMBM_EXEC);
		if (error)
			return error;
	}
	rqp->sr_cred = scred;
	rqp->sr_mid = smb_vc_nextmid(rqp->sr_vc);
	return smb_rq_new(rqp, cmd);
}

static int
smb_rq_new(struct smb_rq *rqp, u_char cmd)
{
	struct smb_vc *vcp = rqp->sr_vc;
	struct mbdata *mbp = &rqp->sr_rq;
	int error;

	mb_done(mbp);
	mb_done(&rqp->sr_rp);
	error = mb_init(mbp);
	if (error)
		return error;
	mb_put_mem(mbp, SMB_SIGNATURE, SMB_SIGLEN, MB_MSYSTEM);
	mb_put_byte(mbp, cmd);
	mb_put_dwordle(mbp, 0);		/* DosError */
	mb_put_byte(mbp, vcp->vc_hflags);
	mb_put_wordle(mbp, vcp->vc_hflags2);
	mb_put_mem(mbp, tzero, 12, MB_MSYSTEM);
	rqp->sr_rqtid = (u_int16_t*)mb_fit(mbp, sizeof(u_int16_t));
	mb_put_wordle(mbp, 1 /*scred->sc_p->p_pid & 0xffff*/);
	rqp->sr_rquid = (u_int16_t*)mb_fit(mbp, sizeof(u_int16_t));
	mb_put_wordle(mbp, rqp->sr_mid);
	return 0;
}

void
smb_rq_done(struct smb_rq *rqp)
{
	mb_done(&rqp->sr_rq);
	mb_done(&rqp->sr_rp);
	if (rqp->sr_flags & SMBR_ALLOCED)
		free(rqp, M_SMBRQ);
}

/*
 * Simple request-reply exchange
 */
int
smb_rq_simple(struct smb_rq *rqp)
{
	struct smb_conn *scp = rqp->sr_conn;
	int error = EINVAL, i;

	for (i = 0; i < SMB_MAXRCN; i++) {
		rqp->sr_flags &= ~SMBR_RESTART;
		rqp->sr_timo = scp->sc_timo;
		error = smb_rq_add(rqp);
		if (error)
			return error;
		error = smb_rq_send(rqp);
		if (!error)
			error = smb_rq_reply(rqp);
		smb_rq_remove(rqp);
		if (error == 0)
			break;
		if ((rqp->sr_flags & (SMBR_RESTART | SMBR_NORESTART)) != SMBR_RESTART)
			break;
	}
	return error;
}

static int
smb_rq_add(struct smb_rq *rqp)
{
	struct smb_vc *vcp = rqp->sr_vc;
	struct proc *p = rqp->sr_cred->scr_p;
	int error;

	SMBSDEBUG("M:%d\n", rqp->sr_mid);
	for (;;) {
		error = smb_vc_rqlock(vcp, p);
		if (error)
			return error;
		/*
		 * if connection is marked as invalid and we're the only request,
		 * try to restore it. Otherwise we have to wait.
		 */
		if ((vcp->vc_flags & (SMBV_INVALID | SMBV_RECONNECTING)) == SMBV_INVALID) {
			vcp->vc_flags |= SMBV_RECONNECTING;
			vcp->vc_scred = rqp->sr_cred;
			smb_vc_rqunlock(vcp, p);
			SMBSDEBUG("reconnect\n");
			error = smb_vc_reconnect(vcp);
			vcp->vc_flags &= ~SMBV_RECONNECTING;
			/*
			 * If we're still not connected, just bail out
			 */
			if (error)
				return error;
			continue;
		}
		if (vcp->vc_flags & SMBV_RECONNECTING) {
			if (vcp->vc_scred == rqp->sr_cred)
				break;
		} else if (vcp->vc_maxmux == 0 ||
		    vcp->vc_muxcnt < vcp->vc_maxmux)
			break;
		SMBSDEBUG("sleep\n");
		vcp->vc_muxwant++;
#ifndef NetBSD
		asleep(&vcp->vc_muxwant, PWAIT, "smbmx", 0);
		smb_vc_rqunlock(vcp, p);
		await(PWAIT, 0);
#else
		smb_vc_rqunlock(vcp, p);
		tsleep(&vcp->vc_muxwant, PWAIT, "smbmx", 0);
#endif
	}
	vcp->vc_muxcnt++;
	TAILQ_INSERT_TAIL(&vcp->vc_rqlist, rqp, sr_link);
	smb_vc_rqunlock(vcp, p);
	return 0;
}

static void
smb_rq_remove(struct smb_rq *rqp)
{
	struct smb_vc *vcp = rqp->sr_vc;
	struct proc *p = rqp->sr_cred->scr_p;

	SMBSDEBUG("M:%d\n", rqp->sr_mid);
	if (smb_vc_rqlock(vcp, p) != 0) {
		SMBERROR("can't lock connection");
		return;
	}
	TAILQ_REMOVE(&vcp->vc_rqlist, rqp, sr_link);
	vcp->vc_muxcnt--;
	if (vcp->vc_muxwant) {
		vcp->vc_muxwant--;
		wakeup(&vcp->vc_muxwant);
	}
	smb_vc_rqunlock(vcp, p);
}

void
smb_rq_wstart(struct smb_rq *rqp)
{
	rqp->sr_wcount = mb_fit(&rqp->sr_rq, sizeof(u_int8_t));
	rqp->sr_rq.mb_count = 0;
}

void
smb_rq_wend(struct smb_rq *rqp)
{
	if (rqp->sr_wcount == NULL) {
		SMBERROR("no wcount\n");	/* actually panic */
		return;
	}
	if (rqp->sr_rq.mb_count & 1)
		SMBERROR("odd word count\n");
	*rqp->sr_wcount = rqp->sr_rq.mb_count / 2;
}

void
smb_rq_bstart(struct smb_rq *rqp)
{
	rqp->sr_bcount = (u_short*)mb_fit(&rqp->sr_rq, sizeof(u_short));
	rqp->sr_rq.mb_count = 0;
}

void
smb_rq_bend(struct smb_rq *rqp)
{
	int bcnt;

	if (rqp->sr_bcount == NULL) {
		SMBERROR("no bcount\n");	/* actually panic */
		return;
	}
	bcnt = rqp->sr_rq.mb_count;
	if (bcnt > 0xffff)
		SMBERROR("byte count too large (%d)\n", bcnt);
	*rqp->sr_bcount = bcnt;
}

int
smb_rq_intr(struct smb_rq *rqp)
{
	struct proc *p = rqp->sr_cred->scr_p;

	if (rqp->sr_flags & SMBR_INTR)
		return EINTR;
	return smb_proc_intr(p);
}

int
smb_tran_sndlock(struct smb_rq *rqp)
{
	u_int *statep = &rqp->sr_vc->vc_locks;
	int slpflag = 0, slptimeo = 0;

	slpflag = PCATCH;
	if (rqp->sr_flags & SMBR_RESTART)
		return EINTR;
	while (*statep & SMBV_SNDLOCK) {
		if (smb_rq_intr(rqp))
			return EINTR;
		if (rqp->sr_flags & SMBR_RESTART)
			return EINTR;
		*statep |= SMBV_SNDWANT;
		(void)tsleep((caddr_t)statep, slpflag | (PZERO - 1),
			"smbslck", slptimeo);
		if (slpflag == PCATCH) {
			slpflag = 0;
			slptimeo = 2 * hz;
		}
	}
	*statep |= SMBV_SNDLOCK;
	return 0;
}

int
smb_tran_sndunlock(struct smb_rq *rqp)
{
	u_int *statep = &rqp->sr_vc->vc_locks;

	if ((*statep & SMBV_SNDLOCK) == 0)
		SMBPANIC("not locked\n");
		*statep &= ~SMBV_SNDLOCK;
	if (*statep & SMBV_SNDWANT) {
		*statep &= ~SMBV_SNDWANT;
		wakeup((caddr_t)statep);
	}
	return 0;
}

static void
smb_printrqlist(struct smb_vc *vcp)
{
#if 0
	struct smb_rq *rqp;
	TAILQ_FOREACH(rqp, &scp->sc_rqlist, sr_link) {
		printf("RQ: MID=%d, GOTRESPONCE=%s\n", (u_int)rqp->sr_mid,
		 rqp->sr_rp.mb_top ? "yes" : "no");
	}
#endif
}

static int
smb_rq_rcvlock(struct smb_rq *rqp)
{
	u_int *statep = &rqp->sr_vc->vc_locks;
	int slpflag, slptimeo = 0;

	if (rqp->sr_rp.mb_top)
		return EALREADY;
	if (rqp->sr_flags & SMBR_RESTART)
		return EINTR;
	slpflag = PCATCH;
	while (*statep & SMBV_RCVLOCK) {
		if (smb_rq_intr(rqp))
			return EINTR;
		*statep |= SMBV_RCVWANT;
		(void)tsleep((caddr_t)statep, slpflag | (PZERO - 1), "smbrlck",
			slptimeo);

		if (rqp->sr_rp.mb_top)
			return EALREADY;
		if (rqp->sr_flags & SMBR_RESTART)
			return EINTR;
		if (slpflag == PCATCH) {
			slpflag = 0;
			slptimeo = 2 * hz;
		}
/*
printf("sleep: MID=%d, MUX=%d, NEXTMID=%d\n", (u_int)rqp->sr_mid,
	 rqp->sr_conn->sc_muxcnt, (u_int)rqp->sr_conn->sc_mid);
smb_printrqlist(rqp->sr_conn);
*/
	}
	*statep |= SMBV_RCVLOCK;
	return 0;
}

static int
smb_rq_rcvunlock(struct smb_rq *rqp)
{
	u_int *statep = &rqp->sr_vc->vc_locks;

	if ((*statep & SMBV_RCVLOCK) == 0)
		SMBPANIC("not locked");
	*statep &= ~SMBV_RCVLOCK;
	if (*statep & SMBV_RCVWANT) {
		*statep &= ~SMBV_RCVWANT;
		wakeup((caddr_t)statep);
	}
	return 0;
}

int
smb_rq_getrequest(struct smb_rq *rqp, struct mbdata **mbpp)
{
	*mbpp = &rqp->sr_rq;
	return 0;
}

int
smb_rq_getreply(struct smb_rq *rqp, struct mbdata **mbpp)
{
	*mbpp = &rqp->sr_rp;
	return 0;
}

static int
smb_rq_getenv(struct tnode *layer, struct smb_conn **scpp,
	struct smb_vc **vcpp, struct smb_share **sspp)
{
	struct smb_conn *scp = NULL;
	struct smb_vc *vcp = NULL;
	struct smb_share *ssp = NULL;
	struct tnode *tp;
	int error = 0;

	switch (TTYPE(layer)) {
	    case TSMB_CONN:
		scp = TNTOCN(layer);
		break;
	    case TSMB_VC:
		vcp = TNTOVC(layer);
		tp = layer->t_parent;
		if (tp == NULL) {
			SMBERROR("zombie VC %s\n", layer->t_name);
			error = EINVAL;
			break;
		}
		scp = TNTOCN(tp);
		break;
	    case TSMB_SHARE:
		ssp = TNTOSS(layer);
		tp = layer->t_parent;
		if (tp == NULL) {
			SMBERROR("zombie share %s\n", layer->t_name);
			error = EINVAL;
			break;
		}
		vcp = TNTOVC(tp);
		tp = tp->t_parent;
		if (tp == NULL) {
			SMBERROR("zombie VC %s\n", layer->t_name);
			error = EINVAL;
			break;
		}
		scp = TNTOCN(tp);
		break;
	    default:
		SMBERROR("invalid layer %d passed\n", TTYPE(layer));
		error = EINVAL;
	}
	if (scpp)
		*scpp = scp;
	if (vcpp)
		*vcpp = vcp;
	if (sspp)
		*sspp = ssp;
	return error;
}

/*
 * Wait for reply on the request
 */
static int
smb_rq_reply(struct smb_rq *rqp)
{
	struct mbdata *mbp = &rqp->sr_rp;
	u_int32_t tdw;
	u_int8_t tb;
	int error, rperror = 0;

	error = smb_rq_recv(rqp);
	if (error)
		return error;
	error = mb_get_dword(mbp, &tdw);
	error = mb_get_byte(mbp, &tb);
	if (rqp->sr_vc->vc_hflags2 & SMB_FLAGS2_ERR_STATUS) {
		error = mb_get_dwordle(mbp, &rqp->sr_error);
	} else {
		error = mb_get_byte(mbp, &rqp->sr_errclass);
		error = mb_get_byte(mbp, &tb);
		error = mb_get_wordle(mbp, &rqp->sr_serror);
		if (!error)
			rperror = smb_maperror(rqp->sr_errclass, rqp->sr_serror);
	}
	error = mb_get_byte(mbp, &rqp->sr_rpflags);
	error = mb_get_wordle(mbp, &rqp->sr_rpflags2);

	error = mb_get_dword(mbp, &tdw);
	error = mb_get_dword(mbp, &tdw);
	error = mb_get_dword(mbp, &tdw);

	error = mb_get_wordle(mbp, &rqp->sr_rptid);
	error = mb_get_wordle(mbp, &rqp->sr_rppid);
	error = mb_get_wordle(mbp, &rqp->sr_rpuid);
	error = mb_get_wordle(mbp, &rqp->sr_rpmid);

	SMBSDEBUG("M:%04x, P:%04x, U:%04x, T:%04x, E: %d:%d\n",
	    rqp->sr_rpmid, rqp->sr_rppid, rqp->sr_rpuid, rqp->sr_rptid,
	    rqp->sr_errclass, rqp->sr_serror);
	return error ? error : rperror;
}

/*
 * Check for fatal errors
 */
static __inline int
smb_rq_fatal(int error)
{
	switch (error) {
	    case ENOTCONN:
	    case ENETRESET:
	    case ECONNABORTED:
		return 1;
	}
	return 0;
}

static int
smb_rq_recv(struct smb_rq *arqp)
{
	struct smb_vc *vcp = arqp->sr_vc;
	struct proc *p = arqp->sr_cred->scr_p;
	struct smb_rq *rqp;
	struct mbdata *mbp, mb;
	struct mbuf *m;
	u_char *hp;
	u_short mid;
	int error;

#define rcverr(err) do { error = err; goto exit; }while(0)

	mbp = &mb;
	for (;;) {
		simple_lock(&arqp->sr_slock);
		error = mb_fetch_record(&arqp->sr_rp);
		simple_unlock(&arqp->sr_slock);
		if (!error)
			return 0;
		bzero(mbp, sizeof(struct mbdata));
		error = smb_rq_rcvlock(arqp);
		if (error == EALREADY)
			return 0;
		if (error)
			return error;
		do {
			error = SMB_TRAN_RECV(vcp, &m, p);
			if (error == EWOULDBLOCK && (arqp->sr_flags & SMBR_INTR)) {
				error = EINTR;
				break;
			}
		} while (error == EWOULDBLOCK);
		if (smb_rq_fatal(error)) {
			smb_vc_invalidate(vcp);
			goto unlock;
		}
		if (error)
			goto unlock;
		if (m == NULL) {
			SMBERROR("tran return a NULL without error\n");
			error = EPIPE;
			goto unlock;
		}
		m = m_pullup(m, SMB_HDRLEN);
		if (m == NULL) {
			smb_rq_rcvunlock(arqp);
			continue;	/* wait for a good packet */
		}
		mb_initm(mbp, m);
		/*
		 * Now we got an entire and possibly invalid SMB packet.
		 * Be careful while parsing it.
		 */
		m_dumpm(mbp->mb_top);
		hp = mtod(mbp->mb_top, u_char*);
		if (bcmp(hp, SMB_SIGNATURE, SMB_SIGLEN) != 0)
			rcverr(EBADRPC);
		mid = SMB_HDRMID(hp);
		SMBSDEBUG("mid %04x\n", (u_int)mid);
		smb_vc_rqlock(vcp, p);
		TAILQ_FOREACH(rqp, &vcp->vc_rqlist, sr_link) {
			if (rqp->sr_mid != mid)
				continue;
			if (rqp->sr_rp.mb_top == NULL)
				mb_initm(&rqp->sr_rp, m);
			else {
				simple_lock(&rqp->sr_slock);
				mb_append_record(&rqp->sr_rp, m);
				simple_unlock(&rqp->sr_slock);
			}
			if (rqp == arqp) {
				smb_vc_rqunlock(vcp, p);
				goto unlock;
			}
			break;
		}
		smb_vc_rqunlock(vcp, p);
		if (rqp == NULL) {
			SMBSDEBUG("drop resp with mid %d\n", (u_int)mid);
			smb_printrqlist(vcp);
			mb_done(mbp);
		}
		smb_rq_rcvunlock(arqp);
	}
exit:
	mb_done(mbp);
unlock:
	smb_rq_rcvunlock(arqp);
	return error;
}

static int
smb_rq_send(struct smb_rq *rqp)
{
	struct proc *p = rqp->sr_cred->scr_p;
	struct smb_vc *vcp = rqp->sr_vc;
	struct smb_share *ssp = rqp->sr_share;
	struct mbuf *m;
	int error, rcnt, rmax;

	rmax = 5;		/* TODO: configurable */
	*rqp->sr_rqtid = htoles(ssp ? ssp->ss_id : SMB_TID_UNKNOWN);
	*rqp->sr_rquid = htoles(vcp ? vcp->vc_id : 0);
	mb_fixhdr(&rqp->sr_rq);
	rqp->sr_flags &= ~SMBR_SENT;
	error = ENOTCONN;
	for (rcnt = 0; rcnt < rmax; rcnt++) {
		error = smb_tran_sndlock(rqp);
		if (error)
			return error;
		if (vcp->vc_tdata == NULL) {
			smb_tran_sndunlock(rqp);
			return ENOTCONN;
		}
		SMBSDEBUG("M:%04x, P:%04x, U:%04x, T:%04x\n",
		    rqp->sr_mid, 0, 0, 0);
		m_dumpm(rqp->sr_rq.mb_top);
		m = m_copym(rqp->sr_rq.mb_top, 0, M_COPYALL, M_WAIT);
		error = SMB_TRAN_SEND(vcp, m, p);
		smb_tran_sndunlock(rqp);
		if (error == 0) {
			rqp->sr_flags |= SMBR_SENT;
			return 0;
		}
		/*
		 * Check for fatal errors
		 */
		if (smb_rq_fatal(error)) {
			smb_vc_invalidate(vcp);
			return error;
		}
		/*
		 * These errors are not fatal and can be recovered later
		 */
		if (error == EINTR || error == ENETDOWN || error == EPIPE)
			return error;
		if (smb_rq_intr(rqp))
			return EINTR;
		rqp->sr_rexmit = 1;
		tsleep(&rqp->sr_rexmit, PWAIT | PCATCH, "smbrxm", 5 * hz);
		if (smb_rq_intr(rqp))
			return EINTR;
	}
	return error;
}


#define ALIGN4(a)	(((a) + 3) & ~3)

/*
 * TRANS2 request implementation
 */
int
smb_t2_alloc(struct tnode *layer, u_short setup, struct smb_cred *scred,
	struct smb_t2rq **t2pp)
{
	struct smb_t2rq *t2p;
	int error;

	MALLOC(t2p, struct smb_t2rq *, sizeof(*t2p), M_SMBRQ, M_WAITOK);
	if (t2p == NULL)
		return ENOMEM;
	error = smb_t2_init(t2p, layer, setup, scred);
	t2p->t2_flags |= SMBT2_ALLOCED;
	if (error) {
		smb_t2_done(t2p);
		return error;
	}
	*t2pp = t2p;
	return 0;
}

int
smb_t2_init(struct smb_t2rq *t2p, struct tnode *source, u_short setup,
	struct smb_cred *scred)
{
	int error;

	bzero(t2p, sizeof(*t2p));
	t2p->t2_source = source;
	t2p->t2_setupcount = 1;
	t2p->t2_setupdata = t2p->t2_setup;
	t2p->t2_setup[0] = setup;
	t2p->t2_fid = 0xffff;
	t2p->t2_cred = scred;
	error = smb_rq_getenv(source, &t2p->t2_conn,&t2p->t2_vc, NULL);
	if (error)
		return error;
	return 0;
}

void
smb_t2_done(struct smb_t2rq *t2p)
{
	mb_done(&t2p->t2_tparam);
	mb_done(&t2p->t2_tdata);
	mb_done(&t2p->t2_rparam);
	mb_done(&t2p->t2_rdata);
	if (t2p->t2_flags & SMBT2_ALLOCED)
		free(t2p, M_SMBRQ);
}

static int
smb_t2_reply(struct smb_t2rq *t2p)
{
	struct mbuf *m, *m0;
	struct mbdata *mbp;
	struct smb_rq *rqp = t2p->t2_rq;
	int error, totpgot, totdgot, len;
	u_int16_t totpcount, totdcount, pcount, poff, doff, pdisp, ddisp;
	u_int16_t tmp, bc, dcount;
	u_int8_t wc;

	error = smb_rq_reply(rqp);
	if (error)
		return error;
	if ((t2p->t2_flags & SMBT2_ALLSENT) == 0) {
		/* 
		 * this is an interim response, ignore it.
		 */
		mb_done(&rqp->sr_rp);
		return 0;
	}
	/*
	 * Now we have to get all subseqent responses. The CIFS specification
	 * says that they can be misordered which is weird.
	 * TODO: timo
	 */
	totpgot = totdgot = 0;
	totpcount = totdcount = 0xffff;
	if (mb_init(&t2p->t2_rparam) != 0 || mb_init(&t2p->t2_rdata) != 0)
		return ENOBUFS;
	mbp = &rqp->sr_rp;
	for (;;) {
		m_dumpm(mbp->mb_top);
		mb_get_byte(mbp, &wc);
		mb_get_wordle(mbp, &tmp);
		if (totpcount > tmp)
			totpcount = tmp;
		mb_get_wordle(mbp, &tmp);
		if (totdcount > tmp)
			totdcount = tmp;
		mb_get_wordle(mbp, &tmp);	/* reserved */
		mb_get_wordle(mbp, &pcount);
		mb_get_wordle(mbp, &poff);
		mb_get_wordle(mbp, &pdisp);
		if (pcount != 0 && pdisp != totpgot) {
			SMBERROR("Can't handle misordered parameters %d:%d\n",
			    pdisp, totpgot);
			error = EINVAL;
			break;
		}
		mb_get_wordle(mbp, &dcount);
		mb_get_wordle(mbp, &doff);
		mb_get_wordle(mbp, &ddisp);
		if (dcount != 0 && ddisp != totdgot) {
			SMBERROR("Can't handle misordered data\n");
			error = EINVAL;
			break;
		}
		mb_get_byte(mbp, &wc);
		mb_get_byte(mbp, NULL);
		tmp = wc;
		while (tmp--)
			mb_get_word(mbp, NULL);
		mb_get_wordle(mbp, &bc);
/*		tmp = SMB_HDRLEN + 1 + 10 * 2 + 2 * wc + 2;*/
		error = EBADRPC;
		if (dcount) {
			m0 = m_split(mbp->mb_top, doff, M_WAIT);
			if (m0 == NULL)
				break;
			for(len = 0, m = m0; m->m_next; m = m->m_next)
				len += m->m_len;
			len += m->m_len;
			m->m_len -= len - dcount;
			m_cat(t2p->t2_rdata.mb_top, m0);
		}
		if (pcount) {
			m0 = m_split(mbp->mb_top, poff, M_WAIT);
			if (m0 == NULL)
				break;
			for(len = 0, m = m0; m->m_next; m = m->m_next)
				len += m->m_len;
			len += m->m_len;
			m->m_len -= len - pcount;
			m_cat(t2p->t2_rparam.mb_top, m0);
		}
		totpgot += pcount;
		totdgot += dcount;
		if (totpgot >= totpcount && totdgot >= totdcount) {
			error = 0;
			t2p->t2_flags |= SMBT2_ALLRECV;
			break;
		}
		error = smb_rq_reply(rqp);
		if (error)
			break;
	}
	return error;
}

/*
 * Perform a full round of TRANS2 request
 */
static int
smb_t2_request_int(struct smb_t2rq *t2p)
{
	struct mbdata *mbp;
	struct mbuf *m;
	struct smb_rq rq, *rqp = &rq;
	struct smb_vc *vcp = t2p->t2_vc;
	struct smb_cred *scred = t2p->t2_cred;
	int totpcount, leftpcount, totdcount, leftdcount, len, txmax, i;
	int error, doff, poff, txdcount, txpcount, nmlen;

	mbp = &t2p->t2_tparam;
	if (mbp->mb_top) {
		mb_initm(mbp, mbp->mb_top);
		totpcount = mb_fixhdr(mbp);
		if (totpcount > 0xffff)		/* maxvalue for u_short */
			return EINVAL;
	} else
		totpcount = 0;
	mbp = &t2p->t2_tdata;
	if (mbp->mb_top) {
		mb_initm(mbp, mbp->mb_top);
		totdcount =  mb_fixhdr(mbp);
		if (totdcount > 0xffff)
			return EINVAL;
	} else
		totdcount = 0;
	leftdcount = totdcount;
	leftpcount = totpcount;
	txmax = vcp->vc_txmax;
	error = smb_rq_init(rqp, t2p->t2_source, t2p->t_name ?
	    SMB_COM_TRANSACTION : SMB_COM_TRANSACTION2, scred);
	if (error)
		return error;
	t2p->t2_rq = rqp;
	mbp = &rqp->sr_rq;
	smb_rq_wstart(rqp);
	mb_put_wordle(mbp, totpcount);
	mb_put_wordle(mbp, totdcount);
	mb_put_wordle(mbp, t2p->t2_maxpcount);
	mb_put_wordle(mbp, t2p->t2_maxdcount);
	mb_put_byte(mbp, t2p->t2_maxscount);
	mb_put_byte(mbp, 0);			/* reserved */
	mb_put_wordle(mbp, 0);			/* flags */
	mb_put_dwordle(mbp, 0);			/* Timeout */
	mb_put_wordle(mbp, 0);			/* reserved 2 */
	len = mb_fixhdr(mbp);
	/*
	 * now we have known packet size as
	 * ALIGN4(len + 5 * 2 + setupcount * 2 + 2 + strlen(name) + 1),
	 * and need to decide which parts should go into the first request
	 */
	nmlen = t2p->t_name ? strlen(t2p->t_name) : 0;
	len = ALIGN4(len + 5 * 2 + t2p->t2_setupcount * 2 + 2 + nmlen + 1);
	if (len + leftpcount > txmax) {
		txpcount = min(leftpcount, txmax - len);
		poff = len;
		txdcount = 0;
		doff = 0;
	} else {
		txpcount = leftpcount;
		poff = txpcount ? len : 0;
		len = ALIGN4(len + txpcount);
		txdcount = min(leftdcount, txmax - len);
		doff = txdcount ? len : 0;
	}
	leftpcount -= txpcount;
	leftdcount -= txdcount;
	mb_put_wordle(mbp, txpcount);
	mb_put_wordle(mbp, poff);
	mb_put_wordle(mbp, txdcount);
	mb_put_wordle(mbp, doff);
	mb_put_byte(mbp, t2p->t2_setupcount);
	mb_put_byte(mbp, 0);
	for (i = 0; i < t2p->t2_setupcount; i++)
		mb_put_wordle(mbp, t2p->t2_setupdata[i]);
	smb_rq_wend(rqp);
	smb_rq_bstart(rqp);
	/* TDUNICODE */
	if (t2p->t_name)
		mb_put_mem(mbp, t2p->t_name, nmlen, MB_MSYSTEM);
	mb_put_byte(mbp, 0);	/* terminating zero */
	len = mb_fixhdr(mbp);
	if (txpcount) {
		mb_put_mem(mbp, NULL, ALIGN4(len) - len, MB_MZERO);
		error = mb_get_mbuf(&t2p->t2_tparam, txpcount, &m);
		SMBSDEBUG("%d:%d:%d\n", error, txpcount, txmax);
		if (error)
			goto freerq;
		mb_put_mbuf(mbp, m);
	}
	len = mb_fixhdr(mbp);
	if (txdcount) {
		mb_put_mem(mbp, NULL, ALIGN4(len) - len, MB_MZERO);
		error = mb_get_mbuf(&t2p->t2_tdata, txdcount, &m);
		if (error)
			goto freerq;
		mb_put_mbuf(mbp, m);
	}
	smb_rq_bend(rqp);	/* incredible, but thats it... */
	error = smb_rq_add(rqp);
	if (error)
		goto freerq;
	error = smb_rq_send(rqp);
	if (error)
		goto bad;
	if (leftpcount == 0 && leftdcount == 0)
		t2p->t2_flags |= SMBT2_ALLSENT;
	error = smb_t2_reply(t2p);
	if (error)
		goto bad;
	while (leftpcount || leftdcount) {
		error = smb_rq_new(rqp, t2p->t_name ? 
		    SMB_COM_TRANSACTION_SECONDARY : SMB_COM_TRANSACTION2_SECONDARY);
		if (error)
			goto bad;
		mbp = &rqp->sr_rq;
		smb_rq_wstart(rqp);
		mb_put_wordle(mbp, totpcount);
		mb_put_wordle(mbp, totdcount);
		len = mb_fixhdr(mbp);
		/*
		 * now we have known packet size as
		 * ALIGN4(len + 7 * 2 + 2) for T2 request, and -2 for T one,
		 * and need to decide which parts should go into request
		 */
		len = ALIGN4(len + 6 * 2 + 2);
		if (t2p->t_name == NULL)
			len += 2;
		if (len + leftpcount > txmax) {
			txpcount = min(leftpcount, txmax - len);
			poff = len;
			txdcount = 0;
			doff = 0;
		} else {
			txpcount = leftpcount;
			poff = txpcount ? len : 0;
			len = ALIGN4(len + txpcount);
			txdcount = min(leftdcount, txmax - len);
			doff = txdcount ? len : 0;
		}
		mb_put_wordle(mbp, txpcount);
		mb_put_wordle(mbp, poff);
		mb_put_wordle(mbp, totpcount - leftpcount);
		mb_put_wordle(mbp, txdcount);
		mb_put_wordle(mbp, doff);
		mb_put_wordle(mbp, totdcount - leftdcount);
		leftpcount -= txpcount;
		leftdcount -= txdcount;
		if (t2p->t_name == NULL)
			mb_put_wordle(mbp, t2p->t2_fid);
		smb_rq_wend(rqp);
		smb_rq_bstart(rqp);
		mb_put_byte(mbp, 0);	/* name */
		len = mb_fixhdr(mbp);
		if (txpcount) {
			mb_put_mem(mbp, NULL, ALIGN4(len) - len, MB_MZERO);
			error = mb_get_mbuf(&t2p->t2_tparam, txpcount, &m);
			if (error)
				goto bad;
			mb_put_mbuf(mbp, m);
		}
		len = mb_fixhdr(mbp);
		if (txdcount) {
			mb_put_mem(mbp, NULL, ALIGN4(len) - len, MB_MZERO);
			error = mb_get_mbuf(&t2p->t2_tdata, txdcount, &m);
			if (error)
				goto bad;
			mb_put_mbuf(mbp, m);
		}
		smb_rq_bend(rqp);
		error = smb_rq_send(rqp);
		if (error)
			goto bad;
	}	/* while left params or data */
	t2p->t2_flags |= SMBT2_ALLSENT;
	mbp = &t2p->t2_rdata;
	if (mbp->mb_top) {
		m_fixhdr(mbp->mb_top);
		mb_initm(mbp, mbp->mb_top);
	}
	mbp = &t2p->t2_rparam;
	if (mbp->mb_top) {
		m_fixhdr(mbp->mb_top);
		mb_initm(mbp, mbp->mb_top);
	}
bad:
	smb_rq_remove(rqp);
freerq:
	if (error && (rqp->sr_flags & SMBR_RESTART))
		t2p->t2_flags |= SMBT2_RESTART;
	smb_rq_done(rqp);
	return error;
}

int
smb_t2_request(struct smb_t2rq *t2p)
{
	int error = EINVAL, i;

	for (i = 0; i < SMB_MAXRCN; i++) {
		t2p->t2_flags &= ~SMBR_RESTART;
		error = smb_t2_request_int(t2p);
		if (error == 0)
			break;
		if ((t2p->t2_flags & (SMBT2_RESTART | SMBT2_NORESTART)) != SMBT2_RESTART)
			break;
	}
	return error;
}
