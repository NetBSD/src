/*	$NetBSD: coda_vfsops.c,v 1.84 2014/12/13 15:59:30 hannken Exp $	*/

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
 * 	@(#) cfs/coda_vfsops.c,v 1.1.1.1 1998/08/29 21:26:45 rvb Exp $
 */

/*
 * Mach Operating System
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
__KERNEL_RCSID(0, "$NetBSD: coda_vfsops.c,v 1.84 2014/12/13 15:59:30 hannken Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/sysctl.h>
#include <sys/malloc.h>
#include <sys/conf.h>
#include <sys/namei.h>
#include <sys/dirent.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/select.h>
#include <sys/kauth.h>
#include <sys/module.h>

#include <coda/coda.h>
#include <coda/cnode.h>
#include <coda/coda_vfsops.h>
#include <coda/coda_venus.h>
#include <coda/coda_subr.h>
#include <coda/coda_opstats.h>
/* for VN_RDEV */
#include <miscfs/specfs/specdev.h>
#include <miscfs/genfs/genfs.h>
 
MODULE(MODULE_CLASS_VFS, coda, "vcoda");

#define ENTRY if(coda_vfsop_print_entry) myprintf(("Entered %s\n",__func__))

extern struct vnode *coda_ctlvp;
extern struct coda_mntinfo coda_mnttbl[NVCODA]; /* indexed by minor device number */

/* structure to keep statistics of internally generated/satisfied calls */

struct coda_op_stats coda_vfsopstats[CODA_VFSOPS_SIZE];

#define MARK_ENTRY(op) (coda_vfsopstats[op].entries++)
#define MARK_INT_SAT(op) (coda_vfsopstats[op].sat_intrn++)
#define MARK_INT_FAIL(op) (coda_vfsopstats[op].unsat_intrn++)
#define MRAK_INT_GEN(op) (coda_vfsopstats[op].gen_intrn++)

extern const struct cdevsw vcoda_cdevsw;
extern const struct vnodeopv_desc coda_vnodeop_opv_desc;

const struct vnodeopv_desc * const coda_vnodeopv_descs[] = {
	&coda_vnodeop_opv_desc,
	NULL,
};

struct vfsops coda_vfsops = {
	.vfs_name = MOUNT_CODA,
	.vfs_min_mount_data = 256,
			/* This is the pathname, unlike every other fs */
	.vfs_mount = coda_mount,
	.vfs_start = coda_start,
	.vfs_unmount = coda_unmount,
	.vfs_root = coda_root,
	.vfs_quotactl = (void *)eopnotsupp,
	.vfs_statvfs = coda_nb_statvfs,
	.vfs_sync = coda_sync,
	.vfs_vget = coda_vget,
	.vfs_loadvnode = coda_loadvnode,
	.vfs_fhtovp = (void *)eopnotsupp,
	.vfs_vptofh = (void *)eopnotsupp,
	.vfs_init = coda_init,
	.vfs_done = coda_done,
	.vfs_mountroot = (void *)eopnotsupp,
	.vfs_snapshot = (void *)eopnotsupp,
	.vfs_extattrctl = vfs_stdextattrctl,
	.vfs_suspendctl = (void *)eopnotsupp,
	.vfs_renamelock_enter = genfs_renamelock_enter,
	.vfs_renamelock_exit = genfs_renamelock_exit,
	.vfs_fsync = (void *)eopnotsupp,
	.vfs_opv_descs = coda_vnodeopv_descs
};

static int
coda_modcmd(modcmd_t cmd, void *arg)
{

	switch (cmd) {
	case MODULE_CMD_INIT:
		return vfs_attach(&coda_vfsops);
	case MODULE_CMD_FINI:
		return vfs_detach(&coda_vfsops);
	default:
		return ENOTTY;
	}
}

int
coda_vfsopstats_init(void)
{
	int i;

	for (i=0;i<CODA_VFSOPS_SIZE;i++) {
		coda_vfsopstats[i].opcode = i;
		coda_vfsopstats[i].entries = 0;
		coda_vfsopstats[i].sat_intrn = 0;
		coda_vfsopstats[i].unsat_intrn = 0;
		coda_vfsopstats[i].gen_intrn = 0;
	}

	return 0;
}

/*
 * cfs mount vfsop
 * Set up mount info record and attach it to vfs struct.
 */
/*ARGSUSED*/
int
coda_mount(struct mount *vfsp,	/* Allocated and initialized by mount(2) */
    const char *path,	/* path covered: ignored by the fs-layer */
    void *data,		/* Need to define a data type for this in netbsd? */
    size_t *data_len)
{
    struct lwp *l = curlwp;
    struct vnode *dvp;
    struct cnode *cp;
    dev_t dev;
    struct coda_mntinfo *mi;
    struct vnode *rtvp;
    const struct cdevsw *cdev;
    CodaFid rootfid = INVAL_FID;
    CodaFid ctlfid = CTL_FID;
    int error;

    if (data == NULL)
	return EINVAL;
    if (vfsp->mnt_flag & MNT_GETARGS)
	return EINVAL;
    ENTRY;

    coda_vfsopstats_init();
    coda_vnodeopstats_init();

    MARK_ENTRY(CODA_MOUNT_STATS);
    if (CODA_MOUNTED(vfsp)) {
	MARK_INT_FAIL(CODA_MOUNT_STATS);
	return(EBUSY);
    }

    /* Validate mount device.  Similar to getmdev(). */

    /*
     * XXX: coda passes the mount device as the entire mount args,
     * All other fs pass a structure contining a pointer.
     * In order to get sys_mount() to do the copyin() we've set a
     * fixed default size for the filename buffer.
     */
    /* Ensure that namei() doesn't run off the filename buffer */
    ((char *)data)[*data_len - 1] = 0;
    error = namei_simple_kernel((char *)data, NSM_FOLLOW_NOEMULROOT,
		&dvp);

    if (error) {
	MARK_INT_FAIL(CODA_MOUNT_STATS);
	return (error);
    }
    if (dvp->v_type != VCHR) {
	MARK_INT_FAIL(CODA_MOUNT_STATS);
	vrele(dvp);
	return(ENXIO);
    }
    dev = dvp->v_rdev;
    vrele(dvp);
    cdev = cdevsw_lookup(dev);
    if (cdev == NULL) {
	MARK_INT_FAIL(CODA_MOUNT_STATS);
	return(ENXIO);
    }

    /*
     * See if the device table matches our expectations.
     */
    if (cdev != &vcoda_cdevsw)
    {
	MARK_INT_FAIL(CODA_MOUNT_STATS);
	return(ENXIO);
    }

    if (minor(dev) >= NVCODA) {
	MARK_INT_FAIL(CODA_MOUNT_STATS);
	return(ENXIO);
    }

    /*
     * Initialize the mount record and link it to the vfs struct
     */
    mi = &coda_mnttbl[minor(dev)];

    if (!VC_OPEN(&mi->mi_vcomm)) {
	MARK_INT_FAIL(CODA_MOUNT_STATS);
	return(ENODEV);
    }

    /* No initialization (here) of mi_vcomm! */
    vfsp->mnt_data = mi;
    vfsp->mnt_stat.f_fsidx.__fsid_val[0] = 0;
    vfsp->mnt_stat.f_fsidx.__fsid_val[1] = makefstype(MOUNT_CODA);
    vfsp->mnt_stat.f_fsid = vfsp->mnt_stat.f_fsidx.__fsid_val[0];
    vfsp->mnt_stat.f_namemax = CODA_MAXNAMLEN;
    mi->mi_vfsp = vfsp;

    /*
     * Make a root vnode to placate the Vnode interface, but don't
     * actually make the CODA_ROOT call to venus until the first call
     * to coda_root in case a server is down while venus is starting.
     */
    cp = make_coda_node(&rootfid, vfsp, VDIR);
    rtvp = CTOV(cp);
    rtvp->v_vflag |= VV_ROOT;

    cp = make_coda_node(&ctlfid, vfsp, VCHR);

    coda_ctlvp = CTOV(cp);

    /* Add vfs and rootvp to chain of vfs hanging off mntinfo */
    mi->mi_vfsp = vfsp;
    mi->mi_rootvp = rtvp;

    /* set filesystem block size */
    vfsp->mnt_stat.f_bsize = 8192;	    /* XXX -JJK */
    vfsp->mnt_stat.f_frsize = 8192;	    /* XXX -JJK */

    /* error is currently guaranteed to be zero, but in case some
       code changes... */
    CODADEBUG(1,
	     myprintf(("coda_mount returned %d\n",error)););
    if (error)
	MARK_INT_FAIL(CODA_MOUNT_STATS);
    else
	MARK_INT_SAT(CODA_MOUNT_STATS);

    return set_statvfs_info("/coda", UIO_SYSSPACE, "CODA", UIO_SYSSPACE,
	vfsp->mnt_op->vfs_name, vfsp, l);
}

int
coda_start(struct mount *vfsp, int flags)
{
    ENTRY;
    vftomi(vfsp)->mi_started = 1;
    return (0);
}

int
coda_unmount(struct mount *vfsp, int mntflags)
{
    struct coda_mntinfo *mi = vftomi(vfsp);
    int active, error = 0;

    ENTRY;
    MARK_ENTRY(CODA_UMOUNT_STATS);
    if (!CODA_MOUNTED(vfsp)) {
	MARK_INT_FAIL(CODA_UMOUNT_STATS);
	return(EINVAL);
    }

    if (mi->mi_vfsp == vfsp) {	/* We found the victim */
	if (!IS_UNMOUNTING(VTOC(mi->mi_rootvp)))
	    return (EBUSY); 	/* Venus is still running */

#ifdef	DEBUG
	printf("coda_unmount: ROOT: vp %p, cp %p\n", mi->mi_rootvp, VTOC(mi->mi_rootvp));
#endif
	mi->mi_started = 0;

	vrele(mi->mi_rootvp);
	vrele(coda_ctlvp);

	active = coda_kill(vfsp, NOT_DOWNCALL);
	mi->mi_rootvp->v_vflag &= ~VV_ROOT;
	error = vflush(mi->mi_vfsp, NULLVP, FORCECLOSE);
	printf("coda_unmount: active = %d, vflush active %d\n", active, error);
	error = 0;

	/* I'm going to take this out to allow lookups to go through. I'm
	 * not sure it's important anyway. -- DCS 2/2/94
	 */
	/* vfsp->VFS_DATA = NULL; */

	/* No more vfsp's to hold onto */
	mi->mi_vfsp = NULL;
	mi->mi_rootvp = NULL;

	if (error)
	    MARK_INT_FAIL(CODA_UMOUNT_STATS);
	else
	    MARK_INT_SAT(CODA_UMOUNT_STATS);

	return(error);
    }
    return (EINVAL);
}

/*
 * find root of cfs
 */
int
coda_root(struct mount *vfsp, struct vnode **vpp)
{
    struct coda_mntinfo *mi = vftomi(vfsp);
    int error;
    struct lwp *l = curlwp;    /* XXX - bnoble */
    CodaFid VFid;
    static const CodaFid invalfid = INVAL_FID;

    ENTRY;
    MARK_ENTRY(CODA_ROOT_STATS);

    if (vfsp == mi->mi_vfsp) {
    	if (memcmp(&VTOC(mi->mi_rootvp)->c_fid, &invalfid, sizeof(CodaFid)))
	    { /* Found valid root. */
		*vpp = mi->mi_rootvp;
		/* On Mach, this is vref.  On NetBSD, VOP_LOCK */
		vref(*vpp);
		vn_lock(*vpp, LK_EXCLUSIVE);
		MARK_INT_SAT(CODA_ROOT_STATS);
		return(0);
	    }
    }

    error = venus_root(vftomi(vfsp), l->l_cred, l->l_proc, &VFid);

    if (!error) {
	struct cnode *cp = VTOC(mi->mi_rootvp);

	/*
	 * Save the new rootfid in the cnode, and rekey the cnode
	 * with the new fid key.
	 */
	error = vcache_rekey_enter(vfsp, mi->mi_rootvp,
	    &invalfid, sizeof(CodaFid), &VFid, sizeof(CodaFid));
	if (error)
	        goto exit;
	cp->c_fid = VFid;
	vcache_rekey_exit(vfsp, mi->mi_rootvp,
	    &invalfid, sizeof(CodaFid), &cp->c_fid, sizeof(CodaFid));

	*vpp = mi->mi_rootvp;
	vref(*vpp);
	vn_lock(*vpp, LK_EXCLUSIVE);
	MARK_INT_SAT(CODA_ROOT_STATS);
	goto exit;
    } else if (error == ENODEV || error == EINTR) {
	/* Gross hack here! */
	/*
	 * If Venus fails to respond to the CODA_ROOT call, coda_call returns
	 * ENODEV. Return the uninitialized root vnode to allow vfs
	 * operations such as unmount to continue. Without this hack,
	 * there is no way to do an unmount if Venus dies before a
	 * successful CODA_ROOT call is done. All vnode operations
	 * will fail.
	 */
	*vpp = mi->mi_rootvp;
	vref(*vpp);
	vn_lock(*vpp, LK_EXCLUSIVE);
	MARK_INT_FAIL(CODA_ROOT_STATS);
	error = 0;
	goto exit;
    } else {
	CODADEBUG( CODA_ROOT, myprintf(("error %d in CODA_ROOT\n", error)); );
	MARK_INT_FAIL(CODA_ROOT_STATS);

	goto exit;
    }
 exit:
    return(error);
}

/*
 * Get file system statistics.
 */
int
coda_nb_statvfs(struct mount *vfsp, struct statvfs *sbp)
{
    struct lwp *l = curlwp;
    struct coda_statfs fsstat;
    int error;

    ENTRY;
    MARK_ENTRY(CODA_STATFS_STATS);
    if (!CODA_MOUNTED(vfsp)) {
/*	MARK_INT_FAIL(CODA_STATFS_STATS); */
	return(EINVAL);
    }

    /* XXX - what to do about f_flags, others? --bnoble */
    /* Below This is what AFS does
    	#define NB_SFS_SIZ 0x895440
     */
    /* Note: Normal fs's have a bsize of 0x400 == 1024 */

    error = venus_statfs(vftomi(vfsp), l->l_cred, l, &fsstat);

    if (!error) {
	sbp->f_bsize = 8192; /* XXX */
	sbp->f_frsize = 8192; /* XXX */
	sbp->f_iosize = 8192; /* XXX */
	sbp->f_blocks = fsstat.f_blocks;
	sbp->f_bfree  = fsstat.f_bfree;
	sbp->f_bavail = fsstat.f_bavail;
	sbp->f_bresvd = 0;
	sbp->f_files  = fsstat.f_files;
	sbp->f_ffree  = fsstat.f_ffree;
	sbp->f_favail = fsstat.f_ffree;
	sbp->f_fresvd = 0;
	copy_statvfs_info(sbp, vfsp);
    }

    MARK_INT_SAT(CODA_STATFS_STATS);
    return(error);
}

/*
 * Flush any pending I/O.
 */
int
coda_sync(struct mount *vfsp, int waitfor,
    kauth_cred_t cred)
{
    ENTRY;
    MARK_ENTRY(CODA_SYNC_STATS);
    MARK_INT_SAT(CODA_SYNC_STATS);
    return(0);
}

int
coda_vget(struct mount *vfsp, ino_t ino,
    struct vnode **vpp)
{
    ENTRY;
    return (EOPNOTSUPP);
}

int
coda_loadvnode(struct mount *mp, struct vnode *vp,
    const void *key, size_t key_len, const void **new_key)
{
	CodaFid fid;
	struct cnode *cp;
	extern int (**coda_vnodeop_p)(void *);

	KASSERT(key_len == sizeof(CodaFid));
	memcpy(&fid, key, key_len);

	cp = kmem_zalloc(sizeof(*cp), KM_SLEEP);
	mutex_init(&cp->c_lock, MUTEX_DEFAULT, IPL_NONE);
	cp->c_fid = fid;
	cp->c_vnode = vp;
	vp->v_op = coda_vnodeop_p;
	vp->v_tag = VT_CODA;
	vp->v_type = VNON;
	vp->v_data = cp;

	*new_key = &cp->c_fid;

	return 0;
}

/*
 * fhtovp is now what vget used to be in 4.3-derived systems.  For
 * some silly reason, vget is now keyed by a 32 bit ino_t, rather than
 * a type-specific fid.
 */
int
coda_fhtovp(struct mount *vfsp, struct fid *fhp, struct mbuf *nam,
    struct vnode **vpp, int *exflagsp,
    kauth_cred_t *creadanonp)
{
    struct cfid *cfid = (struct cfid *)fhp;
    struct cnode *cp = 0;
    int error;
    struct lwp *l = curlwp; /* XXX -mach */
    CodaFid VFid;
    int vtype;

    ENTRY;

    MARK_ENTRY(CODA_VGET_STATS);
    /* Check for vget of control object. */
    if (IS_CTL_FID(&cfid->cfid_fid)) {
	*vpp = coda_ctlvp;
	vref(coda_ctlvp);
	MARK_INT_SAT(CODA_VGET_STATS);
	return(0);
    }

    error = venus_fhtovp(vftomi(vfsp), &cfid->cfid_fid, l->l_cred, l->l_proc, &VFid, &vtype);

    if (error) {
	CODADEBUG(CODA_VGET, myprintf(("vget error %d\n",error));)
	    *vpp = (struct vnode *)0;
    } else {
	CODADEBUG(CODA_VGET,
		 myprintf(("vget: %s type %d result %d\n",
			coda_f2s(&VFid), vtype, error)); )

	cp = make_coda_node(&VFid, vfsp, vtype);
	*vpp = CTOV(cp);
    }
    return(error);
}

int
coda_vptofh(struct vnode *vnp, struct fid *fidp)
{
    ENTRY;
    return (EOPNOTSUPP);
}

void
coda_init(void)
{
    ENTRY;
}

void
coda_done(void)
{
    ENTRY;
}

SYSCTL_SETUP(sysctl_vfs_coda_setup, "sysctl vfs.coda subtree setup")
{

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "coda",
		       SYSCTL_DESCR("code vfs options"),
		       NULL, 0, NULL, 0,
		       CTL_VFS, 18, CTL_EOL);
	/*
	 * XXX the "18" above could be dynamic, thereby eliminating
	 * one more instance of the "number to vfs" mapping problem,
	 * but "18" is the order as taken from sys/mount.h
	 */

/*
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "clusterread",
		       SYSCTL_DESCR( anyone? ),
		       NULL, 0, &doclusterread, 0,
		       CTL_VFS, 18, FFS_CLUSTERREAD, CTL_EOL);
*/
}

