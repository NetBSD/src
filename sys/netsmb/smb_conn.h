/*	$NetBSD: smb_conn.h,v 1.1.2.2 2001/01/08 14:58:06 bouyer Exp $	*/

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

#ifndef _NETINET_IN_H_
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#endif

/*
 * Three levels of connection hierarchy
 */
#define SMBL_CONN	0
#define SMBL_VC		1
#define SMBL_SHARE	2
#define SMBL_NUM	3
#define SMBL_NONE	(-1)

/*
 * access modes
 */
#define	SMBM_READ		0400	/* read conn attrs.(like list shares) */
#define	SMBM_WRITE		0200	/* modify conn attrs */
#define	SMBM_EXEC		0100	/* can send SMB requests */
#define	SMBM_READGRP		0040
#define	SMBM_WRITEGRP		0020
#define	SMBM_EXECGRP		0010
#define	SMBM_READOTH		0004
#define	SMBM_WRITEOTH		0002
#define	SMBM_EXECOTH		0001
#define	SMBM_MASK		0777
#define	SMBM_EXACT		010000	/* check for specified mode exactly */
#define	SMBM_ALL		(SMBM_READ | SMBM_WRITE | SMBM_EXEC)
#define	SMBM_DEFAULT		(SMBM_READ | SMBM_WRITE | SMBM_EXEC)
#define	SMBM_ANY_OWNER		((uid_t)-1)
#define	SMBM_ANY_GROUP		((gid_t)-1)

/*
 * Connection flags
 */
#define SMBC_CREATE		0x0100	/* lookup for create opeartion */


/*
 * VC flags
 */
#define SMBV_INVALID		0x0001
#define SMBV_PERMANENT		0x0002
#define SMBV_LONGNAMES		0x0004	/* connection is configured to use long names */
#define	SMBV_ENCRYPT		0x0008	/* server asked for encrypted password */
#define	SMBV_WIN95		0x0010	/* used to apply bugfixes for this OS */
#define	SMBV_PRIVATE		0x0020	/* connection can be (re)used only by owner */
#define	SMBV_RECONNECTING	0x0040	/* conn is in the process of reconnection */
#define SMBV_SINGLESHARE	0x0080	/* only one share connectin should be allowed */

#define	SMBV_LOCK		0x0400
#define	SMBV_WANT		0x0800
#define	SMBV_SNDLOCK		0x1000
#define	SMBV_SNDWANT		0x2000
#define	SMBV_RCVLOCK		0x4000
#define	SMBV_RCVWANT		0x8000
#define	SMBV_MXLOCK		0x010000
#define	SMBV_MXWANT		0x020000
#define	SMBV_RQLISTLOCK		0x040000
#define	SMBV_RQLISTWANT		0x080000

/*
 * smb_share flags
 */
#define SMBS_PERMANENT		0x0001

/*
 * share types
 */
#define	SMB_ST_DISK		0x0	/* A: */
#define	SMB_ST_PRINTER		0x1	/* LPT: */
#define	SMB_ST_PIPE		0x2	/* IPC */
#define	SMB_ST_COMM		0x3	/* COMM */
#define	SMB_ST_ANY		0x4
#define	SMB_ST_MAX		0x4
#define SMB_ST_NONE		0xff	/* not a part of protocol */

/*
 * Negotiated protocol parameters
 */
struct smb_sopt {
	int		sv_proto;
	int16_t		sv_tz;		/* offset in min relative to UTC */
	u_int32_t	sv_maxtx;	/* maximum transmit buf size */
	u_char		sv_sm;		/* security mode */
	u_int16_t	sv_maxmux;	/* max number of outstanding rq's */
	u_int16_t 	sv_maxvcs;	/* max number of VCs */
	u_int16_t	sv_rawmode;
	u_int32_t	sv_maxraw;	/* maximum raw-buffer size */
	u_int32_t	sv_skey;	/* session key */
	u_int32_t	sv_caps;	/* capabilites SMB_CAP_ */
};

/*
 * Info structures
 */
#define	SMB_INFO_NONE		0
#define	SMB_INFO_CONN		1
#define	SMB_INFO_VC		2
#define	SMB_INFO_SHARE		3

struct smb_conn_info {
	int		itype;
	int		usecount;
	int		flags;
	char		srvname[SMB_MAXSRVNAMELEN];
};

