/*	$NetBSD: puffs_vnops.c,v 1.123 2007/12/30 23:04:12 pooka Exp $	*/

/*
 * Copyright (c) 2005, 2006, 2007  Antti Kantee.  All Rights Reserved.
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
__KERNEL_RCSID(0, "$NetBSD: puffs_vnops.c,v 1.123 2007/12/30 23:04:12 pooka Exp $");

#include <sys/param.h>
#include <sys/fstrans.h>
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/namei.h>
#include <sys/vnode.h>
#include <sys/proc.h>

#include <uvm/uvm.h>

#include <fs/puffs/puffs_msgif.h>
#include <fs/puffs/puffs_sys.h>

#include <miscfs/fifofs/fifo.h>
#include <miscfs/genfs/genfs.h>
#include <miscfs/specfs/specdev.h>

int	puffs_vnop_lookup(void *);
int	puffs_vnop_create(void *);
int	puffs_vnop_access(void *);
int	puffs_vnop_mknod(void *);
int	puffs_vnop_open(void *);
int	puffs_vnop_close(void *);
int	puffs_vnop_getattr(void *);
int	puffs_vnop_setattr(void *);
int	puffs_vnop_reclaim(void *);
int	puffs_vnop_readdir(void *);
int	puffs_vnop_poll(void *);
int	puffs_vnop_fsync(void *);
int	puffs_vnop_seek(void *);
int	puffs_vnop_remove(void *);
int	puffs_vnop_mkdir(void *);
int	puffs_vnop_rmdir(void *);
int	puffs_vnop_link(void *);
int	puffs_vnop_readlink(void *);
int	puffs_vnop_symlink(void *);
int	puffs_vnop_rename(void *);
int	puffs_vnop_read(void *);
int	puffs_vnop_write(void *);
int	puffs_vnop_fcntl(void *);
int	puffs_vnop_ioctl(void *);
int	puffs_vnop_inactive(void *);
int	puffs_vnop_print(void *);
int	puffs_vnop_pathconf(void *);
int	puffs_vnop_advlock(void *);
int	puffs_vnop_strategy(void *);
int	puffs_vnop_bmap(void *);
int	puffs_vnop_mmap(void *);
int	puffs_vnop_getpages(void *);

int	puffs_vnop_spec_read(void *);
int	puffs_vnop_spec_write(void *);
int	puffs_vnop_fifo_read(void *);
int	puffs_vnop_fifo_write(void *);

int	puffs_vnop_checkop(void *);


#if 0
#define puffs_lock genfs_lock
#define puffs_unlock genfs_unlock
#define puffs_islocked genfs_islocked
#else
int puffs_vnop_lock(void *);
int puffs_vnop_unlock(void *);
int puffs_vnop_islocked(void *);
#endif

int (**puffs_vnodeop_p)(void *);
const struct vnodeopv_entry_desc puffs_vnodeop_entries[] = {
	{ &vop_default_desc, vn_default_error },
	{ &vop_lookup_desc, puffs_vnop_lookup },	/* REAL lookup */
	{ &vop_create_desc, puffs_vnop_checkop },	/* create */
        { &vop_mknod_desc, puffs_vnop_checkop },	/* mknod */
        { &vop_open_desc, puffs_vnop_open },		/* REAL open */
        { &vop_close_desc, puffs_vnop_checkop },	/* close */
        { &vop_access_desc, puffs_vnop_access },	/* REAL access */
        { &vop_getattr_desc, puffs_vnop_checkop },	/* getattr */
        { &vop_setattr_desc, puffs_vnop_checkop },	/* setattr */
        { &vop_read_desc, puffs_vnop_checkop },		/* read */
        { &vop_write_desc, puffs_vnop_checkop },	/* write */
        { &vop_fsync_desc, puffs_vnop_fsync },		/* REAL fsync */
        { &vop_seek_desc, puffs_vnop_checkop },		/* seek */
        { &vop_remove_desc, puffs_vnop_checkop },	/* remove */
        { &vop_link_desc, puffs_vnop_checkop },		/* link */
        { &vop_rename_desc, puffs_vnop_checkop },	/* rename */
        { &vop_mkdir_desc, puffs_vnop_checkop },	/* mkdir */
        { &vop_rmdir_desc, puffs_vnop_checkop },	/* rmdir */
        { &vop_symlink_desc, puffs_vnop_checkop },	/* symlink */
        { &vop_readdir_desc, puffs_vnop_checkop },	/* readdir */
        { &vop_readlink_desc, puffs_vnop_checkop },	/* readlink */
        { &vop_getpages_desc, puffs_vnop_checkop },	/* getpages */
        { &vop_putpages_desc, genfs_putpages },		/* REAL putpages */
        { &vop_pathconf_desc, puffs_vnop_checkop },	/* pathconf */
        { &vop_advlock_desc, puffs_vnop_checkop },	/* advlock */
        { &vop_strategy_desc, puffs_vnop_strategy },	/* REAL strategy */
        { &vop_revoke_desc, genfs_revoke },		/* REAL revoke */
        { &vop_abortop_desc, genfs_abortop },		/* REAL abortop */
        { &vop_inactive_desc, puffs_vnop_inactive },	/* REAL inactive */
        { &vop_reclaim_desc, puffs_vnop_reclaim },	/* REAL reclaim */
        { &vop_lock_desc, puffs_vnop_lock },		/* REAL lock */
        { &vop_unlock_desc, puffs_vnop_unlock },	/* REAL unlock */
        { &vop_bmap_desc, puffs_vnop_bmap },		/* REAL bmap */
        { &vop_print_desc, puffs_vnop_print },		/* REAL print */
        { &vop_islocked_desc, puffs_vnop_islocked },	/* REAL islocked */
        { &vop_bwrite_desc, genfs_nullop },		/* REAL bwrite */
        { &vop_mmap_desc, puffs_vnop_mmap },		/* REAL mmap */
        { &vop_poll_desc, puffs_vnop_poll },		/* REAL poll */

        { &vop_kqfilter_desc, genfs_eopnotsupp },	/* kqfilter XXX */
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
	{ &vop_access_desc, puffs_vnop_checkop },		/* access */
	{ &vop_getattr_desc, puffs_vnop_checkop },		/* getattr */
	{ &vop_setattr_desc, puffs_vnop_checkop },		/* setattr */
	{ &vop_read_desc, puffs_vnop_spec_read },		/* update, read */
	{ &vop_write_desc, puffs_vnop_spec_write },		/* update, write */
	{ &vop_lease_desc, spec_lease_check },		/* genfs_nullop */
	{ &vop_ioctl_desc, spec_ioctl },		/* spec_ioctl */
	{ &vop_fcntl_desc, genfs_fcntl },		/* dummy */
	{ &vop_poll_desc, spec_poll },			/* spec_poll */
	{ &vop_kqfilter_desc, spec_kqfilter },		/* spec_kqfilter */
	{ &vop_revoke_desc, spec_revoke },		/* genfs_revoke */
	{ &vop_mmap_desc, spec_mmap },			/* spec_mmap */
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
	{ &vop_inactive_desc, puffs_vnop_inactive },		/* REAL inactive */
	{ &vop_reclaim_desc, puffs_vnop_reclaim },		/* REAL reclaim */
	{ &vop_lock_desc, puffs_vnop_lock },			/* REAL lock */
	{ &vop_unlock_desc, puffs_vnop_unlock },		/* REAL unlock */
	{ &vop_bmap_desc, spec_bmap },			/* dummy */
	{ &vop_strategy_desc, spec_strategy },		/* dev strategy */
	{ &vop_print_desc, puffs_vnop_print },		/* REAL print */
	{ &vop_islocked_desc, puffs_vnop_islocked },		/* REAL islocked */
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
	{ &vop_access_desc, puffs_vnop_checkop },		/* access */
	{ &vop_getattr_desc, puffs_vnop_checkop },		/* getattr */
	{ &vop_setattr_desc, puffs_vnop_checkop },		/* setattr */
	{ &vop_read_desc, puffs_vnop_fifo_read },		/* read, update */
	{ &vop_write_desc, puffs_vnop_fifo_write },		/* write, update */
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
	{ &vop_inactive_desc, puffs_vnop_inactive },		/* REAL inactive */
	{ &vop_reclaim_desc, puffs_vnop_reclaim },		/* REAL reclaim */
	{ &vop_lock_desc, puffs_vnop_lock },			/* REAL lock */
	{ &vop_unlock_desc, puffs_vnop_unlock },		/* REAL unlock */
	{ &vop_bmap_desc, fifo_bmap },			/* dummy */
	{ &vop_strategy_desc, fifo_strategy },		/* genfs_badop */
	{ &vop_print_desc, puffs_vnop_print },		/* REAL print */
	{ &vop_islocked_desc, puffs_vnop_islocked },		/* REAL islocked */
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
	{ &vop_create_desc, puffs_vnop_create },		/* create */
        { &vop_mknod_desc, puffs_vnop_mknod },		/* mknod */
        { &vop_open_desc, puffs_vnop_open },			/* open */
        { &vop_close_desc, puffs_vnop_close },		/* close */
        { &vop_access_desc, puffs_vnop_access },		/* access */
        { &vop_getattr_desc, puffs_vnop_getattr },		/* getattr */
        { &vop_setattr_desc, puffs_vnop_setattr },		/* setattr */
        { &vop_read_desc, puffs_vnop_read },			/* read */
        { &vop_write_desc, puffs_vnop_write },		/* write */
        { &vop_seek_desc, puffs_vnop_seek },			/* seek */
        { &vop_remove_desc, puffs_vnop_remove },		/* remove */
        { &vop_link_desc, puffs_vnop_link },			/* link */
        { &vop_rename_desc, puffs_vnop_rename },		/* rename */
        { &vop_mkdir_desc, puffs_vnop_mkdir },		/* mkdir */
        { &vop_rmdir_desc, puffs_vnop_rmdir },		/* rmdir */
        { &vop_symlink_desc, puffs_vnop_symlink },		/* symlink */
        { &vop_readdir_desc, puffs_vnop_readdir },		/* readdir */
        { &vop_readlink_desc, puffs_vnop_readlink },		/* readlink */
        { &vop_print_desc, puffs_vnop_print },		/* print */
        { &vop_islocked_desc, puffs_vnop_islocked },		/* islocked */
        { &vop_pathconf_desc, puffs_vnop_pathconf },		/* pathconf */
        { &vop_advlock_desc, puffs_vnop_advlock },		/* advlock */
        { &vop_getpages_desc, puffs_vnop_getpages },		/* getpages */
	{ NULL, NULL }
};
const struct vnodeopv_desc puffs_msgop_opv_desc =
	{ &puffs_msgop_p, puffs_msgop_entries };


