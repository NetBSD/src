/*	$NetBSD: smb_tran.h,v 1.1 2000/12/07 03:48:11 deberg Exp $	*/

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
 * Known transports
 */

#define	SMBT_NBTCP	1

struct smb_tran_ops;

#ifndef sa_family_t
#define sa_family_t int
#endif

struct smb_tran_desc {
	sa_family_t	tr_type;
	int	(*tr_create)(struct smb_vc *vcp, struct proc *p);
	int	(*tr_done)(struct smb_vc *vcp, struct proc *p);
	int	(*tr_bind)(struct smb_vc *vcp, struct sockaddr *sap, struct proc *p);
	int	(*tr_connect)(struct smb_vc *vcp, struct sockaddr *sap, struct proc *p);
	int	(*tr_disconnect)(struct smb_vc *vcp, struct proc *p);
	int	(*tr_send)(struct smb_vc *vcp, struct mbuf *m0, struct proc *p);
	int	(*tr_recv)(struct smb_vc *vcp, struct mbuf **mpp, struct proc *p);
	void	(*tr_timo)(struct smb_vc *vcp);
	void	(*tr_intr)(struct smb_vc *vcp);
	int	(*tr_bufsz)(struct smb_vc *vcp, int *sndsz, int *rcvsz);
#ifdef notyet
	int	(*tr_poll)(struct smb_vc *vcp, struct proc *p);
	int	(*tr_cmpaddr)(void *addr1, void *addr2);
#endif
	LIST_ENTRY(smb_tran_desc)	tr_link;
};

#define SMB_TRAN_CREATE(vcp,p)		(vcp)->vc_tdesc->tr_create(vcp,p)
#define SMB_TRAN_DONE(vcp,p)		(vcp)->vc_tdesc->tr_done(vcp,p)
#define	SMB_TRAN_BIND(vcp,sap,p)	(vcp)->vc_tdesc->tr_bind(vcp,sap,p)
#define	SMB_TRAN_CONNECT(vcp,sap,p)	(vcp)->vc_tdesc->tr_connect(vcp,sap,p)
#define	SMB_TRAN_DISCONNECT(vcp,p)	(vcp)->vc_tdesc->tr_disconnect(vcp,p)
#define	SMB_TRAN_SEND(vcp,m0,p)		(vcp)->vc_tdesc->tr_send(vcp,m0,p)
#define	SMB_TRAN_RECV(vcp,m,p)		(vcp)->vc_tdesc->tr_recv(vcp,m,p)
#define	SMB_TRAN_TIMO(vcp)		(vcp)->vc_tdesc->tr_timo(vcp)
#define	SMB_TRAN_INTR(vcp)		(vcp)->vc_tdesc->tr_intr(vcp)
#define	SMB_TRAN_BUFSZ(vcp,sndsz,rcvsz)	(vcp)->vc_tdesc->tr_bufsz(vcp,sndsz,rcvsz)
/*
int  smb_tran_create(int type);
void smb_tran_free(struct smb_vc *vcp);
int  smb_tran_bind(struct smb_vc *vcp, struct sockaddr *sap, struct proc *p);
int  smb_tran_connect(struct smb_vc *vcp, struct sockaddr *sap, struct proc *p);
int  smb_tran_disconnect(struct smb_vc *vcp, struct proc *p);
int  smb_tran_send(struct smb_vc *vcp, struct mbuf *m0, struct proc *p);
int  smb_tran_recv(struct smb_vc *vcp, struct mbuf **mpp, struct proc *p);
void smb_tran_timo(struct smb_vc *vcp);
*/
#ifdef notyet
int  smb_tran_poll(struct smb_vc *vcp, ...);
int  smb_tran_cmpaddr(...);
#endif
