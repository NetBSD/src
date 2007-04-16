/*	$NetBSD: puffs_vnops.c,v 1.59 2007/04/16 13:03:26 pooka Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: puffs_vnops.c,v 1.59 2007/04/16 13:03:26 pooka Exp $");

#include <sys/param.h>
#include <sys/fstrans.h>
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/namei.h>
#include <sys/vnode.h>
#include <uvm/uvm.h>

#include <fs/puffs/puffs_msgif.h>
#include <fs/puffs/puffs_sys.h>

#include <miscfs/fifofs/fifo.h>
#include <miscfs/genfs/genfs.h>
#include <miscfs/specfs/specdev.h>

int	puffs_lookup(void *);
int	puffs_create(void *);
int	puffs_access(void *);
int	puffs_mknod(void *);
int	puffs_open(void *);
int	puffs_close(void *);
int	puffs_getattr(void *);
int	puffs_setattr(void *);
int	puffs_reclaim(void *);
int	puffs_readdir(void *);
int	puffs_poll(void *);
int	puffs_fsync(void *);
int	puffs_seek(void *);
int	puffs_remove(void *);
int	puffs_mkdir(void *);
int	puffs_rmdir(void *);
int	puffs_link(void *);
int	puffs_readlink(void *);
int	puffs_symlink(void *);
int	puffs_rename(void *);
int	puffs_read(void *);
int	puffs_write(void *);
int	puffs_fcntl(void *);
int	puffs_ioctl(void *);
int	puffs_inactive(void *);
int	puffs_print(void *);
int	puffs_pathconf(void *);
int	puffs_advlock(void *);
int	puffs_strategy(void *);
int	puffs_bmap(void *);
int	puffs_mmap(void *);
int	puffs_getpages(void *);

int	puffs_spec_read(void *);
int	puffs_spec_write(void *);
int	puffs_fifo_read(void *);
int	puffs_fifo_write(void *);

int	puffs_checkop(void *);


/* VOP_LEASE() not included */

int	puffs_generic(void *);

#if 0
#define puffs_lock genfs_lock
#define puffs_unlock genfs_unlock
#define puffs_islocked genfs_islocked
#else
int puffs_lock(void *);
int puffs_unlock(void *);
int puffs_islocked(void *);
#endif

int (**puffs_vnodeop_p)(void *);
const struct vnodeopv_entry_desc puffs_vnodeop_entries[] = {
	{ &vop_default_desc, vn_default_error },
	{ &vop_lookup_desc, puffs_lookup },		/* REAL lookup */
	{ &vop_create_desc, puffs_checkop },		/* create */
        { &vop_mknod_desc, puffs_checkop },		/* mknod */
        { &vop_open_desc, puffs_open },			/* REAL open */
        { &vop_close_desc, puffs_checkop },		/* close */
        { &vop_access_desc, puffs_access },		/* REAL access */
        { &vop_getattr_desc, puffs_checkop },		/* getattr */
        { &vop_setattr_desc, puffs_checkop },		/* setattr */
        { &vop_read_desc, puffs_checkop },		/* read */
        { &vop_write_desc, puffs_checkop },		/* write */
        { &vop_fcntl_desc, puffs_checkop },		/* fcntl */
        { &vop_ioctl_desc, puffs_checkop },		/* ioctl */
        { &vop_fsync_desc, puffs_fsync },		/* REAL fsync */
        { &vop_seek_desc, puffs_checkop },		/* seek */
        { &vop_remove_desc, puffs_checkop },		/* remove */
        { &vop_link_desc, puffs_checkop },		/* link */
        { &vop_rename_desc, puffs_checkop },		/* rename */
        { &vop_mkdir_desc, puffs_checkop },		/* mkdir */
        { &vop_rmdir_desc, puffs_checkop },		/* rmdir */
        { &vop_symlink_desc, puffs_checkop },		/* symlink */
        { &vop_readdir_desc, puffs_checkop },		/* readdir */
        { &vop_readlink_desc, puffs_checkop },		/* readlink */
        { &vop_getpages_desc, puffs_checkop },		/* getpages */
        { &vop_putpages_desc, genfs_putpages },		/* REAL putpages */
        { &vop_pathconf_desc, puffs_checkop },		/* pathconf */
        { &vop_advlock_desc, puffs_checkop },		/* advlock */
        { &vop_strategy_desc, puffs_strategy },		/* REAL strategy */
        { &vop_revoke_desc, genfs_revoke },		/* REAL revoke */
        { &vop_abortop_desc, genfs_abortop },		/* REAL abortop */
        { &vop_inactive_desc, puffs_inactive },		/* REAL inactive */
        { &vop_reclaim_desc, puffs_reclaim },		/* REAL reclaim */
        { &vop_lock_desc, puffs_lock },			/* REAL lock */
        { &vop_unlock_desc, puffs_unlock },		/* REAL unlock */
        { &vop_bmap_desc, puffs_bmap },			/* REAL bmap */
        { &vop_print_desc, puffs_print },		/* REAL print */
        { &vop_islocked_desc, puffs_islocked },		/* REAL islocked */
        { &vop_bwrite_desc, genfs_nullop },		/* REAL bwrite */
        { &vop_mmap_desc, puffs_mmap },			/* REAL mmap */

        { &vop_poll_desc, genfs_eopnotsupp },		/* poll XXX */
        { &vop_poll_desc, genfs_eopnotsupp },		/* kqfilter XXX */
	{ NULL, NULL }
};
const struct vnodeopv_desc puffs_vnodeop_opv_desc =
	{ &puffs_vnodeop_p, puffs_vnodeop_entries };


int (**puffs_specop_p)(void *);
const struct vnodeopv_entry_desc puffs_specop_entries[] = {
	{ &vop_default_desc, vn_default_error },
	{ &vop_lookup_desc, spec_lookup },		/* lookup, ENOTDIR */
	{ &vop_create_desc, spec_create },		/* genfs_badop */
	{ &vop_mknod_desc, spec_mknod },		/* genfs_badop */
	{ &vop_open_desc, spec_open },			/* spec_open */
	{ &vop_close_desc, spec_close },		/* spec_close */
	{ &vop_access_desc, puffs_checkop },		/* access */
	{ &vop_getattr_desc, puffs_checkop },		/* getattr */
	{ &vop_setattr_desc, puffs_checkop },		/* setattr */
	{ &vop_read_desc, puffs_spec_read },		/* update, read */
	{ &vop_write_desc, puffs_spec_write },		/* update, write */
	{ &vop_lease_desc, spec_lease_check },		/* genfs_nullop */
	{ &vop_ioctl_desc, spec_ioctl },		/* spec_ioctl */
	{ &vop_fcntl_desc, genfs_fcntl },		/* dummy */
	{ &vop_poll_desc, spec_poll },			/* spec_poll */
	{ &vop_kqfilter_desc, spec_kqfilter },		/* spec_kqfilter */
	{ &vop_revoke_desc, spec_revoke },		/* genfs_revoke */
	{ &vop_mmap_desc, spec_mmap },			/* genfs_mmap (dummy) */
	{ &vop_fsync_desc, spec_fsync },		/* vflushbuf */
	{ &vop_seek_desc, spec_seek },			/* genfs_nullop */
	{ &vop_remove_desc, spec_remove },		/* genfs_badop */
	{ &vop_link_desc, spec_link },			/* genfs_badop */
	{ &vop_rename_desc, spec_rename },		/* genfs_badop */
	{ &vop_mkdir_desc, spec_mkdir },		/* genfs_badop */
	{ &vop_rmdir_desc, spec_rmdir },		/* genfs_badop */
	{ &vop_symlink_desc, spec_symlink },		/* genfs_badop */
	{ &vop_readdir_desc, spec_readdir },		/* genfs_badop */
	{ &vop_readlink_desc, spec_readlink },		/* genfs_badop */
	{ &vop_abortop_desc, spec_abortop },		/* genfs_badop */
	{ &vop_inactive_desc, puffs_inactive },		/* REAL inactive */
	{ &vop_reclaim_desc, puffs_reclaim },		/* REAL reclaim */
	{ &vop_lock_desc, puffs_lock },			/* REAL lock */
	{ &vop_unlock_desc, puffs_unlock },		/* REAL unlock */
	{ &vop_bmap_desc, spec_bmap },			/* dummy */
	{ &vop_strategy_desc, spec_strategy },		/* dev strategy */
	{ &vop_print_desc, puffs_print },		/* REAL print */
	{ &vop_islocked_desc, puffs_islocked },		/* REAL islocked */
	{ &vop_pathconf_desc, spec_pathconf },		/* pathconf */
	{ &vop_advlock_desc, spec_advlock },		/* lf_advlock */
	{ &vop_bwrite_desc, vn_bwrite },		/* bwrite */
	{ &vop_getpages_desc, spec_getpages },		/* genfs_getpages */
	{ &vop_putpages_desc, spec_putpages },		/* genfs_putpages */
#if 0
	{ &vop_openextattr_desc, _openextattr },	/* openextattr */
	{ &vop_closeextattr_desc, _closeextattr },	/* closeextattr */
	{ &vop_getextattr_desc, _getextattr },		/* getextattr */
	{ &vop_setextattr_desc, _setextattr },		/* setextattr */
	{ &vop_listextattr_desc, _listextattr },	/* listextattr */
	{ &vop_deleteextattr_desc, _deleteextattr },	/* deleteextattr */
#endif
	{ NULL, NULL }
};
const struct vnodeopv_desc puffs_specop_opv_desc =
	{ &puffs_specop_p, puffs_specop_entries };


