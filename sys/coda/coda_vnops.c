/*	$NetBSD: coda_vnops.c,v 1.97 2014/07/25 08:20:51 dholland Exp $	*/

/*
 *
 *             Coda: an Experimental Distributed File System
 *                              Release 3.1
 *
 *           Copyright (c) 1987-1998 Carnegie Mellon University
 *                          All Rights Reserved
 *
 * Permission  to  use, copy, modify and distribute this software and its
 * documentation is hereby granted,  provided  that  both  the  copyright
 * notice  and  this  permission  notice  appear  in  all  copies  of the
 * software, derivative works or  modified  versions,  and  any  portions
 * thereof, and that both notices appear in supporting documentation, and
 * that credit is given to Carnegie Mellon University  in  all  documents
 * and publicity pertaining to direct or indirect use of this code or its
 * derivatives.
 *
 * CODA IS AN EXPERIMENTAL SOFTWARE SYSTEM AND IS  KNOWN  TO  HAVE  BUGS,
 * SOME  OF  WHICH MAY HAVE SERIOUS CONSEQUENCES.  CARNEGIE MELLON ALLOWS
 * FREE USE OF THIS SOFTWARE IN ITS "AS IS" CONDITION.   CARNEGIE  MELLON
 * DISCLAIMS  ANY  LIABILITY  OF  ANY  KIND  FOR  ANY  DAMAGES WHATSOEVER
 * RESULTING DIRECTLY OR INDIRECTLY FROM THE USE OF THIS SOFTWARE  OR  OF
 * ANY DERIVATIVE WORK.
 *
 * Carnegie  Mellon  encourages  users  of  this  software  to return any
 * improvements or extensions that  they  make,  and  to  grant  Carnegie
 * Mellon the rights to redistribute these changes without encumbrance.
 *
 * 	@(#) coda/coda_vnops.c,v 1.1.1.1 1998/08/29 21:26:46 rvb Exp $
 */

/*
 * Mach Operating System
 * Copyright (c) 1990 Carnegie-Mellon University
 * Copyright (c) 1989 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */

/*
 * This code was written for the Coda file system at Carnegie Mellon
 * University.  Contributers include David Steere, James Kistler, and
 * M. Satyanarayanan.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: coda_vnops.c,v 1.97 2014/07/25 08:20:51 dholland Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/errno.h>
#include <sys/acct.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <sys/namei.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/select.h>
#include <sys/vnode.h>
#include <sys/kauth.h>

#include <miscfs/genfs/genfs.h>

#include <coda/coda.h>
#include <coda/cnode.h>
#include <coda/coda_vnops.h>
#include <coda/coda_venus.h>
#include <coda/coda_opstats.h>
#include <coda/coda_subr.h>
#include <coda/coda_namecache.h>
#include <coda/coda_pioctl.h>

/*
 * These flags select various performance enhancements.
 */
int coda_attr_cache  = 1;       /* Set to cache attributes in the kernel */
int coda_symlink_cache = 1;     /* Set to cache symbolic link information */
int coda_access_cache = 1;      /* Set to handle some access checks directly */

/* structure to keep track of vfs calls */

struct coda_op_stats coda_vnodeopstats[CODA_VNODEOPS_SIZE];

#define MARK_ENTRY(op) (coda_vnodeopstats[op].entries++)
#define MARK_INT_SAT(op) (coda_vnodeopstats[op].sat_intrn++)
#define MARK_INT_FAIL(op) (coda_vnodeopstats[op].unsat_intrn++)
#define MARK_INT_GEN(op) (coda_vnodeopstats[op].gen_intrn++)

/* What we are delaying for in printf */
static int coda_lockdebug = 0;

#define ENTRY if(coda_vnop_print_entry) myprintf(("Entered %s\n",__func__))

/* Definition of the vnode operation vector */

const struct vnodeopv_entry_desc coda_vnodeop_entries[] = {
    { &vop_default_desc, coda_vop_error },
    { &vop_lookup_desc, coda_lookup },		/* lookup */
    { &vop_create_desc, coda_create },		/* create */
    { &vop_mknod_desc, coda_vop_error },	/* mknod */
    { &vop_open_desc, coda_open },		/* open */
    { &vop_close_desc, coda_close },		/* close */
    { &vop_access_desc, coda_access },		/* access */
    { &vop_getattr_desc, coda_getattr },	/* getattr */
    { &vop_setattr_desc, coda_setattr },	/* setattr */
    { &vop_read_desc, coda_read },		/* read */
    { &vop_write_desc, coda_write },		/* write */
    { &vop_fallocate_desc, genfs_eopnotsupp },	/* fallocate */
    { &vop_fdiscard_desc, genfs_eopnotsupp },	/* fdiscard */
    { &vop_fcntl_desc, genfs_fcntl },		/* fcntl */
    { &vop_ioctl_desc, coda_ioctl },		/* ioctl */
    { &vop_mmap_desc, genfs_mmap },		/* mmap */
    { &vop_fsync_desc, coda_fsync },		/* fsync */
    { &vop_remove_desc, coda_remove },		/* remove */
    { &vop_link_desc, coda_link },		/* link */
    { &vop_rename_desc, coda_rename },		/* rename */
    { &vop_mkdir_desc, coda_mkdir },		/* mkdir */
    { &vop_rmdir_desc, coda_rmdir },		/* rmdir */
    { &vop_symlink_desc, coda_symlink },	/* symlink */
    { &vop_readdir_desc, coda_readdir },	/* readdir */
    { &vop_readlink_desc, coda_readlink },	/* readlink */
    { &vop_abortop_desc, coda_abortop },	/* abortop */
    { &vop_inactive_desc, coda_inactive },	/* inactive */
    { &vop_reclaim_desc, coda_reclaim },	/* reclaim */
    { &vop_lock_desc, coda_lock },		/* lock */
    { &vop_unlock_desc, coda_unlock },		/* unlock */
    { &vop_bmap_desc, coda_bmap },		/* bmap */
    { &vop_strategy_desc, coda_strategy },	/* strategy */
    { &vop_print_desc, coda_vop_error },	/* print */
    { &vop_islocked_desc, coda_islocked },	/* islocked */
    { &vop_pathconf_desc, coda_vop_error },	/* pathconf */
    { &vop_advlock_desc, coda_vop_nop },	/* advlock */
    { &vop_bwrite_desc, coda_vop_error },	/* bwrite */
    { &vop_seek_desc, genfs_seek },		/* seek */
    { &vop_poll_desc, genfs_poll },		/* poll */
    { &vop_getpages_desc, coda_getpages },	/* getpages */
    { &vop_putpages_desc, coda_putpages },	/* putpages */
    { NULL, NULL }
};

static void coda_print_vattr(struct vattr *);

int (**coda_vnodeop_p)(void *);
const struct vnodeopv_desc coda_vnodeop_opv_desc =
        { &coda_vnodeop_p, coda_vnodeop_entries };

/* Definitions of NetBSD vnodeop interfaces */

/*
 * A generic error routine.  Return EIO without looking at arguments.
 */
int
coda_vop_error(void *anon) {
    struct vnodeop_desc **desc = (struct vnodeop_desc **)anon;

    if (codadebug) {
	myprintf(("%s: Vnode operation %s called (error).\n",
	    __func__, (*desc)->vdesc_name));
    }

    return EIO;
}

/* A generic do-nothing. */
int
coda_vop_nop(void *anon) {
    struct vnodeop_desc **desc = (struct vnodeop_desc **)anon;

    if (codadebug) {
	myprintf(("Vnode operation %s called, but unsupported\n",
		  (*desc)->vdesc_name));
    }
   return (0);
}

int
coda_vnodeopstats_init(void)
{
	int i;

	for(i=0;i<CODA_VNODEOPS_SIZE;i++) {
		coda_vnodeopstats[i].opcode = i;
		coda_vnodeopstats[i].entries = 0;
		coda_vnodeopstats[i].sat_intrn = 0;
		coda_vnodeopstats[i].unsat_intrn = 0;
		coda_vnodeopstats[i].gen_intrn = 0;
	}

	return 0;
}

/*
 * XXX The entire relationship between VOP_OPEN and having a container
 * file (via venus_open) needs to be reexamined.  In particular, it's
 * valid to open/mmap/close and then reference.  Instead of doing
 * VOP_OPEN when getpages needs a container, we should do the
 * venus_open part, and record that the vnode has opened the container
 * for getpages, and do the matching logical close on coda_inactive.
 * Further, coda_rdwr needs a container file, and sometimes needs to
 * do the equivalent of open (core dumps).
 */
/*
 * coda_open calls Venus to return the device and inode of the
 * container file, and then obtains a vnode for that file.  The
 * container vnode is stored in the coda vnode, and a reference is
 * added for each open file.
 */