/*
 * To allow for greater ease of use, some vnodes may be orphaned when
 * Venus dies.  Certain operations should still be allowed to go
 * through, but without propagating orphan-ness.  So this function will
 * get a new vnode for the file from the current run of Venus.
 */

int
getNewVnode(struct vnode **vpp)
{
    struct cfid cfid;
    struct coda_mntinfo *mi = vftomi((*vpp)->v_mount);

    ENTRY;

    cfid.cfid_len = (short)sizeof(CodaFid);
    cfid.cfid_fid = VTOC(*vpp)->c_fid;	/* Structure assignment. */
    /* XXX ? */

    /* We're guessing that if set, the 1st element on the list is a
     * valid vnode to use. If not, return ENODEV as venus is dead.
     */
    if (mi->mi_vfsp == NULL)
	return ENODEV;

    return coda_fhtovp(mi->mi_vfsp, (struct fid*)&cfid, NULL, vpp,
		      NULL, NULL);
}

#include <ufs/ufs/quota.h>
#include <ufs/ufs/ufsmount.h>
/* get the mount structure corresponding to a given device.  Assume
 * device corresponds to a UFS. Return NULL if no device is found.
 */
struct mount *devtomp(dev_t dev)
{
    struct mount *mp;

    mutex_enter(&mountlist_lock);
    TAILQ_FOREACH(mp, &mountlist, mnt_list) {
	if ((!strcmp(mp->mnt_op->vfs_name, MOUNT_UFS)) &&
	    ((VFSTOUFS(mp))->um_dev == (dev_t) dev)) {
	    /* mount corresponds to UFS and the device matches one we want */
	    break;
	}
    }
    mutex_exit(&mountlist_lock);
    return mp;
}
