/*	$NetBSD: smb_smb.c,v 1.1.2.2 2001/01/08 14:58:11 bouyer Exp $	*/

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

/*
 * various SMB requests. Most of the routines merely packs data into mbufs.
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
#include <sys/uio.h>

#include <sys/tree.h>

#include "iconv.h"

#include <netsmb/smb.h>
#include <netsmb/smb_rq.h>
#include <netsmb/smb_conn.h>
#include <netsmb/smb_subr.h>
#include <netsmb/smb_tran.h>

struct smb_dialect {
	int		d_id;
	const char *	d_name;
};

static struct smb_dialect smb_dialects[] = {
	{SMB_DIALECT_CORE,	"PC NETWORK PROGRAM 1.0"},
	{SMB_DIALECT_COREPLUS,	"MICROSOFT NETWORKS 1.03"},
	{SMB_DIALECT_LANMAN1_0,	"MICROSOFT NETWORKS 3.0"},
	{SMB_DIALECT_LANMAN1_0,	"LANMAN1.0"},
	{SMB_DIALECT_LANMAN2_0,	"LM1.2X002"},
	{SMB_DIALECT_LANMAN2_0,	"Samba"},
	{SMB_DIALECT_NTLM0_12,	"NT LANMAN 1.0"},
	{SMB_DIALECT_NTLM0_12,	"NT LM 0.12"},
	{-1,			NULL}
};

#define	SMB_DIALECT_MAX	(sizeof(smb_dialects) / sizeof(struct smb_dialect) - 2)

int
smb_smb_negotiate(struct smb_vc *vcp, struct smb_cred *scred)
{
	struct smb_conn *scp = VCTOCN(vcp);
	struct smb_dialect *dp;
	struct smb_sopt *sp = NULL;
	struct smb_rq rq, *rqp = &rq;
	struct mbdata *mbp;
	u_int8_t wc, stime[8], sblen;
	u_int16_t dindex, tw, tw1, swlen, bc;
	int error, maxqsz;

	vcp->vc_hflags = 0;
	vcp->vc_hflags2 = 0;
	vcp->vc_flags &= ~(SMBV_ENCRYPT);
	sp = &vcp->vc_sopt;
	bzero(sp, sizeof(struct smb_sopt));
	error = smb_rq_init(rqp, VCTOTN(vcp), SMB_COM_NEGOTIATE, scred);
	if (error)
		return error;
	smb_rq_getrequest(rqp, &mbp);
	smb_rq_wstart(rqp);
	smb_rq_wend(rqp);
	smb_rq_bstart(rqp);
	for(dp = smb_dialects; dp->d_id != -1; dp++) {
		mb_put_byte(mbp, SMB_DT_DIALECT);
		smb_put_dstring(mbp, scp, dp->d_name, SMB_CS_NONE);
	}
	smb_rq_bend(rqp);
	error = smb_rq_simple(rqp);
	SMBSDEBUG("%d\n", error);
	if (error)
		goto bad;
	smb_rq_getreply(rqp, &mbp);
	do {
		error = mb_get_byte(mbp, &wc);
		if (error)
			break;
		error = mb_get_wordle(mbp, &dindex);
		if (error)
			break;
		if (dindex > 7) {
			SMBERROR("Don't know how to talk with server %s (%d)\n", "xxx", dindex);
			error = EBADRPC;
			break;
		}
		dp = smb_dialects + dindex;
		sp->sv_proto = dp->d_id;
		SMBSDEBUG("Dialect %s (%d, %d)\n", dp->d_name, dindex, wc);
		error = EBADRPC;
		if (dp->d_id >= SMB_DIALECT_NTLM0_12) {
			if (wc != 17)
				break;
			mb_get_byte(mbp, &sp->sv_sm);
			mb_get_wordle(mbp, &sp->sv_maxmux);
			mb_get_wordle(mbp, &sp->sv_maxvcs);
			mb_get_dwordle(mbp, &sp->sv_maxtx);
			mb_get_dwordle(mbp, &sp->sv_maxraw);
			mb_get_dwordle(mbp, &sp->sv_skey);
			mb_get_dwordle(mbp, &sp->sv_caps);
			mb_get_mem(mbp, stime, 8, MB_MSYSTEM);
			mb_get_wordle(mbp, (u_int16_t*)&sp->sv_tz);
			mb_get_byte(mbp, &sblen);
			if (sblen && (sp->sv_sm & SMB_SM_ENCRYPT)) {
				if (sblen != SMB_MAXCHALLENGELEN) {
					SMBERROR("Unexpected length of security blob (%d)\n", sblen);
					break;
				}
				error = mb_get_word(mbp, &bc);
				if (error)
					break;
				if (sp->sv_caps & SMB_CAP_EXT_SECURITY)
					mb_get_mem(mbp, NULL, 16, MB_MSYSTEM);
				error = mb_get_mem(mbp, vcp->vc_ch, sblen, MB_MSYSTEM);
				if (error)
					break;
				vcp->vc_chlen = sblen;
				vcp->vc_flags |= SMBV_ENCRYPT;
			}
			vcp->vc_hflags2 |= SMB_FLAGS2_KNOWS_LONG_NAMES;
			if (dp->d_id == SMB_DIALECT_NTLM0_12 &&
			    sp->sv_maxtx < 4096 &&
			    (sp->sv_caps & SMB_CAP_NT_SMBS) == 0) {
				vcp->vc_flags |= SMBV_WIN95;
				SMBSDEBUG("Win95 detected\n");
			}
		} else if (dp->d_id > SMB_DIALECT_CORE) {
			mb_get_wordle(mbp, &tw);
			sp->sv_sm = tw;
			mb_get_wordle(mbp, &tw);
			sp->sv_maxtx = tw;
			mb_get_wordle(mbp, &sp->sv_maxmux);
			mb_get_wordle(mbp, &sp->sv_maxvcs);
			mb_get_wordle(mbp, &tw);	/* rawmode */
			mb_get_dwordle(mbp, &sp->sv_skey);
			if (wc == 13) {		/* >= LANMAN1 */
				mb_get_word(mbp, &tw);		/* time */
				mb_get_word(mbp, &tw1);		/* date */
				mb_get_wordle(mbp, (u_int16_t*)&sp->sv_tz);
				mb_get_wordle(mbp, &swlen);
				if (swlen > SMB_MAXCHALLENGELEN)
					break;
				mb_get_wordle(mbp, NULL);	/* mbz */
				if (mb_get_word(mbp, &bc) != 0)
					break;
				if (bc < swlen)
					break;
				if (swlen && (sp->sv_sm & SMB_SM_ENCRYPT)) {
					error = mb_get_mem(mbp, vcp->vc_ch, swlen, MB_MSYSTEM);
					if (error)
						break;
					vcp->vc_chlen = swlen;
					vcp->vc_flags |= SMBV_ENCRYPT;
				}
			}
			vcp->vc_hflags2 |= SMB_FLAGS2_KNOWS_LONG_NAMES;
		} else {	/* an old CORE protocol */
			sp->sv_maxmux = 1;
		}
		error = 0;
	} while (0);
	if (error == 0) {
		scp->sc_maxvcs = sp->sv_maxvcs;
		if (scp->sc_maxvcs <= 1) {
			if (scp->sc_maxvcs == 0)
				scp->sc_maxvcs = 1;
		}
		if (sp->sv_maxtx <= 0 || sp->sv_maxtx > 0xffff)
			sp->sv_maxtx = 1024;
		SMB_TRAN_BUFSZ(vcp, &maxqsz, NULL);
		vcp->vc_txmax = min(sp->sv_maxtx, maxqsz);
		SMBSDEBUG("TZ = %d\n", sp->sv_tz);
		SMBSDEBUG("CAPS = %x\n", sp->sv_caps);
		SMBSDEBUG("MAXMUX = %d\n", sp->sv_maxmux);
		SMBSDEBUG("MAXVCS = %d\n", sp->sv_maxvcs);
		SMBSDEBUG("MAXRAW = %d\n", sp->sv_maxraw);
		SMBSDEBUG("MAXTX = %d\n", sp->sv_maxtx);
	}
