/*	$NetBSD: pstat.c,v 1.76 2003/01/20 05:30:13 simonb Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
__COPYRIGHT("@(#) Copyright (c) 1980, 1991, 1993, 1994\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)pstat.c	8.16 (Berkeley) 5/9/95";
#else
__RCSID("$NetBSD: pstat.c,v 1.76 2003/01/20 05:30:13 simonb Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/time.h>
#include <sys/vnode.h>
#include <sys/ucred.h>
#define _KERNEL
#include <sys/file.h>
#include <ufs/ufs/inode.h>
#define NFS
#include <sys/mount.h>
#undef NFS
#include <sys/uio.h>
#include <sys/namei.h>
#include <miscfs/genfs/layer.h>
#include <miscfs/union/union.h>
#undef _KERNEL
#include <sys/stat.h>
#include <nfs/nfsproto.h>
#include <nfs/rpcv2.h>
#include <nfs/nfs.h>
#include <nfs/nfsnode.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/conf.h>
#include <sys/device.h>

#include <sys/sysctl.h>

#include <err.h>
#include <kvm.h>
#include <limits.h>
#include <nlist.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "swapctl.h"

struct nlist nl[] = {
#define	V_MOUNTLIST	0
	{ "_mountlist" },	/* address of head of mount list. */
#define	V_NUMV		1
	{ "_numvnodes" },
#define	FNL_NFILE	2
	{ "_nfiles" },
#define FNL_MAXFILE	3
	{ "_maxfiles" },
#define TTY_NTTY	4
	{ "_tty_count" },
#define TTY_TTYLIST	5
	{ "_ttylist" },
#define NLMANDATORY TTY_TTYLIST	/* names up to here are mandatory */
	{ "" }
};

int	usenumflag;
int	totalflag;
int	kflag;
char	*nlistf	= NULL;
char	*memf	= NULL;
kvm_t	*kd;

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
} while (/* CONSTCOND */0)
#define	KGETRET(addr, p, s, msg) do {					\
	if (kvm_read(kd, (u_long)(addr), p, s) != s) {			\
		warnx("cannot read %s: %s", msg, kvm_geterr(kd));	\
		return (0);						\
	}								\
} while (/* CONSTCOND */0)

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
} while (/* CONSTCOND */0)
#endif

void	filemode __P((void));
int	getfiles __P((char **, int *));
int	getflags __P((const struct flagbit_desc *, char *, u_int));
struct mount *
	getmnt __P((struct mount *));
char *	kinfo_vnodes __P((int *));
void	layer_header __P((void));
int	layer_print __P((struct vnode *, int));
char *	loadvnodes __P((int *));
int	main __P((int, char **));
void	mount_print __P((struct mount *));
void	nfs_header __P((void));
int	nfs_print __P((struct vnode *, int));
void	ttymode __P((void));
void	ttyprt __P((struct tty *));
void	ufs_header __P((void));
int	ufs_print __P((struct vnode *, int));
int	ext2fs_print __P((struct vnode *, int));
void	union_header __P((void));
int	union_print __P((struct vnode *, int));
void	usage __P((void));
void	vnode_header __P((void));
int	vnode_print __P((struct vnode *, struct vnode *));
void	vnodemode __P((void));

int
main(argc, argv)
	int argc;
	char *argv[];
{
	int ch, i, quit, ret;
	int fileflag, swapflag, ttyflag, vnodeflag;
	gid_t egid = getegid();
	char buf[_POSIX2_LINE_MAX];

	setegid(getgid());
	fileflag = swapflag = ttyflag = vnodeflag = 0;
	while ((ch = getopt(argc, argv, "TM:N:fiknstv")) != -1)
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
		list_swap(0, kflag, 0, totalflag, 1);
	exit(0);
}

#define	VPTRSZ  sizeof(struct vnode *)
#define	VNODESZ sizeof(struct vnode)
#define	PTRSTRWIDTH ((int)sizeof(void *) * 2) /* Width of resulting string
						 when pointer is printed
						 in hexadecimal. */