int (**puffs_fifoop_p)(void *);
const struct vnodeopv_entry_desc puffs_fifoop_entries[] = {
	{ &vop_default_desc, vn_default_error },
	{ &vop_lookup_desc, fifo_lookup },		/* lookup, ENOTDIR */
	{ &vop_create_desc, fifo_create },		/* genfs_badop */
	{ &vop_mknod_desc, fifo_mknod },		/* genfs_badop */
	{ &vop_open_desc, fifo_open },			/* open */
	{ &vop_close_desc, fifo_close },		/* close */
	{ &vop_access_desc, puffs_checkop },		/* access */
	{ &vop_getattr_desc, puffs_checkop },		/* getattr */
	{ &vop_setattr_desc, puffs_checkop },		/* setattr */
	{ &vop_read_desc, puffs_fifo_read },		/* read, update */
	{ &vop_write_desc, puffs_fifo_write },		/* write, update */
	{ &vop_lease_desc, fifo_lease_check },		/* genfs_nullop */
	{ &vop_ioctl_desc, fifo_ioctl },		/* ioctl */
	{ &vop_fcntl_desc, genfs_fcntl },		/* dummy */
	{ &vop_poll_desc, fifo_poll },			/* poll */
	{ &vop_kqfilter_desc, fifo_kqfilter },		/* kqfilter */
	{ &vop_revoke_desc, fifo_revoke },		/* genfs_revoke */
	{ &vop_mmap_desc, fifo_mmap },			/* genfs_badop */
	{ &vop_fsync_desc, fifo_fsync },		/* genfs_nullop*/
	{ &vop_seek_desc, fifo_seek },			/* genfs_badop */
	{ &vop_remove_desc, fifo_remove },		/* genfs_badop */
	{ &vop_link_desc, fifo_link },			/* genfs_badop */
	{ &vop_rename_desc, fifo_rename },		/* genfs_badop */
	{ &vop_mkdir_desc, fifo_mkdir },		/* genfs_badop */
	{ &vop_rmdir_desc, fifo_rmdir },		/* genfs_badop */
	{ &vop_symlink_desc, fifo_symlink },		/* genfs_badop */
	{ &vop_readdir_desc, fifo_readdir },		/* genfs_badop */
	{ &vop_readlink_desc, fifo_readlink },		/* genfs_badop */
	{ &vop_abortop_desc, fifo_abortop },		/* genfs_badop */
	{ &vop_inactive_desc, puffs_inactive },		/* REAL inactive */
	{ &vop_reclaim_desc, puffs_reclaim },		/* REAL reclaim */
	{ &vop_lock_desc, puffs_lock },			/* REAL lock */
	{ &vop_unlock_desc, puffs_unlock },		/* REAL unlock */
	{ &vop_bmap_desc, fifo_bmap },			/* dummy */
	{ &vop_strategy_desc, fifo_strategy },		/* genfs_badop */
	{ &vop_print_desc, puffs_print },		/* REAL print */
	{ &vop_islocked_desc, puffs_islocked },		/* REAL islocked */
	{ &vop_pathconf_desc, fifo_pathconf },		/* pathconf */
	{ &vop_advlock_desc, fifo_advlock },		/* genfs_einval */
	{ &vop_bwrite_desc, vn_bwrite },		/* bwrite */
	{ &vop_putpages_desc, fifo_putpages }, 		/* genfs_null_putpages*/
#if 0
	{ &vop_openextattr_desc, _openextattr },	/* openextattr */
	{ &vop_closeextattr_desc, _closeextattr },	/* closeextattr */
	{ &vop_getextattr_desc, _getextattr },		/* getextattr */
	{ &vop_setextattr_desc, _setextattr },		/* setextattr */
	{ &vop_listextattr_desc, _listextattr },	/* listextattr */
	{ &vop_deleteextattr_desc, _deleteextattr },	/* deleteextattr */
#endif
	{ NULL, NULL }
};
const struct vnodeopv_desc puffs_fifoop_opv_desc =
	{ &puffs_fifoop_p, puffs_fifoop_entries };


/* "real" vnode operations */
int (**puffs_msgop_p)(void *);
const struct vnodeopv_entry_desc puffs_msgop_entries[] = {
	{ &vop_default_desc, vn_default_error },
	{ &vop_create_desc, puffs_create },		/* create */
        { &vop_mknod_desc, puffs_mknod },		/* mknod */
        { &vop_open_desc, puffs_open },			/* open */
        { &vop_close_desc, puffs_close },		/* close */
        { &vop_access_desc, puffs_access },		/* access */
        { &vop_getattr_desc, puffs_getattr },		/* getattr */
        { &vop_setattr_desc, puffs_setattr },		/* setattr */
        { &vop_read_desc, puffs_read },			/* read */
        { &vop_write_desc, puffs_write },		/* write */
        { &vop_fcntl_desc, puffs_fcntl },		/* fcntl */
        { &vop_ioctl_desc, puffs_ioctl },		/* ioctl */
        { &vop_seek_desc, puffs_seek },			/* seek */
        { &vop_remove_desc, puffs_remove },		/* remove */
        { &vop_link_desc, puffs_link },			/* link */
        { &vop_rename_desc, puffs_rename },		/* rename */
        { &vop_mkdir_desc, puffs_mkdir },		/* mkdir */
        { &vop_rmdir_desc, puffs_rmdir },		/* rmdir */
        { &vop_symlink_desc, puffs_symlink },		/* symlink */
        { &vop_readdir_desc, puffs_readdir },		/* readdir */
        { &vop_readlink_desc, puffs_readlink },		/* readlink */
        { &vop_print_desc, puffs_print },		/* print */
        { &vop_islocked_desc, puffs_islocked },		/* islocked */
        { &vop_pathconf_desc, puffs_pathconf },		/* pathconf */
        { &vop_advlock_desc, puffs_advlock },		/* advlock */
        { &vop_getpages_desc, puffs_getpages },		/* getpages */
	{ NULL, NULL }
};
const struct vnodeopv_desc puffs_msgop_opv_desc =
	{ &puffs_msgop_p, puffs_msgop_entries };


#define LOCKEDVP(a) (VOP_ISLOCKED(a) ? (a) : NULL)


/*
 * This is a generic vnode operation handler.  It checks if the necessary
 * operations for the called vnode operation are implemented by userspace
 * and either returns a dummy return value or proceeds to call the real
 * vnode operation from puffs_msgop_v.
 *
 * XXX: this should described elsewhere and autogenerated, the complexity
 * of the vnode operations vectors and their interrelationships is also
 * getting a bit out of hand.  Another problem is that we need this same
 * information in the fs server code, so keeping the two in sync manually
 * is not a viable (long term) plan.
 */