bad:
	smb_rq_done(rqp);
	return error;
}

int
smb_smb_ssnsetup(struct smb_vc *vcp, struct smb_cred *scred)
{
	struct smb_conn *scp = TNTOCN(vcp->vc_tnode->t_parent);
	struct smb_rq rq, *rqp = &rq;
	struct mbdata *mbp;
/*	u_int8_t wc;
	u_int16_t tw, tw1;*/
	smb_uniptr unipp, ntencpass = NULL;
	char *pp, *up;
	char pbuf[SMB_MAXPASSWORDLEN + 1];
	char encpass[24];
	int error, plen, uniplen, ulen;

	vcp->vc_id = SMB_UID_UNKNOWN;
	vcp->vc_number = scp->sc_vcnext++;

	error = smb_rq_init(rqp, VCTOTN(vcp), SMB_COM_SESSION_SETUP_ANDX, scred);
	if (error)
		return error;
	if (vcp->vc_sopt.sv_sm & SMB_SM_USER) {
		iconv_convstr(scp->sc_toupper, pbuf, smb_vc_pass(vcp));
		iconv_convstr(scp->sc_toserver, pbuf, pbuf);
		if (vcp->vc_sopt.sv_sm & SMB_SM_ENCRYPT) {
			uniplen = plen = 24;
			smb_encrypt(pbuf, vcp->vc_ch, encpass);
			ntencpass = malloc(uniplen, M_SMBTEMP, M_WAITOK);
			if (ntencpass == NULL) {
				error = ENOMEM;
				goto bad;
			}
			iconv_convstr(scp->sc_toserver, pbuf, smb_vc_pass(vcp));
			smb_ntencrypt(pbuf, vcp->vc_ch, (u_char*)ntencpass);
			pp = encpass;
			unipp = ntencpass;
		} else {
			plen = strlen(pbuf) + 1;
			pp = pbuf;
			uniplen = plen * 2;
			ntencpass = malloc(uniplen, M_SMBTEMP, M_WAITOK);
			if (ntencpass == NULL) {
				error = ENOMEM;
				goto bad;
			}
			smb_strtouni(ntencpass, smb_vc_pass(vcp));
			plen--;
			uniplen = 0/*-= 2*/;
			unipp = ntencpass;
		}
	} else {
		/*
		 * In the share security mode password will be used
		 * only in the tree authentication
		 */
		 pp = "";
		 plen = 1;
		 unipp = &smb_unieol;
		 uniplen = sizeof(smb_unieol);
	}
	smb_rq_wstart(rqp);
	mbp = &rqp->sr_rq;
	up = VCTOTN(vcp)->t_name;
	ulen = strlen(up) + 1;
	mb_put_byte(mbp, 0xff);
	mb_put_byte(mbp, 0);
	mb_put_wordle(mbp, 0);
	mb_put_wordle(mbp, vcp->vc_sopt.sv_maxtx);
	mb_put_wordle(mbp, vcp->vc_sopt.sv_maxmux);
	mb_put_wordle(mbp, vcp->vc_number);
	mb_put_dwordle(mbp, vcp->vc_sopt.sv_skey);
	mb_put_wordle(mbp, plen);
	if (SMB_DIALECT(vcp) < SMB_DIALECT_NTLM0_12) {
		mb_put_dwordle(mbp, 0);
		smb_rq_wend(rqp);
		smb_rq_bstart(rqp);
		mb_put_mem(mbp, pp, plen, MB_MSYSTEM);
		smb_put_dstring(mbp, scp, up, SMB_CS_NONE);
	} else {
		mb_put_wordle(mbp, uniplen);
		mb_put_dwordle(mbp, 0);		/* reserved */
		mb_put_dwordle(mbp, 0);		/* my caps */
		smb_rq_wend(rqp);
		smb_rq_bstart(rqp);
		mb_put_mem(mbp, pp, plen, MB_MSYSTEM);
		mb_put_mem(mbp, (caddr_t)unipp, uniplen, MB_MSYSTEM);
		smb_put_dstring(mbp, scp, up, SMB_CS_NONE);		/* AccountName */
		smb_put_dstring(mbp, scp, vcp->vc_domain, SMB_CS_NONE);	/* PrimaryDomain */
#ifndef NetBSD
		smb_put_dstring(mbp, scp, "FreeBSD", SMB_CS_NONE);	/* Client's OS */
#else
		smb_put_dstring(mbp, scp, "NetBSD", SMB_CS_NONE);	/* Client's OS */
#endif
		smb_put_dstring(mbp, scp, "NETSMB", SMB_CS_NONE);		/* Client name */
	}
	smb_rq_bend(rqp);
	if (ntencpass)
		free(ntencpass, M_SMBTEMP);
	error = smb_rq_simple(rqp);
	SMBSDEBUG("%d\n", error);
	if (error) {
		if (rqp->sr_errclass == ERRDOS && rqp->sr_serror == ERRnoaccess)
			error = EAUTH;
		goto bad;
	}
	vcp->vc_id = rqp->sr_rpuid;
	mbp = &rqp->sr_rp;
bad:
	smb_rq_done(rqp);
	return error;
}