void
vnodemode()
{
	char *e_vnodebase, *endvnode, *evp;
	struct vnode *vp;
	struct mount *maddr, *mp;
	int numvnodes, ovflw;
	int (*vnode_fsprint)
	    __P((struct vnode *, int)); /* per-fs data printer */

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
	(strncmp((mp)->mnt_stat.f_fstypename, (name), MFSNAMELEN) == 0)
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
			} else if (FSTYPE_IS(mp, MOUNT_UNION)) {
				union_header();
				vnode_fsprint = union_print;
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
	free(e_vnodebase);
}

int
getflags(fd, p, flags)
	const struct flagbit_desc *fd;
	char *p;
	u_int flags;
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
	{ VROOT,	'R' },
	{ VTEXT,	'T' },
	{ VSYSTEM,	'S' },
	{ VISTTY,	'I' },
	{ VEXECMAP,	'E' },
	{ VXLOCK,	'L' },
	{ VXWANT,	'W' },
	{ VBWAIT,	'B' },
	{ VALIASED,	'A' },
	{ VDIROP,	'D' },
	{ VLAYER,	'Y' },
	{ VONWORKLST,	'O' },
	{ 0,		'\0' },
};

void
vnode_header()
{

	(void)printf("%-*s TYP VFLAG  USE HOLD TAG NPAGE",
	    PTRSTRWIDTH, "ADDR");
}

int
vnode_print(avnode, vp)
	struct vnode *avnode;
	struct vnode *vp;
{
	char *type, flags[sizeof(vnode_flags) / sizeof(vnode_flags[0])];
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
	(void)getflags(vnode_flags, flags, vp->v_flag);

	ovflw = 0;
	PRWORD(ovflw, "%*lx", PTRSTRWIDTH, 0, (long)avnode);
	PRWORD(ovflw, " %*s", 4, 1, type);
	PRWORD(ovflw, " %*s", 6, 1, flags);
	PRWORD(ovflw, " %*ld", 5, 1, (long)vp->v_usecount);
	PRWORD(ovflw, " %*ld", 5, 1, (long)vp->v_holdcnt);
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
	{ IN_RENAME,	'R' },
	{ IN_SHLOCK,	'S' },
	{ IN_EXLOCK,	'E' },
	{ IN_CLEANING,	'c' },
	{ IN_ADIROP,	'D' },
	{ IN_SPACECOUNTED, 's' },
	{ 0,		'\0' },
};

void
ufs_header()
{

	(void)printf(" FILEID IFLAG RDEV|SZ");
}

int
ufs_print(vp, ovflw)
	struct vnode *vp;
	int ovflw;
{
	struct inode inode, *ip = &inode;
	char flags[sizeof(ufs_flags) / sizeof(ufs_flags[0])];
	char dev[4 + 1 + 7 + 1]; /* 12bit marjor + 20bit minor */
	char *name;
	mode_t type;

	KGETRET(VTOI(vp), &inode, sizeof(struct inode), "vnode's inode");

	/*
	 * XXX need to to locking state.
	 */

	(void)getflags(ufs_flags, flags, ip->i_flag);
	PRWORD(ovflw, " %*d", 7, 1, ip->i_number);
	PRWORD(ovflw, " %*s", 6, 1, flags);
	type = ip->i_ffs_mode & S_IFMT;
	if (S_ISCHR(ip->i_ffs_mode) || S_ISBLK(ip->i_ffs_mode)) {
		if (usenumflag ||
		    (name = devname(ip->i_ffs_rdev, type)) == NULL) {
			snprintf(dev, sizeof(dev), "%d,%d",
			    major(ip->i_ffs_rdev), minor(ip->i_ffs_rdev));
			name = dev;
		}
		PRWORD(ovflw, " %*s", 8, 1, name);
	} else
		PRWORD(ovflw, " %*lld", 8, 1, (long long)ip->i_ffs_size);
	return (0);
}