int
coda_open(void *v)
{
    /*
     * NetBSD can pass the O_EXCL flag in mode, even though the check
     * has already happened.  Venus defensively assumes that if open
     * is passed the EXCL, it must be a bug.  We strip the flag here.
     */
/* true args */
    struct vop_open_args *ap = v;
    vnode_t *vp = ap->a_vp;
    struct cnode *cp = VTOC(vp);
    int flag = ap->a_mode & (~O_EXCL);
    kauth_cred_t cred = ap->a_cred;
/* locals */
    int error;
    dev_t dev;			/* container file device, inode, vnode */
    ino_t inode;
    vnode_t *container_vp;

    MARK_ENTRY(CODA_OPEN_STATS);

    KASSERT(VOP_ISLOCKED(vp));
    /* Check for open of control file. */
    if (IS_CTL_VP(vp)) {
	/* if (WRITABLE(flag)) */
	if (flag & (FWRITE | O_TRUNC | O_CREAT | O_EXCL)) {
	    MARK_INT_FAIL(CODA_OPEN_STATS);
	    return(EACCES);
	}
	MARK_INT_SAT(CODA_OPEN_STATS);
	return(0);
    }

    error = venus_open(vtomi(vp), &cp->c_fid, flag, cred, curlwp, &dev, &inode);
    if (error)
	return (error);
    if (!error) {
	    CODADEBUG(CODA_OPEN, myprintf((
		"%s: dev 0x%llx inode %llu result %d\n", __func__,
		(unsigned long long)dev, (unsigned long long)inode, error));)
    }

    /* 
     * Obtain locked and referenced container vnode from container
     * device/inode.
     */
    error = coda_grab_vnode(vp, dev, inode, &container_vp);
    if (error)
	return (error);

    /* Save the vnode pointer for the container file. */
    if (cp->c_ovp == NULL) {
	cp->c_ovp = container_vp;
    } else {
	if (cp->c_ovp != container_vp)
	    /*
	     * Perhaps venus returned a different container, or
	     * something else went wrong.
	     */
	    panic("%s: cp->c_ovp != container_vp", __func__);
    }
    cp->c_ocount++;

    /* Flush the attribute cache if writing the file. */
    if (flag & FWRITE) {
	cp->c_owrite++;
	cp->c_flags &= ~C_VATTR;
    }

    /* 
     * Save the <device, inode> pair for the container file to speed
     * up subsequent reads while closed (mmap, program execution).
     * This is perhaps safe because venus will invalidate the node
     * before changing the container file mapping.
     */
    cp->c_device = dev;
    cp->c_inode = inode;

    /* Open the container file. */
    error = VOP_OPEN(container_vp, flag, cred);
    /* 
     * Drop the lock on the container, after we have done VOP_OPEN
     * (which requires a locked vnode).
     */
    VOP_UNLOCK(container_vp);
    return(error);
}

/*
 * Close the cache file used for I/O and notify Venus.
 */
int
coda_close(void *v)
{
/* true args */
    struct vop_close_args *ap = v;
    vnode_t *vp = ap->a_vp;
    struct cnode *cp = VTOC(vp);
    int flag = ap->a_fflag;
    kauth_cred_t cred = ap->a_cred;
/* locals */
    int error;

    MARK_ENTRY(CODA_CLOSE_STATS);

    /* Check for close of control file. */
    if (IS_CTL_VP(vp)) {
	MARK_INT_SAT(CODA_CLOSE_STATS);
	return(0);
    }

    /*
     * XXX The IS_UNMOUNTING part of this is very suspect.
     */ 
    if (IS_UNMOUNTING(cp)) {
	if (cp->c_ovp) {
#ifdef	CODA_VERBOSE
	    printf("%s: destroying container %d, ufs vp %p of vp %p/cp %p\n",
		__func__, vp->v_usecount, cp->c_ovp, vp, cp);
#endif
#ifdef	hmm
	    vgone(cp->c_ovp);
#else
	    vn_lock(cp->c_ovp, LK_EXCLUSIVE | LK_RETRY);
	    VOP_CLOSE(cp->c_ovp, flag, cred); /* Do errors matter here? */
	    vput(cp->c_ovp);
#endif
	} else {
#ifdef	CODA_VERBOSE
	    printf("%s: NO container vp %p/cp %p\n", __func__, vp, cp);
#endif
	}
	return ENODEV;
    }

    /* Lock the container node, and VOP_CLOSE it. */
    vn_lock(cp->c_ovp, LK_EXCLUSIVE | LK_RETRY);
    VOP_CLOSE(cp->c_ovp, flag, cred); /* Do errors matter here? */
    /*
     * Drop the lock we just obtained, and vrele the container vnode.
     * Decrement reference counts, and clear container vnode pointer on
     * last close.
     */
    vput(cp->c_ovp);
    if (flag & FWRITE)
	--cp->c_owrite;
    if (--cp->c_ocount == 0)
	cp->c_ovp = NULL;

    error = venus_close(vtomi(vp), &cp->c_fid, flag, cred, curlwp);

    CODADEBUG(CODA_CLOSE, myprintf(("%s: result %d\n", __func__, error)); )
    return(error);
}

int
coda_read(void *v)
{
    struct vop_read_args *ap = v;

    ENTRY;
    return(coda_rdwr(ap->a_vp, ap->a_uio, UIO_READ,
		    ap->a_ioflag, ap->a_cred, curlwp));
}

int
coda_write(void *v)
{
    struct vop_write_args *ap = v;

    ENTRY;
    return(coda_rdwr(ap->a_vp, ap->a_uio, UIO_WRITE,
		    ap->a_ioflag, ap->a_cred, curlwp));
}

int
coda_rdwr(vnode_t *vp, struct uio *uiop, enum uio_rw rw, int ioflag,
	kauth_cred_t cred, struct lwp *l)
{
/* upcall decl */
  /* NOTE: container file operation!!! */
/* locals */
    struct cnode *cp = VTOC(vp);
    vnode_t *cfvp = cp->c_ovp;
    struct proc *p = l->l_proc;
    int opened_internally = 0;
    int error = 0;

    MARK_ENTRY(CODA_RDWR_STATS);

    CODADEBUG(CODA_RDWR, myprintf(("coda_rdwr(%d, %p, %lu, %lld)\n", rw,
	uiop->uio_iov->iov_base, (unsigned long) uiop->uio_resid,
	(long long) uiop->uio_offset)); )

    /* Check for rdwr of control object. */
    if (IS_CTL_VP(vp)) {
	MARK_INT_FAIL(CODA_RDWR_STATS);
	return(EINVAL);
    }

    /* Redirect the request to UFS. */

    /*
     * If file is not already open this must be a page
     * {read,write} request.  Iget the cache file's inode
     * pointer if we still have its <device, inode> pair.
     * Otherwise, we must do an internal open to derive the
     * pair.
     * XXX Integrate this into a coherent strategy for container
     * file acquisition.
     */
    if (cfvp == NULL) {
	/*
	 * If we're dumping core, do the internal open. Otherwise
	 * venus won't have the correct size of the core when
	 * it's completely written.
	 */
	if (cp->c_inode != 0 && !(p && (p->p_acflag & ACORE))) {
#ifdef CODA_VERBOSE
	    printf("%s: grabbing container vnode, losing reference\n",
		__func__);
#endif
	    /* Get locked and refed vnode. */
	    error = coda_grab_vnode(vp, cp->c_device, cp->c_inode, &cfvp);
	    if (error) {
		MARK_INT_FAIL(CODA_RDWR_STATS);
		return(error);
	    }
	    /* 
	     * Drop lock. 
	     * XXX Where is reference released.
	     */
	    VOP_UNLOCK(cfvp);
	}
	else {
#ifdef CODA_VERBOSE
	    printf("%s: internal VOP_OPEN\n", __func__);
#endif
	    opened_internally = 1;
	    MARK_INT_GEN(CODA_OPEN_STATS);
	    error = VOP_OPEN(vp, (rw == UIO_READ ? FREAD : FWRITE), cred);
#ifdef	CODA_VERBOSE
	    printf("%s: Internally Opening %p\n", __func__, vp);
#endif
	    if (error) {
		MARK_INT_FAIL(CODA_RDWR_STATS);
		return(error);
	    }
	    cfvp = cp->c_ovp;
	}
    }

    /* Have UFS handle the call. */
    CODADEBUG(CODA_RDWR, myprintf(("%s: fid = %s, refcnt = %d\n", __func__,
	coda_f2s(&cp->c_fid), CTOV(cp)->v_usecount)); )

    if (rw == UIO_READ) {
	error = VOP_READ(cfvp, uiop, ioflag, cred);
    } else {
	error = VOP_WRITE(cfvp, uiop, ioflag, cred);
    }

    if (error)
	MARK_INT_FAIL(CODA_RDWR_STATS);
    else
	MARK_INT_SAT(CODA_RDWR_STATS);

    /* Do an internal close if necessary. */
    if (opened_internally) {
	MARK_INT_GEN(CODA_CLOSE_STATS);
	(void)VOP_CLOSE(vp, (rw == UIO_READ ? FREAD : FWRITE), cred);
    }

    /* Invalidate cached attributes if writing. */
    if (rw == UIO_WRITE)
	cp->c_flags &= ~C_VATTR;
    return(error);
}