int
smb_smb_ssnclose(struct smb_vc *vcp, struct smb_cred *scred)
{
	struct smb_rq rq, *rqp = &rq;
	struct mbdata *mbp;
	int error;

	if (vcp->vc_id == SMB_UID_UNKNOWN || !smb_vc_valid(vcp))
		return 0;
	error = smb_rq_init(rqp, VCTOTN(vcp), SMB_COM_LOGOFF_ANDX, scred);
	if (error)
		return error;
	mbp = &rqp->sr_rq;
	smb_rq_wstart(rqp);
	mb_put_byte(mbp, 0xff);
	mb_put_byte(mbp, 0);
	mb_put_wordle(mbp, 0);
	smb_rq_wend(rqp);
	smb_rq_bstart(rqp);
	smb_rq_bend(rqp);
	error = smb_rq_simple(rqp);
	SMBSDEBUG("%d\n", error);
	smb_rq_done(rqp);
	vcp->vc_id = SMB_UID_UNKNOWN;
	return error;
}

static char smb_any_share[] = "?????";

static char *
smb_share_typename(int stype)
{
	char *pp;

	switch (stype) {
	    case SMB_ST_DISK:
		pp = "A:";
		break;
	    case SMB_ST_PRINTER:
		pp = smb_any_share;		/* can't use LPT: here... */
		break;
	    case SMB_ST_PIPE:
		pp = "IPC";
		break;
	    case SMB_ST_COMM:
		pp = "COMM";
		break;
	    case SMB_ST_ANY:
	    default:
		pp = smb_any_share;
		break;
	}
	return pp;
}