/* not supported, handle locking protocol */
#define CHECKOP_NOTSUPP(op)						\
case VOP_##op##_DESCOFFSET:						\
	if (pmp->pmp_vnopmask[PUFFS_VN_##op] == 0)			\
		return genfs_eopnotsupp(v);				\
	break

/* always succeed, no locking */
#define CHECKOP_SUCCESS(op)						\
case VOP_##op##_DESCOFFSET:						\
	if (pmp->pmp_vnopmask[PUFFS_VN_##op] == 0)			\
		return 0;						\
	break

int
puffs_checkop(void *v)
{
	struct vop_generic_args /* {
		struct vnodeop_desc *a_desc;
		spooky mystery contents;
	} */ *ap = v;
	struct vnodeop_desc *desc = ap->a_desc;
	struct puffs_mount *pmp;
	struct vnode *vp;
	int offset;

	DPRINTF_VERBOSE(("checkop call %s (%d)\n",
	    ap->a_desc->vdesc_name, ap->a_desc->vdesc_offset));

	offset = ap->a_desc->vdesc_vp_offsets[0];
#ifdef DIAGNOSTIC
	if (offset == VDESC_NO_OFFSET)
		panic("puffs_checkop: no vnode, why did you call me?");
#endif
	vp = *VOPARG_OFFSETTO(struct vnode **, offset, ap);
	pmp = MPTOPUFFSMP(vp->v_mount);

	if ((pmp->pmp_flags & PUFFS_KFLAG_ALLOPS) == 0) {
		switch (desc->vdesc_offset) {
			CHECKOP_NOTSUPP(CREATE);
			CHECKOP_NOTSUPP(MKNOD);
			CHECKOP_NOTSUPP(GETATTR);
			CHECKOP_NOTSUPP(SETATTR);
			CHECKOP_NOTSUPP(READ);
			CHECKOP_NOTSUPP(WRITE);
			CHECKOP_NOTSUPP(FCNTL);
			CHECKOP_NOTSUPP(IOCTL);
			CHECKOP_NOTSUPP(REMOVE);
			CHECKOP_NOTSUPP(LINK);
			CHECKOP_NOTSUPP(RENAME);
			CHECKOP_NOTSUPP(MKDIR);
			CHECKOP_NOTSUPP(RMDIR);
			CHECKOP_NOTSUPP(SYMLINK);
			CHECKOP_NOTSUPP(READDIR);
			CHECKOP_NOTSUPP(READLINK);
			CHECKOP_NOTSUPP(PRINT);
			CHECKOP_NOTSUPP(PATHCONF);
			CHECKOP_NOTSUPP(ADVLOCK);

			CHECKOP_SUCCESS(ACCESS);
			CHECKOP_SUCCESS(CLOSE);
			CHECKOP_SUCCESS(SEEK);

		case VOP_GETPAGES_DESCOFFSET:
			if (!EXISTSOP(pmp, READ))
				return genfs_eopnotsupp(v);
			break;

		default:
			panic("puffs_checkop: unhandled vnop %d",
			    desc->vdesc_offset);
		}
	}

	return VOCALL(puffs_msgop_p, ap->a_desc->vdesc_offset, v);
}


int
puffs_lookup(void *v)
{
        struct vop_lookup_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_dvp;
		struct vnode **a_vpp;
		struct componentname *a_cnp;
        } */ *ap = v;
	struct puffs_mount *pmp;
	struct componentname *cnp;
	struct vnode *vp, *dvp;
	struct puffs_node *dpn;
	int isdot;
	int error;

	PUFFS_VNREQ(lookup);

	pmp = MPTOPUFFSMP(ap->a_dvp->v_mount);
	cnp = ap->a_cnp;
	dvp = ap->a_dvp;
	*ap->a_vpp = NULL;

	/* first things first: check access */
	error = VOP_ACCESS(dvp, VEXEC, cnp->cn_cred, cnp->cn_lwp);
	if (error)
		return error;

	isdot = cnp->cn_namelen == 1 && *cnp->cn_nameptr == '.';

	DPRINTF(("puffs_lookup: \"%s\", parent vnode %p, op: %lx\n",
	    cnp->cn_nameptr, dvp, cnp->cn_nameiop));

	/*
	 * Check if someone fed it into the cache
	 */
	if (PUFFS_DOCACHE(pmp)) {
		error = cache_lookup(dvp, ap->a_vpp, cnp);

		if (error >= 0)
			return error;
	}

	if (isdot) {
		vp = ap->a_dvp;
		vref(vp);
		*ap->a_vpp = vp;
		return 0;
	}

	puffs_makecn(&lookup_arg.pvnr_cn, cnp);

	if (cnp->cn_flags & ISDOTDOT)
		VOP_UNLOCK(dvp, 0);

	error = puffs_vntouser(pmp, PUFFS_VN_LOOKUP,
	    &lookup_arg, sizeof(lookup_arg), 0, VPTOPNC(dvp),
	    LOCKEDVP(dvp), NULL);
	DPRINTF(("puffs_lookup: return of the userspace, part %d\n", error));

	/*
	 * In case of error, there is no new vnode to play with, so be
	 * happy with the NULL value given to vpp in the beginning.
	 * Also, check if this really was an error or the target was not
	 * present.  Either treat it as a non-error for CREATE/RENAME or
	 * enter the component into the negative name cache (if desired).
	 */
	if (error) {
		if (error == ENOENT) {
			if ((cnp->cn_flags & ISLASTCN)
			    && (cnp->cn_nameiop == CREATE
			      || cnp->cn_nameiop == RENAME)) {
				cnp->cn_flags |= SAVENAME;
				error = EJUSTRETURN;
			} else {
				if ((cnp->cn_flags & MAKEENTRY)
				    && PUFFS_DOCACHE(pmp))
					cache_enter(dvp, NULL, cnp);
			}
		} else if (error < 0) {
			error = EINVAL;
		}
		goto errout;
	}

	/*
	 * Check that we don't get our parent node back, that would cause
	 * a pretty obvious deadlock.
	 */
	dpn = dvp->v_data;
	if (lookup_arg.pvnr_newnode == dpn->pn_cookie) {
		error = EINVAL;
		goto errout;
	}

	/* XXX: race here */
	/* XXX2: this check for node existence twice */
	vp = puffs_pnode2vnode(pmp, lookup_arg.pvnr_newnode, 1);
	if (!vp) {
		error = puffs_getvnode(dvp->v_mount,
		    lookup_arg.pvnr_newnode, lookup_arg.pvnr_vtype,
		    lookup_arg.pvnr_size, lookup_arg.pvnr_rdev, &vp);
		if (error) {
			goto errout;
		}
		vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	}
	*ap->a_vpp = vp;

	if ((cnp->cn_flags & MAKEENTRY) != 0 && PUFFS_DOCACHE(pmp))
		cache_enter(dvp, vp, cnp);

 errout:
	if (cnp->cn_flags & ISDOTDOT)
		vn_lock(dvp, LK_EXCLUSIVE | LK_RETRY);
	
	DPRINTF(("puffs_lookup: returning %d %p\n", error, *ap->a_vpp));
	return error;
}

int
puffs_create(void *v)
{
	struct vop_create_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_dvp;
		struct vnode **a_vpp;
		struct componentname *a_cnp;
		struct vattr *a_vap;
	} */ *ap = v;
	int error;

	PUFFS_VNREQ(create);
	DPRINTF(("puffs_create: dvp %p, cnp: %s\n",
	    ap->a_dvp, ap->a_cnp->cn_nameptr));

	puffs_makecn(&create_arg.pvnr_cn, ap->a_cnp);
	create_arg.pvnr_va = *ap->a_vap;

	error = puffs_vntouser(MPTOPUFFSMP(ap->a_dvp->v_mount), PUFFS_VN_CREATE,
	    &create_arg, sizeof(create_arg), 0, VPTOPNC(ap->a_dvp),
	    ap->a_dvp, NULL);
	if (error)
		goto out;

	error = puffs_newnode(ap->a_dvp->v_mount, ap->a_dvp, ap->a_vpp,
	    create_arg.pvnr_newnode, ap->a_cnp, ap->a_vap->va_type, 0);
	/* XXX: in case of error, need to uncommit userspace transaction */

 out:
	if (error || (ap->a_cnp->cn_flags & SAVESTART) == 0)
		PNBUF_PUT(ap->a_cnp->cn_pnbuf);
	vput(ap->a_dvp);

	DPRINTF(("puffs_create: return %d\n", error));
	return error;
}

int
puffs_mknod(void *v)
{
	struct vop_mknod_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_dvp;
		struct vnode **a_vpp;
		struct componentname *a_cnp;
		struct vattr *a_vap;
	} */ *ap = v;
	int error;

	PUFFS_VNREQ(mknod);

	puffs_makecn(&mknod_arg.pvnr_cn, ap->a_cnp);
	mknod_arg.pvnr_va = *ap->a_vap;

	error = puffs_vntouser(MPTOPUFFSMP(ap->a_dvp->v_mount), PUFFS_VN_MKNOD,
	    &mknod_arg, sizeof(mknod_arg), 0, VPTOPNC(ap->a_dvp),
	    ap->a_dvp, NULL);
	if (error)
		goto out;

	error = puffs_newnode(ap->a_dvp->v_mount, ap->a_dvp, ap->a_vpp,
	    mknod_arg.pvnr_newnode, ap->a_cnp, ap->a_vap->va_type,
	    ap->a_vap->va_rdev);

 out:
	if (error || (ap->a_cnp->cn_flags & SAVESTART) == 0)
		PNBUF_PUT(ap->a_cnp->cn_pnbuf);
	vput(ap->a_dvp);
	return error;
}

int
puffs_open(void *v)
{
	struct vop_open_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
		int a_mode;
		kauth_cred_t a_cred;
		struct lwp *a_l;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;
	struct puffs_mount *pmp = MPTOPUFFSMP(vp->v_mount);
	int mode = ap->a_mode;
	int rv;

	PUFFS_VNREQ(open);
	DPRINTF(("puffs_open: vp %p, mode 0x%x\n", vp, mode));

	if (vp->v_type == VREG && mode & FWRITE && !EXISTSOP(pmp, WRITE)) {
		rv = EROFS;
		goto out;
	}

	if (!EXISTSOP(pmp, OPEN)) {
		rv = 0;
		goto out;
	}

	open_arg.pvnr_mode = mode;
	puffs_credcvt(&open_arg.pvnr_cred, ap->a_cred);
	open_arg.pvnr_pid = puffs_lwp2pid(ap->a_l);

	rv = puffs_vntouser(MPTOPUFFSMP(vp->v_mount), PUFFS_VN_OPEN,
	    &open_arg, sizeof(open_arg), 0, VPTOPNC(vp), vp, NULL);

 out:
	DPRINTF(("puffs_open: returning %d\n", rv));
	return rv;
}

int
puffs_close(void *v)
{
	struct vop_close_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
		int a_fflag;
		kauth_cred_t a_cred;
		struct lwp *a_l;
	} */ *ap = v;

	PUFFS_VNREQ(close);

	close_arg.pvnr_fflag = ap->a_fflag;
	puffs_credcvt(&close_arg.pvnr_cred, ap->a_cred);
	close_arg.pvnr_pid = puffs_lwp2pid(ap->a_l);

	return puffs_vntouser(MPTOPUFFSMP(ap->a_vp->v_mount), PUFFS_VN_CLOSE,
	    &close_arg, sizeof(close_arg), 0, VPTOPNC(ap->a_vp),
	    ap->a_vp, NULL);
}

int
puffs_access(void *v)
{
	struct vop_access_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
		int a_mode;
		kauth_cred_t a_cred;
		struct lwp *a_l;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;
	struct puffs_mount *pmp = MPTOPUFFSMP(vp->v_mount);
	int mode = ap->a_mode;

	PUFFS_VNREQ(access);

	if (vp->v_type == VREG && mode & VWRITE && !EXISTSOP(pmp, WRITE))
		return EROFS;

	if (!EXISTSOP(pmp, ACCESS))
		return 0;

	access_arg.pvnr_mode = ap->a_mode;
	access_arg.pvnr_pid = puffs_lwp2pid(ap->a_l);
	puffs_credcvt(&access_arg.pvnr_cred, ap->a_cred);

	return puffs_vntouser(MPTOPUFFSMP(vp->v_mount), PUFFS_VN_ACCESS,
	    &access_arg, sizeof(access_arg), 0, VPTOPNC(vp), vp, NULL);
}

int
puffs_getattr(void *v)
{
	struct vop_getattr_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
		struct vattr *a_vap;
		kauth_cred_t a_cred;
		struct lwp *a_l;
	} */ *ap = v;
	struct mount *mp;
	struct vnode *vp;
	struct vattr *vap;
	struct puffs_node *pn;
	int error;

	PUFFS_VNREQ(getattr);

	vp = ap->a_vp;
	mp = vp->v_mount;
	vap = ap->a_vap;

	vattr_null(&getattr_arg.pvnr_va);
	puffs_credcvt(&getattr_arg.pvnr_cred, ap->a_cred);
	getattr_arg.pvnr_pid = puffs_lwp2pid(ap->a_l);

	/*
	 * XXX + XX (dos equis): this can't go through the unlock/lock
	 * cycle, since it can be called from uvn_attach(), which fiddles
	 * around with VXLOCK and therefore breaks vn_lock().  Proper
	 * fix pending.
	 */
	error = puffs_vntouser(MPTOPUFFSMP(vp->v_mount), PUFFS_VN_GETATTR,
	    &getattr_arg, sizeof(getattr_arg), 0, VPTOPNC(vp),
	    NULL /* XXXseeabove: should be LOCKEDVP(vp) */, NULL);
	if (error)
		return error;

	(void) memcpy(vap, &getattr_arg.pvnr_va, sizeof(struct vattr));
	vap->va_fsid = mp->mnt_stat.f_fsidx.__fsid_val[0];

	pn = VPTOPP(vp);
	if (pn->pn_stat & PNODE_METACACHE_ATIME)
		vap->va_atime = pn->pn_mc_atime;
	if (pn->pn_stat & PNODE_METACACHE_CTIME)
		vap->va_ctime = pn->pn_mc_ctime;
	if (pn->pn_stat & PNODE_METACACHE_MTIME)
		vap->va_mtime = pn->pn_mc_mtime;
	if (pn->pn_stat & PNODE_METACACHE_SIZE) {
		vap->va_size = pn->pn_mc_size;
	} else {
		if (getattr_arg.pvnr_va.va_size != VNOVAL)
			uvm_vnp_setsize(vp, getattr_arg.pvnr_va.va_size);
	}

	return 0;
}