int
coda_ioctl(void *v)
{
/* true args */
    struct vop_ioctl_args *ap = v;
    vnode_t *vp = ap->a_vp;
    int com = ap->a_command;
    void *data = ap->a_data;
    int flag = ap->a_fflag;
    kauth_cred_t cred = ap->a_cred;
/* locals */
    int error;
    vnode_t *tvp;
    struct PioctlData *iap = (struct PioctlData *)data;
    namei_simple_flags_t sflags;

    MARK_ENTRY(CODA_IOCTL_STATS);

    CODADEBUG(CODA_IOCTL, myprintf(("in coda_ioctl on %s\n", iap->path));)

    /* Don't check for operation on a dying object, for ctlvp it
       shouldn't matter */

    /* Must be control object to succeed. */
    if (!IS_CTL_VP(vp)) {
	MARK_INT_FAIL(CODA_IOCTL_STATS);
	CODADEBUG(CODA_IOCTL, myprintf(("%s error: vp != ctlvp", __func__));)
	return (EOPNOTSUPP);
    }
    /* Look up the pathname. */

    /* Should we use the name cache here? It would get it from
       lookupname sooner or later anyway, right? */

    sflags = iap->follow ? NSM_FOLLOW_NOEMULROOT : NSM_NOFOLLOW_NOEMULROOT;
    error = namei_simple_user(iap->path, sflags, &tvp);

    if (error) {
	MARK_INT_FAIL(CODA_IOCTL_STATS);
	CODADEBUG(CODA_IOCTL, myprintf(("%s error: lookup returns %d\n",
	    __func__, error));)
	return(error);
    }

    /*
     * Make sure this is a coda style cnode, but it may be a
     * different vfsp
     */
    /* XXX: this totally violates the comment about vtagtype in vnode.h */
    if (tvp->v_tag != VT_CODA) {
	vrele(tvp);
	MARK_INT_FAIL(CODA_IOCTL_STATS);
	CODADEBUG(CODA_IOCTL, myprintf(("%s error: %s not a coda object\n",
	    __func__, iap->path));)
	return(EINVAL);
    }

    if (iap->vi.in_size > VC_MAXDATASIZE || iap->vi.out_size > VC_MAXDATASIZE) {
	vrele(tvp);
	return(EINVAL);
    }
    error = venus_ioctl(vtomi(tvp), &((VTOC(tvp))->c_fid), com, flag, data,
	cred, curlwp);

    if (error)
	MARK_INT_FAIL(CODA_IOCTL_STATS);
    else
	CODADEBUG(CODA_IOCTL, myprintf(("Ioctl returns %d \n", error)); )

    vrele(tvp);
    return(error);
}

/*
 * To reduce the cost of a user-level venus;we cache attributes in
 * the kernel.  Each cnode has storage allocated for an attribute. If
 * c_vattr is valid, return a reference to it. Otherwise, get the
 * attributes from venus and store them in the cnode.  There is some
 * question if this method is a security leak. But I think that in
 * order to make this call, the user must have done a lookup and
 * opened the file, and therefore should already have access.
 */
int
coda_getattr(void *v)
{
/* true args */
    struct vop_getattr_args *ap = v;
    vnode_t *vp = ap->a_vp;
    struct cnode *cp = VTOC(vp);
    struct vattr *vap = ap->a_vap;
    kauth_cred_t cred = ap->a_cred;
/* locals */
    int error;

    MARK_ENTRY(CODA_GETATTR_STATS);

    /* Check for getattr of control object. */
    if (IS_CTL_VP(vp)) {
	MARK_INT_FAIL(CODA_GETATTR_STATS);
	return(ENOENT);
    }

    /* Check to see if the attributes have already been cached */
    if (VALID_VATTR(cp)) {
	CODADEBUG(CODA_GETATTR, { myprintf(("%s: attr cache hit: %s\n",
	    __func__, coda_f2s(&cp->c_fid)));})
	CODADEBUG(CODA_GETATTR, if (!(codadebug & ~CODA_GETATTR))
	    coda_print_vattr(&cp->c_vattr); )

	*vap = cp->c_vattr;
	MARK_INT_SAT(CODA_GETATTR_STATS);
	return(0);
    }

    error = venus_getattr(vtomi(vp), &cp->c_fid, cred, curlwp, vap);

    if (!error) {
	CODADEBUG(CODA_GETATTR, myprintf(("%s miss %s: result %d\n",
	    __func__, coda_f2s(&cp->c_fid), error)); )

	CODADEBUG(CODA_GETATTR, if (!(codadebug & ~CODA_GETATTR))
	    coda_print_vattr(vap);	)

	/* If not open for write, store attributes in cnode */
	if ((cp->c_owrite == 0) && (coda_attr_cache)) {
	    cp->c_vattr = *vap;
	    cp->c_flags |= C_VATTR;
	}

    }
    return(error);
}

int
coda_setattr(void *v)
{
/* true args */
    struct vop_setattr_args *ap = v;
    vnode_t *vp = ap->a_vp;
    struct cnode *cp = VTOC(vp);
    struct vattr *vap = ap->a_vap;
    kauth_cred_t cred = ap->a_cred;
/* locals */
    int error;

    MARK_ENTRY(CODA_SETATTR_STATS);

    /* Check for setattr of control object. */
    if (IS_CTL_VP(vp)) {
	MARK_INT_FAIL(CODA_SETATTR_STATS);
	return(ENOENT);
    }

    if (codadebug & CODADBGMSK(CODA_SETATTR)) {
	coda_print_vattr(vap);
    }
    error = venus_setattr(vtomi(vp), &cp->c_fid, vap, cred, curlwp);

    if (!error)
	cp->c_flags &= ~C_VATTR;

    CODADEBUG(CODA_SETATTR,	myprintf(("setattr %d\n", error)); )
    return(error);
}

int
coda_access(void *v)
{
/* true args */
    struct vop_access_args *ap = v;
    vnode_t *vp = ap->a_vp;
    struct cnode *cp = VTOC(vp);
    int mode = ap->a_mode;
    kauth_cred_t cred = ap->a_cred;
/* locals */
    int error;

    MARK_ENTRY(CODA_ACCESS_STATS);

    /* Check for access of control object.  Only read access is
       allowed on it. */
    if (IS_CTL_VP(vp)) {
	/* bogus hack - all will be marked as successes */
	MARK_INT_SAT(CODA_ACCESS_STATS);
	return(((mode & VREAD) && !(mode & (VWRITE | VEXEC)))
	       ? 0 : EACCES);
    }

    /*
     * if the file is a directory, and we are checking exec (eg lookup)
     * access, and the file is in the namecache, then the user must have
     * lookup access to it.
     */
    if (coda_access_cache) {
	if ((vp->v_type == VDIR) && (mode & VEXEC)) {
	    if (coda_nc_lookup(cp, ".", 1, cred)) {
		MARK_INT_SAT(CODA_ACCESS_STATS);
		return(0);                     /* it was in the cache */
	    }
	}
    }

    error = venus_access(vtomi(vp), &cp->c_fid, mode, cred, curlwp);

    return(error);
}

/*
 * CODA abort op, called after namei() when a CREATE/DELETE isn't actually
 * done. If a buffer has been saved in anticipation of a coda_create or
 * a coda_remove, delete it.
 */
/* ARGSUSED */
int
coda_abortop(void *v)
{
/* true args */
    struct vop_abortop_args /* {
	vnode_t *a_dvp;
	struct componentname *a_cnp;
    } */ *ap = v;

    (void)ap;
/* upcall decl */
/* locals */

    return (0);
}