int
smb_smb_treeconnect(struct smb_share *ssp, struct smb_cred *scred)
{
	struct smb_conn *scp;
	struct smb_vc *vcp;
	struct smb_rq rq, *rqp = &rq;
	struct mbdata *mbp;
	char *pp;
	char pbuf[SMB_MAXPASSWORDLEN + 1];
	char encpass[24];
	int error, plen, caseopt;

	ssp->ss_id = SMB_TID_UNKNOWN;
	error = smb_rq_init(rqp, SSTOTN(ssp), SMB_COM_TREE_CONNECT_ANDX, scred);
	if (error)
		return error;
	scp = rqp->sr_conn;
	vcp = rqp->sr_vc;
	caseopt = SMB_CS_NONE;
	if (vcp->vc_sopt.sv_sm & SMB_SM_USER) {
		plen = 1;
		pp = "";
	} else {
		iconv_convstr(scp->sc_toupper, pbuf, smb_share_pass(ssp));
		iconv_convstr(scp->sc_toserver, pbuf, pbuf);
		if (vcp->vc_sopt.sv_sm & SMB_SM_ENCRYPT) {
			plen = 24;
			smb_encrypt(pbuf, vcp->vc_ch, encpass);
			pp = encpass;
		} else {
			plen = strlen(pbuf) + 1;
			pp = pbuf;
		}
	}
	mbp = &rqp->sr_rq;
	smb_rq_wstart(rqp);
	mb_put_byte(mbp, 0xff);
	mb_put_byte(mbp, 0);
	mb_put_wordle(mbp, 0);
	mb_put_wordle(mbp, 0);		/* Flags */
	mb_put_wordle(mbp, plen);
	smb_rq_wend(rqp);
	smb_rq_bstart(rqp);
	mb_put_mem(mbp, pp, plen, MB_MSYSTEM);
	smb_put_dmem(mbp, scp, "\\\\", 2, caseopt);
	pp = scp->sc_tnode->t_name;
	smb_put_dmem(mbp, scp, pp, strlen(pp), caseopt);
	smb_put_dmem(mbp, scp, "\\", 1, caseopt);
	pp = ssp->ss_tnode->t_name;
	smb_put_dstring(mbp, scp, pp, caseopt);
	pp = smb_share_typename(ssp->ss_type);
	smb_put_dstring(mbp, scp, pp, caseopt);
	smb_rq_bend(rqp);
	error = smb_rq_simple(rqp);
	SMBSDEBUG("%d\n", error);
	if (error)
		goto bad;
	ssp->ss_id = rqp->sr_rptid;
bad:
	smb_rq_done(rqp);
	return error;
}

