/*	$NetBSD: smbfs_node.c,v 1.47.2.1 2013/01/16 05:33:41 yamt Exp $	*/

/*
 * Copyright (c) 2000-2001 Boris Popov
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
 *
 * FreeBSD: src/sys/fs/smbfs/smbfs_node.c,v 1.5 2001/12/20 22:42:26 dillon Exp
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: smbfs_node.c,v 1.47.2.1 2013/01/16 05:33:41 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/lock.h>
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/namei.h>
#include <sys/proc.h>
#include <sys/queue.h>
#include <sys/sysctl.h>
#include <sys/time.h>
#include <sys/vnode.h>
#include <sys/kauth.h>

#include <netsmb/smb.h>
#include <netsmb/smb_conn.h>
#include <netsmb/smb_subr.h>

#include <uvm/uvm.h>
#include <uvm/uvm_extern.h>

#include <fs/smbfs/smbfs.h>
#include <fs/smbfs/smbfs_node.h>
#include <fs/smbfs/smbfs_subr.h>

#define	SMBFS_NOHASH(smp, hval)	(&(smp)->sm_hash[(hval) & (smp)->sm_hashlen])

MALLOC_JUSTDEFINE(M_SMBNODENAME, "SMBFS nname", "SMBFS node name");

extern int (**smbfs_vnodeop_p)(void *);
extern int prtactive;

static const struct genfs_ops smbfs_genfsops = {
	.gop_write = genfs_compat_gop_write,
};

struct pool smbfs_node_pool;

static inline char *
smbfs_name_alloc(const u_char *name, int nmlen)
{
	u_char *cp;

	cp = malloc(nmlen, M_SMBNODENAME, M_WAITOK);
	memcpy(cp, name, nmlen);

	return cp;
}

static inline void
smbfs_name_free(u_char *name)
{
	free(name, M_SMBNODENAME);
}

static int
smbfs_node_alloc(struct mount *mp, struct vnode *dvp,
	const char *name, int nmlen, struct smbfattr *fap, struct vnode **vpp)
{
	struct vattr vattr;
	struct smbmount *smp = VFSTOSMBFS(mp);
	struct smbnode_hashhead *nhpp;
	struct smbnode *np, *np2, *dnp;
	struct vnode *vp;
	u_long hashval;
	int error;

	/* do not allow allocating root vnode twice */
	KASSERT(dvp != NULL || smp->sm_root == NULL);
	/* do not call with dot */
	KASSERT(nmlen != 1 || name[0] != '.');

	if (nmlen == 2 && memcmp(name, "..", 2) == 0) {
		if (dvp == NULL)
			return EINVAL;
		vp = VTOSMB(VTOSMB(dvp)->n_parent)->n_vnode;
		vref(vp);
		if ((error = vn_lock(vp, LK_EXCLUSIVE | LK_RETRY)) == 0)
			*vpp = vp;
		else
			vrele(vp);
		return (error);
	}

	dnp = dvp ? VTOSMB(dvp) : NULL;
#ifdef DIAGNOSTIC
	if (dnp == NULL && dvp != NULL)
		panic("smbfs_node_alloc: dead parent vnode %p", dvp);
#endif
	hashval = smbfs_hash(name, nmlen);
retry:
	mutex_enter(&smp->sm_hashlock);
	nhpp = SMBFS_NOHASH(smp, hashval);
	LIST_FOREACH(np, nhpp, n_hash) {
		if (np->n_parent != dvp
		    || np->n_nmlen != nmlen
		    || memcmp(name, np->n_name, nmlen) != 0)
			continue;
		vp = SMBTOV(np);
		mutex_enter((vp)->v_interlock);
		mutex_exit(&smp->sm_hashlock);
		if (vget(vp, LK_EXCLUSIVE) != 0)
			goto retry;
		/* Force cached attributes to be refreshed if stale. */
		(void)VOP_GETATTR(vp, &vattr, curlwp->l_cred);
		/*
		 * If the file type on the server is inconsistent with
		 * what it was when we created the vnode, kill the
		 * bogus vnode now and fall through to the code below
		 * to create a new one with the right type.
		 */
		if ((vp->v_type == VDIR && (np->n_dosattr & SMB_FA_DIR) == 0) ||
		    (vp->v_type == VREG && (np->n_dosattr & SMB_FA_DIR) != 0)) {
			VOP_UNLOCK(vp);
			vgone(vp);
			goto allocnew;
		}
		*vpp = vp;
		return (0);
	}
	mutex_exit(&smp->sm_hashlock);

