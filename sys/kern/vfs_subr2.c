/*	$NetBSD: vfs_subr2.c,v 1.1 2007/07/29 14:44:09 pooka Exp $	*/

/*-
 * Copyright (c) 1997, 1998, 2004, 2005 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)vfs_subr.c	8.13 (Berkeley) 4/18/94
 */

/*
 * External virtual filesystem routines.
 *
 * This file contains vfs subroutines which do not heavily depend on
 * the kernel environment and are therefore suitable to be compiled
 * outside of the kernel.
 */

#include <sys/cdefs.h>  
__KERNEL_RCSID(0, "$NetBSD: vfs_subr2.c,v 1.1 2007/07/29 14:44:09 pooka Exp $");

#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/filedesc.h>
#include <sys/kauth.h>
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/systm.h>
#include <sys/vnode.h>

#include <miscfs/specfs/specdev.h>

#include <uvm/uvm_ddb.h>

const enum vtype iftovt_tab[16] = {
	VNON, VFIFO, VCHR, VNON, VDIR, VNON, VBLK, VNON,
	VREG, VNON, VLNK, VNON, VSOCK, VNON, VNON, VBAD,
};
const int	vttoif_tab[9] = {
	0, S_IFREG, S_IFDIR, S_IFBLK, S_IFCHR, S_IFLNK,
	S_IFSOCK, S_IFIFO, S_IFMT,
};

int doforce = 1;		/* 1 => permit forcible unmounting */
int prtactive = 0;		/* 1 => print out reclaim of active vnodes */

struct simplelock mountlist_slock = SIMPLELOCK_INITIALIZER;
static struct simplelock mntid_slock = SIMPLELOCK_INITIALIZER;
struct simplelock mntvnode_slock = SIMPLELOCK_INITIALIZER;
struct simplelock spechash_slock = SIMPLELOCK_INITIALIZER;

/* XXX - gross; single global lock to protect v_numoutput */
struct simplelock global_v_numoutput_slock = SIMPLELOCK_INITIALIZER;

struct mntlist mountlist =			/* mounted filesystem list */
    CIRCLEQ_HEAD_INITIALIZER(mountlist);

/*
 * These define the root filesystem and device.
 */
struct vnode *rootvnode;
struct device *root_device;			/* root device */

#ifdef DEBUG 
void printlockedvnodes(void);
#endif

long numvnodes;

/*
 * Lookup a mount point by filesystem identifier.
 */
struct mount *
vfs_getvfs(fsid_t *fsid)
{
	struct mount *mp;

	simple_lock(&mountlist_slock);
	CIRCLEQ_FOREACH(mp, &mountlist, mnt_list) {
		if (mp->mnt_stat.f_fsidx.__fsid_val[0] == fsid->__fsid_val[0] &&
		    mp->mnt_stat.f_fsidx.__fsid_val[1] == fsid->__fsid_val[1]) {
			simple_unlock(&mountlist_slock);
			return (mp);
		}
	}
	simple_unlock(&mountlist_slock);
	return ((struct mount *)0);
}

/*
 * Get a new unique fsid
 */
void
vfs_getnewfsid(struct mount *mp)
{
	static u_short xxxfs_mntid;
	fsid_t tfsid;
	int mtype;

	simple_lock(&mntid_slock);
	mtype = makefstype(mp->mnt_op->vfs_name);
	mp->mnt_stat.f_fsidx.__fsid_val[0] = makedev(mtype, 0);
	mp->mnt_stat.f_fsidx.__fsid_val[1] = mtype;
	mp->mnt_stat.f_fsid = mp->mnt_stat.f_fsidx.__fsid_val[0];
	if (xxxfs_mntid == 0)
		++xxxfs_mntid;
	tfsid.__fsid_val[0] = makedev(mtype & 0xff, xxxfs_mntid);
	tfsid.__fsid_val[1] = mtype;
	if (!CIRCLEQ_EMPTY(&mountlist)) {
		while (vfs_getvfs(&tfsid)) {
			tfsid.__fsid_val[0]++;
			xxxfs_mntid++;
		}
	}
	mp->mnt_stat.f_fsidx.__fsid_val[0] = tfsid.__fsid_val[0];
	mp->mnt_stat.f_fsid = mp->mnt_stat.f_fsidx.__fsid_val[0];
	simple_unlock(&mntid_slock);
}

/*
 * Make a 'unique' number from a mount type name.
 */
long
makefstype(const char *type)
{
	long rv;

	for (rv = 0; *type; type++) {
		rv <<= 2;
		rv ^= *type;
	}
	return rv;
}