int
puffs_setattr(void *v)
{
	struct vop_getattr_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
		struct vattr *a_vap;
		kauth_cred_t a_cred;
		struct lwp *a_l;
	} */ *ap = v;
	int error;

	PUFFS_VNREQ(setattr);

	(void)memcpy(&setattr_arg.pvnr_va, ap->a_vap, sizeof(struct vattr));
	puffs_credcvt(&setattr_arg.pvnr_cred, ap->a_cred);
	setattr_arg.pvnr_pid = puffs_lwp2pid(ap->a_l);

	error = puffs_vntouser(MPTOPUFFSMP(ap->a_vp->v_mount), PUFFS_VN_SETATTR,
	    &setattr_arg, sizeof(setattr_arg), 0, VPTOPNC(ap->a_vp),
	    ap->a_vp, NULL);
	if (error)
		return error;

	if (ap->a_vap->va_size != VNOVAL)
		uvm_vnp_setsize(ap->a_vp, ap->a_vap->va_size);

	return 0;
}

int
puffs_inactive(void *v)
{
	struct vop_inactive_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
		struct lwp *a_l;
	} */ *ap = v;
	struct puffs_mount *pmp;
	struct puffs_node *pnode;
	int rv, vnrefs;

	PUFFS_VNREQ(inactive);

	/*
	 * XXX: think about this after we really start unlocking
	 * when going to userspace
	 */
	pnode = ap->a_vp->v_data;

	pmp = MPTOPUFFSMP(ap->a_vp->v_mount);

	inactive_arg.pvnr_pid = puffs_lwp2pid(ap->a_l);

	if (EXISTSOP(pmp, INACTIVE))
		rv = puffs_vntouser(pmp, PUFFS_VN_INACTIVE,
		    &inactive_arg, sizeof(inactive_arg), 0, VPTOPNC(ap->a_vp),
		    ap->a_vp, NULL);
	else
		rv = 1; /* see below */

	/* can't trust userspace return value?  simulate safe answer */
	if (rv)
		vnrefs = 1;
	else
		vnrefs = inactive_arg.pvnr_backendrefs;

	VOP_UNLOCK(ap->a_vp, 0);

	/*
	 * user server thinks it's gone?  then don't be afraid care,
	 * node's life was already all it would ever be
	 */
	if (vnrefs == 0) {
		pnode->pn_stat |= PNODE_NOREFS;
		vrecycle(ap->a_vp, NULL, ap->a_l);
	}

	return 0;
}

/*
 * always FAF, we don't really care if the server wants to fail to
 * reclaim the node or not
 */
int
puffs_reclaim(void *v)
{
	struct vop_reclaim_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
		struct lwp *a_l;
	} */ *ap = v;
	struct puffs_mount *pmp;
	struct puffs_vnreq_reclaim *reclaim_argp;

	pmp = MPTOPUFFSMP(ap->a_vp->v_mount);

	/*
	 * first things first: check if someone is trying to reclaim the
	 * root vnode.  do not allow that to travel to userspace.
	 * Note that we don't need to take the lock similarly to
	 * puffs_root(), since there is only one of us.
	 */
	if (ap->a_vp->v_flag & VROOT) {
		mutex_enter(&pmp->pmp_lock);
		KASSERT(pmp->pmp_root != NULL);
		pmp->pmp_root = NULL;
		mutex_exit(&pmp->pmp_lock);
		goto out;
	}

	if (!EXISTSOP(pmp, RECLAIM))
		goto out;

	reclaim_argp = malloc(sizeof(struct puffs_vnreq_reclaim),
	    M_PUFFS, M_WAITOK | M_ZERO);
	reclaim_argp->pvnr_pid = puffs_lwp2pid(ap->a_l);

	puffs_vntouser_faf(pmp, PUFFS_VN_RECLAIM,
	    reclaim_argp, sizeof(struct puffs_vnreq_reclaim),
	    VPTOPNC(ap->a_vp));

 out:
	if (PUFFS_DOCACHE(pmp))
		cache_purge(ap->a_vp);
	puffs_putvnode(ap->a_vp);

	return 0;
}

#define CSIZE sizeof(*ap->a_cookies)
int
puffs_readdir(void *v)
{
	struct vop_readdir_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
		struct uio *a_uio;
		kauth_cred_t a_cred;
		int *a_eofflag;
		off_t **a_cookies;
		int *a_ncookies;
	} */ *ap = v;
	struct puffs_mount *pmp = MPTOPUFFSMP(ap->a_vp->v_mount);
	struct puffs_vnreq_readdir *readdir_argp;
	size_t argsize, cookiemem, cookiesmax;
	struct uio *uio = ap->a_uio;
	size_t howmuch;
	int error;

	if (ap->a_cookies) {
		KASSERT(ap->a_ncookies != NULL);
		if (pmp->pmp_args.pa_fhsize == 0)
			return EOPNOTSUPP;
		cookiesmax = uio->uio_resid/_DIRENT_MINSIZE((struct dirent *)0);
		cookiemem = ALIGN(cookiesmax*CSIZE); /* play safe */
	} else {
		cookiesmax = 0;
		cookiemem = 0;
	}

	argsize = sizeof(struct puffs_vnreq_readdir)
	    + uio->uio_resid + cookiemem;
	readdir_argp = malloc(argsize, M_PUFFS, M_ZERO | M_WAITOK);

	puffs_credcvt(&readdir_argp->pvnr_cred, ap->a_cred);
	readdir_argp->pvnr_offset = uio->uio_offset;
	readdir_argp->pvnr_resid = uio->uio_resid;
	readdir_argp->pvnr_ncookies = cookiesmax;
	readdir_argp->pvnr_eofflag = 0;
	readdir_argp->pvnr_dentoff = cookiemem;

	error = puffs_vntouser(MPTOPUFFSMP(ap->a_vp->v_mount),
	    PUFFS_VN_READDIR, readdir_argp, argsize,
	    uio->uio_resid + cookiemem, VPTOPNC(ap->a_vp), ap->a_vp, NULL);
	if (error)
		goto out;

	/* userspace is cheating? */
	if (readdir_argp->pvnr_resid > uio->uio_resid
	    || readdir_argp->pvnr_ncookies > cookiesmax) {
		error = EINVAL;
		goto out;
	}

	/* check eof */
	if (readdir_argp->pvnr_eofflag)
		*ap->a_eofflag = 1;

	/* bouncy-wouncy with the directory data */
	howmuch = uio->uio_resid - readdir_argp->pvnr_resid;

	/* force eof if no data was returned (getcwd() needs this) */
	if (howmuch == 0) {
		*ap->a_eofflag = 1;
		goto out;
	}

	error = uiomove(readdir_argp->pvnr_data + cookiemem, howmuch, uio);
	if (error)
		goto out;

	/* provide cookies to caller if so desired */
	if (ap->a_cookies) {
		*ap->a_cookies = malloc(readdir_argp->pvnr_ncookies*CSIZE,
		    M_TEMP, M_WAITOK);
		*ap->a_ncookies = readdir_argp->pvnr_ncookies;
		memcpy(*ap->a_cookies, readdir_argp->pvnr_data,
		    *ap->a_ncookies*CSIZE);
	}

	/* next readdir starts here */
	uio->uio_offset = readdir_argp->pvnr_offset;

 out:
	free(readdir_argp, M_PUFFS);
	return error;
}
#undef CSIZE

int
puffs_poll(void *v)
{
	struct vop_poll_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
		int a_events;
		struct lwp *a_l;
	} */ *ap = v;

	PUFFS_VNREQ(poll);

	poll_arg.pvnr_events = ap->a_events;
	poll_arg.pvnr_pid = puffs_lwp2pid(ap->a_l);

	return puffs_vntouser(MPTOPUFFSMP(ap->a_vp->v_mount), PUFFS_VN_POLL,
	    &poll_arg, sizeof(poll_arg), 0, VPTOPNC(ap->a_vp), NULL, NULL);
}

int
puffs_fsync(void *v)
{
	struct vop_fsync_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
		kauth_cred_t a_cred;
		int a_flags;
		off_t a_offlo;
		off_t a_offhi;
		struct lwp *a_l;
	} */ *ap = v;
	struct vattr va;
	struct puffs_mount *pmp;
	struct puffs_vnreq_fsync *fsync_argp;
	struct vnode *vp;
	struct puffs_node *pn;
	int pflags, error, dofaf;

	PUFFS_VNREQ(fsync);

	vp = ap->a_vp;
	pn = VPTOPP(vp);
	pmp = MPTOPUFFSMP(vp->v_mount);

	/* flush out information from our metacache */
	if (pn->pn_stat & PNODE_METACACHE_MASK) {
		vattr_null(&va);
		if (pn->pn_stat & PNODE_METACACHE_ATIME)
			va.va_atime = pn->pn_mc_atime;
		if (pn->pn_stat & PNODE_METACACHE_CTIME)
			va.va_ctime = pn->pn_mc_ctime;
		if (pn->pn_stat & PNODE_METACACHE_MTIME)
			va.va_mtime = pn->pn_mc_ctime;
		if (pn->pn_stat & PNODE_METACACHE_SIZE)
			va.va_size = pn->pn_mc_size;

		error = VOP_SETATTR(vp, &va, FSCRED, NULL); 
		if (error)
			return error;

		pn->pn_stat &= ~PNODE_METACACHE_MASK;
	}

	/*
	 * flush pages to avoid being overly dirty
	 */
	pflags = PGO_CLEANIT;
	if (ap->a_flags & FSYNC_WAIT)
		pflags |= PGO_SYNCIO;
	simple_lock(&vp->v_interlock);
	error = VOP_PUTPAGES(vp, trunc_page(ap->a_offlo),
	    round_page(ap->a_offhi), pflags);
	if (error)
		return error;

	/*
	 * HELLO!  We exit already here if the user server does not
	 * support fsync OR if we should call fsync for a node which
	 * has references neither in the kernel or the fs server.
	 * Otherwise we continue to issue fsync() forward.
	 */
	if (!EXISTSOP(pmp, FSYNC) || (pn->pn_stat & PNODE_NOREFS))
		return 0;

	dofaf = (ap->a_flags & FSYNC_WAIT) == 0 || ap->a_flags == FSYNC_LAZY;
	/*
	 * We abuse VXLOCK to mean "vnode is going to die", so we issue
	 * only FAFs for those.  Otherwise there's a danger of deadlock,
	 * since the execution context here might be the user server
	 * doing some operation on another fs, which in turn caused a
	 * vnode to be reclaimed from the freelist for this fs.
	 */
	if (dofaf == 0) {
		simple_lock(&vp->v_interlock);
		if (vp->v_flag & VXLOCK)
			dofaf = 1;
		simple_unlock(&vp->v_interlock);
	}

	if (dofaf == 0) {
		fsync_argp = &fsync_arg;
	} else {
		fsync_argp = malloc(sizeof(struct puffs_vnreq_fsync),
		    M_PUFFS, M_ZERO | M_NOWAIT);
		if (fsync_argp == NULL)
			return ENOMEM;
	}

	puffs_credcvt(&fsync_argp->pvnr_cred, ap->a_cred);
	fsync_argp->pvnr_flags = ap->a_flags;
	fsync_argp->pvnr_offlo = ap->a_offlo;
	fsync_argp->pvnr_offhi = ap->a_offhi;
	fsync_argp->pvnr_pid = puffs_lwp2pid(ap->a_l);

	/*
	 * XXX: see comment at puffs_getattr about locking
	 *
	 * If we are not required to wait, do a FAF operation.
	 * Otherwise block here.
	 */
	if (dofaf == 0) {
		error =  puffs_vntouser(MPTOPUFFSMP(vp->v_mount),
		    PUFFS_VN_FSYNC, fsync_argp, sizeof(*fsync_argp), 0,
		    VPTOPNC(vp), NULL /* XXXshouldbe: vp */, NULL);
	} else {
		/* FAF is always "succesful" */
		error = 0;
		puffs_vntouser_faf(MPTOPUFFSMP(vp->v_mount),
		    PUFFS_VN_FSYNC, fsync_argp, sizeof(*fsync_argp),
		    VPTOPNC(vp));
	}

	return error;
}

