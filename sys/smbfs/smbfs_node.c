/*	$NetBSD: smbfs_node.c,v 1.1 2000/12/07 03:33:47 deberg Exp $	*/

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
#include <sys/time.h>
#include <sys/proc.h>
#include <sys/mount.h>
#include <sys/vnode.h>
#include <sys/malloc.h>
#include <uvm/uvm_extern.h>
#include <sys/sysctl.h>
#ifndef NetBSD
#include <vm/vm_extern.h>
#endif
/*#include <vm/vm_page.h>
#include <vm/vm_object.h>*/
#include <sys/queue.h>
#include <sys/namei.h>

#include <sys/tree.h>

#include <netsmb/smb.h>
#include <netsmb/smb_conn.h>
#include <netsmb/smb_subr.h>
#include <netsmb/smb_lock.h>

#include <smbfs/smbfs.h>
#include <smbfs/smbfs_node.h>
#include <smbfs/smbfs_subr.h>

#define	SMBFS_NOHASH(hval) 	(&smbfs_hashtbl[(hval) & smbfs_hashlen])
#define	SLOCK_UNLOCK(id)	do { 					\
				    if ((id) < 0) wakeup(&(id));	\
					(id) = 0; 			\
				} while(0);


#ifndef NetBSD
extern vop_t **smbfs_vnodeop_p;

MALLOC_DEFINE(M_SMBNODE, "SMBFS node", "SMBFS vnode private part");
MALLOC_DEFINE(M_SMBFSHASH, "SMBFS hash", "SMBFS hash table");
#endif

static LIST_HEAD(smbnode_hashhead, smbnode) *smbfs_hashtbl;
static u_long smbfs_hashlen;

static int smbfs_hashlock = 0;
#ifndef NetBSD
#if __FreeBSD_version > 500000
static int smbfs_hashprint(SYSCTL_HANDLER_ARGS);
#else
static int smbfs_hashprint SYSCTL_HANDLER_ARGS;
#endif

extern struct linker_set sysctl_vfs_smbfs;
#ifdef SYSCTL_DECL
SYSCTL_DECL(_vfs_smbfs);
#endif
SYSCTL_PROC(_vfs_smbfs, OID_AUTO, vnprint, CTLFLAG_WR|CTLTYPE_OPAQUE,
	    NULL, 0, smbfs_hashprint, "S,vnlist", "vnode hash");
#endif


void
smbfs_hash_init(void)
{
	smbfs_hashtbl = hashinit(desiredvnodes, HASH_LIST, M_SMBFSHASH, 
				 M_WAITOK, &smbfs_hashlen);
}

void
smbfs_hash_free(void)
{
	free(smbfs_hashtbl, M_SMBFSHASH);
}

static u_long
smbfs_hash(const u_char *name, int nmlen)
{
	u_long v;

	for (v = 0; nmlen; name++, nmlen--)
		v += *name;
	return v;
}

#ifndef NetBSD
static int
#if __FreeBSD_version > 500000
smbfs_hashprint(SYSCTL_HANDLER_ARGS)
#else
smbfs_hashprint SYSCTL_HANDLER_ARGS
#endif
{
	struct smbnode_hashhead *nhpp;
	struct smbnode *np;
	struct vnode *vp;
	int i;

	if (smbfs_debuglevel == 0)
		return 0;
	printf("Name:uc:hc:parent\n");
	for(i = 0; i <= smbfs_hashlen; i++) {
		nhpp = &smbfs_hashtbl[i];
		for (np = nhpp->lh_first; np != 0; np = np->n_hash.le_next) {
			vp = SMBTOV(np);
			printf("%s:%d:%d\n", np->n_name, vp->v_usecount,
			    vp->v_holdcnt/*,
			    (np->n_parent != NULL) ? np->n_parent->n_name : "<noparent>"
			*/);
		}
	}
	return 0;
}
#endif

