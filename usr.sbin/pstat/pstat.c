/*	$NetBSD: pstat.c,v 1.50 1999/11/18 08:34:38 enami Exp $	*/

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
__RCSID("$NetBSD: pstat.c,v 1.50 1999/11/18 08:34:38 enami Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/time.h>
#include <sys/vnode.h>
#include <sys/map.h>
#include <sys/ucred.h>
#define _KERNEL
#define _LKM
#include <sys/file.h>
#include <ufs/ufs/quota.h>
#include <ufs/ufs/inode.h>
#define NFS
#include <sys/mount.h>
#undef NFS
#include <sys/uio.h>
#include <sys/namei.h>
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

struct {
	int m_flag;
	const char *m_name;
} mnt_flags[] = {
	{ MNT_RDONLY, "rdonly" },
	{ MNT_SYNCHRONOUS, "sync" },
	{ MNT_NOEXEC, "noexec" },
	{ MNT_NOSUID, "nosuid" },
	{ MNT_NODEV, "nodev" },
	{ MNT_UNION, "union" },
	{ MNT_ASYNC, "async" },
	{ MNT_NOCOREDUMP, "nocoredump" },
	{ MNT_NOATIME, "noatime" },
	{ MNT_SYMPERM, "symperm" },
	{ MNT_NODEVMTIME, "nodevmtime" },
	{ MNT_SOFTDEP, "softdep" },
	{ MNT_EXRDONLY, "exrdonly" },
	{ MNT_EXPORTED, "exported" },
	{ MNT_DEFEXPORTED, "defexported" },
	{ MNT_EXPORTANON, "exportanon" },
	{ MNT_EXKERB, "exkerb" },
	{ MNT_EXNORESPORT, "exnoresport" },
	{ MNT_EXPUBLIC, "expublic" },
	{ MNT_LOCAL, "local" },
	{ MNT_QUOTA, "quota" },
	{ MNT_ROOTFS, "rootfs" },
	{ MNT_UPDATE, "update" },
	{ MNT_DELEXPORT, "delexport" },
	{ MNT_RELOAD, "reload" },
	{ MNT_FORCE, "force" },
	{ MNT_GONE, "gone" },
	{ MNT_UNMOUNT, "unmount" },
	{ MNT_WANTRDWR, "wantrdwr" },
	{ 0 }
};

#define	SVAR(var) __STRING(var)	/* to force expansion */
#define	KGET(idx, var)							\
	KGET1(idx, &var, sizeof(var), SVAR(var))
#define	KGET1(idx, p, s, msg)						\
	KGET2(nl[idx].n_value, p, s, msg)
#define	KGET2(addr, p, s, msg)						\
	if (kvm_read(kd, (u_long)(addr), p, s) != s)			\
		warnx("cannot read %s: %s", msg, kvm_geterr(kd))
#define	KGETRET(addr, p, s, msg)					\
	if (kvm_read(kd, (u_long)(addr), p, s) != s) {			\
		warnx("cannot read %s: %s", msg, kvm_geterr(kd));	\
		return (0);						\
	}

void	filemode __P((void));
int	getfiles __P((char **, int *));
struct mount *
	getmnt __P((struct mount *));
struct e_vnode *
	kinfo_vnodes __P((int *));
struct e_vnode *
	loadvnodes __P((int *));
int	main __P((int, char **));
void	mount_print __P((struct mount *));
void	nfs_header __P((void));
int	nfs_print __P((struct vnode *));
void	ttymode __P((void));
void	ttyprt __P((struct tty *));
void	ufs_getflags __P((struct vnode *, struct inode *, char *));
void	ufs_header __P((void));
int	ufs_print __P((struct vnode *));
int	ext2fs_print __P((struct vnode *));
void	union_header __P((void));
int	union_print __P((struct vnode *));
void	usage __P((void));
void	vnode_header __P((void));
void	vnode_print __P((struct vnode *, struct vnode *));
void	vnodemode __P((void));

int
main(argc, argv)
	int argc;
	char *argv[];
{
	extern char *optarg;
	extern int optind;
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
	exit (0);
}

struct e_vnode {
	struct vnode *avnode;
	struct vnode vnode;
};

void
vnodemode()
{
	struct e_vnode *e_vnodebase, *endvnode, *evp;
	struct vnode *vp;
	struct mount *maddr, *mp;
	int numvnodes;
	int (*vnode_fsprint) __P((struct vnode *)); /* per-fs data printer */

	mp = NULL;
	e_vnodebase = loadvnodes(&numvnodes);
	if (totalflag) {
		(void)printf("%7d vnodes\n", numvnodes);
		return;
	}
	endvnode = e_vnodebase + numvnodes;
	(void)printf("%d active vnodes\n", numvnodes);

#define	ST	mp->mnt_stat
#define	FSTYPE_IS(mp, name)						\
	(strncmp((mp)->mnt_stat.f_fstypename, (name), MFSNAMELEN) == 0)
	maddr = NULL;
	vnode_fsprint = NULL;
	for (evp = e_vnodebase; evp < endvnode; evp++) {
		vp = &evp->vnode;
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
			} else if (FSTYPE_IS(mp, MOUNT_UNION)) {
				union_header();
				vnode_fsprint = union_print;
			} else
				vnode_fsprint = NULL;
			(void)printf("\n");
		}
		vnode_print(evp->avnode, vp);
		if (VTOI(vp) != NULL && vnode_fsprint != NULL)
			(*vnode_fsprint)(vp);
		(void)printf("\n");
	}
	free(e_vnodebase);
}