int
puffs_seek(void *v)
{
	struct vop_seek_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
		off_t a_oldoff;
		off_t a_newoff;
		kauth_cred_t a_cred;
	} */ *ap = v;

	PUFFS_VNREQ(seek);

	seek_arg.pvnr_oldoff = ap->a_oldoff;
	seek_arg.pvnr_newoff = ap->a_newoff;
	puffs_credcvt(&seek_arg.pvnr_cred, ap->a_cred);

	/*
	 * XXX: seems like seek is called with an unlocked vp, but
	 * it can't hurt to play safe
	 */
	return puffs_vntouser(MPTOPUFFSMP(ap->a_vp->v_mount), PUFFS_VN_SEEK,
	    &seek_arg, sizeof(seek_arg), 0, VPTOPNC(ap->a_vp),
	    LOCKEDVP(ap->a_vp), NULL);
}

int
puffs_remove(void *v)
{
	struct vop_remove_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_dvp;
		struct vnode *a_vp;
		struct componentname *a_cnp;
	} */ *ap = v;
	int error;

	PUFFS_VNREQ(remove);

	remove_arg.pvnr_cookie_targ = VPTOPNC(ap->a_vp);
	puffs_makecn(&remove_arg.pvnr_cn, ap->a_cnp);

	error = puffs_vntouser(MPTOPUFFSMP(ap->a_vp->v_mount), PUFFS_VN_REMOVE,
	    &remove_arg, sizeof(remove_arg), 0, VPTOPNC(ap->a_dvp),
	    ap->a_dvp, ap->a_vp);

	vput(ap->a_vp);
	if (ap->a_dvp == ap->a_vp)
		vrele(ap->a_dvp);
	else
		vput(ap->a_dvp);

	return error;
}

int
puffs_mkdir(void *v)
{
	struct vop_mkdir_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_dvp;
		struct vnode **a_vpp;
		struct componentname *a_cnp;
		struct vattr *a_vap;
	} */ *ap = v;
	int error;

	PUFFS_VNREQ(mkdir);

	puffs_makecn(&mkdir_arg.pvnr_cn, ap->a_cnp);
	mkdir_arg.pvnr_va = *ap->a_vap;

	/* XXX: wouldn't need to relock dvp, but that's life */
	error = puffs_vntouser(MPTOPUFFSMP(ap->a_dvp->v_mount), PUFFS_VN_MKDIR,
	    &mkdir_arg, sizeof(mkdir_arg), 0, VPTOPNC(ap->a_dvp),
	    ap->a_dvp, NULL);
	if (error)
		goto out;

	error = puffs_newnode(ap->a_dvp->v_mount, ap->a_dvp, ap->a_vpp,
	    mkdir_arg.pvnr_newnode, ap->a_cnp, VDIR, 0);

 out:
	if (error || (ap->a_cnp->cn_flags & SAVESTART) == 0)
		PNBUF_PUT(ap->a_cnp->cn_pnbuf);
	vput(ap->a_dvp);
	return error;
}

int
puffs_rmdir(void *v)
{
	struct vop_rmdir_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_dvp;
		struct vnode *a_vp;
		struct componentname *a_cnp;
	} */ *ap = v;
	int error;

	PUFFS_VNREQ(rmdir);

	rmdir_arg.pvnr_cookie_targ = VPTOPNC(ap->a_vp);
	puffs_makecn(&rmdir_arg.pvnr_cn, ap->a_cnp);

	error = puffs_vntouser(MPTOPUFFSMP(ap->a_dvp->v_mount), PUFFS_VN_RMDIR,
	    &rmdir_arg, sizeof(rmdir_arg), 0, VPTOPNC(ap->a_dvp),
	    ap->a_dvp, ap->a_vp);

	/* XXX: some call cache_purge() *for both vnodes* here, investigate */

	vput(ap->a_dvp);
	vput(ap->a_vp);

	return error;
}

int
puffs_link(void *v)
{
	struct vop_link_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_dvp;
		struct vnode *a_vp;
		struct componentname *a_cnp;
	} */ *ap = v;
	int error;

	PUFFS_VNREQ(link);

	link_arg.pvnr_cookie_targ = VPTOPNC(ap->a_vp);
	puffs_makecn(&link_arg.pvnr_cn, ap->a_cnp);

	error = puffs_vntouser(MPTOPUFFSMP(ap->a_dvp->v_mount), PUFFS_VN_LINK,
	    &link_arg, sizeof(link_arg), 0, VPTOPNC(ap->a_dvp),
	    ap->a_dvp, NULL);

	/*
	 * XXX: stay in touch with the cache.  I don't like this, but
	 * don't have a better solution either.  See also puffs_rename().
	 */
	if (error == 0)
		puffs_updatenode(ap->a_vp, PUFFS_UPDATECTIME);

	vput(ap->a_dvp);

	return error;
}

int
puffs_symlink(void *v)
{
	struct vop_symlink_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_dvp;
		struct vnode **a_vpp;
		struct componentname *a_cnp;
		struct vattr *a_vap;
		char *a_target;
	} */ *ap = v;
	int error;

	PUFFS_VNREQ(symlink); /* XXX: large structure */

	*ap->a_vpp = NULL;

	puffs_makecn(&symlink_arg.pvnr_cn, ap->a_cnp);
	symlink_arg.pvnr_va = *ap->a_vap;
	(void)strlcpy(symlink_arg.pvnr_link, ap->a_target,
	    sizeof(symlink_arg.pvnr_link));

	/* XXX: don't need to relock parent */
	error =  puffs_vntouser(MPTOPUFFSMP(ap->a_dvp->v_mount),
	    PUFFS_VN_SYMLINK, &symlink_arg, sizeof(symlink_arg), 0,
	    VPTOPNC(ap->a_dvp), ap->a_dvp, NULL);
	if (error)
		goto out;

	error = puffs_newnode(ap->a_dvp->v_mount, ap->a_dvp, ap->a_vpp,
	    symlink_arg.pvnr_newnode, ap->a_cnp, VLNK, 0);

 out:
	if (error || (ap->a_cnp->cn_flags & SAVESTART) == 0)
		PNBUF_PUT(ap->a_cnp->cn_pnbuf);
	vput(ap->a_dvp);
	return error;
}

int
puffs_readlink(void *v)
{
	struct vop_readlink_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
		struct uio *a_uio;
		kauth_cred_t a_cred;
	} */ *ap = v;
	size_t linklen;
	int error;

	PUFFS_VNREQ(readlink);

	puffs_credcvt(&readlink_arg.pvnr_cred, ap->a_cred);
	linklen = sizeof(readlink_arg.pvnr_link);
	readlink_arg.pvnr_linklen = linklen;

	error = puffs_vntouser(MPTOPUFFSMP(ap->a_vp->v_mount),
	    PUFFS_VN_READLINK, &readlink_arg, sizeof(readlink_arg), 0,
	    VPTOPNC(ap->a_vp), ap->a_vp, NULL);
	if (error)
		return error;

	/* bad bad user file server */
	if (readlink_arg.pvnr_linklen > linklen)
		return EINVAL;

	return uiomove(&readlink_arg.pvnr_link, readlink_arg.pvnr_linklen,
	    ap->a_uio);
}

/* XXXXXXX: think about locking & userspace op delocking... */
int
puffs_rename(void *v)
{
	struct vop_rename_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_fdvp;
		struct vnode *a_fvp;
		struct componentname *a_fcnp;
		struct vnode *a_tdvp;
		struct vnode *a_tvp;
		struct componentname *a_tcnp;
	} */ *ap = v;
	int error;

	PUFFS_VNREQ(rename);

	if (ap->a_fvp->v_mount != ap->a_tdvp->v_mount) {
		error = EXDEV;
		goto out;
	}

	rename_arg.pvnr_cookie_src = VPTOPNC(ap->a_fvp);
	rename_arg.pvnr_cookie_targdir = VPTOPNC(ap->a_tdvp);
	if (ap->a_tvp)
		rename_arg.pvnr_cookie_targ = VPTOPNC(ap->a_tvp);
	else
		rename_arg.pvnr_cookie_targ = NULL;
	puffs_makecn(&rename_arg.pvnr_cn_src, ap->a_fcnp);
	puffs_makecn(&rename_arg.pvnr_cn_targ, ap->a_tcnp);

	error = puffs_vntouser(MPTOPUFFSMP(ap->a_fdvp->v_mount),
	    PUFFS_VN_RENAME, &rename_arg, sizeof(rename_arg), 0,
	    VPTOPNC(ap->a_fdvp), NULL, NULL);

	/*
	 * XXX: stay in touch with the cache.  I don't like this, but
	 * don't have a better solution either.  See also puffs_link().
	 */
	if (error == 0)
		puffs_updatenode(ap->a_fvp, PUFFS_UPDATECTIME);

 out:
	if (ap->a_tvp != NULL)
		vput(ap->a_tvp);
	if (ap->a_tdvp == ap->a_tvp)
		vrele(ap->a_tdvp);
	else
		vput(ap->a_tdvp);

	vrele(ap->a_fdvp);
	vrele(ap->a_fvp);

	return error;
}

