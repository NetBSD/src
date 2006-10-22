/*	$NetBSD: puffs_sys.h,v 1.1 2006/10/22 22:43:23 pooka Exp $	*/

/*
 * Copyright (c) 2005, 2006  Antti Kantee.  All Rights Reserved.
 *
 * Development of this software was supported by the
 * Google Summer of Code program and the Ulla Tuominen Foundation.
 * The Google SoC project was mentored by Bill Studenmund.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _PUFFS_SYS_H_
#define _PUFFS_SYS_H_

#include <sys/param.h>
#include <sys/select.h>
#include <sys/kauth.h>
#include <sys/lock.h>
#include <sys/queue.h>
#include <sys/lock.h>

#include <fs/puffs/puffs_msgif.h>

/*
 * While a request is going to userspace, park the caller within the
 * kernel.  This is the kernel counterpart of "struct puffs_req".
 */
struct puffs_park {
	struct puffs_req *park_preq;	/* the relevant preq		*/

	void		*park_kernbuf;	/* kernel buffer address	*/
	size_t		park_buflen;	/* buffer length		*/
	size_t		park_copylen;	/* length to copy to userspace  */

	uint32_t 	park_flags;

	TAILQ_ENTRY(puffs_park) park_entries;
};

#define PUFFS_SIZEOPREQ_UIO_IN 1
#define PUFFS_SIZEOPREQ_UIO_OUT 2
#define PUFFS_SIZEOPREQ_BUF_IN 3
#define PUFFS_SIZEOPREQ_BUF_OUT 4

#define PUFFS_SIZEOP_UIO(a)	\
	(((a)==PUFFS_SIZEOPREQ_UIO_IN)||(a)==PUFFS_SIZEOPREQ_UIO_OUT)
#define PUFFS_SIZEOP_BUF(a)	\
	(((a)==PUFFS_SIZEOPREQ_BUF_IN)||(a)==PUFFS_SIZEOPREQ_BUF_OUT)

/* XXX: alignment-optimization */
struct puffs_sizepark {
	uint64_t	pkso_reqid;
	uint8_t		pkso_reqtype;

	struct uio	*pkso_uio;
	void		*pkso_copybuf;
	size_t		pkso_bufsize;

	TAILQ_ENTRY(puffs_sizepark) pkso_entries;
};

#define DPRINTF(x)

#define MPTOPUFFSMP(mp) ((struct puffs_mount *)((mp)->mnt_data))
#define PMPTOMP(pmp) (pmp->pmp_mp)
#define VPTOPP(vp) ((struct puffs_node *)(vp)->v_data)
#define VPTOPNC(vp) (((struct puffs_node *)(vp)->v_data)->pn_cookie)
#define VPTOPUFFSMP(vp) ((struct puffs_mount*)((struct puffs_node*)vp->v_data))
#define FPTOPMP(fp) (((struct puffs_instance *)fp->f_data)->pi_pmp)
#define FPTOPI(fp) ((struct puffs_instance *)fp->f_data)

TAILQ_HEAD(puffs_wq, puffs_park);
struct puffs_mount {
	struct simplelock		pmp_lock;

	struct puffs_args		pmp_args;

	struct puffs_wq			pmp_req_touser;
	size_t				pmp_req_touser_waiters;
	size_t				pmp_req_maxsize;

	struct puffs_wq			pmp_req_replywait;
	TAILQ_HEAD(, puffs_sizepark)	pmp_req_sizepark;

	LIST_HEAD(, puffs_node)		pmp_pnodelist;

	struct mount			*pmp_mp;
	struct vnode			*pmp_root;
	void				*pmp_rootcookie;
	struct selinfo			*pmp_sel;	/* in puffs_instance */

	unsigned int			pmp_nextreq;
	uint8_t				pmp_status;
};
#define PUFFSTAT_BEFOREINIT	0
#define PUFFSTAT_MOUNTING	1
#define PUFFSTAT_RUNNING	2
#define PUFFSTAT_DYING		3

#define PNODE_INACTIVE	0x01
#define PNODE_LOCKED	0x02
#define PNODE_WANTED	0x04	
struct puffs_node {
	void		*pn_cookie;	/* userspace pnode cookie	*/
	struct vnode	*pn_vp;		/* backpointer to vnode		*/
	uint32_t	pn_stat;	/* node status			*/

	LIST_ENTRY(puffs_node) pn_entries;
};

int	puffs_start2(struct puffs_mount *, struct puffs_vfsreq_start *);

int	puffs_vfstouser(struct puffs_mount *, int, void *, size_t);
int	puffs_vntouser(struct puffs_mount *, int, void *, size_t, void *,
		       struct vnode *, struct vnode *);
int	puffs_vntouser_req(struct puffs_mount *, int, void *, size_t,
			   void *, unsigned int,
			   struct vnode *, struct vnode *);
int	puffs_vntouser_adjbuf(struct puffs_mount *, int, void **, size_t *,
		              size_t, void *, struct vnode *, struct vnode *);

int	puffs_getvnode(struct mount *, void *, struct vnode **);
int	puffs_newnode(struct mount *, struct vnode *, struct vnode **,
		      void *, struct componentname *, enum vtype);
void	puffs_putvnode(struct vnode *);
struct vnode *puffs_pnode2vnode(struct puffs_mount *, void *);
void	puffs_makecn(struct puffs_cn *, const struct componentname *);
void	puffs_credcvt(struct puffs_cred *, kauth_cred_t);
pid_t	puffs_lwp2pid(struct lwp *);

int	puffs_setpmp(pid_t, int, struct puffs_mount *);
void	puffs_nukebypmp(struct puffs_mount *);

unsigned int	puffs_getreqid(struct puffs_mount *);
void		puffs_userdead(struct puffs_mount *);

extern int (**puffs_vnodeop_p)(void *);

MALLOC_DECLARE(M_PUFFS);

#endif /* _PUFFS_SYS_H_ */