/*
 * Set vnode attributes to VNOVAL
 */
void
vattr_null(struct vattr *vap)
{

	vap->va_type = VNON;

	/*
	 * Assign individually so that it is safe even if size and
	 * sign of each member are varied.
	 */
	vap->va_mode = VNOVAL;
	vap->va_nlink = VNOVAL;
	vap->va_uid = VNOVAL;
	vap->va_gid = VNOVAL;
	vap->va_fsid = VNOVAL;
	vap->va_fileid = VNOVAL;
	vap->va_size = VNOVAL;
	vap->va_blocksize = VNOVAL;
	vap->va_atime.tv_sec =
	    vap->va_mtime.tv_sec =
	    vap->va_ctime.tv_sec =
	    vap->va_birthtime.tv_sec = VNOVAL;
	vap->va_atime.tv_nsec =
	    vap->va_mtime.tv_nsec =
	    vap->va_ctime.tv_nsec =
	    vap->va_birthtime.tv_nsec = VNOVAL;
	vap->va_gen = VNOVAL;
	vap->va_flags = VNOVAL;
	vap->va_rdev = VNOVAL;
	vap->va_bytes = VNOVAL;
	vap->va_vaflags = 0;
}

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))
#define ARRAY_PRINT(idx, arr) \
    ((idx) > 0 && (idx) < ARRAY_SIZE(arr) ? (arr)[(idx)] : "UNKNOWN")

const char * const vnode_tags[] = { VNODE_TAGS };
const char * const vnode_types[] = { VNODE_TYPES };
const char vnode_flagbits[] = VNODE_FLAGBITS;

/*
 * Print out a description of a vnode.
 */
void
vprint(const char *label, struct vnode *vp)
{
	char bf[96];

	if (label != NULL)
		printf("%s: ", label);
	printf("tag %s(%d) type %s(%d), usecount %d, writecount %ld, "
	    "refcount %ld,", ARRAY_PRINT(vp->v_tag, vnode_tags), vp->v_tag,
	    ARRAY_PRINT(vp->v_type, vnode_types), vp->v_type,
	    vp->v_usecount, vp->v_writecount, vp->v_holdcnt);
	printf(" flags (%s)",
	    bitmask_snprintf(vp->v_flag, vnode_flagbits, bf, sizeof(bf)));
	if (vp->v_data == NULL) {
		printf("\n");
	} else {
		printf("\n\t");
		VOP_PRINT(vp);
	}
}

#ifdef DEBUG
/*
 * List all of the locked vnodes in the system.
 * Called when debugging the kernel.
 */
void
printlockedvnodes(void)
{
	struct mount *mp, *nmp;
	struct vnode *vp;

	printf("Locked vnodes\n");
	simple_lock(&mountlist_slock);
	for (mp = CIRCLEQ_FIRST(&mountlist); mp != (void *)&mountlist;
	     mp = nmp) {
		if (vfs_busy(mp, LK_NOWAIT, &mountlist_slock)) {
			nmp = CIRCLEQ_NEXT(mp, mnt_list);
			continue;
		}
		TAILQ_FOREACH(vp, &mp->mnt_vnodelist, v_mntvnodes) {
			if (VOP_ISLOCKED(vp))
				vprint(NULL, vp);
		}
		simple_lock(&mountlist_slock);
		nmp = CIRCLEQ_NEXT(mp, mnt_list);
		vfs_unbusy(mp);
	}
	simple_unlock(&mountlist_slock);
}
#endif

/*
 * Do the usual access checking.
 * file_mode, uid and gid are from the vnode in question,
 * while acc_mode and cred are from the VOP_ACCESS parameter list
 */
