/*	$NetBSD: vfs.h,v 1.1 2009/08/07 20:57:58 haad Exp $	*/

/*-
 * Copyright (c) 2007 Pawel Jakub Dawidek <pjd@FreeBSD.org>
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: src/sys/compat/opensolaris/sys/vfs.h,v 1.2 2007/06/04 11:31:45 pjd Exp $
 */

#ifndef _OPENSOLARIS_SYS_VFS_H_
#define	_OPENSOLARIS_SYS_VFS_H_

#include <sys/param.h>

#ifdef _KERNEL

#include <sys/mount.h>
#include <sys/vnode.h>

#define	rootdir	rootvnode

typedef	struct mount	vfs_t;

#define	vfs_flag	mnt_flag
#define	vfs_data	mnt_data
#define	vfs_count	mnt_refcnt
#define	vfs_fsid	mnt_stat.f_fsid
#define	vfs_bsize	mnt_stat.f_bsize

#define	v_flag		v_vflag
#define	v_vfsp		v_mount

#define f_basetype      f_fstypename

#define vfs_setfsops(a, b, c)    vfs_attach(&b)
#define vfs_freevfsops_by_type(zfsfstype) do { } while (0)

#define	VFS_RDONLY	MNT_RDONLY
#define	VFS_NOSETUID	MNT_NOSUID
#define	VFS_NOEXEC	MNT_NOEXEC
#define VFS_NODEVICES   MNT_NODEV
#define VFS_NOTRUNC     0

#define	VFS_HOLD(vfsp)	do {						\
	/* XXXNETBSD nothing */						\
} while (0)
#define	VFS_RELE(vfsp)	do {						\
	/* XXXNETBSD nothing */						\
} while (0)

#define	VROOT		VV_ROOT

/*
 * Structure defining a mount option for a filesystem.
 * option names are found in mntent.h
 */
typedef struct mntopt {
	char	*mo_name;	/* option name */
	char	**mo_cancel;	/* list of options cancelled by this one */
	char	*mo_arg;	/* argument string for this option */
	int	mo_flags;	/* flags for this mount option */
	void	*mo_data;	/* filesystem specific data */
} mntopt_t;

/*
 * Flags that apply to mount options
 */

#define	MO_SET		0x01		/* option is set */
#define	MO_NODISPLAY	0x02		/* option not listed in mnttab */
#define	MO_HASVALUE	0x04		/* option takes a value */
#define	MO_IGNORE	0x08		/* option ignored by parser */
#define	MO_DEFAULT	MO_SET		/* option is on by default */
#define	MO_TAG		0x10		/* flags a tag set by user program */
#define	MO_EMPTY	0x20		/* empty space in option table */

#define	VFS_NOFORCEOPT	0x01		/* honor MO_IGNORE (don't set option) */
#define	VFS_DISPLAY	0x02		/* Turn off MO_NODISPLAY bit for opt */
#define	VFS_NODISPLAY	0x04		/* Turn on MO_NODISPLAY bit for opt */
#define	VFS_CREATEOPT	0x08		/* Create the opt if it's not there */
#define VFS_UNMOUNTED   0x100           /* file system has been unmounted */

#define MS_SYSSPACE     0x0008          /* Mounta already in kernel space */
#define MS_DATA         0

/*
 * Structure holding mount option strings for the mounted file system.
 */
typedef struct mntopts {
	uint_t		mo_count;		/* number of entries in table */
	mntopt_t	*mo_list;		/* list of mount options */
} mntopts_t;

 /*
  * Argument structure for mount(2).
  *
  * Flags are defined in <sys/mount.h>.
  *
  * Note that if the MS_SYSSPACE bit is set in flags, the pointer fields in
  * this structure are to be interpreted as kernel addresses.  File systems
  * should be prepared for this possibility.
  */
struct mounta {
	char fspec[MAXNAMELEN - 1];
	char dataptr[MAXPATHLEN];
	char optptr[MAXPATHLEN];
	char *fstype; /* Unused */
	int  mflag;
	int  datalen;
	int  optlen;
	int  flags; /* Unused */
};

#define vfs_devismounted(dev) 0
#define fs_vscan(vp, cr, async) (0)

void vfs_setmntopt(vfs_t *vfsp, const char *name, const char *arg,
    int flags __unused);
void vfs_clearmntopt(vfs_t *vfsp, const char *name);
int vfs_optionisset(const vfs_t *vfsp, const char *opt, char **argp);
int traverse(vnode_t **cvpp, int lktype);
int domount(kthread_t *td, vnode_t *vp, const char *fstype, char *fspath,
    char *fspec, int fsflags);

#define vfs_set_feature(vfsp, feature)  do { } while (0)
#define vfs_has_feature(vfsp, feature)  (0)

int zfs_vfsinit(int fstype, char *name);
int zfs_vfsfini(void);

#endif	/* _KERNEL */

#endif	/* _OPENSOLARIS_SYS_VFS_H_ */