int
ext2fs_print(vp, ovflw)
	struct vnode *vp;
	int ovflw;
{
	struct inode inode, *ip = &inode;
	char flags[sizeof(ufs_flags) / sizeof(ufs_flags[0])];
	char dev[4 + 1 + 7 + 1]; /* 12bit marjor + 20bit minor */
	char *name;
	mode_t type;

	KGETRET(VTOI(vp), &inode, sizeof(struct inode), "vnode's inode");

	/*
	 * XXX need to to locking state.
	 */

	(void)getflags(ufs_flags, flags, ip->i_flag);
	PRWORD(ovflw, " %*d", 7, 1, ip->i_number);
	PRWORD(ovflw, " %*s", 6, 1, flags);
	type = ip->i_e2fs_mode & S_IFMT;
	if (S_ISCHR(ip->i_e2fs_mode) || S_ISBLK(ip->i_e2fs_mode)) {
		if (usenumflag ||
		    (name = devname(ip->i_e2fs_rdev, type)) == NULL) {
			snprintf(dev, sizeof(dev), "%d,%d",
			    major(ip->i_e2fs_rdev), minor(ip->i_e2fs_rdev));
			name = dev;
		}
		PRWORD(ovflw, " %*s", 8, 1, name);
	} else
		PRWORD(ovflw, " %*u", 8, 1, (u_int)ip->i_e2fs_size);
	return (0);
}

const struct flagbit_desc nfs_flags[] = {
	{ NFLUSHWANT,	'W' },
	{ NFLUSHINPROG,	'P' },
	{ NMODIFIED,	'M' },
	{ NWRITEERR,	'E' },
	{ NQNFSNONCACHE, 'X' },
	{ NQNFSWRITE,	'O' },
	{ NQNFSEVICTED,	'G' },
	{ NACC,		'A' },
	{ NUPD,		'U' },
	{ NCHG,		'C' },
	{ 0,		'\0' },
};

void
nfs_header()
{

	(void)printf(" FILEID NFLAG RDEV|SZ");
}