#define ERROUT(err)							\
do {									\
	error = err;							\
	goto out;							\
} while (/*CONSTCOND*/0)

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
puffs_vnop_checkop(void *v)
{
	struct vop_generic_args /* {
		struct vnodeop_desc *a_desc;
		spooky mystery contents;
	} */ *ap = v;
	struct vnodeop_desc *desc = ap->a_desc;
	struct puffs_mount *pmp;
	struct vnode *vp;
	int offset, rv;

	offset = ap->a_desc->vdesc_vp_offsets[0];
#ifdef DIAGNOSTIC
	if (offset == VDESC_NO_OFFSET)
		panic("puffs_checkop: no vnode, why did you call me?");
#endif
	vp = *VOPARG_OFFSETTO(struct vnode **, offset, ap);
	pmp = MPTOPUFFSMP(vp->v_mount);

	DPRINTF_VERBOSE(("checkop call %s (%d), vp %p\n",
	    ap->a_desc->vdesc_name, ap->a_desc->vdesc_offset, vp));

	if (!ALLOPS(pmp)) {
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

	rv = VOCALL(puffs_msgop_p, ap->a_desc->vdesc_offset, v);

	DPRINTF_VERBOSE(("checkop return %s (%d), vp %p: %d\n",
	    ap->a_desc->vdesc_name, ap->a_desc->vdesc_offset, vp, rv));

	return rv;
}

static int callremove(struct puffs_mount *, void *, void *,
			    struct componentname *);
static int callrmdir(struct puffs_mount *, void *, void *,
			   struct componentname *);
static void callinactive(struct puffs_mount *, void *, int);
static void callreclaim(struct puffs_mount *, void *);

#define PUFFS_ABORT_LOOKUP	1
#define PUFFS_ABORT_CREATE	2
#define PUFFS_ABORT_MKNOD	3
#define PUFFS_ABORT_MKDIR	4
#define PUFFS_ABORT_SYMLINK	5

/*
 * Press the pani^Wabort button!  Kernel resource allocation failed.
 */
static void
puffs_abortbutton(struct puffs_mount *pmp, int what,
	void *dcookie, void *cookie, struct componentname *cnp)
{

	switch (what) {
	case PUFFS_ABORT_CREATE:
	case PUFFS_ABORT_MKNOD:
	case PUFFS_ABORT_SYMLINK:
		callremove(pmp, dcookie, cookie, cnp);
		break;
	case PUFFS_ABORT_MKDIR:
		callrmdir(pmp, dcookie, cookie, cnp);
		break;
	}

	callinactive(pmp, cookie, 0);
	callreclaim(pmp, cookie);
}

/*
 * Begin vnode operations.
 *
 * A word from the keymaster about locks: generally we don't want
 * to use the vnode locks at all: it creates an ugly dependency between
 * the userlandia file server and the kernel.  But we'll play along with
 * the kernel vnode locks for now.  However, even currently we attempt
 * to release locks as early as possible.  This is possible for some
 * operations which a) don't need a locked vnode after the userspace op
 * and b) return with the vnode unlocked.  Theoretically we could
 * unlock-do op-lock for others and order the graph in userspace, but I
 * don't want to think of the consequences for the time being.
 */

int
puffs_vnop_lookup(void *v)
{
        struct vop_lookup_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_dvp;
		struct vnode **a_vpp;
		struct componentname *a_cnp;
        } */ *ap = v;
	PUFFS_MSG_VARS(vn, lookup);
	struct puffs_mount *pmp;
	struct componentname *cnp;
	struct vnode *vp, *dvp;
	struct puffs_node *dpn;
	int isdot;
	int error;

	pmp = MPTOPUFFSMP(ap->a_dvp->v_mount);
	cnp = ap->a_cnp;
	dvp = ap->a_dvp;
	*ap->a_vpp = NULL;

	/* r/o fs?  we check create later to handle EEXIST */
	if ((cnp->cn_flags & ISLASTCN)
	    && (dvp->v_mount->mnt_flag & MNT_RDONLY)
	    && (cnp->cn_nameiop == DELETE || cnp->cn_nameiop == RENAME))
		return EROFS;

	isdot = cnp->cn_namelen == 1 && *cnp->cn_nameptr == '.';

	DPRINTF(("puffs_lookup: \"%s\", parent vnode %p, op: %x\n",
	    cnp->cn_nameptr, dvp, cnp->cn_nameiop));

	/*
	 * Check if someone fed it into the cache
	 */
	if (PUFFS_USE_NAMECACHE(pmp)) {
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

	PUFFS_MSG_ALLOC(vn, lookup);
	puffs_makecn(&lookup_msg->pvnr_cn, &lookup_msg->pvnr_cn_cred,
	    cnp, PUFFS_USE_FULLPNBUF(pmp));

	if (cnp->cn_flags & ISDOTDOT)
		VOP_UNLOCK(dvp, 0);

	puffs_msg_setinfo(park_lookup, PUFFSOP_VN,
	    PUFFS_VN_LOOKUP, VPTOPNC(dvp));
	PUFFS_MSG_ENQUEUEWAIT2(pmp, park_lookup, dvp->v_data, NULL, error);
	DPRINTF(("puffs_lookup: return of the userspace, part %d\n", error));

	/*
	 * In case of error, there is no new vnode to play with, so be
	 * happy with the NULL value given to vpp in the beginning.
	 * Also, check if this really was an error or the target was not
	 * present.  Either treat it as a non-error for CREATE/RENAME or
	 * enter the component into the negative name cache (if desired).
	 */
	if (error) {
		error = checkerr(pmp, error, __func__);
		if (error == ENOENT) {
			/* don't allow to create files on r/o fs */
			if ((dvp->v_mount->mnt_flag & MNT_RDONLY)
			    && cnp->cn_nameiop == CREATE) {
				error = EROFS;

			/* adjust values if we are creating */
			} else if ((cnp->cn_flags & ISLASTCN)
			    && (cnp->cn_nameiop == CREATE
			      || cnp->cn_nameiop == RENAME)) {
				cnp->cn_flags |= SAVENAME;
				error = EJUSTRETURN;

			/* save negative cache entry */
			} else {
				if ((cnp->cn_flags & MAKEENTRY)
				    && PUFFS_USE_NAMECACHE(pmp))
					cache_enter(dvp, NULL, cnp);
			}
		}
		goto out;
	}

	/*
	 * Check that we don't get our parent node back, that would cause
	 * a pretty obvious deadlock.
	 */
	dpn = dvp->v_data;
	if (lookup_msg->pvnr_newnode == dpn->pn_cookie) {
		puffs_senderr(pmp, PUFFS_ERR_LOOKUP, EINVAL,
		    "lookup produced parent cookie", lookup_msg->pvnr_newnode);
		error = EPROTO;
		goto out;
	}

	error = puffs_cookie2vnode(pmp, lookup_msg->pvnr_newnode, 1, 1, &vp);
	if (error == PUFFS_NOSUCHCOOKIE) {
		error = puffs_getvnode(dvp->v_mount,
		    lookup_msg->pvnr_newnode, lookup_msg->pvnr_vtype,
		    lookup_msg->pvnr_size, lookup_msg->pvnr_rdev, &vp);
		if (error) {
			puffs_abortbutton(pmp, PUFFS_ABORT_LOOKUP, VPTOPNC(dvp),
			    lookup_msg->pvnr_newnode, ap->a_cnp);
			goto out;
		}
		vn_lock(vp, LK_EXCLUSIVE | LK_RETRY);
	} else if (error) {
		puffs_abortbutton(pmp, PUFFS_ABORT_LOOKUP, VPTOPNC(dvp),
		    lookup_msg->pvnr_newnode, ap->a_cnp);
		goto out;
	}

	*ap->a_vpp = vp;

	if ((cnp->cn_flags & MAKEENTRY) != 0 && PUFFS_USE_NAMECACHE(pmp))
		cache_enter(dvp, vp, cnp);

	/* XXX */
	if ((lookup_msg->pvnr_cn.pkcn_flags & REQUIREDIR) == 0)
		cnp->cn_flags &= ~REQUIREDIR;
	if (lookup_msg->pvnr_cn.pkcn_consume)
		cnp->cn_consume = MIN(lookup_msg->pvnr_cn.pkcn_consume,
		    strlen(cnp->cn_nameptr) - cnp->cn_namelen);

 out:
	if (cnp->cn_flags & ISDOTDOT)
		vn_lock(dvp, LK_EXCLUSIVE | LK_RETRY);

	DPRINTF(("puffs_lookup: returning %d %p\n", error, *ap->a_vpp));
	PUFFS_MSG_RELEASE(lookup);
	return error;
}

#define REFPN_AND_UNLOCKVP(a, b)					\
do {									\
	mutex_enter(&b->pn_mtx);					\
	puffs_referencenode(b);						\
	mutex_exit(&b->pn_mtx);						\
	VOP_UNLOCK(a, 0);						\
} while (/*CONSTCOND*/0)

#define REFPN(b)							\
do {									\
	mutex_enter(&b->pn_mtx);					\
	puffs_referencenode(b);						\
	mutex_exit(&b->pn_mtx);						\
} while (/*CONSTCOND*/0)

#define RELEPN_AND_VP(a, b)						\
do {									\
	puffs_releasenode(b);						\
	vrele(a);							\
} while (/*CONSTCOND*/0)

int
puffs_vnop_create(void *v)
{
	struct vop_create_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_dvp;
		struct vnode **a_vpp;
		struct componentname *a_cnp;
		struct vattr *a_vap;
	} */ *ap = v;
	PUFFS_MSG_VARS(vn, create);
	struct vnode *dvp = ap->a_dvp;
	struct puffs_node *dpn = VPTOPP(dvp);
	struct componentname *cnp = ap->a_cnp;
	struct mount *mp = dvp->v_mount;
	struct puffs_mount *pmp = MPTOPUFFSMP(mp);
	int error;

	DPRINTF(("puffs_create: dvp %p, cnp: %s\n",
	    dvp, ap->a_cnp->cn_nameptr));

	PUFFS_MSG_ALLOC(vn, create);
	puffs_makecn(&create_msg->pvnr_cn, &create_msg->pvnr_cn_cred,
	    cnp, PUFFS_USE_FULLPNBUF(pmp));
	create_msg->pvnr_va = *ap->a_vap;
	puffs_msg_setinfo(park_create, PUFFSOP_VN,
	    PUFFS_VN_CREATE, VPTOPNC(dvp));

	/*
	 * Do the dance:
	 * + insert into queue ("interlock")
	 * + unlock vnode
	 * + wait for response
	 */
	puffs_msg_enqueue(pmp, park_create);
	REFPN_AND_UNLOCKVP(dvp, dpn);
	error = puffs_msg_wait2(pmp, park_create, dpn, NULL);

	error = checkerr(pmp, error, __func__);
	if (error)
		goto out;

	error = puffs_newnode(mp, dvp, ap->a_vpp,
	    create_msg->pvnr_newnode, cnp, ap->a_vap->va_type, 0);
	if (error)
		puffs_abortbutton(pmp, PUFFS_ABORT_CREATE, dpn->pn_cookie,
		    create_msg->pvnr_newnode, cnp);

 out:
	if (error || (cnp->cn_flags & SAVESTART) == 0)
		PNBUF_PUT(cnp->cn_pnbuf);

	RELEPN_AND_VP(dvp, dpn);
	DPRINTF(("puffs_create: return %d\n", error));
	PUFFS_MSG_RELEASE(create);
	return error;
}