int
coda_readlink(void *v)
{
/* true args */
    struct vop_readlink_args *ap = v;
    vnode_t *vp = ap->a_vp;
    struct cnode *cp = VTOC(vp);
    struct uio *uiop = ap->a_uio;
    kauth_cred_t cred = ap->a_cred;
/* locals */
    struct lwp *l = curlwp;
    int error;
    char *str;
    int len;

    MARK_ENTRY(CODA_READLINK_STATS);

    /* Check for readlink of control object. */
    if (IS_CTL_VP(vp)) {
	MARK_INT_FAIL(CODA_READLINK_STATS);
	return(ENOENT);
    }

    if ((coda_symlink_cache) && (VALID_SYMLINK(cp))) { /* symlink was cached */
	uiop->uio_rw = UIO_READ;
	error = uiomove(cp->c_symlink, (int)cp->c_symlen, uiop);
	if (error)
	    MARK_INT_FAIL(CODA_READLINK_STATS);
	else
	    MARK_INT_SAT(CODA_READLINK_STATS);
	return(error);
    }

    error = venus_readlink(vtomi(vp), &cp->c_fid, cred, l, &str, &len);

    if (!error) {
	uiop->uio_rw = UIO_READ;
	error = uiomove(str, len, uiop);

	if (coda_symlink_cache) {
	    cp->c_symlink = str;
	    cp->c_symlen = len;
	    cp->c_flags |= C_SYMLINK;
	} else
	    CODA_FREE(str, len);
    }

    CODADEBUG(CODA_READLINK, myprintf(("in readlink result %d\n",error));)
    return(error);
}

int
coda_fsync(void *v)
{
/* true args */
    struct vop_fsync_args *ap = v;
    vnode_t *vp = ap->a_vp;
    struct cnode *cp = VTOC(vp);
    kauth_cred_t cred = ap->a_cred;
/* locals */
    vnode_t *convp = cp->c_ovp;
    int error;

    MARK_ENTRY(CODA_FSYNC_STATS);

    /* Check for fsync on an unmounting object */
    /* The NetBSD kernel, in it's infinite wisdom, can try to fsync
     * after an unmount has been initiated.  This is a Bad Thing,
     * which we have to avoid.  Not a legitimate failure for stats.
     */
    if (IS_UNMOUNTING(cp)) {
	return(ENODEV);
    }

    /* Check for fsync of control object. */
    if (IS_CTL_VP(vp)) {
	MARK_INT_SAT(CODA_FSYNC_STATS);
	return(0);
    }

    if (convp)
    	VOP_FSYNC(convp, cred, MNT_WAIT, 0, 0);

    /*
     * We can expect fsync on any vnode at all if venus is pruging it.
     * Venus can't very well answer the fsync request, now can it?
     * Hopefully, it won't have to, because hopefully, venus preserves
     * the (possibly untrue) invariant that it never purges an open
     * vnode.  Hopefully.
     */
    if (cp->c_flags & C_PURGING) {
	return(0);
    }

    error = venus_fsync(vtomi(vp), &cp->c_fid, cred, curlwp);

    CODADEBUG(CODA_FSYNC, myprintf(("in fsync result %d\n",error)); )
    return(error);
}

/*
 * vp is locked on entry, and we must unlock it.
 * XXX This routine is suspect and probably needs rewriting.
 */
int
coda_inactive(void *v)
{
/* true args */
    struct vop_inactive_args *ap = v;
    vnode_t *vp = ap->a_vp;
    struct cnode *cp = VTOC(vp);
    kauth_cred_t cred __unused = NULL;

    /* We don't need to send inactive to venus - DCS */
    MARK_ENTRY(CODA_INACTIVE_STATS);

    if (IS_CTL_VP(vp)) {
	MARK_INT_SAT(CODA_INACTIVE_STATS);
	return 0;
    }

    CODADEBUG(CODA_INACTIVE, myprintf(("in inactive, %s, vfsp %p\n",
				  coda_f2s(&cp->c_fid), vp->v_mount));)

    /* If an array has been allocated to hold the symlink, deallocate it */
    if ((coda_symlink_cache) && (VALID_SYMLINK(cp))) {
	if (cp->c_symlink == NULL)
	    panic("%s: null symlink pointer in cnode", __func__);

	CODA_FREE(cp->c_symlink, cp->c_symlen);
	cp->c_flags &= ~C_SYMLINK;
	cp->c_symlen = 0;
    }

    /* Remove it from the table so it can't be found. */
    coda_unsave(cp);
    if (vp->v_mount->mnt_data == NULL) {
	myprintf(("Help! vfsp->vfs_data was NULL, but vnode %p wasn't dying\n", vp));
	panic("badness in coda_inactive");
    }

#ifdef CODA_VERBOSE
    /* Sanity checks that perhaps should be panic. */
    if (vp->v_usecount > 1)
	printf("%s: %p usecount %d\n", __func__, vp, vp->v_usecount);
    if (cp->c_ovp != NULL)
	printf("%s: %p ovp != NULL\n", __func__, vp);
#endif
    /* XXX Do we need to VOP_CLOSE container vnodes? */
    VOP_UNLOCK(vp);
    if (!IS_UNMOUNTING(cp))
	*ap->a_recycle = true;

    MARK_INT_SAT(CODA_INACTIVE_STATS);
    return(0);
}

/*
 * Coda does not use the normal namecache, but a private version.
 * Consider how to use the standard facility instead.
 */
int
coda_lookup(void *v)
{
/* true args */
    struct vop_lookup_v2_args *ap = v;
    /* (locked) vnode of dir in which to do lookup */
    vnode_t *dvp = ap->a_dvp;
    struct cnode *dcp = VTOC(dvp);
    /* output variable for result */
    vnode_t **vpp = ap->a_vpp;
    /* name to lookup */
    struct componentname *cnp = ap->a_cnp;
    kauth_cred_t cred = cnp->cn_cred;
    struct lwp *l = curlwp;
/* locals */
    struct cnode *cp;
    const char *nm = cnp->cn_nameptr;
    int len = cnp->cn_namelen;
    CodaFid VFid;
    int	vtype;
    int error = 0;

    MARK_ENTRY(CODA_LOOKUP_STATS);

    CODADEBUG(CODA_LOOKUP, myprintf(("%s: %s in %s\n", __func__,
	nm, coda_f2s(&dcp->c_fid)));)

    /*
     * XXX componentname flags in MODMASK are not handled at all
     */

    /*
     * The overall strategy is to switch on the lookup type and get a
     * result vnode that is vref'd but not locked.
     */

    /* Check for lookup of control object. */
    if (IS_CTL_NAME(dvp, nm, len)) {
	*vpp = coda_ctlvp;
	vref(*vpp);
	MARK_INT_SAT(CODA_LOOKUP_STATS);
	goto exit;
    }

    /* Avoid trying to hand venus an unreasonably long name. */
    if (len+1 > CODA_MAXNAMLEN) {
	MARK_INT_FAIL(CODA_LOOKUP_STATS);
	CODADEBUG(CODA_LOOKUP, myprintf(("%s: name too long:, %s (%s)\n",
	    __func__, coda_f2s(&dcp->c_fid), nm));)
	*vpp = (vnode_t *)0;
	error = EINVAL;
	goto exit;
    }

    /*
     * Try to resolve the lookup in the minicache.  If that fails, ask
     * venus to do the lookup.  XXX The interaction between vnode
     * locking and any locking that coda does is not clear.
     */
    cp = coda_nc_lookup(dcp, nm, len, cred);
    if (cp) {
	*vpp = CTOV(cp);
	vref(*vpp);
	CODADEBUG(CODA_LOOKUP,
		 myprintf(("lookup result %d vpp %p\n",error,*vpp));)
    } else {
	/* The name wasn't cached, so ask Venus. */
	error = venus_lookup(vtomi(dvp), &dcp->c_fid, nm, len, cred, l, &VFid,
	    &vtype);

	if (error) {
	    MARK_INT_FAIL(CODA_LOOKUP_STATS);
	    CODADEBUG(CODA_LOOKUP, myprintf(("%s: lookup error on %s (%s)%d\n",
		__func__, coda_f2s(&dcp->c_fid), nm, error));)
	    *vpp = (vnode_t *)0;
	} else {
	    MARK_INT_SAT(CODA_LOOKUP_STATS);
	    CODADEBUG(CODA_LOOKUP, myprintf(("%s: %s type %o result %d\n",
		__func__, coda_f2s(&VFid), vtype, error)); )

	    cp = make_coda_node(&VFid, dvp->v_mount, vtype);
	    *vpp = CTOV(cp);
	    /* vpp is now vrefed. */

	    /*
	     * Unless this vnode is marked CODA_NOCACHE, enter it into
	     * the coda name cache to avoid a future venus round-trip.
	     * XXX Interaction with componentname NOCACHE is unclear.
	     */
	    if (!(vtype & CODA_NOCACHE))
		coda_nc_enter(VTOC(dvp), nm, len, cred, VTOC(*vpp));
	}
    }

 exit:
    /*
     * If we are creating, and this was the last name to be looked up,
     * and the error was ENOENT, then make the leaf NULL and return
     * success.
     * XXX Check against new lookup rules.
     */
    if (((cnp->cn_nameiop == CREATE) || (cnp->cn_nameiop == RENAME))
	&& (cnp->cn_flags & ISLASTCN)
	&& (error == ENOENT))
    {
	error = EJUSTRETURN;
	*ap->a_vpp = NULL;
    }

    return(error);
}

