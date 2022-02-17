/*	$NetBSD: pstat.c,v 1.134 2022/02/17 14:39:14 hannken Exp $	*/

/*-
 * Copyright (c) 1980, 1991, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
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
 */

#include <sys/cdefs.h>
#ifndef lint
__COPYRIGHT("@(#) Copyright (c) 1980, 1991, 1993, 1994\
 The Regents of the University of California.  All rights reserved.");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)pstat.c	8.16 (Berkeley) 5/9/95";
#else
__RCSID("$NetBSD: pstat.c,v 1.134 2022/02/17 14:39:14 hannken Exp $");
#endif
#endif /* not lint */

#define _KERNEL
#include <sys/types.h>
#undef _KERNEL
#include <sys/param.h>
#include <sys/time.h>
#include <sys/vnode.h>
#include <sys/ucred.h>
#include <stdbool.h>
#define _KERNEL
#define NFS
#include <sys/mount.h>
#undef NFS
#include <sys/file.h>
#include <ufs/ufs/inode.h>
#include <ufs/ufs/ufsmount.h>
#include <sys/uio.h>
#include <miscfs/genfs/layer.h>
#undef _KERNEL
#include <sys/stat.h>
#include <nfs/nfsproto.h>
#include <nfs/rpcv2.h>
#include <nfs/nfs.h>
#include <nfs/nfsnode.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/conf.h>

#include <sys/sysctl.h>

#include <err.h>
#include <errno.h>
#include <kvm.h>
#include <limits.h>
#include <nlist.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "swapctl.h"

struct nlist nl[] = {
#define	V_LRU_LIST	0
	{ "_lru_list", 0, 0, 0, 0 }	,	/* address of lru lists. */
#define	V_NUMV		1
	{ "_numvnodes", 0, 0, 0, 0 },
#define	V_NEXT_OFFSET	2
	{ "_vnode_offset_next_by_lru", 0, 0, 0, 0 },
#define	FNL_NFILE	3
	{ "_nfiles", 0, 0, 0, 0 },
#define FNL_MAXFILE	4
	{ "_maxfiles", 0, 0, 0, 0 },
#define TTY_NTTY	5
	{ "_tty_count", 0, 0, 0, 0 },
#define TTY_TTYLIST	6
	{ "_ttylist", 0, 0, 0, 0 },
#define NLMANDATORY TTY_TTYLIST	/* names up to here are mandatory */
	{ "", 0, 0, 0, 0 }
};

int	usenumflag;
int	totalflag;
int	kflag;
int	hflag;
char	*nlistf	= NULL;
char	*memf	= NULL;
kvm_t	*kd;

static const char * const dtypes[] = { DTYPE_NAMES };


static const struct {
	u_int m_flag;
	u_int m_visible;
	const char *m_name;
} mnt_flags[] = {
	__MNT_FLAGS
};

struct flagbit_desc {
	u_int fd_flags;
	char fd_mark;
};

#define	SVAR(var) __STRING(var)	/* to force expansion */
#define	KGET(idx, var)							\
	KGET1(idx, &var, sizeof(var), SVAR(var))
#define	KGET1(idx, p, s, msg)						\
	KGET2(nl[idx].n_value, p, s, msg)
#define	KGET2(addr, p, s, msg) do {					\
	if (kvm_read(kd, (u_long)(addr), p, s) != s)			\
		warnx("cannot read %s: %s", msg, kvm_geterr(kd));	\
} while (0)
#define	KGETRET(addr, p, s, msg) do {					\
	if (kvm_read(kd, (u_long)(addr), p, s) != s) {			\
		warnx("cannot read %s: %s", msg, kvm_geterr(kd));	\
		return (0);						\
	}								\
} while (0)

#if 1				/* This is copied from vmstat/vmstat.c */
/*
 * Print single word.  `ovflow' is number of characters didn't fit
 * on the last word.  `fmt' is a format string to print this word.
 * It must contain asterisk for field width.  `width' is a width
 * occupied by this word.  `fixed' is a number of constant chars in
 * `fmt'.  `val' is a value to be printed using format string `fmt'.
 */