int
puffs_vnop_mknod(void *v)
{
	struct vop_mknod_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_dvp;
		struct vnode **a_vpp;
		struct componentname *a_cnp;
		struct vattr *a_vap;
	} */ *ap = v;
	PUFFS_MSG_VARS(vn, mknod);
	struct vnode *dvp = ap->a_dvp;
	struct puffs_node *dpn = VPTOPP(dvp);
	struct componentname *cnp = ap->a_cnp;
	struct mount *mp = dvp->v_mount;
	struct puffs_mount *pmp = MPTOPUFFSMP(mp);
	int error;

	PUFFS_MSG_ALLOC(vn, mknod);
	puffs_makecn(&mknod_msg->pvnr_cn, &mknod_msg->pvnr_cn_cred,
	    cnp, PUFFS_USE_FULLPNBUF(pmp));
	mknod_msg->pvnr_va = *ap->a_vap;
	puffs_msg_setinfo(park_mknod, PUFFSOP_VN,
	    PUFFS_VN_MKNOD, VPTOPNC(dvp));

	puffs_msg_enqueue(pmp, park_mknod);
	REFPN_AND_UNLOCKVP(dvp, dpn);
	error = puffs_msg_wait2(pmp, park_mknod, dpn, NULL);

	error = checkerr(pmp, error, __func__);
	if (error)
		goto out;

	error = puffs_newnode(mp, dvp, ap->a_vpp,
	    mknod_msg->pvnr_newnode, cnp, ap->a_vap->va_type,
	    ap->a_vap->va_rdev);
	if (error)
		puffs_abortbutton(pmp, PUFFS_ABORT_MKNOD, dpn->pn_cookie,
		    mknod_msg->pvnr_newnode, cnp);

 out:
	PUFFS_MSG_RELEASE(mknod);
	if (error || (cnp->cn_flags & SAVESTART) == 0)
		PNBUF_PUT(cnp->cn_pnbuf);
	RELEPN_AND_VP(dvp, dpn);
	return error;
}

int
puffs_vnop_open(void *v)
{
	struct vop_open_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
		int a_mode;
		kauth_cred_t a_cred;
	} */ *ap = v;
	PUFFS_MSG_VARS(vn, open);
	struct vnode *vp = ap->a_vp;
	struct puffs_mount *pmp = MPTOPUFFSMP(vp->v_mount);
	int mode = ap->a_mode;
	int error;

	DPRINTF(("puffs_open: vp %p, mode 0x%x\n", vp, mode));

	if (vp->v_type == VREG && mode & FWRITE && !EXISTSOP(pmp, WRITE))
		ERROUT(EROFS);

	if (!EXISTSOP(pmp, OPEN))
		ERROUT(0);

	PUFFS_MSG_ALLOC(vn, open);
	open_msg->pvnr_mode = mode;
	puffs_credcvt(&open_msg->pvnr_cred, ap->a_cred);
	puffs_msg_setinfo(park_open, PUFFSOP_VN,
	    PUFFS_VN_OPEN, VPTOPNC(vp));

	PUFFS_MSG_ENQUEUEWAIT2(pmp, park_open, vp->v_data, NULL, error);
	error = checkerr(pmp, error, __func__);

 out:
	DPRINTF(("puffs_open: returning %d\n", error));
	PUFFS_MSG_RELEASE(open);
	return error;
}

int
puffs_vnop_close(void *v)
{
	struct vop_close_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
		int a_fflag;
		kauth_cred_t a_cred;
	} */ *ap = v;
	PUFFS_MSG_VARS(vn, close);
	struct vnode *vp = ap->a_vp;
	struct puffs_mount *pmp = MPTOPUFFSMP(vp->v_mount);

	PUFFS_MSG_ALLOC(vn, close);
	puffs_msg_setfaf(park_close);
	close_msg->pvnr_fflag = ap->a_fflag;
	puffs_credcvt(&close_msg->pvnr_cred, ap->a_cred);
	puffs_msg_setinfo(park_close, PUFFSOP_VN,
	    PUFFS_VN_CLOSE, VPTOPNC(vp));

	puffs_msg_enqueue(pmp, park_close);
	PUFFS_MSG_RELEASE(close);
	return 0;
}

int
puffs_vnop_access(void *v)
{
	struct vop_access_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
		int a_mode;
		kauth_cred_t a_cred;
	} */ *ap = v;
	PUFFS_MSG_VARS(vn, access);
	struct vnode *vp = ap->a_vp;
	struct puffs_mount *pmp = MPTOPUFFSMP(vp->v_mount);
	int mode = ap->a_mode;
	int error;

	if (mode & VWRITE) {
		switch (vp->v_type) {
		case VDIR:
		case VLNK:
		case VREG:
			if ((vp->v_mount->mnt_flag & MNT_RDONLY)
			    || !EXISTSOP(pmp, WRITE))
				return EROFS;
			break;
		default:
			break;
		}
	}

	if (!EXISTSOP(pmp, ACCESS))
		return 0;

	PUFFS_MSG_ALLOC(vn, access);
	access_msg->pvnr_mode = ap->a_mode;
	puffs_credcvt(&access_msg->pvnr_cred, ap->a_cred);
	puffs_msg_setinfo(park_access, PUFFSOP_VN,
	    PUFFS_VN_ACCESS, VPTOPNC(vp));

	PUFFS_MSG_ENQUEUEWAIT2(pmp, park_access, vp->v_data, NULL, error);
	error = checkerr(pmp, error, __func__);
	PUFFS_MSG_RELEASE(access);

	return error;
}

int
puffs_vnop_getattr(void *v)
{
	struct vop_getattr_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
		struct vattr *a_vap;
		kauth_cred_t a_cred;
	} */ *ap = v;
	PUFFS_MSG_VARS(vn, getattr);
	struct vnode *vp = ap->a_vp;
	struct mount *mp = vp->v_mount;
	struct puffs_mount *pmp = MPTOPUFFSMP(mp);
	struct vattr *vap, *rvap;
	struct puffs_node *pn;
	int error = 0;

	vap = ap->a_vap;

	PUFFS_MSG_ALLOC(vn, getattr);
	vattr_null(&getattr_msg->pvnr_va);
	puffs_credcvt(&getattr_msg->pvnr_cred, ap->a_cred);
	puffs_msg_setinfo(park_getattr, PUFFSOP_VN,
	    PUFFS_VN_GETATTR, VPTOPNC(vp));

	PUFFS_MSG_ENQUEUEWAIT2(pmp, park_getattr, vp->v_data, NULL, error);
	error = checkerr(pmp, error, __func__);
	if (error)
		goto out;

	rvap = &getattr_msg->pvnr_va;
	/*
	 * Don't listen to the file server regarding special device
	 * size info, the file server doesn't know anything about them.
	 */
	if (vp->v_type == VBLK || vp->v_type == VCHR)
		rvap->va_size = vp->v_size;

	/* Ditto for blocksize (ufs comment: this doesn't belong here) */
	if (vp->v_type == VBLK)
		rvap->va_blocksize = BLKDEV_IOSIZE;
	else if (vp->v_type == VCHR)
		rvap->va_blocksize = MAXBSIZE;

	(void) memcpy(vap, rvap, sizeof(struct vattr));
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
		if (rvap->va_size != VNOVAL
		    && vp->v_type != VBLK && vp->v_type != VCHR) {
			uvm_vnp_setsize(vp, rvap->va_size);
			pn->pn_serversize = rvap->va_size;
		}
	}

 out:
	PUFFS_MSG_RELEASE(getattr);
	return error;
}