/*ARGSUSED*/
int
coda_create(void *v)
{
/* true args */
    struct vop_create_v3_args *ap = v;
    vnode_t *dvp = ap->a_dvp;
    struct cnode *dcp = VTOC(dvp);
    struct vattr *va = ap->a_vap;
    int exclusive = 1;
    int mode = ap->a_vap->va_mode;
    vnode_t **vpp = ap->a_vpp;
    struct componentname  *cnp = ap->a_cnp;
    kauth_cred_t cred = cnp->cn_cred;
    struct lwp *l = curlwp;
/* locals */
    int error;
    struct cnode *cp;
    const char *nm = cnp->cn_nameptr;
    int len = cnp->cn_namelen;
    CodaFid VFid;
    struct vattr attr;

    MARK_ENTRY(CODA_CREATE_STATS);

    /* All creates are exclusive XXX */
    /* I'm assuming the 'mode' argument is the file mode bits XXX */

    /* Check for create of control object. */
    if (IS_CTL_NAME(dvp, nm, len)) {
	*vpp = (vnode_t *)0;
	MARK_INT_FAIL(CODA_CREATE_STATS);
	return(EACCES);
    }

    error = venus_create(vtomi(dvp), &dcp->c_fid, nm, len, exclusive, mode, va, cred, l, &VFid, &attr);

    if (!error) {

        /*
	 * XXX Violation of venus/kernel invariants is a difficult case,
	 * but venus should not be able to cause a panic.
	 */
	/* If this is an exclusive create, panic if the file already exists. */
	/* Venus should have detected the file and reported EEXIST. */

	if ((exclusive == 1) &&
	    (coda_find(&VFid) != NULL))
	    panic("cnode existed for newly created file!");

	cp = make_coda_node(&VFid, dvp->v_mount, attr.va_type);
	*vpp = CTOV(cp);

	/* XXX vnodeops doesn't say this argument can be changed. */
	/* Update va to reflect the new attributes. */
	(*va) = attr;

	/* Update the attribute cache and mark it as valid */
	if (coda_attr_cache) {
	    VTOC(*vpp)->c_vattr = attr;
	    VTOC(*vpp)->c_flags |= C_VATTR;
	}

	/* Invalidate parent's attr cache (modification time has changed). */
	VTOC(dvp)->c_flags &= ~C_VATTR;

	/* enter the new vnode in the Name Cache */
	coda_nc_enter(VTOC(dvp), nm, len, cred, VTOC(*vpp));

	CODADEBUG(CODA_CREATE, myprintf(("%s: %s, result %d\n", __func__,
	    coda_f2s(&VFid), error)); )
    } else {
	*vpp = (vnode_t *)0;
	CODADEBUG(CODA_CREATE, myprintf(("%s: create error %d\n", __func__,
	    error));)
    }

    if (!error) {
#ifdef CODA_VERBOSE
	if ((cnp->cn_flags & LOCKLEAF) == 0)
	    /* This should not happen; flags are for lookup only. */
	    printf("%s: LOCKLEAF not set!\n", __func__);
#endif
    }

    return(error);
}

int
coda_remove(void *v)
{
/* true args */
    struct vop_remove_args *ap = v;
    vnode_t *dvp = ap->a_dvp;
    struct cnode *cp = VTOC(dvp);
    vnode_t *vp = ap->a_vp;
    struct componentname  *cnp = ap->a_cnp;
    kauth_cred_t cred = cnp->cn_cred;
    struct lwp *l = curlwp;
/* locals */
    int error;
    const char *nm = cnp->cn_nameptr;
    int len = cnp->cn_namelen;
    struct cnode *tp;

    MARK_ENTRY(CODA_REMOVE_STATS);

    CODADEBUG(CODA_REMOVE, myprintf(("%s: %s in %s\n", __func__,
	nm, coda_f2s(&cp->c_fid)));)

    /* Remove the file's entry from the CODA Name Cache */
    /* We're being conservative here, it might be that this person
     * doesn't really have sufficient access to delete the file
     * but we feel zapping the entry won't really hurt anyone -- dcs
     */
    /* I'm gonna go out on a limb here. If a file and a hardlink to it
     * exist, and one is removed, the link count on the other will be
     * off by 1. We could either invalidate the attrs if cached, or
     * fix them. I'll try to fix them. DCS 11/8/94
     */
    tp = coda_nc_lookup(VTOC(dvp), nm, len, cred);
    if (tp) {
	if (VALID_VATTR(tp)) {	/* If attrs are cached */
	    if (tp->c_vattr.va_nlink > 1) {	/* If it's a hard link */
		tp->c_vattr.va_nlink--;
	    }
	}

	coda_nc_zapfile(VTOC(dvp), nm, len);
	/* No need to flush it if it doesn't exist! */
    }
    /* Invalidate the parent's attr cache, the modification time has changed */
    VTOC(dvp)->c_flags &= ~C_VATTR;

    /* Check for remove of control object. */
    if (IS_CTL_NAME(dvp, nm, len)) {
	MARK_INT_FAIL(CODA_REMOVE_STATS);
	return(ENOENT);
    }

    error = venus_remove(vtomi(dvp), &cp->c_fid, nm, len, cred, l);

    CODADEBUG(CODA_REMOVE, myprintf(("in remove result %d\n",error)); )

    /*
     * Unlock parent and child (avoiding double if ".").
     */
    if (dvp == vp) {
	vrele(vp);
    } else {
	vput(vp);
    }
    vput(dvp);

    return(error);
}

/*
 * dvp is the directory where the link is to go, and is locked.
 * vp is the object to be linked to, and is unlocked.
 * At exit, we must unlock dvp, and vput dvp.
 */
int
coda_link(void *v)
{
/* true args */
    struct vop_link_args *ap = v;
    vnode_t *vp = ap->a_vp;
    struct cnode *cp = VTOC(vp);
    vnode_t *dvp = ap->a_dvp;
    struct cnode *dcp = VTOC(dvp);
    struct componentname *cnp = ap->a_cnp;
    kauth_cred_t cred = cnp->cn_cred;
    struct lwp *l = curlwp;
/* locals */
    int error;
    const char *nm = cnp->cn_nameptr;
    int len = cnp->cn_namelen;

    MARK_ENTRY(CODA_LINK_STATS);

    if (codadebug & CODADBGMSK(CODA_LINK)) {

	myprintf(("%s: vp fid: %s\n", __func__, coda_f2s(&cp->c_fid)));
	myprintf(("%s: dvp fid: %s)\n", __func__, coda_f2s(&dcp->c_fid)));

    }
    if (codadebug & CODADBGMSK(CODA_LINK)) {
	myprintf(("%s: vp fid: %s\n", __func__, coda_f2s(&cp->c_fid)));
	myprintf(("%s: dvp fid: %s\n", __func__, coda_f2s(&dcp->c_fid)));

    }

    /* Check for link to/from control object. */
    if (IS_CTL_NAME(dvp, nm, len) || IS_CTL_VP(vp)) {
	MARK_INT_FAIL(CODA_LINK_STATS);
	return(EACCES);
    }

    /* If linking . to a name, error out earlier. */
    if (vp == dvp) {
#ifdef CODA_VERBOSE
        printf("%s coda_link vp==dvp\n", __func__);
#endif
	error = EISDIR;
	goto exit;
    }

    /* XXX Why does venus_link need the vnode to be locked?*/
    if ((error = vn_lock(vp, LK_EXCLUSIVE)) != 0) {
#ifdef CODA_VERBOSE
	printf("%s: couldn't lock vnode %p\n", __func__, vp);
#endif
	error = EFAULT;		/* XXX better value */
	goto exit;
    }
    error = venus_link(vtomi(vp), &cp->c_fid, &dcp->c_fid, nm, len, cred, l);
    VOP_UNLOCK(vp);

    /* Invalidate parent's attr cache (the modification time has changed). */
    VTOC(dvp)->c_flags &= ~C_VATTR;
    /* Invalidate child's attr cache (XXX why). */
    VTOC(vp)->c_flags &= ~C_VATTR;

    CODADEBUG(CODA_LINK,	myprintf(("in link result %d\n",error)); )

exit:
    vput(dvp);
    return(error);
}