struct smb_vc_info {
	int		itype;
	int		usecount;
	uid_t		uid;		/* user id of connection */
	gid_t		gid;		/* group of connection */
	mode_t		mode;		/* access mode */
	int		flags;
	struct smb_sopt	sopt;
	char		vcname[128];
};

struct smb_share_info {
	int		itype;
	int		usecount;
	u_short		id;		/* TID */
	int		type;		/* share type */
	uid_t		uid;		/* user id of connection */
	gid_t		gid;		/* group of connection */
	mode_t		mode;		/* access mode */
	int		flags;
	char		sname[128];
};

#ifdef _KERNEL

#define CONNADDREQ(a1,a2)	((a1)->sa_len == (a2)->sa_len && \
				 bcmp(a1, a2, (a1)->sa_len) == 0)

struct smb_vc;
struct smb_share;
struct smb_cred;
struct smb_rq;
struct mbdata;
struct smbioc_oshare;
struct smbioc_ossn;
struct uio;

TAILQ_HEAD(smb_rqhead, smb_rq);

/*
 * Node types for connection/share hierarchy
 */
#define	TSMB_CE		TTYPE1		/* list of connections to server */
#define	TSMB_CONN	TTYPE2		/* server connection */
#define	TSMB_VC		TTYPE3		/* user/VC connection */
#define	TSMB_SHARE	TTYPE4		/* share/tree connection */

/*
 * smb_ce structure gathers all resources together (connection engine).
 * It is possible to declare members of this structure as separate static
 * variables, _but_ don't do that ! (I have some unimplemented ideas...)
 */
typedef struct smb_ce {
	int			ce_conncnt;	/* number of active conns */
	int			ce_nextconn;	/* next unique id */
	struct tnode *		ce_tnode;	/* backlink */
} *smb_rc_t;

#define	SMBSO_LOCK	1
#define	SMBSO_UNLOCK	2


#define SMB_DEFRQTIMO	5

#define SMB_DIALECT(vcp)	((vcp)->vc_sopt.sv_proto)

struct smb_tran_desc;

/*
 * smb_conn structure describes physical connection to an SMB server.
 * There will be only one smb_conn structure per each server.
 */
struct smb_conn {
	int		sc_index;	/* unique number of connection */
	u_int		sc_flags;	/* SMBC_* */
	int		sc_vcnext;	/* next VC number */
	int		sc_vcnt;	/* number of active VCs */
	int		sc_maxvcs;	/* maximum number of VC per connection */
	u_int		sc_timo;	/* default request timeout */
	struct sockaddr*sc_paddr;	/* server addr */
	struct sockaddr*sc_laddr;	/* local addr, if any */
	struct tnode *	sc_tnode;	/* backing object */

	struct iconv_drv *sc_tolower;	/* local charset */
	struct iconv_drv *sc_toupper;	/* local charset */
	struct iconv_drv *sc_toserver;	/* local charset to server one */
	struct iconv_drv *sc_tolocal;	/* server charset to local one */
};

#define	sc_srvname	sc_tnode->t_name

/*
 * Virtual Circuit (session) to a server.
 * This is the most (over)complicated part of SMB protocol.
 * For the user security level (usl), each session with different remote
 * user name has its own VC.
 * It is unclear however, should share security level (ssl) allow additional
 * VCs, because user name is not used and can be the same. On other hand,
 * multiple VCs allows us to create separate sessions to server on a per
 * user basis.
 */
struct smb_vc {
	u_short		vc_id;		/* unique vc id (uid) started from zero */
	int		vc_number;	/* number of this VC from the client side */
	int		vc_flags;
	char *		vc_domain;	/* workgroup/primary domain */
	char *		vc_pass;	/* password for usl case */
	uid_t		vc_uid;		/* user id of connection */
	gid_t		vc_grp;		/* group of connection */
	mode_t		vc_mode;	/* access mode */
	struct tnode *	vc_tnode;	/* backing object */

	u_char		vc_hflags;	/* or'ed with flags in the smb header */
	u_short		vc_hflags2;	/* or'ed with flags in the smb header */
	void *		vc_tdata;	/* transport control block */
	struct smb_tran_desc *vc_tdesc;
	u_int		vc_locks;
	int		vc_chlen;	/* actual challenge length */
	u_char 		vc_ch[SMB_MAXCHALLENGELEN];
	u_short		vc_mid;		/* multiplex id */
	struct smb_sopt	vc_sopt;	/* server options */
	struct smb_rqhead vc_rqlist;	/* list of outstanding requests */
	struct smb_cred *vc_scred;	/* used in reconnect procedure */
	int		vc_txmax;	/* max tx/rx packet size */
	int		vc_muxcnt;	/* number of active outstanding requests */
	int		vc_muxwant;
};