static int
dosetattr(struct vnode *vp, struct vattr *vap, kauth_cred_t cred, int chsize)
{
	PUFFS_MSG_VARS(vn, setattr);
	struct puffs_mount *pmp = MPTOPUFFSMP(vp->v_mount);
	struct puffs_node *pn = vp->v_data;
	int error;

	if ((vp->v_mount->mnt_flag & MNT_RDONLY) &&
	    (vap->va_uid != (uid_t)VNOVAL || vap->va_gid != (gid_t)VNOVAL
	    || vap->va_atime.tv_sec != VNOVAL || vap->va_mtime.tv_sec != VNOVAL
	    || vap->va_mode != (mode_t)VNOVAL))
		return EROFS;

	if ((vp->v_mount->mnt_flag & MNT_RDONLY)
	    && vp->v_type == VREG && vap->va_size != VNOVAL)
		return EROFS;

	/*
	 * Flush metacache first.  If we are called with some explicit
	 * parameters, treat them as information overriding metacache
	 * information.
	 */
	if (pn->pn_stat & PNODE_METACACHE_MASK) {
		if ((pn->pn_stat & PNODE_METACACHE_ATIME)
		    && vap->va_atime.tv_sec == VNOVAL)
			vap->va_atime = pn->pn_mc_atime;
		if ((pn->pn_stat & PNODE_METACACHE_CTIME)
		    && vap->va_ctime.tv_sec == VNOVAL)
			vap->va_ctime = pn->pn_mc_ctime;
		if ((pn->pn_stat & PNODE_METACACHE_MTIME)
		    && vap->va_mtime.tv_sec == VNOVAL)
			vap->va_mtime = pn->pn_mc_mtime;
		if ((pn->pn_stat & PNODE_METACACHE_SIZE)
		    && vap->va_size == VNOVAL)
			vap->va_size = pn->pn_mc_size;

		pn->pn_stat &= ~PNODE_METACACHE_MASK;
	}

	PUFFS_MSG_ALLOC(vn, setattr);
	(void)memcpy(&setattr_msg->pvnr_va, vap, sizeof(struct vattr));
	puffs_credcvt(&setattr_msg->pvnr_cred, cred);
	puffs_msg_setinfo(park_setattr, PUFFSOP_VN,
	    PUFFS_VN_SETATTR, VPTOPNC(vp));

	PUFFS_MSG_ENQUEUEWAIT2(pmp, park_setattr, vp->v_data, NULL, error);
	PUFFS_MSG_RELEASE(setattr);
	error = checkerr(pmp, error, __func__);
	if (error)
		return error;

	if (vap->va_size != VNOVAL) {
		pn->pn_serversize = vap->va_size;
		if (chsize)
			uvm_vnp_setsize(vp, vap->va_size);
	}

	return 0;
}

int
puffs_vnop_setattr(void *v)
{
	struct vop_getattr_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
		struct vattr *a_vap;
		kauth_cred_t a_cred;
	} */ *ap = v;

	return dosetattr(ap->a_vp, ap->a_vap, ap->a_cred, 1);
}

static __inline int
doinact(struct puffs_mount *pmp, int iaflag)
{

	if (EXISTSOP(pmp, INACTIVE))
		if (pmp->pmp_flags & PUFFS_KFLAG_IAONDEMAND)
			if (iaflag || ALLOPS(pmp))
				return 1;
			else
				return 0;
		else
			return 1;
	else
		return 0;
}

static void
callinactive(struct puffs_mount *pmp, void *cookie, int iaflag)
{
	int error;
	PUFFS_MSG_VARS(vn, inactive);

	if (doinact(pmp, iaflag)) {
		PUFFS_MSG_ALLOC(vn, inactive);
		puffs_msg_setinfo(park_inactive, PUFFSOP_VN,
		    PUFFS_VN_INACTIVE, cookie);

		PUFFS_MSG_ENQUEUEWAIT(pmp, park_inactive, error);
		PUFFS_MSG_RELEASE(inactive);
	}
}

/* XXX: callinactive can't setback */
int
puffs_vnop_inactive(void *v)
{
	struct vop_inactive_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
	} */ *ap = v;
	PUFFS_MSG_VARS(vn, inactive);
	struct vnode *vp = ap->a_vp;
	struct puffs_mount *pmp = MPTOPUFFSMP(vp->v_mount);
	struct puffs_node *pnode;
	int error;

	pnode = vp->v_data;

	if (doinact(pmp, pnode->pn_stat & PNODE_DOINACT)) {
		PUFFS_MSG_ALLOC(vn, inactive);
		puffs_msg_setinfo(park_inactive, PUFFSOP_VN,
		    PUFFS_VN_INACTIVE, VPTOPNC(vp));

		PUFFS_MSG_ENQUEUEWAIT2(pmp, park_inactive, vp->v_data,
		    NULL, error);
		PUFFS_MSG_RELEASE(inactive);
	}
	pnode->pn_stat &= ~PNODE_DOINACT;

	VOP_UNLOCK(vp, 0);

	/*
	 * file server thinks it's gone?  then don't be afraid care,
	 * node's life was already all it would ever be
	 */
	if (pnode->pn_stat & PNODE_NOREFS) {
		pnode->pn_stat |= PNODE_DYING;
		vrecycle(vp, NULL, curlwp);
	}

	return 0;
}

static void
callreclaim(struct puffs_mount *pmp, void *cookie)
{
	PUFFS_MSG_VARS(vn, reclaim);

	if (!EXISTSOP(pmp, RECLAIM))
		return;

	PUFFS_MSG_ALLOC(vn, reclaim);
	puffs_msg_setfaf(park_reclaim);
	puffs_msg_setinfo(park_reclaim, PUFFSOP_VN, PUFFS_VN_RECLAIM, cookie);

	puffs_msg_enqueue(pmp, park_reclaim);
	PUFFS_MSG_RELEASE(reclaim);
}

/*
 * always FAF, we don't really care if the server wants to fail to
 * reclaim the node or not
 */
int
puffs_vnop_reclaim(void *v)
{
	struct vop_reclaim_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
	} */ *ap = v;
	struct vnode *vp = ap->a_vp;
	struct puffs_mount *pmp = MPTOPUFFSMP(vp->v_mount);

	/*
	 * first things first: check if someone is trying to reclaim the
	 * root vnode.  do not allow that to travel to userspace.
	 * Note that we don't need to take the lock similarly to
	 * puffs_root(), since there is only one of us.
	 */
	if (vp->v_vflag & VV_ROOT) {
		mutex_enter(&pmp->pmp_lock);
		KASSERT(pmp->pmp_root != NULL);
		pmp->pmp_root = NULL;
		mutex_exit(&pmp->pmp_lock);
		goto out;
	}

	callreclaim(MPTOPUFFSMP(vp->v_mount), VPTOPNC(vp));

 out:
	if (PUFFS_USE_NAMECACHE(pmp))
		cache_purge(vp);
	puffs_putvnode(vp);

	return 0;
}

#define CSIZE sizeof(**ap->a_cookies)
int
puffs_vnop_readdir(void *v)
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
	PUFFS_MSG_VARS(vn, readdir);
	struct vnode *vp = ap->a_vp;
	struct puffs_mount *pmp = MPTOPUFFSMP(vp->v_mount);
	size_t argsize, tomove, cookiemem, cookiesmax;
	struct uio *uio = ap->a_uio;
	size_t howmuch, resid;
	int error;

	/*
	 * ok, so we need: resid + cookiemem = maxreq
	 * => resid + cookiesize * (resid/minsize) = maxreq
	 * => resid + cookiesize/minsize * resid = maxreq
	 * => (cookiesize/minsize + 1) * resid = maxreq
	 * => resid = maxreq / (cookiesize/minsize + 1)
	 * 
	 * Since cookiesize <= minsize and we're not very big on floats,
	 * we approximate that to be 1.  Therefore:
	 * 
	 * resid = maxreq / 2;
	 *
	 * Well, at least we didn't have to use differential equations
	 * or the Gram-Schmidt process.
	 *
	 * (yes, I'm very afraid of this)
	 */
	KASSERT(CSIZE <= _DIRENT_MINSIZE((struct dirent *)0));

	if (ap->a_cookies) {
		KASSERT(ap->a_ncookies != NULL);
		if (pmp->pmp_args.pa_fhsize == 0)
			return EOPNOTSUPP;
		resid = PUFFS_TOMOVE(uio->uio_resid, pmp) / 2;
		cookiesmax = resid/_DIRENT_MINSIZE((struct dirent *)0);
		cookiemem = ALIGN(cookiesmax*CSIZE); /* play safe */
	} else {
		resid = PUFFS_TOMOVE(uio->uio_resid, pmp);
		cookiesmax = 0;
		cookiemem = 0;
	}

	argsize = sizeof(struct puffs_vnmsg_readdir);
	tomove = resid + cookiemem;
	puffs_msgmem_alloc(argsize + tomove, &park_readdir,
	    (void **)&readdir_msg, 1);

	puffs_credcvt(&readdir_msg->pvnr_cred, ap->a_cred);
	readdir_msg->pvnr_offset = uio->uio_offset;
	readdir_msg->pvnr_resid = resid;
	readdir_msg->pvnr_ncookies = cookiesmax;
	readdir_msg->pvnr_eofflag = 0;
	readdir_msg->pvnr_dentoff = cookiemem;
	puffs_msg_setinfo(park_readdir, PUFFSOP_VN,
	    PUFFS_VN_READDIR, VPTOPNC(vp));
	puffs_msg_setdelta(park_readdir, tomove);

	PUFFS_MSG_ENQUEUEWAIT2(pmp, park_readdir, vp->v_data, NULL, error);
	error = checkerr(pmp, error, __func__);
	if (error)
		goto out;

	/* userspace is cheating? */
	if (readdir_msg->pvnr_resid > resid) {
		puffs_senderr(pmp, PUFFS_ERR_READDIR, E2BIG,
		    "resid grew", VPTOPNC(vp));
		ERROUT(EPROTO);
	}
	if (readdir_msg->pvnr_ncookies > cookiesmax) {
		puffs_senderr(pmp, PUFFS_ERR_READDIR, E2BIG,
		    "too many cookies", VPTOPNC(vp));
		ERROUT(EPROTO);
	}

	/* check eof */
	if (readdir_msg->pvnr_eofflag)
		*ap->a_eofflag = 1;

	/* bouncy-wouncy with the directory data */
	howmuch = resid - readdir_msg->pvnr_resid;

	/* force eof if no data was returned (getcwd() needs this) */
	if (howmuch == 0) {
		*ap->a_eofflag = 1;
		goto out;
	}

	error = uiomove(readdir_msg->pvnr_data + cookiemem, howmuch, uio);
	if (error)
		goto out;

	/* provide cookies to caller if so desired */
	if (ap->a_cookies) {
		*ap->a_cookies = malloc(readdir_msg->pvnr_ncookies*CSIZE,
		    M_TEMP, M_WAITOK);
		*ap->a_ncookies = readdir_msg->pvnr_ncookies;
		memcpy(*ap->a_cookies, readdir_msg->pvnr_data,
		    *ap->a_ncookies*CSIZE);
	}

	/* next readdir starts here */
	uio->uio_offset = readdir_msg->pvnr_offset;

 out:
	puffs_msgmem_release(park_readdir);
	return error;
}
#undef CSIZE