int
puffs_read(void *v)
{
	struct vop_read_args /* { 
		const struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
		struct uio *a_uio;
		int a_ioflag;
		kauth_cred_t a_cred;
	} */ *ap = v;
	struct puffs_vnreq_read *read_argp;
	struct puffs_mount *pmp;
	struct vnode *vp;
	struct uio *uio;
	void *win;
	size_t tomove, argsize;
	vsize_t bytelen;
	int error, ubcflags;

	uio = ap->a_uio;
	vp = ap->a_vp;
	read_argp = NULL;
	error = 0;
	pmp = MPTOPUFFSMP(vp->v_mount);

	/* std sanity */
	if (uio->uio_resid == 0)
		return 0;
	if (uio->uio_offset < 0)
		return EINVAL;

	if (vp->v_type == VREG && PUFFS_DOCACHE(pmp)) {
		const int advice = IO_ADV_DECODE(ap->a_ioflag);

		ubcflags = 0;
		if (UBC_WANT_UNMAP(vp))
			ubcflags = UBC_UNMAP;

		while (uio->uio_resid > 0) {
			bytelen = MIN(uio->uio_resid,
			    vp->v_size - uio->uio_offset);
			if (bytelen == 0)
				break;

			win = ubc_alloc(&vp->v_uobj, uio->uio_offset,
			    &bytelen, advice, UBC_READ);
			error = uiomove(win, bytelen, uio);
			ubc_release(win, ubcflags);
			if (error)
				break;
		}

		if ((vp->v_mount->mnt_flag & MNT_NOATIME) == 0)
			puffs_updatenode(vp, PUFFS_UPDATEATIME);
	} else {
		/*
		 * in case it's not a regular file or we're operating
		 * uncached, do read in the old-fashioned style,
		 * i.e. explicit read operations
		 */

		tomove = PUFFS_TOMOVE(uio->uio_resid, pmp);
		argsize = sizeof(struct puffs_vnreq_read);
		read_argp = malloc(argsize + tomove,
		    M_PUFFS, M_WAITOK | M_ZERO);

		error = 0;
		while (uio->uio_resid > 0) {
			read_argp->pvnr_ioflag = ap->a_ioflag;
			read_argp->pvnr_resid = tomove;
			read_argp->pvnr_offset = uio->uio_offset;
			puffs_credcvt(&read_argp->pvnr_cred, ap->a_cred);

			argsize = sizeof(struct puffs_vnreq_read);
			error = puffs_vntouser(pmp, PUFFS_VN_READ,
			    read_argp, argsize, tomove,
			    VPTOPNC(ap->a_vp), ap->a_vp, NULL);
			if (error)
				break;

			if (read_argp->pvnr_resid > tomove) {
				error = EINVAL;
				break;
			}

			error = uiomove(read_argp->pvnr_data,
			    tomove - read_argp->pvnr_resid, uio);

			/*
			 * in case the file is out of juice, resid from
			 * userspace is != 0.  and the error-case is
			 * quite obvious
			 */
			if (error || read_argp->pvnr_resid)
				break;

			tomove = PUFFS_TOMOVE(uio->uio_resid, pmp);
		}
	}

	if (read_argp)
		free(read_argp, M_PUFFS);
	return error;
}

int
puffs_write(void *v)
{
	struct vop_write_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
		struct uio *a_uio;
		int a_ioflag;
		kauth_cred_t a_cred;
	} */ *ap = v;
	struct puffs_vnreq_write *write_argp;
	struct puffs_mount *pmp;
	struct uio *uio;
	struct vnode *vp;
	void *win;
	size_t tomove, argsize;
	off_t oldoff, newoff, origoff;
	vsize_t bytelen;
	int error, uflags;
	int ubcflags;

	vp = ap->a_vp;
	uio = ap->a_uio;
	error = uflags = 0;
	write_argp = NULL;
	pmp = MPTOPUFFSMP(ap->a_vp->v_mount);

	if (vp->v_type == VREG && PUFFS_DOCACHE(pmp)) {
		ubcflags = 0;
		if (UBC_WANT_UNMAP(vp))
			ubcflags = UBC_UNMAP;

		/*
		 * userspace *should* be allowed to control this,
		 * but with UBC it's a bit unclear how to handle it
		 */
		if (ap->a_ioflag & IO_APPEND)
			uio->uio_offset = vp->v_size;

		origoff = uio->uio_offset;
		while (uio->uio_resid > 0) {
			uflags |= PUFFS_UPDATECTIME;
			uflags |= PUFFS_UPDATEMTIME;
			oldoff = uio->uio_offset;
			bytelen = uio->uio_resid;

			win = ubc_alloc(&vp->v_uobj, oldoff, &bytelen,
			    UVM_ADV_NORMAL, UBC_WRITE);
			error = uiomove(win, bytelen, uio);

			/*
			 * did we grow the file?
			 * XXX: should probably ask userspace to extend
			 * it's idea of the *first* before growing it
			 * here.  Or we need some mechanism to "rollback"
			 * in case putpages fails.
			 */
			newoff = oldoff + bytelen;
			if (vp->v_size < newoff) {
				uflags |= PUFFS_UPDATESIZE;
				uvm_vnp_setsize(vp, newoff);

				/*
				 * in case we couldn't copy data to the
				 * window, zero it out so that we don't
				 * have any random leftovers in there.
				 */
				if (error)
					memset(win, 0, bytelen);
			}

			ubc_release(win, ubcflags);
			if (error)
				break;

			/*
			 * If we're writing large files, flush to file server
			 * every 64k.  Otherwise we can very easily exhaust
			 * kernel and user memory, as the file server cannot
			 * really keep up with our writing speed.
			 *
			 * Note: this does *NOT* honor MNT_ASYNC, because
			 * that gives userland too much say in the kernel.
			 */
			if (oldoff >> 16 != uio->uio_offset >> 16) {
				simple_lock(&vp->v_interlock);
				error = VOP_PUTPAGES(vp, oldoff & ~0xffff,
				    uio->uio_offset & ~0xffff,
				    PGO_CLEANIT | PGO_SYNCIO);
				if (error)
					break;
			}
		}

		if (error == 0 && ap->a_ioflag & IO_SYNC) {
			simple_lock(&vp->v_interlock);
			error = VOP_PUTPAGES(vp, trunc_page(origoff),
			    round_page(uio->uio_offset),
			    PGO_CLEANIT | PGO_SYNCIO);
		}

		puffs_updatenode(vp, uflags);
	} else {
		/* tomove is non-increasing */
		tomove = PUFFS_TOMOVE(uio->uio_resid, pmp);
		argsize = sizeof(struct puffs_vnreq_write) + tomove;
		write_argp = malloc(argsize, M_PUFFS, M_WAITOK | M_ZERO);

		while (uio->uio_resid > 0) {
			write_argp->pvnr_ioflag = ap->a_ioflag;
			write_argp->pvnr_resid = tomove;
			write_argp->pvnr_offset = uio->uio_offset;
			puffs_credcvt(&write_argp->pvnr_cred, ap->a_cred);
			error = uiomove(write_argp->pvnr_data, tomove, uio);
			if (error)
				break;

			error = puffs_vntouser(MPTOPUFFSMP(ap->a_vp->v_mount),
			    PUFFS_VN_WRITE, write_argp, argsize, 0,
			    VPTOPNC(ap->a_vp), ap->a_vp, NULL);
			if (error) {
				/* restore uiomove */
				uio->uio_resid += tomove;
				uio->uio_offset -= tomove;
				break;
			}
			if (write_argp->pvnr_resid > tomove) {
				/*
				 * XXX: correct file size is a mystery,
				 * we can only guess
				 */
				error = EINVAL;
				break;
			}

			/* adjust file size */
			if (vp->v_size < uio->uio_offset)
				uvm_vnp_setsize(vp, uio->uio_offset);

			/* didn't move everything?  bad userspace.  bail */
			if (write_argp->pvnr_resid != 0) {
				uio->uio_resid += write_argp->pvnr_resid;
				uio->uio_offset -= write_argp->pvnr_resid;
				error = EIO;
				break;
			}

			tomove = PUFFS_TOMOVE(uio->uio_resid, pmp);
		}
	}

	if (write_argp)
		free(write_argp, M_PUFFS);
	return error;
}

static int	puffs_fcnioctl(struct vop_ioctl_args * /*XXX*/, int);

#define FCNIOCTL_ARG_MAX 1<<16
int
puffs_fcnioctl(struct vop_ioctl_args *ap, int puffsop)
{

	/* currently not supported */
	return EOPNOTSUPP;
#if 0
	/* struct vop_ioctl_args {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
		u_long a_command;
		void *a_data;
		int a_fflag;
		kauth_cred_t a_cred;
		struct lwp *a_l;
	} *ap = v; */
	struct puffs_mount *pmp;
	struct puffs_sizepark pspark;
	void *kernbuf;
	size_t copylen;
	int error;

	PUFFS_VNREQ(fcnioctl);

	/*
	 * Since this op gives the filesystem (almost) complete control on
	 * how much it is allowed to copy from the calling process
	 * address space, do not enable it by default, since it would
	 * be a whopping security hole.
	 */
	pmp = MPTOPUFFSMP(ap->a_vp->v_mount);
	if ((pmp->pmp_flags & PUFFS_KFLAG_ALLOWCTL) == 0)
		return EINVAL; /* only shoe that fits */

	/* fill in sizereq and store it */
	pspark.pkso_reqid = puffs_getreqid(pmp);
	pspark.pkso_reqtype = PUFFS_SIZEOPREQ_BUF_IN;
	pspark.pkso_copybuf = ap->a_data;
	pspark.pkso_bufsize = FCNIOCTL_ARG_MAX;
	TAILQ_INSERT_TAIL(&pmp->pmp_req_sizepark, &pspark, pkso_entries);

	/* then fill in actual request and shoot it off */
	fcnioctl_arg.pvnr_command = ap->a_command;
	fcnioctl_arg.pvnr_fflag = ap->a_fflag;
	puffs_credcvt(&fcnioctl_arg.pvnr_cred, ap->a_cred);
	fcnioctl_arg.pvnr_pid = puffs_lwp2pid(ap->a_l);

	error = puffs_vntouser_req(MPTOPUFFSMP(ap->a_vp->v_mount), puffsop,
	    &fcnioctl_arg, sizeof(fcnioctl_arg), 0, VPTOPNC(ap->a_vp),
	    pspark.pkso_reqid, NULL, NULL);

	/* if we don't need to copy data, we're done */
	if (error || !fcnioctl_arg.pvnr_copyback)
		return error;

	copylen = MIN(FCNIOCTL_ARG_MAX, fcnioctl_arg.pvnr_datalen);
	kernbuf = malloc(copylen, M_PUFFS, M_WAITOK);
	error = copyin(fcnioctl_arg.pvnr_data, kernbuf, copylen);
	if (error)
		goto out;
	error = copyout(kernbuf, ap->a_data, copylen);

 out:
	free(kernbuf, M_PUFFS);
	return error;
#endif
}