int
smb_smb_treedisconnect(struct smb_share *ssp, struct smb_cred *scred)
{
	struct smb_rq rq, *rqp = &rq;
	struct mbdata *mbp;
	int error;

	if (ssp->ss_id == SMB_TID_UNKNOWN)
		return 0;
	error = smb_rq_init(rqp, SSTOTN(ssp), SMB_COM_TREE_DISCONNECT, scred);
	if (error)
		return error;
	mbp = &rqp->sr_rq;
	smb_rq_wstart(rqp);
	smb_rq_wend(rqp);
	smb_rq_bstart(rqp);
	smb_rq_bend(rqp);
	error = smb_rq_simple(rqp);
	SMBSDEBUG("%d\n", error);
	smb_rq_done(rqp);
	ssp->ss_id = SMB_TID_UNKNOWN;
	return error;
}

static __inline int
smb_smb_read(struct smb_share *ssp, u_int16_t fid,
	int *len, int *rresid, struct uio *uio, struct smb_cred *scred)
{
	struct smb_rq rq, *rqp = &rq;
	struct mbdata *mbp;
	u_int16_t resid, bc;
	u_int8_t wc;
	int error, rlen, blksz;

	error = smb_rq_init(rqp, SSTOTN(ssp), SMB_COM_READ, scred);
	if (error)
		return error;

	blksz = SSTOVC(ssp)->vc_txmax - SMB_HDRLEN - 16;
	rlen = *len = min(blksz, *len);

	smb_rq_getrequest(rqp, &mbp);
	smb_rq_wstart(rqp);
	mb_put_mem(mbp, (caddr_t)&fid, sizeof(fid), MB_MSYSTEM);
	mb_put_wordle(mbp, rlen);
	mb_put_dwordle(mbp, uio->uio_offset);
	mb_put_wordle(mbp, min(uio->uio_resid, 0xffff));
	smb_rq_wend(rqp);
	smb_rq_bstart(rqp);
	smb_rq_bend(rqp);
	do {
		error = smb_rq_simple(rqp);
		if (error)
			break;
		smb_rq_getreply(rqp, &mbp);
		mb_get_byte(mbp, &wc);
		if (wc != 5) {
			error = EBADRPC;
			break;
		}
		mb_get_wordle(mbp, &resid);
		mb_get_mem(mbp, NULL, 4 * 2, MB_MSYSTEM);
		mb_get_wordle(mbp, &bc);
		mb_get_byte(mbp, NULL);		/* ignore buffer type */
		mb_get_wordle(mbp, &resid);
		if (resid == 0) {
			*rresid = resid;
			break;
		}
		error = mb_mbtouio(mbp, uio, resid);
		if (error)
			break;
		*rresid = resid;
	} while(0);
	smb_rq_done(rqp);
	return error;
}