int
coda_rename(void *v)
{
/* true args */
    struct vop_rename_args *ap = v;
    vnode_t *odvp = ap->a_fdvp;
    struct cnode *odcp = VTOC(odvp);
    struct componentname  *fcnp = ap->a_fcnp;
    vnode_t *ndvp = ap->a_tdvp;
    struct cnode *ndcp = VTOC(ndvp);
    struct componentname  *tcnp = ap->a_tcnp;
    kauth_cred_t cred = fcnp->cn_cred;
    struct lwp *l = curlwp;
/* true args */
    int error;
    const char *fnm = fcnp->cn_nameptr;
    int flen = fcnp->cn_namelen;
    const char *tnm = tcnp->cn_nameptr;
    int tlen = tcnp->cn_namelen;

    MARK_ENTRY(CODA_RENAME_STATS);

    /* Hmmm.  The vnodes are already looked up.  Perhaps they are locked?
       This could be Bad. XXX */
#ifdef OLD_DIAGNOSTIC
    if ((fcnp->cn_cred != tcnp->cn_cred)
	|| (fcnp->cn_lwp != tcnp->cn_lwp))
    {
	panic("%s: component names don't agree", __func__);
    }
#endif

    /* Check for rename involving control object. */
    if (IS_CTL_NAME(odvp, fnm, flen) || IS_CTL_NAME(ndvp, tnm, tlen)) {
	MARK_INT_FAIL(CODA_RENAME_STATS);
	return(EACCES);
    }

    /* Problem with moving directories -- need to flush entry for .. */
    if (odvp != ndvp) {
	struct cnode *ovcp = coda_nc_lookup(VTOC(odvp), fnm, flen, cred);
	if (ovcp) {
	    vnode_t *ovp = CTOV(ovcp);
	    if ((ovp) &&
		(ovp->v_type == VDIR)) /* If it's a directory */
		coda_nc_zapfile(VTOC(ovp),"..", 2);
	}
    }

    /* Remove the entries for both source and target files */
    coda_nc_zapfile(VTOC(odvp), fnm, flen);
    coda_nc_zapfile(VTOC(ndvp), tnm, tlen);

    /* Invalidate the parent's attr cache, the modification time has changed */
    VTOC(odvp)->c_flags &= ~C_VATTR;
    VTOC(ndvp)->c_flags &= ~C_VATTR;

    if (flen+1 > CODA_MAXNAMLEN) {
	MARK_INT_FAIL(CODA_RENAME_STATS);
	error = EINVAL;
	goto exit;
    }

    if (tlen+1 > CODA_MAXNAMLEN) {
	MARK_INT_FAIL(CODA_RENAME_STATS);
	error = EINVAL;
	goto exit;
    }

    error = venus_rename(vtomi(odvp), &odcp->c_fid, &ndcp->c_fid, fnm, flen, tnm, tlen, cred, l);

 exit:
    CODADEBUG(CODA_RENAME, myprintf(("in rename result %d\n",error));)
    /* XXX - do we need to call cache pureg on the moved vnode? */
    cache_purge(ap->a_fvp);

    /* It seems to be incumbent on us to drop locks on all four vnodes */
    /* From-vnodes are not locked, only ref'd.  To-vnodes are locked. */

    vrele(ap->a_fvp);
    vrele(odvp);

    if (ap->a_tvp) {
	if (ap->a_tvp == ndvp) {
	    vrele(ap->a_tvp);
	} else {
	    vput(ap->a_tvp);
	}
    }

    vput(ndvp);
    return(error);
}

int
coda_mkdir(void *v)
{
/* true args */
    struct vop_mkdir_v3_args *ap = v;
    vnode_t *dvp = ap->a_dvp;
    struct cnode *dcp = VTOC(dvp);
    struct componentname  *cnp = ap->a_cnp;
    struct vattr *va = ap->a_vap;
    vnode_t **vpp = ap->a_vpp;
    kauth_cred_t cred = cnp->cn_cred;
    struct lwp *l = curlwp;
/* locals */
    int error;
    const char *nm = cnp->cn_nameptr;
    int len = cnp->cn_namelen;
    struct cnode *cp;
    CodaFid VFid;
    struct vattr ova;

    MARK_ENTRY(CODA_MKDIR_STATS);

    /* Check for mkdir of target object. */
    if (IS_CTL_NAME(dvp, nm, len)) {
	*vpp = (vnode_t *)0;
	MARK_INT_FAIL(CODA_MKDIR_STATS);
	return(EACCES);
    }

    if (len+1 > CODA_MAXNAMLEN) {
	*vpp = (vnode_t *)0;
	MARK_INT_FAIL(CODA_MKDIR_STATS);
	return(EACCES);
    }

    error = venus_mkdir(vtomi(dvp), &dcp->c_fid, nm, len, va, cred, l, &VFid, &ova);

    if (!error) {
	if (coda_find(&VFid) != NULL)
	    panic("cnode existed for newly created directory!");


	cp =  make_coda_node(&VFid, dvp->v_mount, va->va_type);
	*vpp = CTOV(cp);

	/* enter the new vnode in the Name Cache */
	coda_nc_enter(VTOC(dvp), nm, len, cred, VTOC(*vpp));

	/* as a side effect, enter "." and ".." for the directory */
	coda_nc_enter(VTOC(*vpp), ".", 1, cred, VTOC(*vpp));
	coda_nc_enter(VTOC(*vpp), "..", 2, cred, VTOC(dvp));

	if (coda_attr_cache) {
	    VTOC(*vpp)->c_vattr = ova;		/* update the attr cache */
	    VTOC(*vpp)->c_flags |= C_VATTR;	/* Valid attributes in cnode */
	}

	/* Invalidate the parent's attr cache, the modification time has changed */
	VTOC(dvp)->c_flags &= ~C_VATTR;

	CODADEBUG( CODA_MKDIR, myprintf(("%s: %s result %d\n", __func__,
	    coda_f2s(&VFid), error)); )
    } else {
	*vpp = (vnode_t *)0;
	CODADEBUG(CODA_MKDIR, myprintf(("%s error %d\n", __func__, error));)
    }

    return(error);
}

int
coda_rmdir(void *v)
{
/* true args */
    struct vop_rmdir_args *ap = v;
    vnode_t *dvp = ap->a_dvp;
    struct cnode *dcp = VTOC(dvp);
    vnode_t *vp = ap->a_vp;
    struct componentname  *cnp = ap->a_cnp;
    kauth_cred_t cred = cnp->cn_cred;
    struct lwp *l = curlwp;
/* true args */
    int error;
    const char *nm = cnp->cn_nameptr;
    int len = cnp->cn_namelen;
    struct cnode *cp;

    MARK_ENTRY(CODA_RMDIR_STATS);

    /* Check for rmdir of control object. */
    if (IS_CTL_NAME(dvp, nm, len)) {
	MARK_INT_FAIL(CODA_RMDIR_STATS);
	return(ENOENT);
    }

    /* Can't remove . in self. */
    if (dvp == vp) {
#ifdef CODA_VERBOSE
	printf("%s: dvp == vp\n", __func__);
#endif
	error = EINVAL;
	goto exit;
    }

    /*
     * The caller may not have adequate permissions, and the venus
     * operation may fail, but it doesn't hurt from a correctness
     * viewpoint to invalidate cache entries.
     * XXX Why isn't this done after the venus_rmdir call?
     */
    /* Look up child in name cache (by name, from parent). */
    cp = coda_nc_lookup(dcp, nm, len, cred);
    /* If found, remove all children of the child (., ..). */
    if (cp) coda_nc_zapParentfid(&(cp->c_fid), NOT_DOWNCALL);

    /* Remove child's own entry. */
    coda_nc_zapfile(dcp, nm, len);

    /* Invalidate parent's attr cache (the modification time has changed). */
    dcp->c_flags &= ~C_VATTR;

    error = venus_rmdir(vtomi(dvp), &dcp->c_fid, nm, len, cred, l);

    CODADEBUG(CODA_RMDIR, myprintf(("in rmdir result %d\n", error)); )

exit:
    /* vput both vnodes */
    vput(dvp);
    if (dvp == vp) {
	vrele(vp);
    } else {
	vput(vp);
    }

    return(error);
}