int
puffs_ioctl(void *v)
{

	return puffs_fcnioctl(v, PUFFS_VN_IOCTL);
}

int
puffs_fcntl(void *v)
{

	return puffs_fcnioctl(v, PUFFS_VN_FCNTL);
}

int
puffs_print(void *v)
{
	struct vop_print_args /* {
		struct vnode *a_vp;
	} */ *ap = v;
	struct puffs_mount *pmp;
	struct vnode *vp = ap->a_vp;
	struct puffs_node *pn = vp->v_data;

	PUFFS_VNREQ(print);

	pmp = MPTOPUFFSMP(vp->v_mount);

	/* kernel portion */
	printf("tag VT_PUFFS, vnode %p, puffs node: %p,\n"
	    "    userspace cookie: %p\n", vp, pn, pn->pn_cookie);
	if (vp->v_type == VFIFO)
		fifo_printinfo(vp);
	lockmgr_printinfo(&vp->v_lock);

	/* userspace portion */
	if (EXISTSOP(pmp, PRINT))
		puffs_vntouser(pmp, PUFFS_VN_PRINT,
		    &print_arg, sizeof(print_arg), 0, VPTOPNC(ap->a_vp),
		    LOCKEDVP(ap->a_vp), NULL);
	
	return 0;
}

int
puffs_pathconf(void *v)
{
	struct vop_pathconf_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
		int a_name;
		register_t *a_retval;
	} */ *ap = v;
	int error;

	PUFFS_VNREQ(pathconf);

	pathconf_arg.pvnr_name = ap->a_name;

	error = puffs_vntouser(MPTOPUFFSMP(ap->a_vp->v_mount),
	    PUFFS_VN_PATHCONF, &pathconf_arg, sizeof(pathconf_arg), 0,
	    VPTOPNC(ap->a_vp), ap->a_vp, NULL);
	if (error)
		return error;

	*ap->a_retval = pathconf_arg.pvnr_retval;

	return 0;
}

int
puffs_advlock(void *v)
{
	struct vop_advlock_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
		void *a_id;
		int a_op;
		struct flock *a_fl;
		int a_flags;
	} */ *ap = v;
	int error;

	PUFFS_VNREQ(advlock);

	error = copyin(ap->a_fl, &advlock_arg.pvnr_fl, sizeof(struct flock));
	if (error)
		return error;
	advlock_arg.pvnr_id = ap->a_id;
	advlock_arg.pvnr_op = ap->a_op;
	advlock_arg.pvnr_flags = ap->a_flags;

	return puffs_vntouser(MPTOPUFFSMP(ap->a_vp->v_mount), PUFFS_VN_ADVLOCK,
	    &advlock_arg, sizeof(advlock_arg), 0, VPTOPNC(ap->a_vp),
	    NULL, NULL);
}
/*
 * This maps itself to PUFFS_VN_READ/WRITE for data transfer.
 */
int
puffs_strategy(void *v)
{
	struct vop_strategy_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
		struct buf *a_bp;
	} */ *ap = v;
	struct puffs_mount *pmp;
	struct vnode *vp = ap->a_vp;
	struct puffs_node *pn;
	struct puffs_vnreq_read *read_argp = NULL;
	struct puffs_vnreq_write *write_argp = NULL;
	struct buf *bp;
	size_t argsize;
	size_t tomove, moved;
	int error, dowritefaf;

	pmp = MPTOPUFFSMP(vp->v_mount);
	bp = ap->a_bp;
	error = 0;
	dowritefaf = 0;
	pn = VPTOPP(vp);

	if (((bp->b_flags & B_READ) && !EXISTSOP(pmp, READ))
	    || (((bp->b_flags & B_READ) == 0) && !EXISTSOP(pmp, WRITE))) {
		error = EOPNOTSUPP;
		goto out;
	}

	/*
	 * Short-circuit optimization: don't flush buffer in between
	 * VOP_INACTIVE and VOP_RECLAIM in case the node has no references.
	 */
	if (pn->pn_stat & PNODE_NOREFS) {
		bp->b_resid = 0;
		goto out;
	}

#ifdef DIAGNOSTIC
	if (bp->b_bcount > pmp->pmp_req_maxsize - PUFFS_REQSTRUCT_MAX)
		panic("puffs_strategy: wildly inappropriate buf bcount %d",
		    bp->b_bcount);
#endif

	/*
	 * See explanation for the necessity of a FAF in puffs_fsync.
	 *
	 * Also, do FAF in case we're suspending.
	 * See puffs_vfsops.c:pageflush()
	 *
	 * XXgoddamnX: B_WRITE is a "pseudo flag"
	 */
	if ((bp->b_flags & B_READ) == 0) {
		simple_lock(&vp->v_interlock);
		if (vp->v_flag & VXLOCK)
			dowritefaf = 1;
		if (pn->pn_stat & PNODE_SUSPEND)
			dowritefaf = 1;
		simple_unlock(&vp->v_interlock);
	}

	if (bp->b_flags & B_ASYNC)
		dowritefaf = 1;

#ifdef DIAGNOSTIC
	if (curproc == uvm.pagedaemon_proc)
		KASSERT(dowritefaf);
#endif

	tomove = PUFFS_TOMOVE(bp->b_bcount, pmp);

	if ((bp->b_flags & (B_READ | B_ASYNC)) == (B_READ | B_ASYNC)) {
		argsize = sizeof(struct puffs_vnreq_read);
		read_argp = malloc(argsize + tomove,
		    M_PUFFS, M_NOWAIT | M_ZERO);
		if (read_argp == NULL) {
			error = ENOMEM;
			goto out;
		}

		read_argp->pvnr_ioflag = 0;
		read_argp->pvnr_resid = tomove;
		read_argp->pvnr_offset = bp->b_blkno << DEV_BSHIFT;
		puffs_credcvt(&read_argp->pvnr_cred, FSCRED);

		puffs_vntouser_call(pmp, PUFFS_VN_READ, read_argp,
		    argsize, tomove, VPTOPNC(vp),
		    puffs_parkdone_asyncbioread, bp,
		    LOCKEDVP(vp), NULL);
		error = 0;
		goto wayout;
	} else if (bp->b_flags & B_READ) {
		argsize = sizeof(struct puffs_vnreq_read);
		read_argp = malloc(argsize + tomove,
		    M_PUFFS, M_WAITOK | M_ZERO);

		read_argp->pvnr_ioflag = 0;
		read_argp->pvnr_resid = tomove;
		read_argp->pvnr_offset = bp->b_blkno << DEV_BSHIFT;
		puffs_credcvt(&read_argp->pvnr_cred, FSCRED);

		error = puffs_vntouser(pmp, PUFFS_VN_READ,
		    read_argp, argsize, tomove,
		    VPTOPNC(vp), LOCKEDVP(vp), NULL);

		if (error)
			goto out;

		if (read_argp->pvnr_resid > tomove) {
			error = EINVAL;
			goto out;
		}

		moved = tomove - read_argp->pvnr_resid;

		(void)memcpy(bp->b_data, read_argp->pvnr_data, moved);
		bp->b_resid = bp->b_bcount - moved;
	} else {
		/*
		 * make pages read-only before we write them if we want
		 * write caching info
		 */
		if (PUFFS_WCACHEINFO(pmp)) {
			struct uvm_object *uobj = &vp->v_uobj;
			int npages = (bp->b_bcount + PAGE_SIZE-1) >> PAGE_SHIFT;
			struct vm_page *vmp;
			int i;

			for (i = 0; i < npages; i++) {
				vmp= uvm_pageratop((vaddr_t)bp->b_data
				    + (i << PAGE_SHIFT));
				DPRINTF(("puffs_strategy: write-protecting "
				    "vp %p page %p, offset %" PRId64"\n",
				    vp, vmp, vmp->offset));
				simple_lock(&uobj->vmobjlock);
				vmp->flags |= PG_RDONLY;
				pmap_page_protect(vmp, VM_PROT_READ);
				simple_unlock(&uobj->vmobjlock);
			}
		}

		argsize = sizeof(struct puffs_vnreq_write) + bp->b_bcount;
		write_argp = malloc(argsize, M_PUFFS, M_NOWAIT | M_ZERO);
		if (write_argp == NULL) {
			error = ENOMEM;
			goto out;
		}

		write_argp->pvnr_ioflag = 0;
		write_argp->pvnr_resid = tomove;
		write_argp->pvnr_offset = bp->b_blkno << DEV_BSHIFT;
		puffs_credcvt(&write_argp->pvnr_cred, FSCRED);

		(void)memcpy(&write_argp->pvnr_data, bp->b_data, tomove);

		if (dowritefaf) {
			/*
			 * assume FAF moves everything.  frankly, we don't
			 * really have a choice.
			 */
			puffs_vntouser_faf(MPTOPUFFSMP(vp->v_mount),
			    PUFFS_VN_WRITE, write_argp, argsize, VPTOPNC(vp));
			bp->b_resid = bp->b_bcount - tomove;
		} else {
			error = puffs_vntouser(MPTOPUFFSMP(vp->v_mount),
			    PUFFS_VN_WRITE, write_argp, argsize, 0, VPTOPNC(vp),
			    vp, NULL);
			if (error)
				goto out;

			moved = tomove - write_argp->pvnr_resid;
			if (write_argp->pvnr_resid > tomove) {
				error = EINVAL;
				goto out;
			}

			bp->b_resid = bp->b_bcount - moved;
			if (write_argp->pvnr_resid != 0)
				error = EIO;
		}
	}

 out:
	if (read_argp)
		free(read_argp, M_PUFFS);
	if (write_argp && !dowritefaf)
		free(write_argp, M_PUFFS);

	if (error) {
		bp->b_error = error;
		bp->b_flags |= B_ERROR;
	}

	if ((bp->b_flags & (B_READ | B_ASYNC)) != (B_READ | B_ASYNC))
		biodone(bp);
 wayout:
	return error;
}