/*
 * poll works by consuming the bitmask in pn_revents.  If there are
 * events available, poll returns immediately.  If not, it issues a
 * poll to userspace, selrecords itself and returns with no available
 * events.  When the file server returns, it executes puffs_parkdone_poll(),
 * where available events are added to the bitmask.  selnotify() is
 * then also executed by that function causing us to enter here again
 * and hopefully find the missing bits (unless someone got them first,
 * in which case it starts all over again).
 */
int
puffs_vnop_poll(void *v)
{
	struct vop_poll_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
		int a_events;
	} */ *ap = v;
	PUFFS_MSG_VARS(vn, poll);
	struct vnode *vp = ap->a_vp;
	struct puffs_mount *pmp = MPTOPUFFSMP(vp->v_mount);
	struct puffs_node *pn = vp->v_data;
	int events, error;

	if (EXISTSOP(pmp, POLL)) {
		mutex_enter(&pn->pn_mtx);
		events = pn->pn_revents & ap->a_events;
		if (events & ap->a_events) {
			pn->pn_revents &= ~ap->a_events;
			mutex_exit(&pn->pn_mtx);

			return events;
		} else {
			puffs_referencenode(pn);
			mutex_exit(&pn->pn_mtx);

			PUFFS_MSG_ALLOC(vn, poll);
			poll_msg->pvnr_events = ap->a_events;
			puffs_msg_setinfo(park_poll, PUFFSOP_VN,
			    PUFFS_VN_POLL, VPTOPNC(vp));
			puffs_msg_setcall(park_poll, puffs_parkdone_poll, pn);
			selrecord(curlwp, &pn->pn_sel);

			PUFFS_MSG_ENQUEUEWAIT2(pmp, park_poll, vp->v_data,
			    NULL, error);
			PUFFS_MSG_RELEASE(poll);

			return 0;
		}
	} else {
		return genfs_poll(v);
	}
}

int
puffs_vnop_fsync(void *v)
{
	struct vop_fsync_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
		kauth_cred_t a_cred;
		int a_flags;
		off_t a_offlo;
		off_t a_offhi;
	} */ *ap = v;
	PUFFS_MSG_VARS(vn, fsync);
	struct vnode *vp = ap->a_vp;
	struct puffs_mount *pmp = MPTOPUFFSMP(vp->v_mount);
	struct puffs_node *pn;
	struct vattr va;
	int pflags, error, dofaf;

	pn = VPTOPP(vp);

	/* flush out information from our metacache, see vop_setattr */
	if (pn->pn_stat & PNODE_METACACHE_MASK
	    && (pn->pn_stat & PNODE_DYING) == 0) {
		vattr_null(&va);
		error = VOP_SETATTR(vp, &va, FSCRED); 
		if (error)
			return error;
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
	if (!EXISTSOP(pmp, FSYNC) || (pn->pn_stat & PNODE_DYING))
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
		if (vp->v_iflag & VI_XLOCK)
			dofaf = 1;
		simple_unlock(&vp->v_interlock);
	}

	PUFFS_MSG_ALLOC(vn, fsync);
	if (dofaf)
		puffs_msg_setfaf(park_fsync);

	puffs_credcvt(&fsync_msg->pvnr_cred, ap->a_cred);
	fsync_msg->pvnr_flags = ap->a_flags;
	fsync_msg->pvnr_offlo = ap->a_offlo;
	fsync_msg->pvnr_offhi = ap->a_offhi;
	puffs_msg_setinfo(park_fsync, PUFFSOP_VN,
	    PUFFS_VN_FSYNC, VPTOPNC(vp));

	PUFFS_MSG_ENQUEUEWAIT2(pmp, park_fsync, vp->v_data, NULL, error);
	PUFFS_MSG_RELEASE(fsync);

	error = checkerr(pmp, error, __func__);

	return error;
}

int
puffs_vnop_seek(void *v)
{
	struct vop_seek_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
		off_t a_oldoff;
		off_t a_newoff;
		kauth_cred_t a_cred;
	} */ *ap = v;
	PUFFS_MSG_VARS(vn, seek);
	struct vnode *vp = ap->a_vp;
	struct puffs_mount *pmp = MPTOPUFFSMP(vp->v_mount);
	int error;

	PUFFS_MSG_ALLOC(vn, seek);
	seek_msg->pvnr_oldoff = ap->a_oldoff;
	seek_msg->pvnr_newoff = ap->a_newoff;
	puffs_credcvt(&seek_msg->pvnr_cred, ap->a_cred);
	puffs_msg_setinfo(park_seek, PUFFSOP_VN,
	    PUFFS_VN_SEEK, VPTOPNC(vp));

	PUFFS_MSG_ENQUEUEWAIT2(pmp, park_seek, vp->v_data, NULL, error);
	PUFFS_MSG_RELEASE(seek);
	return checkerr(pmp, error, __func__);
}

static int
callremove(struct puffs_mount *pmp, void *dcookie, void *cookie,
	struct componentname *cnp)
{
	PUFFS_MSG_VARS(vn, remove);
	int error;

	PUFFS_MSG_ALLOC(vn, remove);
	remove_msg->pvnr_cookie_targ = cookie;
	puffs_makecn(&remove_msg->pvnr_cn, &remove_msg->pvnr_cn_cred,
	    cnp, PUFFS_USE_FULLPNBUF(pmp));
	puffs_msg_setinfo(park_remove, PUFFSOP_VN, PUFFS_VN_REMOVE, dcookie);

	PUFFS_MSG_ENQUEUEWAIT(pmp, park_remove, error);
	PUFFS_MSG_RELEASE(remove);

	return checkerr(pmp, error, __func__);
}

/*
 * XXX: can't use callremove now because can't catch setbacks with
 * it due to lack of a pnode argument.
 */
int
puffs_vnop_remove(void *v)
{
	struct vop_remove_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_dvp;
		struct vnode *a_vp;
		struct componentname *a_cnp;
	} */ *ap = v;
	PUFFS_MSG_VARS(vn, remove);
	struct vnode *dvp = ap->a_dvp;
	struct vnode *vp = ap->a_vp;
	struct puffs_node *dpn = VPTOPP(dvp);
	struct puffs_node *pn = VPTOPP(vp);
	struct componentname *cnp = ap->a_cnp;
	struct mount *mp = dvp->v_mount;
	struct puffs_mount *pmp = MPTOPUFFSMP(mp);
	int error;

	PUFFS_MSG_ALLOC(vn, remove);
	remove_msg->pvnr_cookie_targ = VPTOPNC(vp);
	puffs_makecn(&remove_msg->pvnr_cn, &remove_msg->pvnr_cn_cred,
	    cnp, PUFFS_USE_FULLPNBUF(pmp));
	puffs_msg_setinfo(park_remove, PUFFSOP_VN,
	    PUFFS_VN_REMOVE, VPTOPNC(dvp));

	puffs_msg_enqueue(pmp, park_remove);
	REFPN_AND_UNLOCKVP(dvp, dpn);
	if (dvp == vp)
		REFPN(pn);
	else
		REFPN_AND_UNLOCKVP(vp, pn);
	error = puffs_msg_wait2(pmp, park_remove, dpn, pn);

	PUFFS_MSG_RELEASE(remove);

	RELEPN_AND_VP(dvp, dpn);
	RELEPN_AND_VP(vp, pn);

	error = checkerr(pmp, error, __func__);
	return error;
}

int
puffs_vnop_mkdir(void *v)
{
	struct vop_mkdir_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_dvp;
		struct vnode **a_vpp;
		struct componentname *a_cnp;
		struct vattr *a_vap;
	} */ *ap = v;
	PUFFS_MSG_VARS(vn, mkdir);
	struct vnode *dvp = ap->a_dvp;
	struct puffs_node *dpn = VPTOPP(dvp);
	struct componentname *cnp = ap->a_cnp;
	struct mount *mp = dvp->v_mount;
	struct puffs_mount *pmp = MPTOPUFFSMP(mp);
	int error;

	PUFFS_MSG_ALLOC(vn, mkdir);
	puffs_makecn(&mkdir_msg->pvnr_cn, &mkdir_msg->pvnr_cn_cred,
	    cnp, PUFFS_USE_FULLPNBUF(pmp));
	mkdir_msg->pvnr_va = *ap->a_vap;
	puffs_msg_setinfo(park_mkdir, PUFFSOP_VN,
	    PUFFS_VN_MKDIR, VPTOPNC(dvp));

	puffs_msg_enqueue(pmp, park_mkdir);
	REFPN_AND_UNLOCKVP(dvp, dpn);
	error = puffs_msg_wait2(pmp, park_mkdir, dpn, NULL);

	error = checkerr(pmp, error, __func__);
	if (error)
		goto out;

	error = puffs_newnode(mp, dvp, ap->a_vpp,
	    mkdir_msg->pvnr_newnode, cnp, VDIR, 0);
	if (error)
		puffs_abortbutton(pmp, PUFFS_ABORT_MKDIR, dpn->pn_cookie,
		    mkdir_msg->pvnr_newnode, cnp);

 out:
	PUFFS_MSG_RELEASE(mkdir);
	if (error || (cnp->cn_flags & SAVESTART) == 0)
		PNBUF_PUT(cnp->cn_pnbuf);
	RELEPN_AND_VP(dvp, dpn);
	return error;
}

static int
callrmdir(struct puffs_mount *pmp, void *dcookie, void *cookie,
	struct componentname *cnp)
{
	PUFFS_MSG_VARS(vn, rmdir);
	int error;

	PUFFS_MSG_ALLOC(vn, rmdir);
	rmdir_msg->pvnr_cookie_targ = cookie;
	puffs_makecn(&rmdir_msg->pvnr_cn, &rmdir_msg->pvnr_cn_cred,
	    cnp, PUFFS_USE_FULLPNBUF(pmp));
	puffs_msg_setinfo(park_rmdir, PUFFSOP_VN, PUFFS_VN_RMDIR, dcookie);

	PUFFS_MSG_ENQUEUEWAIT(pmp, park_rmdir, error);
	PUFFS_MSG_RELEASE(rmdir);

	return checkerr(pmp, error, __func__);
}