int
vaccess(enum vtype type, mode_t file_mode, uid_t uid, gid_t gid,
    mode_t acc_mode, kauth_cred_t cred)
{
	mode_t mask;
	int error, ismember;

	/*
	 * Super-user always gets read/write access, but execute access depends
	 * on at least one execute bit being set.
	 */
	if (kauth_authorize_generic(cred, KAUTH_GENERIC_ISSUSER, NULL) == 0) {
		if ((acc_mode & VEXEC) && type != VDIR &&
		    (file_mode & (S_IXUSR|S_IXGRP|S_IXOTH)) == 0)
			return (EACCES);
		return (0);
	}

	mask = 0;

	/* Otherwise, check the owner. */
	if (kauth_cred_geteuid(cred) == uid) {
		if (acc_mode & VEXEC)
			mask |= S_IXUSR;
		if (acc_mode & VREAD)
			mask |= S_IRUSR;
		if (acc_mode & VWRITE)
			mask |= S_IWUSR;
		return ((file_mode & mask) == mask ? 0 : EACCES);
	}

	/* Otherwise, check the groups. */
	error = kauth_cred_ismember_gid(cred, gid, &ismember);
	if (error)
		return (error);
	if (kauth_cred_getegid(cred) == gid || ismember) {
		if (acc_mode & VEXEC)
			mask |= S_IXGRP;
		if (acc_mode & VREAD)
			mask |= S_IRGRP;
		if (acc_mode & VWRITE)
			mask |= S_IWGRP;
		return ((file_mode & mask) == mask ? 0 : EACCES);
	}

	/* Otherwise, check everyone else. */
	if (acc_mode & VEXEC)
		mask |= S_IXOTH;
	if (acc_mode & VREAD)
		mask |= S_IROTH;
	if (acc_mode & VWRITE)
		mask |= S_IWOTH;
	return ((file_mode & mask) == mask ? 0 : EACCES);
}

/*
 * Given a file system name, look up the vfsops for that
 * file system, or return NULL if file system isn't present
 * in the kernel.
 */
struct vfsops *
vfs_getopsbyname(const char *name)
{
	struct vfsops *v;

	LIST_FOREACH(v, &vfs_list, vfs_list) {
		if (strcmp(v->vfs_name, name) == 0)
			break;
	}

	return (v);
}

void
copy_statvfs_info(struct statvfs *sbp, const struct mount *mp)
{
	const struct statvfs *mbp;

	if (sbp == (mbp = &mp->mnt_stat))
		return;

	(void)memcpy(&sbp->f_fsidx, &mbp->f_fsidx, sizeof(sbp->f_fsidx));
	sbp->f_fsid = mbp->f_fsid;
	sbp->f_owner = mbp->f_owner;
	sbp->f_flag = mbp->f_flag;
	sbp->f_syncwrites = mbp->f_syncwrites;
	sbp->f_asyncwrites = mbp->f_asyncwrites;
	sbp->f_syncreads = mbp->f_syncreads;
	sbp->f_asyncreads = mbp->f_asyncreads;
	(void)memcpy(sbp->f_spare, mbp->f_spare, sizeof(mbp->f_spare));
	(void)memcpy(sbp->f_fstypename, mbp->f_fstypename,
	    sizeof(sbp->f_fstypename));
	(void)memcpy(sbp->f_mntonname, mbp->f_mntonname,
	    sizeof(sbp->f_mntonname));
	(void)memcpy(sbp->f_mntfromname, mp->mnt_stat.f_mntfromname,
	    sizeof(sbp->f_mntfromname));
	sbp->f_namemax = mbp->f_namemax;
}

int
set_statvfs_info(const char *onp, int ukon, const char *fromp, int ukfrom,
    const char *vfsname, struct mount *mp, struct lwp *l)
{
	int error;
	size_t size;
	struct statvfs *sfs = &mp->mnt_stat;
	int (*fun)(const void *, void *, size_t, size_t *);

	(void)strlcpy(mp->mnt_stat.f_fstypename, vfsname,
	    sizeof(mp->mnt_stat.f_fstypename));

	if (onp) {
		struct cwdinfo *cwdi = l->l_proc->p_cwdi;
		fun = (ukon == UIO_SYSSPACE) ? copystr : copyinstr;
		if (cwdi->cwdi_rdir != NULL) {
			size_t len;
			char *bp;
			char *path = malloc(MAXPATHLEN, M_TEMP, M_WAITOK);

			if (!path) /* XXX can't happen with M_WAITOK */
				return ENOMEM;

			bp = path + MAXPATHLEN;
			*--bp = '\0';
			error = getcwd_common(cwdi->cwdi_rdir, rootvnode, &bp,
			    path, MAXPATHLEN / 2, 0, l);
			if (error) {
				free(path, M_TEMP);
				return error;
			}

			len = strlen(bp);
			if (len > sizeof(sfs->f_mntonname) - 1)
				len = sizeof(sfs->f_mntonname) - 1;
			(void)strncpy(sfs->f_mntonname, bp, len);
			free(path, M_TEMP);

			if (len < sizeof(sfs->f_mntonname) - 1) {
				error = (*fun)(onp, &sfs->f_mntonname[len],
				    sizeof(sfs->f_mntonname) - len - 1, &size);
				if (error)
					return error;
				size += len;
			} else {
				size = len;
			}
		} else {
			error = (*fun)(onp, &sfs->f_mntonname,
			    sizeof(sfs->f_mntonname) - 1, &size);
			if (error)
				return error;
		}
		(void)memset(sfs->f_mntonname + size, 0,
		    sizeof(sfs->f_mntonname) - size);
	}

	if (fromp) {
		fun = (ukfrom == UIO_SYSSPACE) ? copystr : copyinstr;
		error = (*fun)(fromp, sfs->f_mntfromname,
		    sizeof(sfs->f_mntfromname) - 1, &size);
		if (error)
			return error;
		(void)memset(sfs->f_mntfromname + size, 0,
		    sizeof(sfs->f_mntfromname) - size);
	}
	return 0;
}