int
smb_read(struct smb_share *ssp, u_int16_t fid, struct uio *uio,
	struct smb_cred *scred)
{
	int tsize, len, resid;
	int error = 0;

	tsize = uio->uio_resid;
	while (tsize > 0) {
		len = tsize;
		error = smb_smb_read(ssp, fid, &len, &resid, uio, scred);
		if (error)
			break;
		tsize -= resid;
		if (resid < len)
			break;
	}
	return error;
}

static __inline int
smb_smb_write(struct smb_share *ssp, u_int16_t fid, int *len, int *rresid,
	struct uio *uio, struct smb_cred *scred)
{
	struct smb_rq rq, *rqp = &rq;
	struct mbdata *mbp;
	u_int16_t resid;
	u_int8_t wc;
	int error, blksz;

	blksz = SSTOVC(ssp)->vc_txmax - SMB_HDRLEN - 16;
	if (blksz > 0xffff)
		blksz = 0xffff;

	resid = *len = min(blksz, *len);

	error = smb_rq_init(rqp, SSTOTN(ssp), SMB_COM_WRITE, scred);
	if (error)
		return error;
	smb_rq_getrequest(rqp, &mbp);
	smb_rq_wstart(rqp);
	mb_put_mem(mbp, (caddr_t)&fid, sizeof(fid), MB_MSYSTEM);
	mb_put_wordle(mbp, resid);
	mb_put_dwordle(mbp, uio->uio_offset);
	mb_put_wordle(mbp, min(uio->uio_resid, 0xffff));
	smb_rq_wend(rqp);
	smb_rq_bstart(rqp);
	mb_put_byte(mbp, SMB_DT_DATA);
	mb_put_wordle(mbp, resid);
	do {
		error = mb_uiotomb(mbp, uio, resid);
		if (error)
			break;
		smb_rq_bend(rqp);
		error = smb_rq_simple(rqp);
		if (error)
			break;
		smb_rq_getreply(rqp, &mbp);
		mb_get_byte(mbp, &wc);
		if (wc != 1) {
			error = EBADRPC;
			break;
		}
		mb_get_wordle(mbp, &resid);
		*rresid = resid;
	} while(0);
	smb_rq_done(rqp);
	return error;
}

int
smb_write(struct smb_share *ssp, u_int16_t fid, struct uio *uio,
	struct smb_cred *scred)
{
	int error = 0, len, tsize, resid;
	struct uio olduio;

	/*
	 * review: manage iov more precisely
	 */
	if (uio->uio_iovcnt != 1) {
		SMBERROR("can't handle iovcnt > 1\n");
		return EIO;
	}
	tsize = uio->uio_resid;
	olduio = *uio;
	while (tsize > 0) {
		len = tsize;
		error = smb_smb_write(ssp, fid, &len, &resid, uio, scred);
		if (error)
			break;
		if (resid < len) {
			error = EIO;
			break;
		}
		tsize -= resid;
	}
	if (error) {
		*uio = olduio;
	}
	return error;
}