int
puffs_mmap(void *v)
{
	struct vop_mmap_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
		int a_fflags;
		kauth_cred_t a_cred;
		struct lwp *a_l;
	} */ *ap = v;
	struct puffs_mount *pmp;
	int error;

	PUFFS_VNREQ(mmap);

	pmp = MPTOPUFFSMP(ap->a_vp->v_mount);

	if (!PUFFS_DOCACHE(pmp))
		return genfs_eopnotsupp(v);

	if (EXISTSOP(pmp, MMAP)) {
		mmap_arg.pvnr_fflags = ap->a_fflags;
		puffs_credcvt(&mmap_arg.pvnr_cred, ap->a_cred);
		mmap_arg.pvnr_pid = puffs_lwp2pid(ap->a_l);

		error = puffs_vntouser(pmp, PUFFS_VN_MMAP,
		    &mmap_arg, sizeof(mmap_arg), 0,
		    VPTOPNC(ap->a_vp), NULL, NULL);
	} else {
		error = genfs_mmap(v);
	}

	return error;
}


/*
 * The rest don't get a free trip to userspace and back, they
 * have to stay within the kernel.
 */

/*
 * bmap doesn't really make any sense for puffs, so just 1:1 map it.
 * well, maybe somehow, somewhere, some day ....
 */
int
puffs_bmap(void *v)
{
	struct vop_bmap_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
		daddr_t a_bn;
		struct vnode **a_vpp;
		daddr_t *a_bnp;
		int *a_runp;
	} */ *ap = v;
	struct puffs_mount *pmp;

	pmp = MPTOPUFFSMP(ap->a_vp->v_mount);

	if (ap->a_vpp)
		*ap->a_vpp = ap->a_vp;
	if (ap->a_bnp)
		*ap->a_bnp = ap->a_bn;
	if (ap->a_runp)
		*ap->a_runp
		    = (PUFFS_TOMOVE(pmp->pmp_req_maxsize, pmp)>>DEV_BSHIFT) - 1;

	return 0;
}

/*
 * Handle getpages faults in puffs.  We let genfs_getpages() do most
 * of the dirty work, but we come in this route to do accounting tasks.
 * If the user server has specified functions for cache notifications
 * about reads and/or writes, we record which type of operation we got,
 * for which page range, and proceed to issue a FAF notification to the
 * server about it.
 */
int
puffs_getpages(void *v)
{
	struct vop_getpages_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
		voff_t a_offset;
		struct vm_page **a_m;
		int *a_count;
		int a_centeridx;
		vm_prot_t a_access_type;
		int a_advice;
		int a_flags;
	} */ *ap = v;
	struct puffs_mount *pmp;
	struct vnode *vp;
	struct vm_page **pgs;
	struct puffs_cacheinfo *pcinfo = NULL;
	struct puffs_cacherun *pcrun;
	void *parkmem = NULL;
	size_t runsizes;
	int i, npages, si, streakon;
	int error, locked, write;

	pmp = MPTOPUFFSMP(ap->a_vp->v_mount);
	npages = *ap->a_count;
	pgs = ap->a_m;
	vp = ap->a_vp;
	locked = (ap->a_flags & PGO_LOCKED) != 0;
	write = (ap->a_access_type & VM_PROT_WRITE) != 0;

	/* ccg xnaht - gets Wuninitialized wrong */
	pcrun = NULL;
	runsizes = 0;

	if (write && PUFFS_WCACHEINFO(pmp)) {
		/* allocate worst-case memory */
		runsizes = ((npages / 2) + 1) * sizeof(struct puffs_cacherun);
		pcinfo = malloc(sizeof(struct puffs_cacheinfo) + runsizes,
		    M_PUFFS, M_ZERO | locked ? M_NOWAIT : M_WAITOK);

		/*
		 * can't block if we're locked and can't mess up caching
		 * information for fs server.  so come back later, please
		 */
		if (pcinfo == NULL) {
			error = ENOMEM;
			goto out;
		}

		parkmem = puffs_park_alloc(locked == 0);
		if (parkmem == NULL) {
			error = ENOMEM;
			goto out;
		}

		pcrun = pcinfo->pcache_runs;
	}

	error = genfs_getpages(v);
	if (error)
		goto out;

	if (PUFFS_WCACHEINFO(pmp) == 0)
		goto out;

	/*
	 * Let's see whose fault it was and inform the user server of
	 * possibly read/written pages.  Map pages from read faults
	 * strictly read-only, since otherwise we might miss info on
	 * when the page is actually write-faulted to.
	 */
	if (!locked)
		simple_lock(&vp->v_uobj.vmobjlock);
	for (i = 0, si = 0, streakon = 0; i < npages; i++) {
		if (pgs[i] == NULL || pgs[i] == PGO_DONTCARE) {
			if (streakon && write) {
				streakon = 0;
				pcrun[si].pcache_runend
				    = trunc_page(pgs[i]->offset) + PAGE_MASK;
				si++;
			}
			continue;
		}
		if (streakon == 0 && write) {
			streakon = 1;
			pcrun[si].pcache_runstart = pgs[i]->offset;
		}
			
		if (!write)
			pgs[i]->flags |= PG_RDONLY;
	}
	/* was the last page part of our streak? */
	if (streakon) {
		pcrun[si].pcache_runend
		    = trunc_page(pgs[i-1]->offset) + PAGE_MASK;
		si++;
	}
	if (!locked)
		simple_unlock(&vp->v_uobj.vmobjlock);

	KASSERT(si <= (npages / 2) + 1);

	/* send results to userspace */
	if (write)
		puffs_cacheop(pmp, parkmem, pcinfo,
		    sizeof(struct puffs_cacheinfo) + runsizes, VPTOPNC(vp));

 out:
	if (error) {
		if (pcinfo != NULL)
			free(pcinfo, M_PUFFS);
		if (parkmem != NULL)
			puffs_park_release(parkmem, 1);
	}

	return error;
}

int
puffs_lock(void *v)
{
	struct vop_lock_args /* {
		struct vnode *a_vp;
		int a_flags;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;
	struct mount *mp = vp->v_mount;

#if 0
	DPRINTF(("puffs_lock: lock %p, args 0x%x\n", vp, ap->a_flags));
#endif

	/*
	 * XXX: this avoids deadlocking when we're suspending.
	 * e.g. some ops holding the vnode lock might be blocked for
	 * the vfs transaction lock so we'd deadlock.
	 *
	 * Now once again this is skating on the thin ice of modern life,
	 * since we are breaking the consistency guarantee provided
	 * _to the user server_ by vnode locking.  Hopefully this will
	 * get fixed soon enough by getting rid of the dependency on
	 * vnode locks alltogether.
	 */
	if (fstrans_is_owner(mp) && fstrans_getstate(mp) == FSTRANS_SUSPENDING){
		if (ap->a_flags & LK_INTERLOCK)
			simple_unlock(&vp->v_interlock);
		return 0;
	}

	return lockmgr(&vp->v_lock, ap->a_flags, &vp->v_interlock);
}

int
puffs_unlock(void *v)
{
	struct vop_unlock_args /* {
		struct vnode *a_vp;
		int a_flags;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;
	struct mount *mp = vp->v_mount;

#if 0
	DPRINTF(("puffs_unlock: lock %p, args 0x%x\n", vp, ap->a_flags));
#endif

	/* XXX: see puffs_lock() */
	if (fstrans_is_owner(mp) && fstrans_getstate(mp) == FSTRANS_SUSPENDING){
		if (ap->a_flags & LK_INTERLOCK)
			simple_unlock(&vp->v_interlock);
		return 0;
	}

	return lockmgr(&vp->v_lock, ap->a_flags | LK_RELEASE, &vp->v_interlock);
}

int
puffs_islocked(void *v)
{
	struct vop_islocked_args *ap = v;
	int rv;

	rv = lockstatus(&ap->a_vp->v_lock);
	return rv;
}

int
puffs_generic(void *v)
{
	struct vop_generic_args *ap = v;

	(void)ap;
	DPRINTF(("puffs_generic: ap->a_desc = %s\n", ap->a_desc->vdesc_name));

	return EOPNOTSUPP;
}


/*
 * spec & fifo.  These call the miscfs spec and fifo vectors, but issue
 * FAF update information for the puffs node first.
 */
int
puffs_spec_read(void *v)
{
	struct vop_read_args /* { 
		const struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
		struct uio *a_uio;
		int a_ioflag;
		kauth_cred_t a_cred;
	} */ *ap = v;

	puffs_updatenode(ap->a_vp, PUFFS_UPDATEATIME);
	return VOCALL(spec_vnodeop_p, VOFFSET(vop_read), v);
}

int
puffs_spec_write(void *v)
{
	struct vop_write_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
		struct uio *a_uio;
		int a_ioflag;
		kauth_cred_t a_cred;
	} */ *ap = v;

	puffs_updatenode(ap->a_vp, PUFFS_UPDATEMTIME);
	return VOCALL(spec_vnodeop_p, VOFFSET(vop_write), v);
}

int
puffs_fifo_read(void *v)
{
	struct vop_read_args /* { 
		const struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
		struct uio *a_uio;
		int a_ioflag;
		kauth_cred_t a_cred;
	} */ *ap = v;

	puffs_updatenode(ap->a_vp, PUFFS_UPDATEATIME);
	return VOCALL(fifo_vnodeop_p, VOFFSET(vop_read), v);
}

int
puffs_fifo_write(void *v)
{
	struct vop_write_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
		struct uio *a_uio;
		int a_ioflag;
		kauth_cred_t a_cred;
	} */ *ap = v;

	puffs_updatenode(ap->a_vp, PUFFS_UPDATEMTIME);
	return VOCALL(fifo_vnodeop_p, VOFFSET(vop_write), v);
}