void
vfs_timestamp(struct timespec *ts)
{

	nanotime(ts);
}

#ifdef DDB
static const char buf_flagbits[] = BUF_FLAGBITS;

void
vfs_buf_print(struct buf *bp, int full, void (*pr)(const char *, ...))
{
	char bf[1024];

	(*pr)("  vp %p lblkno 0x%"PRIx64" blkno 0x%"PRIx64" rawblkno 0x%"
	    PRIx64 " dev 0x%x\n",
	    bp->b_vp, bp->b_lblkno, bp->b_blkno, bp->b_rawblkno, bp->b_dev);

	bitmask_snprintf(bp->b_flags, buf_flagbits, bf, sizeof(bf));
	(*pr)("  error %d flags 0x%s\n", bp->b_error, bf);

	(*pr)("  bufsize 0x%lx bcount 0x%lx resid 0x%lx\n",
		  bp->b_bufsize, bp->b_bcount, bp->b_resid);
	(*pr)("  data %p saveaddr %p dep %p\n",
		  bp->b_data, bp->b_saveaddr, LIST_FIRST(&bp->b_dep));
	(*pr)("  iodone %p\n", bp->b_iodone);
}


void
vfs_vnode_print(struct vnode *vp, int full, void (*pr)(const char *, ...))
{
	char bf[256];

	uvm_object_printit(&vp->v_uobj, full, pr);
	bitmask_snprintf(vp->v_flag, vnode_flagbits, bf, sizeof(bf));
	(*pr)("\nVNODE flags %s\n", bf);
	(*pr)("mp %p numoutput %d size 0x%llx writesize 0x%llx\n",
	      vp->v_mount, vp->v_numoutput, vp->v_size, vp->v_writesize);

	(*pr)("data %p usecount %d writecount %ld holdcnt %ld numoutput %d\n",
	      vp->v_data, vp->v_usecount, vp->v_writecount,
	      vp->v_holdcnt, vp->v_numoutput);

	(*pr)("tag %s(%d) type %s(%d) mount %p typedata %p\n",
	      ARRAY_PRINT(vp->v_tag, vnode_tags), vp->v_tag,
	      ARRAY_PRINT(vp->v_type, vnode_types), vp->v_type,
	      vp->v_mount, vp->v_mountedhere);

	if (full) {
		struct buf *bp;

		(*pr)("clean bufs:\n");
		LIST_FOREACH(bp, &vp->v_cleanblkhd, b_vnbufs) {
			(*pr)(" bp %p\n", bp);
			vfs_buf_print(bp, full, pr);
		}

		(*pr)("dirty bufs:\n");
		LIST_FOREACH(bp, &vp->v_dirtyblkhd, b_vnbufs) {
			(*pr)(" bp %p\n", bp);
			vfs_buf_print(bp, full, pr);
		}
	}
}