int
puffs_vnop_rmdir(void *v)
{
	struct vop_rmdir_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_dvp;
		struct vnode *a_vp;
		struct componentname *a_cnp;
	} */ *ap = v;
	PUFFS_MSG_VARS(vn, rmdir);
	struct vnode *dvp = ap->a_dvp;
	struct vnode *vp = ap->a_vp;
	struct puffs_node *dpn = VPTOPP(dvp);
	struct puffs_node *pn = VPTOPP(vp);
	struct puffs_mount *pmp = MPTOPUFFSMP(dvp->v_mount);
	struct componentname *cnp = ap->a_cnp;
	int error;

	PUFFS_MSG_ALLOC(vn, rmdir);
	rmdir_msg->pvnr_cookie_targ = VPTOPNC(vp);
	puffs_makecn(&rmdir_msg->pvnr_cn, &rmdir_msg->pvnr_cn_cred,
	    cnp, PUFFS_USE_FULLPNBUF(pmp));
	puffs_msg_setinfo(park_rmdir, PUFFSOP_VN,
	    PUFFS_VN_RMDIR, VPTOPNC(dvp));

	puffs_msg_enqueue(pmp, park_rmdir);
	REFPN_AND_UNLOCKVP(dvp, dpn);
	REFPN_AND_UNLOCKVP(vp, pn);
	error = puffs_msg_wait2(pmp, park_rmdir, dpn, pn);

	PUFFS_MSG_RELEASE(rmdir);

	/* XXX: some call cache_purge() *for both vnodes* here, investigate */
	RELEPN_AND_VP(dvp, dpn);
	RELEPN_AND_VP(vp, pn);

	return error;
}

int
puffs_vnop_link(void *v)
{
	struct vop_link_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_dvp;
		struct vnode *a_vp;
		struct componentname *a_cnp;
	} */ *ap = v;
	PUFFS_MSG_VARS(vn, link);
	struct vnode *dvp = ap->a_dvp;
	struct vnode *vp = ap->a_vp;
	struct puffs_node *dpn = VPTOPP(dvp);
	struct puffs_node *pn = VPTOPP(vp);
	struct puffs_mount *pmp = MPTOPUFFSMP(dvp->v_mount);
	struct componentname *cnp = ap->a_cnp;
	int error;

	PUFFS_MSG_ALLOC(vn, link);
	link_msg->pvnr_cookie_targ = VPTOPNC(vp);
	puffs_makecn(&link_msg->pvnr_cn, &link_msg->pvnr_cn_cred,
	    cnp, PUFFS_USE_FULLPNBUF(pmp));
	puffs_msg_setinfo(park_link, PUFFSOP_VN,
	    PUFFS_VN_LINK, VPTOPNC(dvp));

	puffs_msg_enqueue(pmp, park_link);
	REFPN_AND_UNLOCKVP(dvp, dpn);
	REFPN(pn);
	error = puffs_msg_wait2(pmp, park_link, dpn, pn);

	PUFFS_MSG_RELEASE(link);

	error = checkerr(pmp, error, __func__);

	/*
	 * XXX: stay in touch with the cache.  I don't like this, but
	 * don't have a better solution either.  See also puffs_rename().
	 */
	if (error == 0)
		puffs_updatenode(pn, PUFFS_UPDATECTIME, 0);

	PNBUF_PUT(cnp->cn_pnbuf);
	RELEPN_AND_VP(dvp, dpn);
	puffs_releasenode(pn);

	return error;
}

int
puffs_vnop_symlink(void *v)
{
	struct vop_symlink_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_dvp;
		struct vnode **a_vpp;
		struct componentname *a_cnp;
		struct vattr *a_vap;
		char *a_target;
	} */ *ap = v;
	PUFFS_MSG_VARS(vn, symlink);
	struct vnode *dvp = ap->a_dvp;
	struct puffs_node *dpn = VPTOPP(dvp);
	struct mount *mp = dvp->v_mount;
	struct puffs_mount *pmp = MPTOPUFFSMP(dvp->v_mount);
	struct componentname *cnp = ap->a_cnp;
	int error;

	*ap->a_vpp = NULL;

	PUFFS_MSG_ALLOC(vn, symlink);
	puffs_makecn(&symlink_msg->pvnr_cn, &symlink_msg->pvnr_cn_cred,
		cnp, PUFFS_USE_FULLPNBUF(pmp));
	symlink_msg->pvnr_va = *ap->a_vap;
	(void)strlcpy(symlink_msg->pvnr_link, ap->a_target,
	    sizeof(symlink_msg->pvnr_link));
	puffs_msg_setinfo(park_symlink, PUFFSOP_VN,
	    PUFFS_VN_SYMLINK, VPTOPNC(dvp));

	puffs_msg_enqueue(pmp, park_symlink);
	REFPN_AND_UNLOCKVP(dvp, dpn);
	error = puffs_msg_wait2(pmp, park_symlink, dpn, NULL);

	error = checkerr(pmp, error, __func__);
	if (error)
		goto out;

	error = puffs_newnode(mp, dvp, ap->a_vpp,
	    symlink_msg->pvnr_newnode, cnp, VLNK, 0);
	if (error)
		puffs_abortbutton(pmp, PUFFS_ABORT_SYMLINK, dpn->pn_cookie,
		    symlink_msg->pvnr_newnode, cnp);

 out:
	PUFFS_MSG_RELEASE(symlink);
	if (error || (cnp->cn_flags & SAVESTART) == 0)
		PNBUF_PUT(cnp->cn_pnbuf);
	RELEPN_AND_VP(dvp, dpn);

	return error;
}

int
puffs_vnop_readlink(void *v)
{
	struct vop_readlink_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
		struct uio *a_uio;
		kauth_cred_t a_cred;
	} */ *ap = v;
	PUFFS_MSG_VARS(vn, readlink);
	struct vnode *vp = ap->a_vp;
	struct puffs_mount *pmp = MPTOPUFFSMP(ap->a_vp->v_mount);
	size_t linklen;
	int error;

	PUFFS_MSG_ALLOC(vn, readlink);
	puffs_credcvt(&readlink_msg->pvnr_cred, ap->a_cred);
	linklen = sizeof(readlink_msg->pvnr_link);
	readlink_msg->pvnr_linklen = linklen;
	puffs_msg_setinfo(park_readlink, PUFFSOP_VN,
	    PUFFS_VN_READLINK, VPTOPNC(vp));

	PUFFS_MSG_ENQUEUEWAIT2(pmp, park_readlink, vp->v_data, NULL, error);
	error = checkerr(pmp, error, __func__);
	if (error)
		goto out;

	/* bad bad user file server */
	if (readlink_msg->pvnr_linklen > linklen) {
		puffs_senderr(pmp, PUFFS_ERR_READLINK, E2BIG,
		    "linklen too big", VPTOPNC(ap->a_vp));
		error = EPROTO;
		goto out;
	}

	error = uiomove(&readlink_msg->pvnr_link, readlink_msg->pvnr_linklen,
	    ap->a_uio);
 out:
	PUFFS_MSG_RELEASE(readlink);
	return error;
}

int
puffs_vnop_rename(void *v)
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
	PUFFS_MSG_VARS(vn, rename);
	struct vnode *fdvp = ap->a_fdvp;
	struct puffs_node *fpn = ap->a_fvp->v_data;
	struct puffs_mount *pmp = MPTOPUFFSMP(fdvp->v_mount);
	int error;

	if (ap->a_fvp->v_mount != ap->a_tdvp->v_mount)
		ERROUT(EXDEV);

	PUFFS_MSG_ALLOC(vn, rename);
	rename_msg->pvnr_cookie_src = VPTOPNC(ap->a_fvp);
	rename_msg->pvnr_cookie_targdir = VPTOPNC(ap->a_tdvp);
	if (ap->a_tvp)
		rename_msg->pvnr_cookie_targ = VPTOPNC(ap->a_tvp);
	else
		rename_msg->pvnr_cookie_targ = NULL;
	puffs_makecn(&rename_msg->pvnr_cn_src, &rename_msg->pvnr_cn_src_cred,
	    ap->a_fcnp, PUFFS_USE_FULLPNBUF(pmp));
	puffs_makecn(&rename_msg->pvnr_cn_targ, &rename_msg->pvnr_cn_targ_cred,
	    ap->a_tcnp, PUFFS_USE_FULLPNBUF(pmp));
	puffs_msg_setinfo(park_rename, PUFFSOP_VN,
	    PUFFS_VN_RENAME, VPTOPNC(fdvp));

	PUFFS_MSG_ENQUEUEWAIT2(pmp, park_rename, fdvp->v_data, NULL, error);
	error = checkerr(pmp, error, __func__);

	/*
	 * XXX: stay in touch with the cache.  I don't like this, but
	 * don't have a better solution either.  See also puffs_link().
	 */
	if (error == 0)
		puffs_updatenode(fpn, PUFFS_UPDATECTIME, 0);

 out:
	PUFFS_MSG_RELEASE(rename);
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

#define RWARGS(cont, iofl, move, offset, creds)				\
	(cont)->pvnr_ioflag = (iofl);					\
	(cont)->pvnr_resid = (move);					\
	(cont)->pvnr_offset = (offset);					\
	puffs_credcvt(&(cont)->pvnr_cred, creds)