void
vnode_header()
{

	(void)printf("ADDR     TYP VFLAG  USE HOLD TAG");
}

void
vnode_print(avnode, vp)
	struct vnode *avnode;
	struct vnode *vp;
{
	char *type, flags[16];
	char *fp = flags;
	int flag;

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
	flag = vp->v_flag;
	if (flag & VROOT)
		*fp++ = 'R';
	if (flag & VTEXT)
		*fp++ = 'T';
	if (flag & VSYSTEM)
		*fp++ = 'S';
	if (flag & VISTTY)
		*fp++ = 'I';
	if (flag & VXLOCK)
		*fp++ = 'L';
	if (flag & VXWANT)
		*fp++ = 'W';
	if (flag & VBWAIT)
		*fp++ = 'B';
	if (flag & VALIASED)
		*fp++ = 'A';
	if (flag & VDIROP)
		*fp++ = 'D';
	if (flag & VLAYER)
		*fp++ = 'Y';
	if (flag & VONWORKLST)
		*fp++ = 'O';
	if (flag == 0)
		*fp++ = '-';
	*fp = '\0';
	(void)printf("%8lx %s %5s %4ld %4ld %3d",
	    (long)avnode, type, flags, (long)vp->v_usecount,
	    (long)vp->v_holdcnt, vp->v_tag);
}

void
ufs_getflags(vp, ip, flags)
	struct vnode *vp;
	struct inode *ip;
	char *flags;
{
	int flag;

	/*
	 * XXX need to to locking state.
	 */

	flag = ip->i_flag;
	if (flag & IN_RENAME)
		*flags++ = 'R';
	if (flag & IN_UPDATE)
		*flags++ = 'U';
	if (flag & IN_ACCESS)
		*flags++ = 'A';
	if (flag & IN_CHANGE)
		*flags++ = 'C';
	if (flag & IN_MODIFIED)
		*flags++ = 'M';
	if (flag & IN_SHLOCK)
		*flags++ = 'S';
	if (flag & IN_EXLOCK)
		*flags++ = 'E';
	if (flag == 0)
		*flags++ = '-';
	*flags = '\0';

}

void
ufs_header()
{

	(void)printf(" FILEID IFLAG RDEV|SZ");
}

int
ufs_print(vp)
	struct vnode *vp;
{
	struct inode inode, *ip = &inode;
	char flagbuf[16];
	char *name;
	mode_t type;

	KGETRET(VTOI(vp), &inode, sizeof(struct inode), "vnode's inode");
	ufs_getflags(vp, ip, flagbuf);
	(void)printf(" %6d %5s", ip->i_number, flagbuf);
	type = ip->i_ffs_mode & S_IFMT;
	if (S_ISCHR(ip->i_ffs_mode) || S_ISBLK(ip->i_ffs_mode))
		if (usenumflag ||
		    ((name = devname(ip->i_ffs_rdev, type)) == NULL))
			(void)printf("   %2d,%-2d",
			    major(ip->i_ffs_rdev), minor(ip->i_ffs_rdev));
		else
			(void)printf(" %7s", name);
	else
		(void)printf(" %7qd", (long long)ip->i_ffs_size);
	return (0);
}

int
ext2fs_print(vp)
	struct vnode *vp;
{
	struct inode inode, *ip = &inode;
	char flagbuf[16];
	char *name;
	mode_t type;