void
vfs_mount_print(struct mount *mp, int full, void (*pr)(const char *, ...))
{
	char sbuf[256];

	(*pr)("vnodecovered = %p syncer = %p data = %p\n",
			mp->mnt_vnodecovered,mp->mnt_syncer,mp->mnt_data);

	(*pr)("fs_bshift %d dev_bshift = %d\n",
			mp->mnt_fs_bshift,mp->mnt_dev_bshift);

	bitmask_snprintf(mp->mnt_flag, __MNT_FLAG_BITS, sbuf, sizeof(sbuf));
	(*pr)("flag = %s\n", sbuf);

	bitmask_snprintf(mp->mnt_iflag, __IMNT_FLAG_BITS, sbuf, sizeof(sbuf));
	(*pr)("iflag = %s\n", sbuf);

	/* XXX use lockmgr_printinfo */
	if (mp->mnt_lock.lk_sharecount)
		(*pr)(" lock type %s: SHARED (count %d)", mp->mnt_lock.lk_wmesg,
		    mp->mnt_lock.lk_sharecount);
	else if (mp->mnt_lock.lk_flags & LK_HAVE_EXCL) {
		(*pr)(" lock type %s: EXCL (count %d) by ",
		    mp->mnt_lock.lk_wmesg, mp->mnt_lock.lk_exclusivecount);
		(*pr)("pid %d.%d", mp->mnt_lock.lk_lockholder,
		    mp->mnt_lock.lk_locklwp);
	} else
		(*pr)(" not locked");
	if (mp->mnt_lock.lk_waitcount > 0)
		(*pr)(" with %d pending", mp->mnt_lock.lk_waitcount);

	(*pr)("\n");

	if (mp->mnt_unmounter) {
		(*pr)("unmounter pid = %d ",mp->mnt_unmounter->l_proc);
	}

	(*pr)("statvfs cache:\n");
	(*pr)("\tbsize = %lu\n",mp->mnt_stat.f_bsize);
	(*pr)("\tfrsize = %lu\n",mp->mnt_stat.f_frsize);
	(*pr)("\tiosize = %lu\n",mp->mnt_stat.f_iosize);

	(*pr)("\tblocks = %"PRIu64"\n",mp->mnt_stat.f_blocks);
	(*pr)("\tbfree = %"PRIu64"\n",mp->mnt_stat.f_bfree);
	(*pr)("\tbavail = %"PRIu64"\n",mp->mnt_stat.f_bavail);
	(*pr)("\tbresvd = %"PRIu64"\n",mp->mnt_stat.f_bresvd);

	(*pr)("\tfiles = %"PRIu64"\n",mp->mnt_stat.f_files);
	(*pr)("\tffree = %"PRIu64"\n",mp->mnt_stat.f_ffree);
	(*pr)("\tfavail = %"PRIu64"\n",mp->mnt_stat.f_favail);
	(*pr)("\tfresvd = %"PRIu64"\n",mp->mnt_stat.f_fresvd);

	(*pr)("\tf_fsidx = { 0x%"PRIx32", 0x%"PRIx32" }\n",
			mp->mnt_stat.f_fsidx.__fsid_val[0],
			mp->mnt_stat.f_fsidx.__fsid_val[1]);

	(*pr)("\towner = %"PRIu32"\n",mp->mnt_stat.f_owner);
	(*pr)("\tnamemax = %lu\n",mp->mnt_stat.f_namemax);

	bitmask_snprintf(mp->mnt_stat.f_flag, __MNT_FLAG_BITS, sbuf,
	    sizeof(sbuf));
	(*pr)("\tflag = %s\n",sbuf);
	(*pr)("\tsyncwrites = %" PRIu64 "\n",mp->mnt_stat.f_syncwrites);
	(*pr)("\tasyncwrites = %" PRIu64 "\n",mp->mnt_stat.f_asyncwrites);
	(*pr)("\tsyncreads = %" PRIu64 "\n",mp->mnt_stat.f_syncreads);
	(*pr)("\tasyncreads = %" PRIu64 "\n",mp->mnt_stat.f_asyncreads);
	(*pr)("\tfstypename = %s\n",mp->mnt_stat.f_fstypename);
	(*pr)("\tmntonname = %s\n",mp->mnt_stat.f_mntonname);
	(*pr)("\tmntfromname = %s\n",mp->mnt_stat.f_mntfromname);

	{
		int cnt = 0;
		struct vnode *vp;
		(*pr)("locked vnodes =");
		/* XXX would take mountlist lock, except ddb may not have context */
		TAILQ_FOREACH(vp, &mp->mnt_vnodelist, v_mntvnodes) {
			if (VOP_ISLOCKED(vp)) {
				if ((++cnt % 6) == 0) {
					(*pr)(" %p,\n\t", vp);
				} else {
					(*pr)(" %p,", vp);
				}
			}
		}
		(*pr)("\n");
	}

	if (full) {
		int cnt = 0;
		struct vnode *vp;
		(*pr)("all vnodes =");
		/* XXX would take mountlist lock, except ddb may not have context */
		TAILQ_FOREACH(vp, &mp->mnt_vnodelist, v_mntvnodes) {
			if (!TAILQ_NEXT(vp, v_mntvnodes)) {
				(*pr)(" %p", vp);
			} else if ((++cnt % 6) == 0) {
				(*pr)(" %p,\n\t", vp);
			} else {
				(*pr)(" %p,", vp);
			}
		}
		(*pr)("\n", vp);
	}
}
#endif /* DDB */