#define	vc_user		vc_tnode->t_name
#define vc_maxmux	vc_sopt.sv_maxmux


/*
 * smb_share structure describes connection to the given SMB share (tree).
 * Connection to share is always built on top of the VC.
 */
struct smb_share {
	u_short		ss_id;		/* TID */
	int		ss_type;	/* share type */
	uid_t		ss_uid;		/* user id of connection */
	gid_t		ss_grp;		/* group of connection */
	mode_t		ss_mode;	/* access mode */
	u_int		ss_flags;
	char *		ss_pass;	/* password to a share, can be null */
	struct tnode *	ss_tnode;	/* backing object */
};

#define	ss_name		ss_tnode->t_name

#define	TNTOCE(tp)	((struct smb_ce*)(tp)->t_mdata->tm_data)
#define	CETOTN(ce)	((cep)->ce_tnode)
#define	TNTOCN(tp)	((struct smb_conn*)(tp)->t_mdata->tm_data)
#define	CNTOTN(scp)	((scp)->sc_tnode)
#define	TNTOVC(tp)	((struct smb_vc*)(tp)->t_mdata->tm_data)
#define	VCTOTN(vcp)	((vcp)->vc_tnode)
#define	TNTOSS(tp)	((struct smb_share*)(tp)->t_mdata->tm_data)
#define	SSTOTN(ssp)	((ssp)->ss_tnode)

#define	VCTOCN(vcp)	TNTOCN(VCTOTN((vcp))->t_parent)
#define	SSTOVC(ssp)	TNTOVC(SSTOTN(ssp)->t_parent)
#define	SSTOCN(ssp)	VCTOCN(TNTOVC(SSTOTN(ssp)->t_parent))

struct smb_connspec {
	char *		name;
	struct sockaddr*sap;
	struct smb_vcspec*vcspec;
	struct smb_vc *	vcp;		/* returned */
	/*
	 * The rest is an internal data.
	 */
	struct smb_cred *	scred;
};

struct smb_vcspec {
	int		flags;
	char *		user;
	mode_t		mode;
	mode_t		rights;
	uid_t		owner;
	gid_t		group;
	struct smb_sharespec *shspec;
	struct smb_share *ssp;		/* returned */
	/*
	 * The rest is an internal data
	 */
	struct smb_cred *scred;
};

struct smb_sharespec {
	char *		name;
	mode_t		mode;
	mode_t		rights;
	uid_t		owner;
	gid_t		group;
	int		stype;
	/*
	 * The rest is an internal data
	 */
	struct smb_cred *scred;
};

/*
 * Session level functions
 */
int  smb_sm_init(void);
int  smb_sm_done(void);
int  smb_sm_lookup(struct smb_connspec *cnspec, struct smb_vcspec *vcspec,
	struct smb_sharespec *shspec, struct smb_cred *scred,
	struct smb_conn **scpp);

/*
 * ce level functions
 */
int  smb_ce_init(void);
int  smb_ce_lock(smb_rc_t cep, int flags, struct proc *p);
void smb_ce_unlock(smb_rc_t cep, struct proc *p);
/*int  smb_ce_addconn(smb_rc_t cep, struct smb_conn *scp, struct proc *p);*/
int  smb_ce_createconn(struct smb_connspec *cnspec,
	struct smb_cred *scred, struct smb_conn **scpp);

/*
 * connection level functions
 */
/*int  smb_conn_alloc(struct proc *p, struct ucred *cred, struct smb_conn **conn);*/
void smb_conn_rele(struct smb_conn *scp, struct smb_cred *scred);
int  smb_conn_get(struct smb_conn *scp, struct smb_cred *scred);
void smb_conn_put(struct smb_conn *scp, struct smb_cred *scred);
int  smb_conn_lookupvc(struct smb_conn *scp, struct smb_vcspec *vcspec,
	struct smb_cred *scred, struct smb_vc **vcpp);