int
smbfs_node_alloc(struct mount *mp, struct vnode *dvp,
	const char *name, int nmlen, struct vnode **vpp)
{
	/*  struct proc *p = curproc; */	/* XXX */
	struct smbnode_hashhead *nhpp;
	struct smbnode *np, *np2, *dnp;
	extern int (**smbfs_vnodeop_p)__P((void *));
	struct vnode *vp;
	u_long hashval;
	int error;

	*vpp = NULL;
	if (nmlen == 2 && bcmp(name, "..", 2) == 0) {
		if (dvp == NULL)
			return EINVAL;
		vp = VTOSMB(dvp)->n_parent->n_vnode;
		error = vget(vp, LK_EXCLUSIVE/*  , p */);
		if (error == 0)
			*vpp = vp;
		return error;
	} else if (nmlen == 1 && name[0] == '.') {
		SMBERROR("do not call me with dot!\n");
		return EINVAL;
	}
	hashval = smbfs_hash(name, nmlen);
	dnp = dvp ? VTOSMB(dvp) : NULL;
retry:
	nhpp = SMBFS_NOHASH(hashval);
loop:
	for (np = nhpp->lh_first; np; np = np->n_hash.le_next) {
		vp = SMBTOV(np);
		if (mp != vp->v_mount || np->n_parent != dnp ||
		    np->n_nmlen != nmlen || strcmp(name, np->n_name) != 0)
			continue;
		if (vget(vp, LK_EXCLUSIVE/*  , p */))
			goto loop;
		*vpp = vp;
		return(0);
	}

	if (smbfs_hashlock) {
		while(smbfs_hashlock) {
			smbfs_hashlock = -1;
			tsleep((caddr_t)&smbfs_hashlock, PVFS, "smbhash", 0);
		}
		goto loop;
	}
	smbfs_hashlock = 1;

	MALLOC(np, struct smbnode *, sizeof *np, M_SMBNODE, M_WAITOK);
	error = getnewvnode(VT_SMBFS, mp, smbfs_vnodeop_p, &vp);
	if (error) {
		SLOCK_UNLOCK(smbfs_hashlock);
		FREE(np, M_SMBNODE);
		return error;
	}
	bzero(np, sizeof(*np));
	vp->v_data = np;
	np->n_vnode = vp;
	np->n_mount = VFSTOSMBFS(mp);
	np->n_nmlen = nmlen;
	np->n_name = smb_memdup(name, nmlen + 1);
	np->n_name[nmlen] = 0;
	simple_spinlock_init(&np->n_rdirlock, "dirlck", PVFS);
	for (np2 = nhpp->lh_first; np2 != 0; np2 = np->n_hash.le_next) {
		if (mp != SMBTOV(np2)->v_mount || np->n_parent != dnp ||
		    np2->n_nmlen != nmlen || strcmp(name, np2->n_name) != 0)
			continue;
		vrele(vp);
		smb_memfree(np->n_name);
		FREE(np, M_SMBNODE);
		SLOCK_UNLOCK(smbfs_hashlock);
		goto retry;
	}
	LIST_INSERT_HEAD(nhpp, np, n_hash);
	SLOCK_UNLOCK(smbfs_hashlock);
	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY/*  , p */);
	np->n_flag |= NNEW;
	*vpp = vp;
	return error;
}

/*
 * Free smbnode, and give vnode back to system
 */
int
smbfs_reclaim(ap)                     
        struct vop_reclaim_args /* {
        struct vnode *a_vp;
        } */ *ap;
{
	struct vnode *vp = ap->a_vp;
	struct vnode *dvp = NULL;
	struct smbnode *np = VTOSMB(vp);
	struct smbmount *smp = VTOSMBFS(vp);
	
	SMBVDEBUG("%s,%ld\n", np->n_name, vp->v_usecount);
	if (np->n_parent && (np->n_flag & NREFPARENT))
		dvp = np->n_parent->n_vnode;
	LIST_REMOVE(np, n_hash);
	cache_purge(vp);
	if (smp->sm_root == np)
		smp->sm_root = NULL;
	vp->v_data = NULL;
	if (np->n_name)
		smb_memfree(np->n_name);
	FREE(np, M_SMBNODE);
	if (dvp)
		vrele(dvp);
	return 0;
}