int
nfs_print(vp, ovflw)
	struct vnode *vp;
	int ovflw;
{
	struct nfsnode nfsnode, *np = &nfsnode;
	char flags[sizeof(nfs_flags) / sizeof(nfs_flags[0])];
	char dev[4 + 1 + 7 + 1]; /* 12bit marjor + 20bit minor */
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
			(void)snprintf(dev, sizeof(dev), "%d,%d",
			    major(va.va_rdev), minor(va.va_rdev));
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
layer_header()
{

	(void)printf(" %*s", PTRSTRWIDTH, "LOWER");
}

int
layer_print(vp, ovflw)
	struct vnode *vp;
	int ovflw;
{
	struct layer_node lnode, *lp = &lnode;

	KGETRET(VTOLAYER(vp), &lnode, sizeof(lnode), "layer vnode");

	PRWORD(ovflw, " %*lx", PTRSTRWIDTH + 1, 1, (long)lp->layer_lowervp);
	return (0);
}

void
union_header()
{

	(void)printf(" %*s %*s", PTRSTRWIDTH, "UPPER", PTRSTRWIDTH, "LOWER");
}

int
union_print(vp, ovflw)
	struct vnode *vp;
	int ovflw;
{
	struct union_node unode, *up = &unode;

	KGETRET(VTOUNION(vp), &unode, sizeof(unode), "vnode's unode");

	PRWORD(ovflw, " %*lx", PTRSTRWIDTH + 1, 1, (long)up->un_uppervp);
	PRWORD(ovflw, " %*lx", PTRSTRWIDTH + 1, 1, (long)up->un_lowervp);
	return (0);
}

/*
 * Given a pointer to a mount structure in kernel space,
 * read it in and return a usable pointer to it.
 */
struct mount *
getmnt(maddr)
	struct mount *maddr;
{
	static struct mtab {
		struct mtab *next;
		struct mount *maddr;
		struct mount mount;
	} *mhead = NULL;
	struct mtab *mt;

	for (mt = mhead; mt != NULL; mt = mt->next)
		if (maddr == mt->maddr)
			return (&mt->mount);
	if ((mt = malloc(sizeof(struct mtab))) == NULL)
		err(1, "malloc");
	KGETRET(maddr, &mt->mount, sizeof(struct mount), "mount table");
	mt->maddr = maddr;
	mt->next = mhead;
	mhead = mt;
	return (&mt->mount);
}

void
mount_print(mp)
	struct mount *mp;
{
	int flags;

	(void)printf("*** MOUNT %s %s on %s", ST.f_fstypename,
	    ST.f_mntfromname, ST.f_mntonname);
	if ((flags = mp->mnt_flag) != 0) {
		int i;
		const char *sep = " (";

		for (i = 0; i <= sizeof mnt_flags / sizeof mnt_flags[0]; i++) {
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
loadvnodes(avnodes)
	int *avnodes;
{
	int mib[2];
	size_t copysize;
	char *vnodebase;

	if (memf != NULL) {
		/*
		 * do it by hand
		 */
		return (kinfo_vnodes(avnodes));
	}
	mib[0] = CTL_KERN;
	mib[1] = KERN_VNODE;
	if (sysctl(mib, 2, NULL, &copysize, NULL, 0) == -1)
		err(1, "sysctl: KERN_VNODE");
	if ((vnodebase = malloc(copysize)) == NULL)
		err(1, "malloc");
	if (sysctl(mib, 2, vnodebase, &copysize, NULL, 0) == -1)
		err(1, "sysctl: KERN_VNODE");
	if (copysize % (VPTRSZ + VNODESZ))
		errx(1, "vnode size mismatch");
	*avnodes = copysize / (VPTRSZ + VNODESZ);

	return (vnodebase);
}

/*
 * simulate what a running kernel does in in kinfo_vnode
 */
char *
kinfo_vnodes(avnodes)
	int *avnodes;
{
	struct mntlist mountlist;
	struct mount *mp, mount;
	struct vnode *vp, vnode;
	char *beg, *bp, *ep;
	int numvnodes;

	KGET(V_NUMV, numvnodes);
	if ((bp = malloc((numvnodes + 20) * (VPTRSZ + VNODESZ))) == NULL)
		err(1, "malloc");
	beg = bp;
	ep = bp + (numvnodes + 20) * (VPTRSZ + VNODESZ);
	KGET(V_MOUNTLIST, mountlist);
	for (mp = mountlist.cqh_first;
	    mp = mount.mnt_list.cqe_next) {
		KGET2(mp, &mount, sizeof(mount), "mount entry");
		for (vp = mount.mnt_vnodelist.lh_first;
		    vp != NULL; vp = vnode.v_mntvnodes.le_next) {
			KGET2(vp, &vnode, sizeof(vnode), "vnode");
			if (bp + VPTRSZ + VNODESZ > ep)
				/* XXX - should realloc */
				errx(1, "no more room for vnodes");
			memmove(bp, &vp, VPTRSZ);
			bp += VPTRSZ;
			memmove(bp, &vnode, VNODESZ);
			bp += VNODESZ;
		}
		if (mp == mountlist.cqh_last)
			break;
	}
	*avnodes = (bp - beg) / (VPTRSZ + VNODESZ);
	return (beg);
}

void
ttymode()
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
	{ TS_ASLEEP,	'A'},
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
ttyprt(tp)
	struct tty *tp;
{
	char state[sizeof(ttystates) / sizeof(ttystates[0]) + 1];
	char dev[2 + 3 + 1 + 5 + 1]; /* 12bit major + 20bit minor */
	struct linesw t_linesw;
	char *name, buffer;
	pid_t pgid;
	int n, ovflw;

	if (usenumflag || (name = devname(tp->t_dev, S_IFCHR)) == NULL) {
		(void)snprintf(dev, sizeof(dev), "0x%3x:%x",
		    major(tp->t_dev), minor(tp->t_dev));
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
filemode()
{
	struct file *fp;
	struct file *addr;
	char flags[sizeof(filemode_flags) / sizeof(filemode_flags[0])];
	char *buf;
	int len, maxfile, nfile, ovflw;
	static const char * const dtypes[] =
		{ "???", "inode", "socket", "pipe" };

	KGET(FNL_MAXFILE, maxfile);
	if (totalflag) {
		KGET(FNL_NFILE, nfile);
		(void)printf("%3d/%3d files\n", nfile, maxfile);
		return;
	}
	if (getfiles(&buf, &len) == -1)
		return;
	/*
	 * Getfiles returns in malloc'd memory a pointer to the first file
	 * structure, and then an array of file structs (whose addresses are
	 * derivable from the previous entry).
	 */
	addr = ((struct filelist *)buf)->lh_first;
	fp = (struct file *)(buf + sizeof(struct filelist));
	nfile = (len - sizeof(struct filelist)) / sizeof(struct file);

	(void)printf("%d/%d open files\n", nfile, maxfile);
	(void)printf("%*s%s%*s TYPE    FLG     CNT  MSG  %*s%s%*s  OFFSET\n",
	    (PTRSTRWIDTH - 4) / 2, "", " LOC", (PTRSTRWIDTH - 4) / 2, "",
	    (PTRSTRWIDTH - 4) / 2, "", "DATA", (PTRSTRWIDTH - 4) / 2, "");
	for (; (char *)fp < buf + len; addr = fp->f_list.le_next, fp++) {
		if ((unsigned)fp->f_type > DTYPE_PIPE)
			continue;
		ovflw = 0;
		(void)getflags(filemode_flags, flags, fp->f_flag);
		PRWORD(ovflw, "%*lx", PTRSTRWIDTH, 0, (long)addr);
		PRWORD(ovflw, " %-*s", 9, 1, dtypes[fp->f_type]);
		PRWORD(ovflw, " %*s", 6, 1, flags);
		PRWORD(ovflw, " %*d", 5, 1, fp->f_count);
		PRWORD(ovflw, " %*d", 5, 1, fp->f_msgcount);
		PRWORD(ovflw, "  %*lx", PTRSTRWIDTH + 1, 2, (long)fp->f_data);
		if (fp->f_offset < 0)
			PRWORD(ovflw, "  %-*llx\n", PTRSTRWIDTH + 1, 2,
			    (long long)fp->f_offset);
		else
			PRWORD(ovflw, "  %-*lld\n", PTRSTRWIDTH + 1, 2,
			    (long long)fp->f_offset);
	}
	free(buf);
}

int
getfiles(abuf, alen)
	char **abuf;
	int *alen;
{
	size_t len;
	int mib[2];
	char *buf;

	/*
	 * XXX
	 * Add emulation of KINFO_FILE here.
	 */
	if (memf != NULL)
		errx(1, "files on dead kernel, not implemented");

	mib[0] = CTL_KERN;
	mib[1] = KERN_FILE;
	if (sysctl(mib, 2, NULL, &len, NULL, 0) == -1) {
		warn("sysctl: KERN_FILE");
		return (-1);
	}
	if ((buf = malloc(len)) == NULL)
		err(1, "malloc");
	if (sysctl(mib, 2, buf, &len, NULL, 0) == -1) {
		warn("sysctl: KERN_FILE");
		return (-1);
	}
	*abuf = buf;
	*alen = len;
	return (0);
}

void
usage()
{

	(void)fprintf(stderr,
	    "usage: %s [-T|-f|-s|-t|-v] [-kn] [-M core] [-N system]\n",
	    getprogname());
	exit(1);
}