int
puffs_vnop_read(void *v)
{
	struct vop_read_args /* { 
		const struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
		struct uio *a_uio;
		int a_ioflag;
		kauth_cred_t a_cred;
	} */ *ap = v;
	PUFFS_MSG_VARS(vn, read);
	struct vnode *vp = ap->a_vp;
	struct puffs_mount *pmp = MPTOPUFFSMP(vp->v_mount);
	struct uio *uio = ap->a_uio;
	void *win;
	size_t tomove, argsize;
	vsize_t bytelen;
	int error, ubcflags;

	read_msg = NULL;
	error = 0;

	/* std sanity */
	if (uio->uio_resid == 0)
		return 0;
	if (uio->uio_offset < 0)
		return EINVAL;

	if (vp->v_type == VREG && PUFFS_USE_PAGECACHE(pmp)) {
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
			puffs_updatenode(VPTOPP(vp), PUFFS_UPDATEATIME, 0);
	} else {
		/*
		 * in case it's not a regular file or we're operating
		 * uncached, do read in the old-fashioned style,
		 * i.e. explicit read operations
		 */

		tomove = PUFFS_TOMOVE(uio->uio_resid, pmp);
		argsize = sizeof(struct puffs_vnmsg_read);
		puffs_msgmem_alloc(argsize + tomove, &park_read,
		    (void **)&read_msg, 1);

		error = 0;
		while (uio->uio_resid > 0) {
			tomove = PUFFS_TOMOVE(uio->uio_resid, pmp);
			memset(read_msg, 0, argsize); /* XXX: touser KASSERT */
			RWARGS(read_msg, ap->a_ioflag, tomove,
			    uio->uio_offset, ap->a_cred);
			puffs_msg_setinfo(park_read, PUFFSOP_VN,
			    PUFFS_VN_READ, VPTOPNC(vp));
			puffs_msg_setdelta(park_read, tomove);

			PUFFS_MSG_ENQUEUEWAIT2(pmp, park_read, vp->v_data,
			    NULL, error);
			error = checkerr(pmp, error, __func__);
			if (error)
				break;

			if (read_msg->pvnr_resid > tomove) {
				puffs_senderr(pmp, PUFFS_ERR_READ,
				    E2BIG, "resid grew", VPTOPNC(ap->a_vp));
				error = EPROTO;
				break;
			}

			error = uiomove(read_msg->pvnr_data,
			    tomove - read_msg->pvnr_resid, uio);

			/*
			 * in case the file is out of juice, resid from
			 * userspace is != 0.  and the error-case is
			 * quite obvious
			 */
			if (error || read_msg->pvnr_resid)
				break;
		}

		puffs_msgmem_release(park_read);
	}

	return error;
}

/*
 * XXX: in case of a failure, this leaves uio in a bad state.
 * We could theoretically copy the uio and iovecs and "replay"
 * them the right amount after the userspace trip, but don't
 * bother for now.
 */
int
puffs_vnop_write(void *v)
{
	struct vop_write_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
		struct uio *a_uio;
		int a_ioflag;
		kauth_cred_t a_cred;
	} */ *ap = v;
	PUFFS_MSG_VARS(vn, write);
	struct vnode *vp = ap->a_vp;
	struct puffs_mount *pmp = MPTOPUFFSMP(vp->v_mount);
	struct uio *uio = ap->a_uio;
	size_t tomove, argsize;
	off_t oldoff, newoff, origoff;
	vsize_t bytelen;
	int error, uflags;
	int ubcflags;

	error = uflags = 0;
	write_msg = NULL;

	if (vp->v_type == VREG && PUFFS_USE_PAGECACHE(pmp)) {
		ubcflags = UBC_WRITE | UBC_PARTIALOK;
		if (UBC_WANT_UNMAP(vp))
			ubcflags |= UBC_UNMAP;

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

			newoff = oldoff + bytelen;
			if (vp->v_size < newoff) {
				uvm_vnp_setwritesize(vp, newoff);
			}
			error = ubc_uiomove(&vp->v_uobj, uio, bytelen,
			    UVM_ADV_RANDOM, ubcflags);

			/*
			 * In case of a ubc_uiomove() error,
			 * opt to not extend the file at all and
			 * return an error.  Otherwise, if we attempt
			 * to clear the memory we couldn't fault to,
			 * we might generate a kernel page fault.
			 */
			if (vp->v_size < newoff) {
				if (error == 0) {
					uflags |= PUFFS_UPDATESIZE;
					uvm_vnp_setsize(vp, newoff);
				} else {
					uvm_vnp_setwritesize(vp, vp->v_size);
				}
			}
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

		/* synchronous I/O? */
		if (error == 0 && ap->a_ioflag & IO_SYNC) {
			simple_lock(&vp->v_interlock);
			error = VOP_PUTPAGES(vp, trunc_page(origoff),
			    round_page(uio->uio_offset),
			    PGO_CLEANIT | PGO_SYNCIO);

		/* write through page cache? */
		} else if (error == 0 && pmp->pmp_flags & PUFFS_KFLAG_WTCACHE) {
			simple_lock(&vp->v_interlock);
			error = VOP_PUTPAGES(vp, trunc_page(origoff),
			    round_page(uio->uio_offset), PGO_CLEANIT);
		}

		puffs_updatenode(VPTOPP(vp), uflags, vp->v_size);
	} else {
		/* tomove is non-increasing */
		tomove = PUFFS_TOMOVE(uio->uio_resid, pmp);
		argsize = sizeof(struct puffs_vnmsg_write) + tomove;
		puffs_msgmem_alloc(argsize, &park_write, (void **)&write_msg,1);

		while (uio->uio_resid > 0) {
			/* move data to buffer */
			tomove = PUFFS_TOMOVE(uio->uio_resid, pmp);
			memset(write_msg, 0, argsize); /* XXX: touser KASSERT */
			RWARGS(write_msg, ap->a_ioflag, tomove,
			    uio->uio_offset, ap->a_cred);
			error = uiomove(write_msg->pvnr_data, tomove, uio);
			if (error)
				break;

			/* move buffer to userspace */
			puffs_msg_setinfo(park_write, PUFFSOP_VN,
			    PUFFS_VN_WRITE, VPTOPNC(vp));
			PUFFS_MSG_ENQUEUEWAIT2(pmp, park_write, vp->v_data,
			    NULL, error);
			error = checkerr(pmp, error, __func__);
			if (error)
				break;

			if (write_msg->pvnr_resid > tomove) {
				puffs_senderr(pmp, PUFFS_ERR_WRITE,
				    E2BIG, "resid grew", VPTOPNC(ap->a_vp));
				error = EPROTO;
				break;
			}

			/* adjust file size */
			if (vp->v_size < uio->uio_offset)
				uvm_vnp_setsize(vp, uio->uio_offset);

			/* didn't move everything?  bad userspace.  bail */
			if (write_msg->pvnr_resid != 0) {
				error = EIO;
				break;
			}
		}
		puffs_msgmem_release(park_write);
	}

	return error;
}

int
puffs_vnop_print(void *v)
{
	struct vop_print_args /* {
		struct vnode *a_vp;
	} */ *ap = v;
	PUFFS_MSG_VARS(vn, print);
	struct vnode *vp = ap->a_vp;
	struct puffs_mount *pmp = MPTOPUFFSMP(vp->v_mount);
	struct puffs_node *pn = vp->v_data;
	int error;

	/* kernel portion */
	printf("tag VT_PUFFS, vnode %p, puffs node: %p,\n"
	    "    userspace cookie: %p\n", vp, pn, pn->pn_cookie);
	if (vp->v_type == VFIFO)
		fifo_printinfo(vp);
	lockmgr_printinfo(&vp->v_lock);

	/* userspace portion */
	if (EXISTSOP(pmp, PRINT)) {
		PUFFS_MSG_ALLOC(vn, print);
		puffs_msg_setinfo(park_print, PUFFSOP_VN,
		    PUFFS_VN_PRINT, VPTOPNC(vp));
		PUFFS_MSG_ENQUEUEWAIT2(pmp, park_print, vp->v_data,
		    NULL, error);
		PUFFS_MSG_RELEASE(print);
	}
	
	return 0;
}

int
puffs_vnop_pathconf(void *v)
{
	struct vop_pathconf_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
		int a_name;
		register_t *a_retval;
	} */ *ap = v;
	PUFFS_MSG_VARS(vn, pathconf);
	struct vnode *vp = ap->a_vp;
	struct puffs_mount *pmp = MPTOPUFFSMP(vp->v_mount);
	int error;

	PUFFS_MSG_ALLOC(vn, pathconf);
	pathconf_msg->pvnr_name = ap->a_name;
	puffs_msg_setinfo(park_pathconf, PUFFSOP_VN,
	    PUFFS_VN_PATHCONF, VPTOPNC(vp));
	PUFFS_MSG_ENQUEUEWAIT2(pmp, park_pathconf, vp->v_data, NULL, error);
	error = checkerr(pmp, error, __func__);
	if (!error)
		*ap->a_retval = pathconf_msg->pvnr_retval;
	PUFFS_MSG_RELEASE(pathconf);

	return error;
}

int
puffs_vnop_advlock(void *v)
{
	struct vop_advlock_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
		void *a_id;
		int a_op;
		struct flock *a_fl;
		int a_flags;
	} */ *ap = v;
	PUFFS_MSG_VARS(vn, advlock);
	struct vnode *vp = ap->a_vp;
	struct puffs_mount *pmp = MPTOPUFFSMP(vp->v_mount);
	int error;

	PUFFS_MSG_ALLOC(vn, advlock);
	error = copyin(ap->a_fl, &advlock_msg->pvnr_fl, sizeof(struct flock));
	if (error)
		goto out;
	advlock_msg->pvnr_id = ap->a_id;
	advlock_msg->pvnr_op = ap->a_op;
	advlock_msg->pvnr_flags = ap->a_flags;
	puffs_msg_setinfo(park_advlock, PUFFSOP_VN,
	    PUFFS_VN_ADVLOCK, VPTOPNC(vp));

	PUFFS_MSG_ENQUEUEWAIT2(pmp, park_advlock, vp->v_data, NULL, error);
	error = checkerr(pmp, error, __func__);

 out:
	PUFFS_MSG_RELEASE(advlock);
	return error;
}

#define BIOASYNC(bp) (bp->b_flags & B_ASYNC)

/*
 * This maps itself to PUFFS_VN_READ/WRITE for data transfer.
 */
int
puffs_vnop_strategy(void *v)
{
	struct vop_strategy_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
		struct buf *a_bp;
	} */ *ap = v;
	PUFFS_MSG_VARS(vn, rw);
	struct vnode *vp = ap->a_vp;
	struct puffs_mount *pmp = MPTOPUFFSMP(vp->v_mount);
	struct puffs_node *pn;
	struct buf *bp;
	size_t argsize;
	size_t tomove, moved;
	int error, dofaf, dobiodone;

	pmp = MPTOPUFFSMP(vp->v_mount);
	bp = ap->a_bp;
	error = 0;
	dofaf = 0;
	pn = VPTOPP(vp);
	park_rw = NULL; /* explicit */
	dobiodone = 1;

	if ((BUF_ISREAD(bp) && !EXISTSOP(pmp, READ))
	    || (BUF_ISWRITE(bp) && !EXISTSOP(pmp, WRITE)))
		ERROUT(EOPNOTSUPP);

	/*
	 * Short-circuit optimization: don't flush buffer in between
	 * VOP_INACTIVE and VOP_RECLAIM in case the node has no references.
	 */
	if (pn->pn_stat & PNODE_DYING) {
		KASSERT(BUF_ISWRITE(bp));
		bp->b_resid = 0;
		goto out;
	}