int
smbfs_inactive(ap)
	struct vop_inactive_args /* {
		struct vnode *a_vp;
		struct proc *a_p;
	} */ *ap;
{
	struct proc *p = ap->a_p;
	struct ucred *cred = p->p_ucred;
	struct vnode *vp = ap->a_vp;
	struct smbnode *np = VTOSMB(vp);
	struct smb_cred scred;
	int error;

	SMBVDEBUG("%s: %ld\n", VTOSMB(vp)->n_name, vp->v_usecount);
	if (np->opened) {
		error = smbfs_vinvalbuf(vp, V_SAVE, cred, p, 1);
		smb_makescred(&scred, p, cred);
		error = smbfs_smb_close(np->n_mount->sm_share, np->n_fid, 
		   &np->n_mtime, &scred);
		np->opened = 0;
	}
	VOP_UNLOCK(vp, 0/*  , p */);
	return (0);
}
/*
 * routines to maintain vnode attributes cache
 * smbfs_attr_cacheenter: unpack np.i to vattr structure
 */
void
smbfs_attr_cacheenter(struct vnode *vp, struct smbfattr *fap)
{
	struct smbnode *np = VTOSMB(vp);

	if (vp->v_type == VREG) {
		if (np->n_size != fap->fa_size) {
			np->n_size = fap->fa_size;
#ifndef NetBSD
			vnode_pager_setsize(vp, np->n_size);
#else
			uvm_vnp_setsize(vp, np->n_size);
#endif
		}
	} else if (vp->v_type == VDIR) {
		np->n_size = 16384; 		/* should be a better way ... */
	} else
		return;
	np->n_mtime = fap->fa_mtime;
	np->n_dosattr = fap->fa_attr;
	np->n_attrage = mono_time.tv_sec;
	return;
}

int
smbfs_attr_cachelookup(struct vnode *vp, struct vattr *va)
{
	struct smbnode *np = VTOSMB(vp);
	struct smbmount *smp = VTOSMBFS(vp);
	int diff;

	diff = mono_time.tv_sec - np->n_attrage;
	if (diff > 2)	/* XXX should be configurable */
		return ENOENT;
	va->va_type = vp->v_type;		/* vnode type (for create) */
	if (vp->v_type == VREG) {
		va->va_mode = smp->sm_args.file_mode;	/* files access mode and type */
	} else if (vp->v_type == VDIR) {
		va->va_mode = smp->sm_args.dir_mode;	/* files access mode and type */
	} else
		return EINVAL;
	va->va_size = np->n_size;
	va->va_nlink = 1;		/* number of references to file */
	va->va_uid = smp->sm_args.uid;	/* owner user id */
	va->va_gid = smp->sm_args.gid;	/* owner group id */
	va->va_fsid = vp->v_mount->mnt_stat.f_fsid.val[0];
	va->va_fileid = np->n_ino;	/* file id */
	if (va->va_fileid == 0)
		va->va_fileid = 2;
	va->va_blocksize = SSTOVC(smp->sm_share)->vc_txmax;
	va->va_mtime = np->n_mtime;
	va->va_atime = va->va_ctime = va->va_mtime;	/* time file changed */
	va->va_gen = VNOVAL;		/* generation number of file */
	va->va_flags = 0;		/* flags defined for file */
	va->va_rdev = VNOVAL;		/* device the special file represents */
	va->va_bytes = va->va_size;	/* bytes of disk space held by file */
	va->va_filerev = 0;		/* file modification number */
	va->va_vaflags = 0;		/* operations flags */
	return 0;
}