	KGETRET(VTOI(vp), &inode, sizeof(struct inode), "vnode's inode");
	ufs_getflags(vp, ip, flagbuf);
	(void)printf(" %6d %5s", ip->i_number, flagbuf);
	type = ip->i_e2fs_mode & S_IFMT;
	if (S_ISCHR(ip->i_e2fs_mode) || S_ISBLK(ip->i_e2fs_mode))
		if (usenumflag ||
		    ((name = devname(ip->i_e2fs_rdev, type)) == NULL))
			(void)printf("   %2d,%-2d",
			    major(ip->i_e2fs_rdev), minor(ip->i_e2fs_rdev));
		else
			(void)printf(" %7s", name);
	else
		(void)printf(" %7u", (u_int)ip->i_e2fs_size);
	return (0);
}

void
nfs_header()
{

	(void)printf(" FILEID NFLAG RDEV|SZ");
}

int
nfs_print(vp)
	struct vnode *vp;
{
	struct nfsnode nfsnode, *np = &nfsnode;
	char flagbuf[16], *flags = flagbuf;
	int flag;
	struct vattr va;
	char *name;
	mode_t type;

	KGETRET(VTONFS(vp), &nfsnode, sizeof(nfsnode), "vnode's nfsnode");
	flag = np->n_flag;
	if (flag & NFLUSHWANT)
		*flags++ = 'W';
	if (flag & NFLUSHINPROG)
		*flags++ = 'P';
	if (flag & NMODIFIED)
		*flags++ = 'M';
	if (flag & NWRITEERR)
		*flags++ = 'E';
	if (flag & NQNFSNONCACHE)
		*flags++ = 'X';
	if (flag & NQNFSWRITE)
		*flags++ = 'O';
	if (flag & NQNFSEVICTED)
		*flags++ = 'G';
	if (flag == 0)
		*flags++ = '-';
	*flags = '\0';

	KGETRET(np->n_vattr, &va, sizeof(va), "vnode attr");
	(void)printf(" %6ld %5s", (long)va.va_fileid, flagbuf);
	type = va.va_mode & S_IFMT;
	if (S_ISCHR(va.va_mode) || S_ISBLK(va.va_mode))
		if (usenumflag || ((name = devname(va.va_rdev, type)) == NULL))
			(void)printf("   %2d,%-2d",
			    major(va.va_rdev), minor(va.va_rdev));
		else
			(void)printf(" %7s", name);
	else
		(void)printf(" %7qd", (long long)np->n_size);
	return (0);
}

void
union_header()
{

	(void)printf("    UPPER    LOWER");
}

int
union_print(vp)
	struct vnode *vp;
{
	struct union_node unode, *up = &unode;

	KGETRET(VTOUNION(vp), &unode, sizeof(unode), "vnode's unode");

	(void)printf(" %8lx %8lx", (long)up->un_uppervp, (long)up->un_lowervp);
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