int  smb_conn_lock(struct smb_conn *scp, struct proc *p);
int  smb_conn_unlock(struct smb_conn *scp, struct proc *p);

/*
 * session level functions
 */
int  smb_vc_create(struct smb_vcspec *vcspec, char *pass,
	const char *domain,
	struct smb_conn *scp, struct smb_cred *scred, struct smb_vc **vcpp);
int  smb_vc_connect(struct smb_vc *vcp, struct smb_cred *scred);
int  smb_vc_disconnect(struct smb_vc *vcp, struct smb_cred *scred);
void smb_vc_invalidate(struct smb_vc *vcp);
int  smb_vc_valid(struct smb_vc *vcp);
int  smb_vc_reconnect(struct smb_vc *vcp);
int  smb_vc_access(struct smb_vc *vcp, struct smb_cred *scred, mode_t mode);
int  smb_vc_get(struct smb_vc *vcp, struct smb_cred *scred);
void smb_vc_put(struct smb_vc *vcp, struct smb_cred *scred);
void smb_vc_rele(struct smb_vc *vcp, struct smb_cred *scred);
void smb_vc_unlock(struct smb_vc *vcp, int flags, struct smb_cred *scred);
int  smb_vc_lookupshare(struct smb_vc *vcp, struct smb_sharespec *shspec,
	struct smb_cred *scred, struct smb_share **sspp);
char*smb_vc_pass(struct smb_vc *vcp);
u_short smb_vc_nextmid(struct smb_vc *vcp);
int  smb_vc_tranlock(struct smb_vc *vcp, struct proc *p);
int  smb_vc_tranunlock(struct smb_vc *vcp, struct proc *p);
int  smb_vc_rqlock(struct smb_vc *vcp, struct proc *p);
int  smb_vc_rqunlock(struct smb_vc *vcp, struct proc *p);

/*
 * share level functions
 */
int  smb_share_create(struct smb_vc *vcp, struct smb_sharespec *shspec,
	struct smb_cred *scred, struct smb_share **sspp);
int  smb_share_access(struct smb_share *ssp, struct smb_cred *scred, mode_t mode);
int  smb_share_get(struct smb_share *ssp, struct smb_cred *scred);
void smb_share_put(struct smb_share *ssp, struct proc *p);
void smb_share_unlock(struct smb_share *ssp, int flags, struct smb_cred *scred);
void smb_share_rele(struct smb_share *ssp, struct smb_cred *scred);
int  smb_share_connect(struct smb_share *ssp, struct smb_cred *scred);
int  smb_share_detach(struct smb_share *ssp, struct smb_cred *scred);
void smb_share_invalidate(struct smb_share *ssp);
int  smb_share_valid(struct smb_share *ssp);
char*smb_share_pass(struct smb_share *ssp);

/*
 * Link (transport) level functions
 */
int  smb_tran_open(struct smb_conn *scp, struct proc *p);
int  smb_tran_close(struct smb_conn *scp, struct proc *p);
int  smb_tran_bind(struct smb_conn *scp, struct sockaddr *sap, struct proc *p);
int  smb_tran_connect(struct smb_conn *scp, struct sockaddr *sap, struct proc *p);
int  smb_tran_disconnect(struct smb_conn *scp, struct proc *p);
int  smb_tran_sndlock(struct smb_rq *rqp);
int  smb_tran_sndunlock(struct smb_rq *rqp);
int  smb_tran_send(struct smb_rq *rqp);
int  smb_tran_receive(struct smb_rq *rqp, struct mbdata *mbp);
void smb_conn_timo(struct smb_conn *scp);

/*
 * SMB protocol level functions
 */
int  smb_smb_negotiate(struct smb_vc *vcp, struct smb_cred *scred);
int  smb_smb_ssnsetup(struct smb_vc *vcp, struct smb_cred *scred);
int  smb_smb_ssnclose(struct smb_vc *vcp, struct smb_cred *scred);
int  smb_smb_treeconnect(struct smb_share *ssp, struct smb_cred *scred);
int  smb_smb_treedisconnect(struct smb_share *ssp, struct smb_cred *scred);
int  smb_read(struct smb_share *ssp, u_int16_t fid, struct uio *uio,
	struct smb_cred *scred);
int  smb_write(struct smb_share *ssp, u_int16_t fid, struct uio *uio,
	struct smb_cred *scred);

#endif /* _KERNEL */