#ifdef DIAGNOSTIC
	if (bp->b_bcount > pmp->pmp_msg_maxsize - PUFFS_MSGSTRUCT_MAX)
		panic("puffs_strategy: wildly inappropriate buf bcount %d",
		    bp->b_bcount);
#endif

	/*
	 * See explanation for the necessity of a FAF in puffs_fsync.
	 *
	 * Also, do FAF in case we're suspending.
	 * See puffs_vfsops.c:pageflush()
	 */
	if (BUF_ISWRITE(bp)) {
		simple_lock(&vp->v_interlock);
		if (vp->v_iflag & VI_XLOCK)
			dofaf = 1;
		if (pn->pn_stat & PNODE_SUSPEND)
			dofaf = 1;
		simple_unlock(&vp->v_interlock);
	}

#ifdef DIAGNOSTIC
		if (curlwp == uvm.pagedaemon_lwp)
			KASSERT(dofaf || BIOASYNC(bp));
#endif

	/* allocate transport structure */
	tomove = PUFFS_TOMOVE(bp->b_bcount, pmp);
	argsize = sizeof(struct puffs_vnmsg_rw);
	error = puffs_msgmem_alloc(argsize + tomove, &park_rw,
	    (void **)&rw_msg, dofaf ? 0 : 1);
	if (error)
		goto out;
	RWARGS(rw_msg, 0, tomove, bp->b_blkno << DEV_BSHIFT, FSCRED);

	/* 2x2 cases: read/write, faf/nofaf */
	if (BUF_ISREAD(bp)) {
		puffs_msg_setinfo(park_rw, PUFFSOP_VN,
		    PUFFS_VN_READ, VPTOPNC(vp));
		puffs_msg_setdelta(park_rw, tomove);
		if (BIOASYNC(bp)) {
			puffs_msg_setcall(park_rw,
			    puffs_parkdone_asyncbioread, bp);
			puffs_msg_enqueue(pmp, park_rw);
			dobiodone = 0;
		} else {
			PUFFS_MSG_ENQUEUEWAIT2(pmp, park_rw, vp->v_data,
			    NULL, error);
			error = checkerr(pmp, error, __func__);
			if (error)
				goto out;

			if (rw_msg->pvnr_resid > tomove) {
				puffs_senderr(pmp, PUFFS_ERR_READ,
				    E2BIG, "resid grew", VPTOPNC(vp));
				ERROUT(EPROTO);
			}

			moved = tomove - rw_msg->pvnr_resid;

			(void)memcpy(bp->b_data, rw_msg->pvnr_data, moved);
			bp->b_resid = bp->b_bcount - moved;
		}
	} else {
		puffs_msg_setinfo(park_rw, PUFFSOP_VN,
		    PUFFS_VN_WRITE, VPTOPNC(vp));
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

		(void)memcpy(&rw_msg->pvnr_data, bp->b_data, tomove);
		if (dofaf) {
			puffs_msg_setfaf(park_rw);
		} else if (BIOASYNC(bp)) {
			puffs_msg_setcall(park_rw,
			    puffs_parkdone_asyncbiowrite, bp);
			dobiodone = 0;
		}

		PUFFS_MSG_ENQUEUEWAIT2(pmp, park_rw, vp->v_data, NULL, error);

		if (dobiodone == 0)
			goto out;

		/*
		 * XXXXXXXX: wrong, but kernel can't survive strategy
		 * failure currently.  Here, have one more X: X.
		 */
		if (error != ENOMEM)
			error = 0;

		error = checkerr(pmp, error, __func__);
		if (error)
			goto out;

		if (rw_msg->pvnr_resid > tomove) {
			puffs_senderr(pmp, PUFFS_ERR_WRITE,
			    E2BIG, "resid grew", VPTOPNC(vp));
			ERROUT(EPROTO);
		}

		/*
		 * FAF moved everything.  Frankly, we don't
		 * really have a choice.
		 */
		if (dofaf && error == 0)
			moved = tomove;
		else 
			moved = tomove - rw_msg->pvnr_resid;

		bp->b_resid = bp->b_bcount - moved;
		if (bp->b_resid != 0) {
			ERROUT(EIO);
		}
	}

 out:
	if (park_rw)
		puffs_msgmem_release(park_rw);

	if (error)
		bp->b_error = error;

	if (error || dobiodone)
		biodone(bp);

	return error;
}

int
puffs_vnop_mmap(void *v)
{
	struct vop_mmap_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
		vm_prot_t a_prot;
		kauth_cred_t a_cred;
	} */ *ap = v;
	PUFFS_MSG_VARS(vn, mmap);
	struct vnode *vp = ap->a_vp;
	struct puffs_mount *pmp = MPTOPUFFSMP(vp->v_mount);
	int error;

	if (!PUFFS_USE_PAGECACHE(pmp))
		return genfs_eopnotsupp(v);

	if (EXISTSOP(pmp, MMAP)) {
		PUFFS_MSG_ALLOC(vn, mmap);
		mmap_msg->pvnr_prot = ap->a_prot;
		puffs_credcvt(&mmap_msg->pvnr_cred, ap->a_cred);
		puffs_msg_setinfo(park_mmap, PUFFSOP_VN,
		    PUFFS_VN_MMAP, VPTOPNC(vp));

		PUFFS_MSG_ENQUEUEWAIT2(pmp, park_mmap, vp->v_data, NULL, error);
		error = checkerr(pmp, error, __func__);
		PUFFS_MSG_RELEASE(mmap);
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
puffs_vnop_bmap(void *v)
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
		    = (PUFFS_TOMOVE(pmp->pmp_msg_maxsize, pmp)>>DEV_BSHIFT) - 1;

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
puffs_vnop_getpages(void *v)
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
	struct puffs_node *pn;
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
	pn = vp->v_data;
	locked = (ap->a_flags & PGO_LOCKED) != 0;
	write = (ap->a_access_type & VM_PROT_WRITE) != 0;

	/* ccg xnaht - gets Wuninitialized wrong */
	pcrun = NULL;
	runsizes = 0;

	/*
	 * Check that we aren't trying to fault in pages which our file
	 * server doesn't know about.  This happens if we extend a file by
	 * skipping some pages and later try to fault in pages which
	 * are between pn_serversize and vp_size.  This check optimizes
	 * away the common case where a file is being extended.
	 */
	if (ap->a_offset >= pn->pn_serversize && ap->a_offset < vp->v_size) {
		struct vattr va;

		/* try again later when we can block */
		if (locked)
			ERROUT(EBUSY);

		simple_unlock(&vp->v_interlock);
		vattr_null(&va);
		va.va_size = vp->v_size;
		error = dosetattr(vp, &va, FSCRED, 0);
		if (error)
			ERROUT(error);
		simple_lock(&vp->v_interlock);
	}

	if (write && PUFFS_WCACHEINFO(pmp)) {
#ifdef notnowjohn
		/* allocate worst-case memory */
		runsizes = ((npages / 2) + 1) * sizeof(struct puffs_cacherun);
		pcinfo = kmem_zalloc(sizeof_puffs_cacheinfo) + runsize,
		    locked ? KM_NOSLEEP : KM_SLEEP);

		/*
		 * can't block if we're locked and can't mess up caching
		 * information for fs server.  so come back later, please
		 */
		if (pcinfo == NULL)
			ERROUT(ENOMEM);

		parkmem = puffs_park_alloc(locked == 0);
		if (parkmem == NULL)
			ERROUT(ENOMEM);

		pcrun = pcinfo->pcache_runs;
#else
		(void)parkmem;
#endif
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

#ifdef notnowjohn
	/* send results to userspace */
	if (write)
		puffs_cacheop(pmp, parkmem, pcinfo,
		    sizeof(struct puffs_cacheinfo) + runsizes, VPTOPNC(vp));
#endif

 out:
	if (error) {
		if (pcinfo != NULL)
			kmem_free(pcinfo,
			    sizeof(struct puffs_cacheinfo) + runsizes);
#ifdef notnowjohn
		if (parkmem != NULL)
			puffs_park_release(parkmem, 1);
#endif
	}

	return error;
}

int
puffs_vnop_lock(void *v)
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
puffs_vnop_unlock(void *v)
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
puffs_vnop_islocked(void *v)
{
	struct vop_islocked_args *ap = v;
	int rv;

	rv = lockstatus(&ap->a_vp->v_lock);
	return rv;
}


/*
 * spec & fifo.  These call the miscfs spec and fifo vectors, but issue
 * FAF update information for the puffs node first.
 */
int
puffs_vnop_spec_read(void *v)
{
	struct vop_read_args /* { 
		const struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
		struct uio *a_uio;
		int a_ioflag;
		kauth_cred_t a_cred;
	} */ *ap = v;

	puffs_updatenode(VPTOPP(ap->a_vp), PUFFS_UPDATEATIME, 0);
	return VOCALL(spec_vnodeop_p, VOFFSET(vop_read), v);
}

int
puffs_vnop_spec_write(void *v)
{
	struct vop_write_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
		struct uio *a_uio;
		int a_ioflag;
		kauth_cred_t a_cred;
	} */ *ap = v;

	puffs_updatenode(VPTOPP(ap->a_vp), PUFFS_UPDATEMTIME, 0);
	return VOCALL(spec_vnodeop_p, VOFFSET(vop_write), v);
}

int
puffs_vnop_fifo_read(void *v)
{
	struct vop_read_args /* { 
		const struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
		struct uio *a_uio;
		int a_ioflag;
		kauth_cred_t a_cred;
	} */ *ap = v;

	puffs_updatenode(VPTOPP(ap->a_vp), PUFFS_UPDATEATIME, 0);
	return VOCALL(fifo_vnodeop_p, VOFFSET(vop_read), v);
}

int
puffs_vnop_fifo_write(void *v)
{
	struct vop_write_args /* {
		const struct vnodeop_desc *a_desc;
		struct vnode *a_vp;
		struct uio *a_uio;
		int a_ioflag;
		kauth_cred_t a_cred;
	} */ *ap = v;

	puffs_updatenode(VPTOPP(ap->a_vp), PUFFS_UPDATEMTIME, 0);
	return VOCALL(fifo_vnodeop_p, VOFFSET(vop_write), v);
}