int
coda_symlink(void *v)
{
/* true args */
    struct vop_symlink_v3_args *ap = v;
    vnode_t *dvp = ap->a_dvp;
    struct cnode *dcp = VTOC(dvp);
    /* a_vpp is used in place below */
    struct componentname *cnp = ap->a_cnp;
    struct vattr *tva = ap->a_vap;
    char *path = ap->a_target;
    kauth_cred_t cred = cnp->cn_cred;
    struct lwp *l = curlwp;
/* locals */
    int error;
    u_long saved_cn_flags;
    const char *nm = cnp->cn_nameptr;
    int len = cnp->cn_namelen;
    int plen = strlen(path);

    /*
     * Here's the strategy for the moment: perform the symlink, then
     * do a lookup to grab the resulting vnode.  I know this requires
     * two communications with Venus for a new sybolic link, but
     * that's the way the ball bounces.  I don't yet want to change
     * the way the Mach symlink works.  When Mach support is
     * deprecated, we should change symlink so that the common case
     * returns the resultant vnode in a vpp argument.
     */

    MARK_ENTRY(CODA_SYMLINK_STATS);

    /* Check for symlink of control object. */
    if (IS_CTL_NAME(dvp, nm, len)) {
	MARK_INT_FAIL(CODA_SYMLINK_STATS);
	error = EACCES; 
	goto exit;
    }

    if (plen+1 > CODA_MAXPATHLEN) {
	MARK_INT_FAIL(CODA_SYMLINK_STATS);
	error = EINVAL;
	goto exit;
    }

    if (len+1 > CODA_MAXNAMLEN) {
	MARK_INT_FAIL(CODA_SYMLINK_STATS);
	error = EINVAL;
	goto exit;
    }

    error = venus_symlink(vtomi(dvp), &dcp->c_fid, path, plen, nm, len, tva, cred, l);

    /* Invalidate the parent's attr cache (modification time has changed). */
    dcp->c_flags &= ~C_VATTR;

    if (!error) {
	/*
	 * VOP_SYMLINK is not defined to pay attention to cnp->cn_flags;
	 * these are defined only for VOP_LOOKUP.   We desire to reuse
	 * cnp for a VOP_LOOKUP operation, and must be sure to not pass
	 * stray flags passed to us.  Such stray flags can occur because
	 * sys_symlink makes a namei call and then reuses the
	 * componentname structure.
	 */
	/*
	 * XXX Arguably we should create our own componentname structure
	 * and not reuse the one that was passed in.
	 */
	saved_cn_flags = cnp->cn_flags;
	cnp->cn_flags &= ~(MODMASK | OPMASK);
	cnp->cn_flags |= LOOKUP;
	error = VOP_LOOKUP(dvp, ap->a_vpp, cnp);
	cnp->cn_flags = saved_cn_flags;
    }

 exit:
    CODADEBUG(CODA_SYMLINK, myprintf(("in symlink result %d\n",error)); )
    return(error);
}

/*
 * Read directory entries.
 */
int
coda_readdir(void *v)
{
/* true args */
    struct vop_readdir_args *ap = v;
    vnode_t *vp = ap->a_vp;
    struct cnode *cp = VTOC(vp);
    struct uio *uiop = ap->a_uio;
    kauth_cred_t cred = ap->a_cred;
    int *eofflag = ap->a_eofflag;
    off_t **cookies = ap->a_cookies;
    int *ncookies = ap->a_ncookies;
/* upcall decl */
/* locals */
    int error = 0;

    MARK_ENTRY(CODA_READDIR_STATS);

    CODADEBUG(CODA_READDIR, myprintf(("%s: (%p, %lu, %lld)\n", __func__,
	uiop->uio_iov->iov_base, (unsigned long) uiop->uio_resid,
	(long long) uiop->uio_offset)); )

    /* Check for readdir of control object. */
    if (IS_CTL_VP(vp)) {
	MARK_INT_FAIL(CODA_READDIR_STATS);
	return(ENOENT);
    }

    {
	/* Redirect the request to UFS. */

	/* If directory is not already open do an "internal open" on it. */
	int opened_internally = 0;
	if (cp->c_ovp == NULL) {
	    opened_internally = 1;
	    MARK_INT_GEN(CODA_OPEN_STATS);
	    error = VOP_OPEN(vp, FREAD, cred);
#ifdef	CODA_VERBOSE
	    printf("%s: Internally Opening %p\n", __func__, vp);
#endif
	    if (error) return(error);
	} else
	    vp = cp->c_ovp;

	/* Have UFS handle the call. */
	CODADEBUG(CODA_READDIR, myprintf(("%s: fid = %s, refcnt = %d\n",
	    __func__, coda_f2s(&cp->c_fid), vp->v_usecount)); )
	error = VOP_READDIR(vp, uiop, cred, eofflag, cookies, ncookies);
	if (error)
	    MARK_INT_FAIL(CODA_READDIR_STATS);
	else
	    MARK_INT_SAT(CODA_READDIR_STATS);

	/* Do an "internal close" if necessary. */
	if (opened_internally) {
	    MARK_INT_GEN(CODA_CLOSE_STATS);
	    (void)VOP_CLOSE(vp, FREAD, cred);
	}
    }

    return(error);
}

/*
 * Convert from file system blocks to device blocks
 */
int
coda_bmap(void *v)
{
    /* XXX on the global proc */
/* true args */
    struct vop_bmap_args *ap = v;
    vnode_t *vp __unused = ap->a_vp;	/* file's vnode */
    daddr_t bn __unused = ap->a_bn;	/* fs block number */
    vnode_t **vpp = ap->a_vpp;			/* RETURN vp of device */
    daddr_t *bnp __unused = ap->a_bnp;	/* RETURN device block number */
    struct lwp *l __unused = curlwp;
/* upcall decl */
/* locals */

	*vpp = (vnode_t *)0;
	myprintf(("coda_bmap called!\n"));
	return(EINVAL);
}

/*
 * I don't think the following two things are used anywhere, so I've
 * commented them out
 *
 * struct buf *async_bufhead;
 * int async_daemon_count;
 */
int
coda_strategy(void *v)
{
/* true args */
    struct vop_strategy_args *ap = v;
    struct buf *bp __unused = ap->a_bp;
    struct lwp *l __unused = curlwp;
/* upcall decl */
/* locals */

	myprintf(("coda_strategy called!  "));
	return(EINVAL);
}

int
coda_reclaim(void *v)
{
/* true args */
    struct vop_reclaim_args *ap = v;
    vnode_t *vp = ap->a_vp;
    struct cnode *cp = VTOC(vp);
/* upcall decl */
/* locals */

/*
 * Forced unmount/flush will let vnodes with non zero use be destroyed!
 */
    ENTRY;

    if (IS_UNMOUNTING(cp)) {
#ifdef	DEBUG
	if (VTOC(vp)->c_ovp) {
	    if (IS_UNMOUNTING(cp))
		printf("%s: c_ovp not void: vp %p, cp %p\n", __func__, vp, cp);
	}
#endif
    } else {
#ifdef OLD_DIAGNOSTIC
	if (vp->v_usecount != 0)
	    print("%s: pushing active %p\n", __func__, vp);
	if (VTOC(vp)->c_ovp) {
	    panic("%s: c_ovp not void", __func__);
	}
#endif
    }
    coda_free(VTOC(vp));
    SET_VTOC(vp) = NULL;
    return (0);
}

int
coda_lock(void *v)
{
/* true args */
    struct vop_lock_args *ap = v;
    vnode_t *vp = ap->a_vp;
    struct cnode *cp = VTOC(vp);
/* upcall decl */
/* locals */

    ENTRY;

    if (coda_lockdebug) {
	myprintf(("Attempting lock on %s\n",
		  coda_f2s(&cp->c_fid)));
    }

    return genfs_lock(v);
}

int
coda_unlock(void *v)
{
/* true args */
    struct vop_unlock_args *ap = v;
    vnode_t *vp = ap->a_vp;
    struct cnode *cp = VTOC(vp);
/* upcall decl */
/* locals */

    ENTRY;
    if (coda_lockdebug) {
	myprintf(("Attempting unlock on %s\n",
		  coda_f2s(&cp->c_fid)));
    }

    return genfs_unlock(v);
}

int
coda_islocked(void *v)
{
/* true args */
    ENTRY;

    return genfs_islocked(v);
}

/*
 * Given a device and inode, obtain a locked vnode.  One reference is
 * obtained and passed back to the caller.
 */
int
coda_grab_vnode(vnode_t *uvp, dev_t dev, ino_t ino, vnode_t **vpp)
{
    int           error;
    struct mount *mp;

    /* Obtain mount point structure from device. */
    if (!(mp = devtomp(dev))) {
	myprintf(("%s: devtomp(0x%llx) returns NULL\n", __func__,
	    (unsigned long long)dev));
	return(ENXIO);
    }

    /*
     * Obtain vnode from mount point and inode.
     */
    error = VFS_VGET(mp, ino, vpp);
    if (error) {
	myprintf(("%s: iget/vget(0x%llx, %llu) returns %p, err %d\n", __func__,
	    (unsigned long long)dev, (unsigned long long)ino, *vpp, error));
	return(ENOENT);
    }
    /* share the underlying vnode lock with the coda vnode */
    mutex_obj_hold((*vpp)->v_interlock);
    uvm_obj_setlock(&uvp->v_uobj, (*vpp)->v_interlock);
    KASSERT(VOP_ISLOCKED(*vpp));
    return(0);
}