#define	PRWORD(ovflw, fmt, width, fixed, val) do {	\
	(ovflw) += printf((fmt),			\
	    (width) - (fixed) - (ovflw) > 0 ?		\
	    (width) - (fixed) - (ovflw) : 0,		\
	    (val)) - (width);				\
	if ((ovflw) < 0)				\
		(ovflw) = 0;				\
} while (0)
#endif

void	filemode(void);
int	getfiles(char **, int *, char **);
int	getflags(const struct flagbit_desc *, char *, u_int);
struct mount *
	getmnt(struct mount *);
char *	kinfo_vnodes(int *);
void	layer_header(void);
int	layer_print(struct vnode *, int);
char *	loadvnodes(int *);
void	mount_print(struct mount *);
void	nfs_header(void);
int	nfs_print(struct vnode *, int);
void	ttymode(void);
void	ttyprt(struct tty *);
void	ufs_header(void);
int	ufs_print(struct vnode *, int);
int	ext2fs_print(struct vnode *, int);
__dead void	usage(void);
void	vnode_header(void);
int	vnode_print(struct vnode *, struct vnode *);
void	vnodemode(void);

int
main(int argc, char *argv[])
{
	int ch, i, quit, ret, use_sysctl;
	int fileflag, swapflag, ttyflag, vnodeflag;
	gid_t egid = getegid();
	char buf[_POSIX2_LINE_MAX];

	setegid(getgid());
	fileflag = swapflag = ttyflag = vnodeflag = 0;
	while ((ch = getopt(argc, argv, "TM:N:fghikmnstv")) != -1)
		switch (ch) {
		case 'f':
			fileflag = 1;
			break;
		case 'M':
			memf = optarg;
			break;
		case 'N':
			nlistf = optarg;
			break;
		case 'n':
			usenumflag = 1;
			break;
		case 's':
			swapflag = 1;
			break;
		case 'T':
			totalflag = 1;
			break;
		case 't':
			ttyflag = 1;
			break;
		case 'k':
			kflag = 1;
			break;
		case 'g':
			kflag = 3; /* 1k ^ 3 */
			break;
		case 'h':
			hflag = 1;
			break;
		case 'm':
			kflag = 2; /* 1k ^ 2 */
			break;
		case 'v':
		case 'i':		/* Backward compatibility. */
			vnodeflag = 1;
			break;
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	/*
	 * Discard setgid privileges.  If not the running kernel, we toss
	 * them away totally so that bad guys can't print interesting stuff
	 * from kernel memory, otherwise switch back to kmem for the
	 * duration of the kvm_openfiles() call.
	 */
	if (nlistf != NULL || memf != NULL)
		(void)setgid(getgid());
	else
		(void)setegid(egid);

	use_sysctl = (nlistf == NULL && memf == NULL);

	if ((kd = kvm_openfiles(nlistf, memf, NULL, O_RDONLY, buf)) == 0)
		errx(1, "kvm_openfiles: %s", buf);

	/* get rid of it now anyway */
	if (nlistf == NULL && memf == NULL)
		(void)setgid(getgid());
	if ((ret = kvm_nlist(kd, nl)) != 0) {
		if (ret == -1)
			errx(1, "kvm_nlist: %s", kvm_geterr(kd));
		for (i = quit = 0; i <= NLMANDATORY; i++)
			if (!nl[i].n_value) {
				quit = 1;
				warnx("undefined symbol: %s", nl[i].n_name);
			}
		if (quit)
			exit(1);
	}
	if (!(fileflag | vnodeflag | ttyflag | swapflag | totalflag))
		usage();
	if (fileflag || totalflag)
		filemode();
	if (vnodeflag || totalflag)
		vnodemode();
	if (ttyflag)
		ttymode();
	if (swapflag || totalflag)
		if (use_sysctl)
			list_swap(0, kflag, 0, totalflag, 1, hflag);
	exit(0);
}

#define	VPTRSZ  sizeof(struct vnode *)
#define	VNODESZ sizeof(struct vnode)
#define	PTRSTRWIDTH ((int)sizeof(void *) * 2) /* Width of resulting string
						 when pointer is printed
						 in hexadecimal. */

static void
devprintf(char *buf, size_t buflen, dev_t dev)
{
	(void)snprintf(buf, buflen, "%llu,%llu",
	    (unsigned long long)major(dev),
	    (unsigned long long)minor(dev));
}

void
vnodemode(void)
{
	char *e_vnodebase, *endvnode, *evp;
	struct vnode *vp;
	struct mount *maddr, *mp;
	int numvnodes, ovflw;
	int (*vnode_fsprint) (struct vnode *, int); /* per-fs data printer */

	mp = NULL;
	e_vnodebase = loadvnodes(&numvnodes);
	if (totalflag) {
		(void)printf("%7d vnodes\n", numvnodes);
		goto out;
	}
	endvnode = e_vnodebase + numvnodes * (VPTRSZ + VNODESZ);
	(void)printf("%d active vnodes\n", numvnodes);

#define	ST	mp->mnt_stat
#define	FSTYPE_IS(mp, name)						\
	(strncmp((mp)->mnt_stat.f_fstypename, (name), 			\
	sizeof((mp)->mnt_stat.f_fstypename)) == 0)
	maddr = NULL;
	vnode_fsprint = NULL;
	for (evp = e_vnodebase; evp < endvnode; evp += VPTRSZ + VNODESZ) {
		vp = (struct vnode *)(evp + VPTRSZ);
		if (vp->v_mount != maddr) {
			/*
			 * New filesystem
			 */
			if ((mp = getmnt(vp->v_mount)) == NULL)
				continue;
			maddr = vp->v_mount;
			mount_print(mp);
			vnode_header();
			/*
			 * XXX do this in a more fs-independent way
			 */
			if (FSTYPE_IS(mp, MOUNT_FFS) ||
			    FSTYPE_IS(mp, MOUNT_MFS)) {
				ufs_header();
				vnode_fsprint = ufs_print;
			} else if (FSTYPE_IS(mp, MOUNT_NFS)) {
				nfs_header();
				vnode_fsprint = nfs_print;
			} else if (FSTYPE_IS(mp, MOUNT_EXT2FS)) {
				ufs_header();
				vnode_fsprint = ext2fs_print;
			} else if (FSTYPE_IS(mp, MOUNT_NULL) ||
			    FSTYPE_IS(mp, MOUNT_OVERLAY) ||
			    FSTYPE_IS(mp, MOUNT_UMAP)) {
				layer_header();
				vnode_fsprint = layer_print;
			} else
				vnode_fsprint = NULL;
			(void)printf("\n");
		}
		ovflw = vnode_print(*(struct vnode **)evp, vp);
		if (VTOI(vp) != NULL && vnode_fsprint != NULL)
			(*vnode_fsprint)(vp, ovflw);
		(void)printf("\n");
	}

 out:
	if (e_vnodebase)
		free(e_vnodebase);
}

int
getflags(const struct flagbit_desc *fd, char *p, u_int flags)
{
	char *q = p;

	if (flags == 0) {
		*p++ = '-';
		*p = '\0';
		return (0);
	}

	for (; fd->fd_flags != 0; fd++)
		if ((flags & fd->fd_flags) != 0)
			*p++ = fd->fd_mark;
	*p = '\0';
	return (p - q);
}

const struct flagbit_desc vnode_flags[] = {
	{ VV_ROOT,	'R' },
	{ VI_TEXT,	'T' },
	{ VV_SYSTEM,	'S' },
	{ VV_ISTTY,	'I' },
	{ VI_EXECMAP,	'E' },
	{ VU_DIROP,	'D' },
	{ VI_ONWORKLST,	'O' },
	{ VV_MPSAFE,    'M' },
	{ 0,		'\0' },
};

void
vnode_header(void)
{

	(void)printf("%-*s TYP VFLAG  USE HOLD TAG NPAGE",
	    PTRSTRWIDTH, "ADDR");
}

int
vnode_print(struct vnode *avnode, struct vnode *vp)
{
	const char *type;
	char flags[sizeof(vnode_flags) / sizeof(vnode_flags[0])];
	int ovflw;

	/*
	 * set type
	 */
	switch (vp->v_type) {
	case VNON:
		type = "non"; break;
	case VREG:
		type = "reg"; break;
	case VDIR:
		type = "dir"; break;
	case VBLK:
		type = "blk"; break;
	case VCHR:
		type = "chr"; break;
	case VLNK:
		type = "lnk"; break;
	case VSOCK:
		type = "soc"; break;
	case VFIFO:
		type = "fif"; break;
	case VBAD:
		type = "bad"; break;
	default:
		type = "unk"; break;
	}
	/*
	 * gather flags
	 */
	(void)getflags(vnode_flags, flags,
	    vp->v_uflag | vp->v_iflag | vp->v_vflag);

	ovflw = 0;
	PRWORD(ovflw, "%*lx", PTRSTRWIDTH, 0, (long)avnode);
	PRWORD(ovflw, " %*s", 4, 1, type);
	PRWORD(ovflw, " %*s", 6, 1, flags);
#define   VUSECOUNT_MASK  0x3fffffff	/* XXX: kernel private */
	PRWORD(ovflw, " %*d", 5, 1, vp->v_usecount & VUSECOUNT_MASK);
	PRWORD(ovflw, " %*d", 5, 1, vp->v_holdcnt);
	PRWORD(ovflw, " %*d", 4, 1, vp->v_tag);
	PRWORD(ovflw, " %*d", 6, 1, vp->v_uobj.uo_npages);
	return (ovflw);
}

const struct flagbit_desc ufs_flags[] = {
	{ IN_ACCESS,	'A' },
	{ IN_CHANGE,	'C' },
	{ IN_UPDATE,	'U' },
	{ IN_MODIFIED,	'M' },
	{ IN_ACCESSED,	'a' },
	{ IN_SHLOCK,	'S' },
	{ IN_EXLOCK,	'E' },
	{ IN_SPACECOUNTED, 's' },
	{ 0,		'\0' },
};

void
ufs_header(void)
{

	(void)printf(" FILEID IFLAG RDEV|SZ");
}

int
ufs_print(struct vnode *vp, int ovflw)
{
	struct inode inode, *ip = &inode;
	union dinode {
		struct ufs1_dinode dp1;
		struct ufs2_dinode dp2;
	} dip;
	struct ufsmount ump;
	char flags[sizeof(ufs_flags) / sizeof(ufs_flags[0])];
	char dev[4 + 1 + 7 + 1]; /* 12bit major + 20bit minor */
	char *name;
	mode_t type;
	dev_t rdev;

	KGETRET(VTOI(vp), &inode, sizeof(struct inode), "vnode's inode");
	KGETRET(ip->i_ump, &ump, sizeof(struct ufsmount),
	    "vnode's mount point");

	if (ump.um_fstype == UFS1) {
		KGETRET(ip->i_din.ffs1_din, &dip, sizeof (struct ufs1_dinode),
		    "inode's dinode");
		rdev = (uint32_t)dip.dp1.di_rdev;
	} else {
		KGETRET(ip->i_din.ffs2_din, &dip, sizeof (struct ufs2_dinode),
		    "inode's UFS2 dinode");
		rdev = dip.dp2.di_rdev;
	}

	/*
	 * XXX need to to locking state.
	 */

	(void)getflags(ufs_flags, flags, ip->i_flag);
	PRWORD(ovflw, " %*llu", 7, 1, (unsigned long long)ip->i_number);
	PRWORD(ovflw, " %*s", 6, 1, flags);
	type = ip->i_mode & S_IFMT;
	if (S_ISCHR(ip->i_mode) || S_ISBLK(ip->i_mode)) {
		if (usenumflag ||
		    (name = devname(rdev, type)) == NULL) {
			devprintf(dev, sizeof(dev), rdev);
			name = dev;
		}
		PRWORD(ovflw, " %*s", 8, 1, name);
	} else
		PRWORD(ovflw, " %*lld", 8, 1, (long long)ip->i_size);
	return 0;
}

int
ext2fs_print(struct vnode *vp, int ovflw)
{
	struct inode inode, *ip = &inode;
	struct ext2fs_dinode dip;
	char flags[sizeof(ufs_flags) / sizeof(ufs_flags[0])];
	char dev[4 + 1 + 7 + 1]; /* 12bit major + 20bit minor */
	char *name;
	mode_t type;

	KGETRET(VTOI(vp), &inode, sizeof(struct inode), "vnode's inode");
	KGETRET(ip->i_din.e2fs_din, &dip, sizeof (struct ext2fs_dinode),
	    "inode's dinode");

	/*
	 * XXX need to to locking state.
	 */

	(void)getflags(ufs_flags, flags, ip->i_flag);
	PRWORD(ovflw, " %*llu", 7, 1, (unsigned long long)ip->i_number);
	PRWORD(ovflw, " %*s", 6, 1, flags);
	type = dip.e2di_mode & S_IFMT;
	if (S_ISCHR(dip.e2di_mode) || S_ISBLK(dip.e2di_mode)) {
		if (usenumflag ||
		    (name = devname(dip.e2di_rdev, type)) == NULL) {
			devprintf(dev, sizeof(dev), dip.e2di_rdev);
			name = dev;
		}
		PRWORD(ovflw, " %*s", 8, 1, name);
	} else
		PRWORD(ovflw, " %*u", 8, 1, (u_int)dip.e2di_size);
	return (0);
}

const struct flagbit_desc nfs_flags[] = {
	{ NFLUSHWANT,	'W' },
	{ NFLUSHINPROG,	'P' },
	{ NMODIFIED,	'M' },
	{ NWRITEERR,	'E' },
	{ NACC,		'A' },
	{ NUPD,		'U' },
	{ NCHG,		'C' },
	{ 0,		'\0' },
};

void
nfs_header(void)
{

	(void)printf(" FILEID NFLAG RDEV|SZ");
}

int
nfs_print(struct vnode *vp, int ovflw)
{
	struct nfsnode nfsnode, *np = &nfsnode;
	char flags[sizeof(nfs_flags) / sizeof(nfs_flags[0])];
	char dev[4 + 1 + 7 + 1]; /* 12bit major + 20bit minor */
	struct vattr va;
	char *name;
	mode_t type;

	KGETRET(VTONFS(vp), &nfsnode, sizeof(nfsnode), "vnode's nfsnode");
	(void)getflags(nfs_flags, flags, np->n_flag);

	KGETRET(np->n_vattr, &va, sizeof(va), "vnode attr");
	PRWORD(ovflw, " %*ld", 7, 1, (long)va.va_fileid);
	PRWORD(ovflw, " %*s", 6, 1, flags);
	switch (va.va_type) {
	case VCHR:
		type = S_IFCHR;
		goto device;
		
	case VBLK:
		type = S_IFBLK;
	device:
		if (usenumflag || (name = devname(va.va_rdev, type)) == NULL) {
			devprintf(dev, sizeof(dev), va.va_rdev);
			name = dev;
		}
		PRWORD(ovflw, " %*s", 8, 1, name);
		break;
	default:
		PRWORD(ovflw, " %*lld", 8, 1, (long long)np->n_size);
		break;
	}
	return (0);
}

void
layer_header(void)
{

	(void)printf(" %*s", PTRSTRWIDTH, "LOWER");
}

int
layer_print(struct vnode *vp, int ovflw)
{
	struct layer_node lnode, *lp = &lnode;

	KGETRET(VTOLAYER(vp), &lnode, sizeof(lnode), "layer vnode");

	PRWORD(ovflw, " %*lx", PTRSTRWIDTH + 1, 1, (long)lp->layer_lowervp);
	return (0);
}

/*
 * Given a pointer to a mount structure in kernel space,
 * read it in and return a usable pointer to it.
 */
struct mount *
getmnt(struct mount *maddr)
{
	static struct mtab {
		struct mtab *next;
		struct mount *maddr;
		struct mount mount;
	} *mhead = NULL;
	struct mtab *mt;
	struct mount mb;

	for (mt = mhead; mt != NULL; mt = mt->next)
		if (maddr == mt->maddr)
			return (&mt->mount);
	KGETRET(maddr, &mb, sizeof(struct mount), "mount table");
	if ((mt = malloc(sizeof(struct mtab))) == NULL)
		err(1, "malloc");
	mt->mount = mb;
	mt->maddr = maddr;
	mt->next = mhead;
	mhead = mt;
	return (&mt->mount);
}

void
mount_print(struct mount *mp)
{
	int flags;

	(void)printf("*** MOUNT %s %s on %s", ST.f_fstypename,
	    ST.f_mntfromname, ST.f_mntonname);
	if ((flags = mp->mnt_flag) != 0) {
		size_t i;
		const char *sep = " (";

		for (i = 0; i < sizeof mnt_flags / sizeof mnt_flags[0]; i++) {
			if (flags & mnt_flags[i].m_flag) {
				(void)printf("%s%s", sep, mnt_flags[i].m_name);
				flags &= ~mnt_flags[i].m_flag;
				sep = ",";
			}
		}
		if (flags)
			(void)printf("%sunknown_flags:%x", sep, flags);
		(void)printf(")");
	}
	(void)printf("\n");
}

char *
loadvnodes(int *avnodes)
{
	int mib[2];
	int status;
	size_t copysize;
#if 0
	size_t oldsize;
#endif
	char *vnodebase;

	if (totalflag) {
		KGET(V_NUMV, *avnodes);
		return NULL;
	}
	if (memf != NULL) {
		/*
		 * do it by hand
		 */
		return (kinfo_vnodes(avnodes));
	}
	mib[0] = CTL_KERN;
	mib[1] = KERN_VNODE;
	/*
	 * First sysctl call gets the necessary buffer size; second
	 * sysctl call gets the data.  We allow for some growth in the
	 * data size between the two sysctl calls (increases of a few
	 * thousand vnodes in between the two calls have been observed).
	 * We ignore ENOMEM from the second sysctl call, which can
	 * happen if the kernel's data grew by even more than we allowed
	 * for.
	 */
	if (sysctl(mib, 2, NULL, &copysize, NULL, 0) == -1)
		err(1, "sysctl: KERN_VNODE");
#if 0
	oldsize = copysize;
#endif
	copysize += 100 * sizeof(struct vnode) + copysize / 20;
	if ((vnodebase = malloc(copysize)) == NULL)
		err(1, "malloc");
	status = sysctl(mib, 2, vnodebase, &copysize, NULL, 0);
	if (status == -1 && errno != ENOMEM)
		err(1, "sysctl: KERN_VNODE");
#if 0 /* for debugging the amount of growth allowed for */
	if (copysize != oldsize) {
		warnx("count changed from %ld to %ld (%+ld)%s",
		    (long)(oldsize / sizeof(struct vnode)),
		    (long)(copysize / sizeof(struct vnode)),
		    (long)(copysize / sizeof(struct vnode)) -
			(long)(oldsize / sizeof(struct vnode)),
		    (status == 0 ? "" : ", and errno = ENOMEM"));
	}
#endif
	if (copysize % (VPTRSZ + VNODESZ))
		errx(1, "vnode size mismatch");
	*avnodes = copysize / (VPTRSZ + VNODESZ);

	return (vnodebase);
}

/*
 * simulate what a running kernel does in in kinfo_vnode
 */
static int
vnode_cmp(const void *p1, const void *p2)
{
	const char *s1 = (const char *)p1;
	const char *s2 = (const char *)p2;
	const struct vnode *v1 = (const struct vnode *)(s1 + VPTRSZ);
	const struct vnode *v2 = (const struct vnode *)(s2 + VPTRSZ);

	return (v2->v_mount - v1->v_mount);
}

char *
kinfo_vnodes(int *avnodes)
{
	int i;
	char *beg, *bp, *ep;
	int numvnodes, next_offset;

	KGET(V_NUMV, numvnodes);
	if ((bp = malloc((numvnodes + 20) * (VPTRSZ + VNODESZ))) == NULL)
		err(1, "malloc");
	beg = bp;
	ep = bp + (numvnodes + 20) * (VPTRSZ + VNODESZ);
	KGET(V_NEXT_OFFSET, next_offset);

	for (i = 0; i < 3; i++) {
		TAILQ_HEAD(vnodelst, vnode) lru_head;
		struct vnode *vp, vnode;

		KGET2((nl[V_LRU_LIST].n_value + sizeof(lru_head) * i), &lru_head,
		    sizeof(lru_head), "lru_list");
		vp = TAILQ_FIRST(&lru_head);
		while (vp != NULL) {
			KGET2(vp, &vnode, sizeof(vnode), "vnode");
			if (bp + VPTRSZ + VNODESZ > ep)
				/* XXX - should realloc */
				errx(1, "no more room for vnodes");
			memmove(bp, &vp, VPTRSZ);
			bp += VPTRSZ;
			memmove(bp, &vnode, VNODESZ);
			bp += VNODESZ;
			KGET2((char *)vp + next_offset, &vp, sizeof(vp), "nvp");
		}
	}
	*avnodes = (bp - beg) / (VPTRSZ + VNODESZ);
	/* Sort by mount like we get it from sysctl. */
	qsort(beg, *avnodes, VPTRSZ + VNODESZ, vnode_cmp);
	return (beg);
}

void
ttymode(void)
{
	int ntty;
	struct ttylist_head tty_head;
	struct tty *tp, tty;

	KGET(TTY_NTTY, ntty);
	(void)printf("%d terminal device%s\n", ntty, ntty == 1 ? "" : "s");
	KGET(TTY_TTYLIST, tty_head);
	(void)printf(
	    "  LINE RAW CAN OUT  HWT LWT     COL STATE  %-*s  PGID DISC\n",
	    PTRSTRWIDTH, "SESS");
	for (tp = tty_head.tqh_first; tp; tp = tty.tty_link.tqe_next) {
		KGET2(tp, &tty, sizeof tty, "tty struct");
		ttyprt(&tty);
	}
}

static const struct flagbit_desc ttystates[] = {
	{ TS_ISOPEN,	'O'},
	{ TS_DIALOUT,	'>'},
	{ TS_CARR_ON,	'C'},
	{ TS_TIMEOUT,	'T'},
	{ TS_FLUSH,	'F'},
	{ TS_BUSY,	'B'},
	{ TS_XCLUDE,	'X'},
	{ TS_TTSTOP,	'S'},
	{ TS_TBLOCK,	'K'},
	{ TS_ASYNC,	'Y'},
	{ TS_BKSL,	'D'},
	{ TS_ERASE,	'E'},
	{ TS_LNCH,	'L'},
	{ TS_TYPEN,	'P'},
	{ TS_CNTTB,	'N'},
	{ 0,		'\0'},
};

void
ttyprt(struct tty *tp)
{
	char state[sizeof(ttystates) / sizeof(ttystates[0]) + 1];
	char dev[4 + 1 + 7 + 1]; /* 12bit major + 20bit minor */
	struct linesw t_linesw;
	const char *name;
	char buffer;
	pid_t pgid;
	int n, ovflw;

	if (usenumflag || (name = devname(tp->t_dev, S_IFCHR)) == NULL) {
		devprintf(dev, sizeof(dev), tp->t_dev);
		name = dev;
	}
	ovflw = 0;
	PRWORD(ovflw, "%-*s", 7, 0, name);
	PRWORD(ovflw, " %*d", 3, 1, tp->t_rawq.c_cc);
	PRWORD(ovflw, " %*d", 4, 1, tp->t_canq.c_cc);
	PRWORD(ovflw, " %*d", 4, 1, tp->t_outq.c_cc);
	PRWORD(ovflw, " %*d", 5, 1, tp->t_hiwat);
	PRWORD(ovflw, " %*d", 4, 1, tp->t_lowat);
	PRWORD(ovflw, " %*d", 8, 1, tp->t_column);
	n = getflags(ttystates, state, tp->t_state);
	if (tp->t_wopen) {
		state[n++] = 'W';
		state[n] = '\0';
	}
	PRWORD(ovflw, " %-*s", 7, 1, state);
	PRWORD(ovflw, " %*lX", PTRSTRWIDTH + 1, 1, (u_long)tp->t_session);
	pgid = 0;
	if (tp->t_pgrp != NULL)
		KGET2(&tp->t_pgrp->pg_id, &pgid, sizeof(pid_t), "pgid");
	PRWORD(ovflw, " %*d", 6, 1, pgid);
	KGET2(tp->t_linesw, &t_linesw, sizeof(t_linesw),
	    "line discipline switch table");
	name = t_linesw.l_name;
	(void)putchar(' ');
	for (;;) {
		KGET2(name, &buffer, sizeof(buffer), "line discipline name");
		if (buffer == '\0')
			break;
		(void)putchar(buffer);
		name++;
	}
	(void)putchar('\n');
}

static const struct flagbit_desc filemode_flags[] = {
	{ FREAD,	'R' },
	{ FWRITE,	'W' },
	{ FAPPEND,	'A' },
#ifdef FSHLOCK	/* currently gone */
	{ FSHLOCK,	'S' },
	{ FEXLOCK,	'X' },
#endif
	{ FASYNC,	'I' },
	{ 0,		'\0' },
};

void
filemode(void)
{
	struct kinfo_file *ki;
	char flags[sizeof(filemode_flags) / sizeof(filemode_flags[0])];
	char *buf, *offset;
	int len, maxfile, nfile, ovflw;

	KGET(FNL_MAXFILE, maxfile);
	if (totalflag) {
		KGET(FNL_NFILE, nfile);
		(void)printf("%3d/%3d files\n", nfile, maxfile);
		return;
	}
	if (getfiles(&buf, &len, &offset) == -1)
		return;
	/*
	 * Getfiles returns in malloc'd memory to an array of kinfo_file2
	 * structures.
	 */
	nfile = len / sizeof(struct kinfo_file);

	(void)printf("%d/%d open files\n", nfile, maxfile);
	(void)printf("%*s%s%*s TYPE    FLG     CNT  MSG  %*s%s%*s IFLG OFFSET\n",
	    (PTRSTRWIDTH - 4) / 2, "", " LOC", (PTRSTRWIDTH - 4) / 2, "",
	    (PTRSTRWIDTH - 4) / 2, "", "DATA", (PTRSTRWIDTH - 4) / 2, "");
	for (ki = (struct kinfo_file *)offset; nfile--; ki++) {
		if ((unsigned)ki->ki_ftype >= sizeof(dtypes) / sizeof(dtypes[0]))
			continue;
		ovflw = 0;
		(void)getflags(filemode_flags, flags, ki->ki_flag);
		PRWORD(ovflw, "%*lx", PTRSTRWIDTH, 0, (long)ki->ki_fileaddr);
		PRWORD(ovflw, " %-*s", 9, 1, dtypes[ki->ki_ftype]);
		PRWORD(ovflw, " %*s", 6, 1, flags);
		PRWORD(ovflw, " %*d", 5, 1, ki->ki_count);
		PRWORD(ovflw, " %*d", 5, 1, ki->ki_msgcount);
		PRWORD(ovflw, "  %*lx", PTRSTRWIDTH + 1, 2, (long)ki->ki_fdata);
		PRWORD(ovflw, " %*x", 5, 1, 0);
		if ((off_t)ki->ki_foffset < 0)
			PRWORD(ovflw, "  %-*lld\n", PTRSTRWIDTH + 1, 2,
			    (long long)ki->ki_foffset);
		else
			PRWORD(ovflw, "  %-*lld\n", PTRSTRWIDTH + 1, 2,
			    (long long)ki->ki_foffset);
	}
	free(buf);
}

int
getfiles(char **abuf, int *alen, char **aoffset)
{
	size_t len;
	int mib[6];
	char *buf;
	size_t offset;

	/*
	 * XXX
	 * Add emulation of KINFO_FILE here.
	 */
	if (memf != NULL)
		errx(1, "files on dead kernel, not implemented");

	mib[0] = CTL_KERN;
	mib[1] = KERN_FILE2;
	mib[2] = KERN_FILE_BYFILE;
	mib[3] = 0;
	mib[4] = sizeof(struct kinfo_file);
	mib[5] = 0;
	if (sysctl(mib, 6, NULL, &len, NULL, 0) == -1) {
		warn("sysctl: KERN_FILE2");
		return (-1);
	}
	/* We need to align (struct kinfo_file *) in the buffer. */
	offset = len % sizeof(off_t);
	mib[5] = len / sizeof(struct kinfo_file);
	if ((buf = malloc(len + offset)) == NULL)
		err(1, "malloc");
	if (sysctl(mib, 6, buf + offset, &len, NULL, 0) == -1) {
		warn("sysctl: 2nd KERN_FILE2");
		free(buf);
		return (-1);
	}
	*abuf = buf;
	*alen = len;
	*aoffset = (buf + offset);
	return (0);
}

void
usage(void)
{

	(void)fprintf(stderr,
	    "usage: %s [-T|-f|-s|-t|-v] [-ghkmn] [-M core] [-N system]\n",
	    getprogname());
	exit(1);
}