allocnew:
	/*
	 * If we don't have node attributes, then it is an explicit lookup
	 * for an existing vnode.
	 */
	if (fap == NULL)
		return ENOENT;

	np = pool_get(&smbfs_node_pool, PR_WAITOK);
	memset(np, 0, sizeof(*np));

	error = getnewvnode(VT_SMBFS, mp, smbfs_vnodeop_p, NULL, &vp);
	if (error) {
		pool_put(&smbfs_node_pool, np);
		return error;
	}

	if (dvp) {
		np->n_parent = dvp;
		if (/*vp->v_type == VDIR &&*/ (dvp->v_vflag & VV_ROOT) == 0) {
			vref(dvp);
			np->n_flag |= NREFPARENT;
		}
	}

	vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);

	mutex_enter(&smp->sm_hashlock);
	/*
	 * Check if the vnode wasn't added while we were in getnewvnode/
	 * malloc.
	 */
	LIST_FOREACH(np2, nhpp, n_hash) {
		if (np2->n_parent != dvp
		    || np2->n_nmlen != nmlen
		    || memcmp(name, np2->n_name, nmlen) != 0)
			continue;
		mutex_exit(&smp->sm_hashlock);
		pool_put(&smbfs_node_pool, np);
		ungetnewvnode(vp);
		if ((np->n_flag & NREFPARENT) != 0)
			vrele(dvp);
		goto retry;
	}

	vp->v_type = fap->fa_attr & SMB_FA_DIR ? VDIR : VREG;
	vp->v_data = np;
	genfs_node_init(vp, &smbfs_genfsops);

	np->n_vnode = vp;
	np->n_mount = VFSTOSMBFS(mp);
	np->n_nmlen = nmlen;
	np->n_name = smbfs_name_alloc(name, nmlen);
	np->n_ino = fap->fa_ino;
	np->n_size = fap->fa_size;

	/* new file vnode has to have a parent */
	KASSERT(vp->v_type != VREG || dvp != NULL);

	/* Not on hash list, add it now */
	LIST_INSERT_HEAD(nhpp, np, n_hash);
	uvm_vnp_setsize(vp, np->n_size);
	mutex_exit(&smp->sm_hashlock);

	*vpp = vp;
	return 0;
}

int
smbfs_nget(struct mount *mp, struct vnode *dvp, const char *name, int nmlen,
	struct smbfattr *fap, struct vnode **vpp)
{
	struct vnode *vp;
	int error;

	error = smbfs_node_alloc(mp, dvp, name, nmlen, fap, &vp);
	if (error)
		return error;
	if (fap)
		smbfs_attr_cacheenter(vp, fap);
	*vpp = vp;
	return 0;
}

/*
 * Free smbnode, and give vnode back to system
 */
int
smbfs_reclaim(void *v)
{
        struct vop_reclaim_args /* {
		struct vnode *a_vp;
		struct thread *a_p;
        } */ *ap = v;
	struct vnode *vp = ap->a_vp;
	struct vnode *dvp;
	struct smbnode *np = VTOSMB(vp);
	struct smbmount *smp = VTOSMBFS(vp);

	if (prtactive && vp->v_usecount > 1)
		vprint("smbfs_reclaim(): pushing active", vp);

	SMBVDEBUG("%.*s,%d\n", (int) np->n_nmlen, np->n_name, vp->v_usecount);

	mutex_enter(&smp->sm_hashlock);

	dvp = (np->n_parent && (np->n_flag & NREFPARENT)) ?
	    np->n_parent : NULL;

	LIST_REMOVE(np, n_hash);

	if (smp->sm_root == np) {
		SMBVDEBUG0("root vnode\n");
		smp->sm_root = NULL;
	}
	genfs_node_destroy(vp);
	vp->v_data = NULL;
	mutex_exit(&smp->sm_hashlock);
	if (np->n_name)
		smbfs_name_free(np->n_name);
	pool_put(&smbfs_node_pool, np);
	if (dvp) {
		vrele(dvp);
		/*
		 * Indicate that we released something; see comment
		 * in smbfs_unmount().
		 */
		smp->sm_didrele = 1;
	}
	return 0;
}

int
smbfs_inactive(void *v)
{
	struct vop_inactive_args /* {
		struct vnode *a_vp;
		bool *a_recycle;
	} */ *ap = v;
	struct lwp *l = curlwp;
	kauth_cred_t cred = l->l_cred;
	struct vnode *vp = ap->a_vp;
	struct smbnode *np = VTOSMB(vp);
	struct smb_cred scred;

	SMBVDEBUG("%.*s: %d\n", (int) np->n_nmlen, np->n_name, vp->v_usecount);
	if ((np->n_flag & NOPEN) != 0) {
		struct smb_share *ssp = np->n_mount->sm_share;

		smbfs_vinvalbuf(vp, V_SAVE, cred, l, 1);
		smb_makescred(&scred, l, cred);

		if (vp->v_type == VDIR && np->n_dirseq) {
			smbfs_findclose(np->n_dirseq, &scred);
			np->n_dirseq = NULL;
		}

		if (vp->v_type != VDIR
		    || SMB_CAPS(SSTOVC(ssp)) & SMB_CAP_NT_SMBS)
			smbfs_smb_close(ssp, np->n_fid, &np->n_mtime, &scred);

		np->n_flag &= ~NOPEN;
		smbfs_attr_cacheremove(vp);
	}
	*ap->a_recycle = ((np->n_flag & NGONE) != 0);
	VOP_UNLOCK(vp);

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
			uvm_vnp_setsize(vp, np->n_size);
		}
	} else if (vp->v_type == VDIR) {
		np->n_size = 16384; 		/* should be a better way ... */
	} else
		return;

	np->n_mtime = fap->fa_mtime;
	np->n_dosattr = fap->fa_attr;

	np->n_attrage = time_uptime;
}

int
smbfs_attr_cachelookup(struct vnode *vp, struct vattr *va)
{
	struct smbnode *np = VTOSMB(vp);
	struct smbmount *smp = VTOSMBFS(vp);
	time_t diff;

	diff = time_uptime - np->n_attrage;
	if (diff > SMBFS_ATTRTIMO)	/* XXX should be configurable */
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
	va->va_fsid = vp->v_mount->mnt_stat.f_fsidx.__fsid_val[0];
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