static void
coda_print_vattr(struct vattr *attr)
{
    const char *typestr;

    switch (attr->va_type) {
    case VNON:
	typestr = "VNON";
	break;
    case VREG:
	typestr = "VREG";
	break;
    case VDIR:
	typestr = "VDIR";
	break;
    case VBLK:
	typestr = "VBLK";
	break;
    case VCHR:
	typestr = "VCHR";
	break;
    case VLNK:
	typestr = "VLNK";
	break;
    case VSOCK:
	typestr = "VSCK";
	break;
    case VFIFO:
	typestr = "VFFO";
	break;
    case VBAD:
	typestr = "VBAD";
	break;
    default:
	typestr = "????";
	break;
    }


    myprintf(("attr: type %s mode %d uid %d gid %d fsid %d rdev %d\n",
	      typestr, (int)attr->va_mode, (int)attr->va_uid,
	      (int)attr->va_gid, (int)attr->va_fsid, (int)attr->va_rdev));

    myprintf(("      fileid %d nlink %d size %d blocksize %d bytes %d\n",
	      (int)attr->va_fileid, (int)attr->va_nlink,
	      (int)attr->va_size,
	      (int)attr->va_blocksize,(int)attr->va_bytes));
    myprintf(("      gen %ld flags %ld vaflags %d\n",
	      attr->va_gen, attr->va_flags, attr->va_vaflags));
    myprintf(("      atime sec %d nsec %d\n",
	      (int)attr->va_atime.tv_sec, (int)attr->va_atime.tv_nsec));
    myprintf(("      mtime sec %d nsec %d\n",
	      (int)attr->va_mtime.tv_sec, (int)attr->va_mtime.tv_nsec));
    myprintf(("      ctime sec %d nsec %d\n",
	      (int)attr->va_ctime.tv_sec, (int)attr->va_ctime.tv_nsec));
}

/*
 * Return a vnode for the given fid.
 * If no cnode exists for this fid create one and put it
 * in a table hashed by coda_f2i().  If the cnode for
 * this fid is already in the table return it (ref count is
 * incremented by coda_find.  The cnode will be flushed from the
 * table when coda_inactive calls coda_unsave.
 */
struct cnode *
make_coda_node(CodaFid *fid, struct mount *fvsp, short type)
{
    struct cnode *cp;
    int          error;

    if ((cp = coda_find(fid)) == NULL) {
	vnode_t *vp;

	cp = coda_alloc();
	cp->c_fid = *fid;

	error = getnewvnode(VT_CODA, fvsp, coda_vnodeop_p, NULL, &vp);
	if (error) {
	    panic("%s: getnewvnode returned error %d", __func__, error);
	}
	vp->v_data = cp;
	vp->v_type = type;
	cp->c_vnode = vp;
	uvm_vnp_setsize(vp, 0);
	coda_save(cp);

    } else {
	vref(CTOV(cp));
    }

    return cp;
}

/*
 * coda_getpages may be called on a vnode which has not been opened,
 * e.g. to fault in pages to execute a program.  In that case, we must
 * open the file to get the container.  The vnode may or may not be
 * locked, and we must leave it in the same state.
 */
int
coda_getpages(void *v)
{
	struct vop_getpages_args /* {
		vnode_t *a_vp;
		voff_t a_offset;
		struct vm_page **a_m;
		int *a_count;
		int a_centeridx;
		vm_prot_t a_access_type;
		int a_advice;
		int a_flags;
	} */ *ap = v;
	vnode_t *vp = ap->a_vp, *cvp;
	struct cnode *cp = VTOC(vp);
	struct lwp *l = curlwp;
	kauth_cred_t cred = l->l_cred;
	int error, cerror;
	int waslocked;	       /* 1 if vnode lock was held on entry */
	int didopen = 0;	/* 1 if we opened container file */

	/*
	 * Handle a case that uvm_fault doesn't quite use yet.
	 * See layer_vnops.c. for inspiration.
	 */
	if (ap->a_flags & PGO_LOCKED) {
		return EBUSY;
	}

	KASSERT(mutex_owned(vp->v_interlock));

	/* Check for control object. */
	if (IS_CTL_VP(vp)) {
#ifdef CODA_VERBOSE
		printf("%s: control object %p\n", __func__, vp);
#endif
		return(EINVAL);
	}

	/*
	 * XXX It's really not ok to be releasing the lock we get,
	 * because we could be overlapping with another call to
	 * getpages and drop a lock they are relying on.  We need to
	 * figure out whether getpages ever is called holding the
	 * lock, and if we should serialize getpages calls by some
	 * mechanism.
	 */
	/* XXX VOP_ISLOCKED() may not be used for lock decisions. */
	waslocked = VOP_ISLOCKED(vp);

	/* Get container file if not already present. */
	cvp = cp->c_ovp;
	if (cvp == NULL) {
		/*
		 * VOP_OPEN requires a locked vnode.  We must avoid
		 * locking the vnode if it is already locked, and
		 * leave it in the same state on exit.
		 */
		if (waslocked == 0) {
			mutex_exit(vp->v_interlock);
			cerror = vn_lock(vp, LK_EXCLUSIVE);
			if (cerror) {
#ifdef CODA_VERBOSE
				printf("%s: can't lock vnode %p\n",
				    __func__, vp);
#endif
				return cerror;
			}
#ifdef CODA_VERBOSE
			printf("%s: locked vnode %p\n", __func__, vp);
#endif
		}

		/*
		 * Open file (causes upcall to venus).
		 * XXX Perhaps we should not fully open the file, but
		 * simply obtain a container file.
		 */
		/* XXX Is it ok to do this while holding the mutex? */
		cerror = VOP_OPEN(vp, FREAD, cred);

		if (cerror) {
#ifdef CODA_VERBOSE
			printf("%s: cannot open vnode %p => %d\n", __func__,
			    vp, cerror);
#endif
			if (waslocked == 0)
				VOP_UNLOCK(vp);
			return cerror;
		}

#ifdef CODA_VERBOSE
		printf("%s: opened vnode %p\n", __func__, vp);
#endif
		cvp = cp->c_ovp;
		didopen = 1;
		if (waslocked == 0)
			mutex_enter(vp->v_interlock);
	}
	KASSERT(cvp != NULL);

	/* Munge the arg structure to refer to the container vnode. */
	KASSERT(cvp->v_interlock == vp->v_interlock);
	ap->a_vp = cp->c_ovp;

	/* Finally, call getpages on it. */
	error = VCALL(ap->a_vp, VOFFSET(vop_getpages), ap);

	/* If we opened the vnode, we must close it. */
	if (didopen) {
		/*
		 * VOP_CLOSE requires a locked vnode, but we are still
		 * holding the lock (or riding a caller's lock).
		 */
		cerror = VOP_CLOSE(vp, FREAD, cred);
#ifdef CODA_VERBOSE
		if (cerror != 0)
			/* XXX How should we handle this? */
			printf("%s: closed vnode %p -> %d\n", __func__,
			    vp, cerror);
#endif

		/* If we obtained a lock, drop it. */
		if (waslocked == 0)
			VOP_UNLOCK(vp);
	}

	return error;
}

/*
 * The protocol requires v_interlock to be held by the caller.
 */
int
coda_putpages(void *v)
{
	struct vop_putpages_args /* {
		vnode_t *a_vp;
		voff_t a_offlo;
		voff_t a_offhi;
		int a_flags;
	} */ *ap = v;
	vnode_t *vp = ap->a_vp, *cvp;
	struct cnode *cp = VTOC(vp);
	int error;

	KASSERT(mutex_owned(vp->v_interlock));

	/* Check for control object. */
	if (IS_CTL_VP(vp)) {
		mutex_exit(vp->v_interlock);
#ifdef CODA_VERBOSE
		printf("%s: control object %p\n", __func__, vp);
#endif
		return(EINVAL);
	}

	/*
	 * If container object is not present, then there are no pages
	 * to put; just return without error.  This happens all the
	 * time, apparently during discard of a closed vnode (which
	 * trivially can't have dirty pages).
	 */
	cvp = cp->c_ovp;
	if (cvp == NULL) {
		mutex_exit(vp->v_interlock);
		return 0;
	}

	/* Munge the arg structure to refer to the container vnode. */
	KASSERT(cvp->v_interlock == vp->v_interlock);
	ap->a_vp = cvp;

	/* Finally, call putpages on it. */
	error = VCALL(ap->a_vp, VOFFSET(vop_putpages), ap);

	return error;
}