		for (i = 0; mnt_flags[i].m_flag; i++) {
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

struct e_vnode *
loadvnodes(avnodes)
	int *avnodes;
{
	int mib[2];
	size_t copysize;
	struct e_vnode *vnodebase;

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
	if (copysize % sizeof(struct e_vnode))
		errx(1, "vnode size mismatch");
	*avnodes = copysize / sizeof(struct e_vnode);

	return (vnodebase);
}

/*
 * simulate what a running kernel does in in kinfo_vnode
 */
struct e_vnode *
kinfo_vnodes(avnodes)
	int *avnodes;
{
	struct mntlist mountlist;
	struct mount *mp, mount;
	struct vnode *vp, vnode;
	char *vbuf, *evbuf, *bp;
	int num, numvnodes;

#define VPTRSZ  sizeof(struct vnode *)
#define VNODESZ sizeof(struct vnode)

	KGET(V_NUMV, numvnodes);
	if ((vbuf = malloc((numvnodes + 20) * (VPTRSZ + VNODESZ))) == NULL)
		err(1, "malloc");
	bp = vbuf;
	evbuf = vbuf + (numvnodes + 20) * (VPTRSZ + VNODESZ);
	KGET(V_MOUNTLIST, mountlist);
	for (num = 0, mp = mountlist.cqh_first;;
	    mp = mount.mnt_list.cqe_next) {
		KGET2(mp, &mount, sizeof(mount), "mount entry");
		for (vp = mount.mnt_vnodelist.lh_first;
		    vp != NULL; vp = vnode.v_mntvnodes.le_next) {
			KGET2(vp, &vnode, sizeof(vnode), "vnode");
			if ((bp + VPTRSZ + VNODESZ) > evbuf)
				/* XXX - should realloc */
				errx(1, "no more room for vnodes");
			memmove(bp, &vp, VPTRSZ);
			bp += VPTRSZ;
			memmove(bp, &vnode, VNODESZ);
			bp += VNODESZ;
			num++;
		}
		if (mp == mountlist.cqh_last)
			break;
	}
	*avnodes = num;
	return ((struct e_vnode *)vbuf);
}

char hdr[]="  LINE RAW CAN OUT  HWT LWT     COL STATE  SESS      PGID DISC\n";
int ttyspace = 128;

void
ttymode()
{
	int ntty;
	struct ttylist_head tty_head;
	struct tty *tp, tty;

	KGET(TTY_NTTY, ntty);
	(void)printf("%d terminal device%s\n", ntty, ntty == 1 ? "" : "s");
	KGET(TTY_TTYLIST, tty_head);
	(void)printf(hdr);
	for (tp = tty_head.tqh_first; tp; tp = tty.tty_link.tqe_next) {
		KGET2(tp, &tty, sizeof tty, "tty struct");
		ttyprt(&tty);
	}
}

struct {
	int flag;
	char val;
} ttystates[] = {
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
	{ 0,	       '\0'},
};

void
ttyprt(tp)
	struct tty *tp;
{
	int i, j;
	pid_t pgid;
	char *name, state[20];

	if (usenumflag || (name = devname(tp->t_dev, S_IFCHR)) == NULL)
		(void)printf("0x%3x:%1x ", major(tp->t_dev), minor(tp->t_dev));
	else
		(void)printf("%-7s ", name);
	(void)printf("%2d %3d ", tp->t_rawq.c_cc, tp->t_canq.c_cc);
	(void)printf("%3d %4d %3d %7d ", tp->t_outq.c_cc,
		tp->t_hiwat, tp->t_lowat, tp->t_column);
	for (i = j = 0; ttystates[i].flag; i++)
		if (tp->t_state&ttystates[i].flag)
			state[j++] = ttystates[i].val;
	if (tp->t_wopen)
		state[j++] = 'W';
	if (j == 0)
		state[j++] = '-';
	state[j] = '\0';
	(void)printf("%-6s %8lX", state, (u_long)tp->t_session);
	pgid = 0;
	if (tp->t_pgrp != NULL)
		KGET2(&tp->t_pgrp->pg_id, &pgid, sizeof(pid_t), "pgid");
	(void)printf("%6d ", pgid);
	switch (tp->t_line) {
	case TTYDISC:
		(void)printf("term\n");
		break;
	case TABLDISC:
		(void)printf("tab\n");
		break;
	case SLIPDISC:
		(void)printf("slip\n");
		break;
	case PPPDISC:
		(void)printf("ppp\n");
		break;
	case STRIPDISC:
		(void)printf("strip\n");
		break;
	default:
		(void)printf("%d\n", tp->t_line);
		break;
	}
}

void
filemode()
{
	struct file *fp;
	struct file *addr;
	char *buf, flagbuf[16], *fbp;
	int len, maxfile, nfile;
	static char *dtypes[] = { "???", "inode", "socket" };

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
	(void)printf("   LOC   TYPE    FLG     CNT  MSG    DATA    OFFSET\n");
	for (; (char *)fp < buf + len; addr = fp->f_list.le_next, fp++) {
		if ((unsigned)fp->f_type > DTYPE_SOCKET)
			continue;
		(void)printf("%lx ", (long)addr);
		(void)printf("%-8.8s", dtypes[fp->f_type]);
		fbp = flagbuf;
		if (fp->f_flag & FREAD)
			*fbp++ = 'R';
		if (fp->f_flag & FWRITE)
			*fbp++ = 'W';
		if (fp->f_flag & FAPPEND)
			*fbp++ = 'A';
#ifdef FSHLOCK	/* currently gone */
		if (fp->f_flag & FSHLOCK)
			*fbp++ = 'S';
		if (fp->f_flag & FEXLOCK)
			*fbp++ = 'X';
#endif
		if (fp->f_flag & FASYNC)
			*fbp++ = 'I';
		*fbp = '\0';
		(void)printf("%6s  %3d", flagbuf, fp->f_count);
		(void)printf("  %3d", fp->f_msgcount);
		(void)printf("  %8.1lx", (long)fp->f_data);
		if (fp->f_offset < 0)
			(void)printf("  %qx\n", (long long)fp->f_offset);
		else
			(void)printf("  %qd\n", (long long)fp->f_offset);
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
		errx(1, "files on dead kernel, not implemented\n");

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
	    "usage: pstat [-T|-f|-s|-t|-v] [-kn] [-M core] [-N system]\n");
	exit(1);
}
