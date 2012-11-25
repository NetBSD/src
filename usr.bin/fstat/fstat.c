/*	$NetBSD: fstat.c,v 1.100 2012/11/25 00:36:23 christos Exp $	*/

/*-
 * Copyright (c) 1988, 1993
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
__COPYRIGHT("@(#) Copyright (c) 1988, 1993\
 The Regents of the University of California.  All rights reserved.");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)fstat.c	8.3 (Berkeley) 5/2/95";
#else
__RCSID("$NetBSD: fstat.c,v 1.100 2012/11/25 00:36:23 christos Exp $");
#endif
#endif /* not lint */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/proc.h>
#include <sys/stat.h>
#include <sys/vnode.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/domain.h>
#include <sys/protosw.h>
#include <sys/unpcb.h>
#include <sys/sysctl.h>
#include <sys/filedesc.h>
#include <sys/pipe.h>
#define _KERNEL
#include <sys/mount.h>
#undef _KERNEL
#define	_KERNEL
#include <sys/file.h>
#include <ufs/ufs/inode.h>
#include <ufs/ufs/ufsmount.h>
#undef _KERNEL
#define NFS
#include <nfs/nfsproto.h>
#include <nfs/rpcv2.h>
#include <nfs/nfs.h>
#include <nfs/nfsnode.h>
#undef NFS
#include <msdosfs/denode.h>
#include <msdosfs/bpb.h>
#define	_KERNEL
#include <msdosfs/msdosfsmount.h>
#undef _KERNEL
#define	_KERNEL
#include <miscfs/genfs/layer.h>
#undef _KERNEL

#include <net/route.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/in_pcb.h>

#ifdef INET6
#include <netinet/ip6.h>
#include <netinet6/ip6_var.h>
#include <netinet6/in6_pcb.h>
#endif

#include <netatalk/at.h>
#include <netatalk/ddp_var.h>

#include <netdb.h>
#include <arpa/inet.h>

#include <ctype.h>
#include <errno.h>
#include <kvm.h>
#include <limits.h>
#include <nlist.h>
#include <paths.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <err.h>
#include <util.h>

#include "fstat.h"

#define	TEXT	-1
#define	CDIR	-2
#define	RDIR	-3
#define	TRACE	-4

typedef struct devs {
	struct	devs *next;
	long	fsid;
	ino_t	ino;
	const char *name;
} DEVS;
static DEVS *devs;

static int 	fsflg,	/* show files on same filesystem as file(s) argument */
	pflg,	/* show files open by a particular pid */
	uflg;	/* show files open by a particular (effective) user */
static int 	checkfile; /* true if restricting to particular files or filesystems */
static int	nflg;	/* (numerical) display f.s. and rdev as dev_t */
int	vflg;	/* display errors in locating kernel data objects etc... */

static fdfile_t **ofiles; /* buffer of pointers to file structures */
static int fstat_maxfiles;
#define ALLOC_OFILES(d)	\
	if ((d) > fstat_maxfiles) { \
		size_t len = (d) * sizeof(fdfile_t *); \
		free(ofiles); \
		ofiles = malloc(len); \
		if (ofiles == NULL) { \
			err(1, "malloc(%zu)", len);	\
		} \
		fstat_maxfiles = (d); \
	}

kvm_t *kd;

static const char *const dtypes[] = {
	DTYPE_NAMES
};

static void	dofiles(struct kinfo_proc2 *);
static int	ext2fs_filestat(struct vnode *, struct filestat *);
static int	getfname(const char *);
static void	getinetproto(int);
static void	getatproto(int);
static char   *getmnton(struct mount *);
static const char   *layer_filestat(struct vnode *, struct filestat *);
static int	msdosfs_filestat(struct vnode *, struct filestat *);
static int	nfs_filestat(struct vnode *, struct filestat *);
static const char *inet_addrstr(char *, size_t, const struct in_addr *,
    uint16_t);
#ifdef INET6
static const char *inet6_addrstr(char *, size_t, const struct in6_addr *,
    uint16_t);
#endif
static const char *at_addrstr(char *, size_t, const struct sockaddr_at *);
static void	socktrans(struct socket *, int);
static void	misctrans(struct file *, int);
static int	ufs_filestat(struct vnode *, struct filestat *);
static void	usage(void) __dead;
static const char   *vfilestat(struct vnode *, struct filestat *);
static void	vtrans(struct vnode *, int, int);
static void	ftrans(fdfile_t *, int);
static void	ptrans(struct file *, struct pipe *, int);
static void	kdriver_init(void);

int
main(int argc, char **argv)
{
	struct passwd *passwd;
	struct kinfo_proc2 *p, *plast;
	int arg, ch, what;
	char *memf, *nlistf;
	char buf[_POSIX2_LINE_MAX];
	int cnt;
	gid_t egid = getegid();

	(void)setegid(getgid());
	arg = 0;
	what = KERN_PROC_ALL;
	nlistf = memf = NULL;
	while ((ch = getopt(argc, argv, "fnp:u:vN:M:")) != -1)
		switch((char)ch) {
		case 'f':
			fsflg = 1;
			break;
		case 'M':
			memf = optarg;
			break;
		case 'N':
			nlistf = optarg;
			break;
		case 'n':
			nflg = 1;
			break;
		case 'p':
			if (pflg++)
				usage();
			if (!isdigit((unsigned char)*optarg)) {
				warnx("-p requires a process id");
				usage();
			}
			what = KERN_PROC_PID;
			arg = atoi(optarg);
			break;
		case 'u':
			if (uflg++)
				usage();
			if (!(passwd = getpwnam(optarg))) {
				errx(1, "%s: unknown uid", optarg);
			}
			what = KERN_PROC_UID;
			arg = passwd->pw_uid;
			break;
		case 'v':
			vflg = 1;
			break;
		case '?':
		default:
			usage();
		}

	kdriver_init();

	if (*(argv += optind)) {
		for (; *argv; ++argv) {
			if (getfname(*argv))
				checkfile = 1;
		}
		if (!checkfile)	/* file(s) specified, but none accessible */
			exit(1);
	}

	ALLOC_OFILES(256);	/* reserve space for file pointers */

	if (fsflg && !checkfile) {	
		/* -f with no files means use wd */
		if (getfname(".") == 0)
			exit(1);
		checkfile = 1;
	}

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

	if ((kd = kvm_openfiles(nlistf, memf, NULL, O_RDONLY, buf)) == NULL)
		errx(1, "%s", buf);

	/* get rid of it now anyway */
	if (nlistf == NULL && memf == NULL)
		(void)setgid(getgid());

	if ((p = kvm_getproc2(kd, what, arg, sizeof *p, &cnt)) == NULL) {
		errx(1, "%s", kvm_geterr(kd));
	}
	if (nflg)
		(void)printf("%s",
"USER     CMD          PID   FD  DEV     INUM  MODE  SZ|DV R/W");
	else
		(void)printf("%s",
"USER     CMD          PID   FD MOUNT       INUM MODE         SZ|DV R/W");
	if (checkfile && fsflg == 0)
		(void)printf(" NAME\n");
	else
		(void)putchar('\n');

	for (plast = &p[cnt]; p < plast; ++p) {
		if (p->p_stat == SZOMB)
			continue;
		dofiles(p);
	}
	return 0;
}

static const	char *Uname, *Comm;
pid_t	Pid;

#define PREFIX(i) (void)printf("%-8.8s %-10s %5d", Uname, Comm, Pid); \
	switch(i) { \
	case TEXT: \
		(void)printf(" text"); \
		break; \
	case CDIR: \
		(void)printf("   wd"); \
		break; \
	case RDIR: \
		(void)printf(" root"); \
		break; \
	case TRACE: \
		(void)printf("   tr"); \
		break; \
	default: \
		(void)printf(" %4d", i); \
		break; \
	}

static struct kinfo_drivers *kdriver;
static size_t kdriverlen;

static int
kdriver_comp(const void *a, const void *b)
{
	const struct kinfo_drivers *ka = a;
	const struct kinfo_drivers *kb = b;
	int kac = ka->d_cmajor == -1 ? 0 : ka->d_cmajor;
	int kbc = kb->d_cmajor == -1 ? 0 : kb->d_cmajor;
	int kab = ka->d_bmajor == -1 ? 0 : ka->d_bmajor;
	int kbb = kb->d_bmajor == -1 ? 0 : kb->d_bmajor;
	int c = kac - kbc;
	if (c == 0)
		return kab - kbb;
	else
		return c;
}

static const char *
kdriver_search(int type, dev_t num)
{
	struct kinfo_drivers k, *kp;
	static char buf[64];

	if (nflg)
		goto out;

	if (type == VBLK) {
		k.d_bmajor = num;
		k.d_cmajor = -1;
	} else {
		k.d_bmajor = -1;
		k.d_cmajor = num;
	}
	kp = bsearch(&k, kdriver, kdriverlen, sizeof(*kdriver), kdriver_comp);
	if (kp)
		return kp->d_name;
out:	
	snprintf(buf, sizeof(buf), "%llu", (unsigned long long)num);
	return buf;
}


static void
kdriver_init(void)
{
	size_t sz;
	int error;
	static const int name[2] = { CTL_KERN, KERN_DRIVERS };

	error = sysctl(name, __arraycount(name), NULL, &sz, NULL, 0);
	if (error == -1) {
		warn("sysctl kern.drivers");
		return;
	}

	if (sz % sizeof(*kdriver)) {
		warnx("bad size %zu for kern.drivers", sz);
		return;
	}

	kdriver = malloc(sz);
	if (kdriver == NULL) {
		warn("malloc");
		return;
	}

	error = sysctl(name, __arraycount(name), kdriver, &sz, NULL, 0);
	if (error == -1) {
		warn("sysctl kern.drivers");
		return;
	}

	kdriverlen = sz / sizeof(*kdriver);
	qsort(kdriver, kdriverlen, sizeof(*kdriver), kdriver_comp);
#ifdef DEBUG
	for (size_t i = 0; i < kdriverlen; i++)
		printf("%d %d %s\n", kdriver[i].d_cmajor, kdriver[i].d_bmajor,
		    kdriver[i].d_name);
#endif
}

/*
 * print open files attributed to this process
 */
static void
dofiles(struct kinfo_proc2 *p)
{
	int i;
	struct filedesc filed;
	struct cwdinfo cwdi;
	struct fdtab dt;

	Uname = user_from_uid(p->p_uid, 0);
	Pid = p->p_pid;
	Comm = p->p_comm;

	if (p->p_fd == 0 || p->p_cwdi == 0)
		return;
	if (!KVM_READ(p->p_fd, &filed, sizeof (filed))) {
		warnx("can't read filedesc at %p for pid %d",
		    (void *)(uintptr_t)p->p_fd, Pid);
		return;
	}
	if (filed.fd_lastfile == -1)
		return;
	if (!KVM_READ(p->p_cwdi, &cwdi, sizeof(cwdi))) {
		warnx("can't read cwdinfo at %p for pid %d",
		    (void *)(uintptr_t)p->p_cwdi, Pid);
		return;
	}
	if (!KVM_READ(filed.fd_dt, &dt, sizeof(dt))) {
		warnx("can't read dtab at %p for pid %d", filed.fd_dt, Pid);
		return;
	}
	if ((unsigned)filed.fd_lastfile >= dt.dt_nfiles ||
	    filed.fd_freefile > filed.fd_lastfile + 1) {
		dprintf("filedesc corrupted at %p for pid %d",
		    (void *)(uintptr_t)p->p_fd, Pid);
		return;
	}
	/*
	 * root directory vnode, if one
	 */
	if (cwdi.cwdi_rdir)
		vtrans(cwdi.cwdi_rdir, RDIR, FREAD);
	/*
	 * current working directory vnode
	 */
	vtrans(cwdi.cwdi_cdir, CDIR, FREAD);
#if 0
	/*
	 * Disable for now, since p->p_tracep appears to point to a ktr_desc *
	 * ktrace vnode, if one
	 */
	if (p->p_tracep)
		ftrans(p->p_tracep, TRACE);
#endif
	/*
	 * open files
	 */
#define FPSIZE	(sizeof (fdfile_t *))
	ALLOC_OFILES(filed.fd_lastfile+1);
	if (!KVM_READ(&filed.fd_dt->dt_ff, ofiles,
	    (filed.fd_lastfile+1) * FPSIZE)) {
		dprintf("can't read file structures at %p for pid %d",
		    &filed.fd_dt->dt_ff, Pid);
		return;
	}
	for (i = 0; i <= filed.fd_lastfile; i++) {
		if (ofiles[i] == NULL)
			continue;
		ftrans(ofiles[i], i);
	}
}

static void
ftrans(fdfile_t *fp, int i)
{
	struct file file;
	fdfile_t fdfile;

	if (!KVM_READ(fp, &fdfile, sizeof(fdfile))) {
		dprintf("can't read file %d at %p for pid %d",
		    i, fp, Pid);
		return;
	}
	if (fdfile.ff_file == NULL) {
		dprintf("null ff_file for %d at %p for pid %d",
		    i, fp, Pid);
		return;
	}
	if (!KVM_READ(fdfile.ff_file, &file, sizeof(file))) {
		dprintf("can't read file %d at %p for pid %d",
		    i, fdfile.ff_file, Pid);
		return;
	}
	switch (file.f_type) {
	case DTYPE_VNODE:
		vtrans(file.f_data, i, file.f_flag);
		break;
	case DTYPE_SOCKET:
		if (checkfile == 0)
			socktrans(file.f_data, i);
		break;
	case DTYPE_PIPE:
		if (checkfile == 0)
			ptrans(&file, file.f_data, i);
		break;
	case DTYPE_MISC:
	case DTYPE_KQUEUE:
	case DTYPE_CRYPTO:
	case DTYPE_MQUEUE:
	case DTYPE_SEM:
		if (checkfile == 0)
			misctrans(&file, i);
		break;
	default:
		dprintf("unknown file type %d for file %d of pid %d",
		    file.f_type, i, Pid);
		break;
	}
}

static const char dead[] = "dead";

static const char *
vfilestat(struct vnode *vp, struct filestat *fsp)
{
	const char *badtype = NULL;

	if (vp->v_type == VNON)
		badtype = "none";
	else if (vp->v_type == VBAD)
		badtype = "bad";
	else
		switch (vp->v_tag) {
		case VT_NON:
			badtype = dead;
			break;
		case VT_UFS:
		case VT_LFS:
		case VT_MFS:
			if (!ufs_filestat(vp, fsp))
				badtype = "error";
			break;
		case VT_MSDOSFS:
			if (!msdosfs_filestat(vp, fsp))
				badtype = "error";
			break;
		case VT_NFS:
			if (!nfs_filestat(vp, fsp))
				badtype = "error";
			break;
		case VT_EXT2FS:
			if (!ext2fs_filestat(vp, fsp))
				badtype = "error";
			break;
		case VT_ISOFS:
			if (!isofs_filestat(vp, fsp))
				badtype = "error";
			break;
		case VT_NTFS:
			if (!ntfs_filestat(vp, fsp))
				badtype = "error";
			break;
		case VT_PTYFS:
			if (!ptyfs_filestat(vp, fsp))
				badtype = "error";
			break;
		case VT_TMPFS:
			if (!tmpfs_filestat(vp, fsp))
				badtype = "error";
			break;
		case VT_NULL:
		case VT_OVERLAY:
		case VT_UMAP:
			badtype = layer_filestat(vp, fsp);
			break;
		default: {
			static char unknown[10];
			(void)snprintf(unknown, sizeof unknown,
			    "?(%x)", vp->v_tag);
			badtype = unknown;
			break;
		}
	}
	return badtype;
}

static void
vtrans(struct vnode *vp, int i, int flag)
{
	struct vnode vn;
	struct filestat fst;
	char mode[15], rw[3];
	const char *badtype, *filename;

	filename = NULL;
	if (!KVM_READ(vp, &vn, sizeof(struct vnode))) {
		dprintf("can't read vnode at %p for pid %d", vp, Pid);
		return;
	}
	badtype = vfilestat(&vn, &fst);
	if (checkfile) {
		int fsmatch = 0;
		DEVS *d;

		if (badtype && badtype != dead)
			return;
		for (d = devs; d != NULL; d = d->next)
			if (d->fsid == fst.fsid) {
				fsmatch = 1;
				if (d->ino == fst.fileid) {
					filename = d->name;
					break;
				}
			}
		if (fsmatch == 0 || (filename == NULL && fsflg == 0))
			return;
	}
	PREFIX(i);
	if (badtype == dead) {
		char buf[1024];
		(void)snprintb(buf, sizeof(buf), VNODE_FLAGBITS,
		    vn.v_iflag | vn.v_vflag | vn.v_uflag);
		(void)printf(" flags %s\n", buf);
		return;
	} else if (badtype) {
		(void)printf(" -         -  %10s    -\n", badtype);
		return;
	}
	if (nflg)
		(void)printf(" %2llu,%-2llu",
		    (unsigned long long)major(fst.fsid),
		    (unsigned long long)minor(fst.fsid));
	else
		(void)printf(" %-8s", getmnton(vn.v_mount));
	if (nflg)
		(void)snprintf(mode, sizeof mode, "%o", fst.mode);
	else
		strmode(fst.mode, mode);
	(void)printf(" %7"PRIu64" %*s", fst.fileid, nflg ? 5 : 10, mode);
	switch (vn.v_type) {
	case VBLK:
	case VCHR: {
		char *name;

		if (nflg || ((name = devname(fst.rdev, vn.v_type == VCHR ? 
		    S_IFCHR : S_IFBLK)) == NULL))
			(void)printf("  %s,%-2llu",
			    kdriver_search(vn.v_type, major(fst.rdev)),
			    (unsigned long long)minor(fst.rdev));
		else
			(void)printf(" %6s", name);
		break;
	}
	default:
		(void)printf(" %6lld", (long long)fst.size);
	}
	rw[0] = '\0';
	if (flag & FREAD)
		(void)strlcat(rw, "r", sizeof(rw));
	if (flag & FWRITE)
		(void)strlcat(rw, "w", sizeof(rw));
	(void)printf(" %-2s", rw);
	if (filename && !fsflg)
		(void)printf("  %s", filename);
	(void)putchar('\n');
}

static int
ufs_filestat(struct vnode *vp, struct filestat *fsp)
{
	struct inode inode;
	struct ufsmount ufsmount;
	union dinode {
		struct ufs1_dinode dp1;
		struct ufs2_dinode dp2;
	} dip;

	if (!KVM_READ(VTOI(vp), &inode, sizeof (inode))) {
		dprintf("can't read inode at %p for pid %d", VTOI(vp), Pid);
		return 0;
	}

	if (!KVM_READ(inode.i_ump, &ufsmount, sizeof (struct ufsmount))) {
		dprintf("can't read ufsmount at %p for pid %d", inode.i_ump, Pid);
		return 0;
	}

	switch (ufsmount.um_fstype) {
	case UFS1:
		if (!KVM_READ(inode.i_din.ffs1_din, &dip,
		    sizeof(struct ufs1_dinode))) {
			dprintf("can't read dinode at %p for pid %d",
				inode.i_din.ffs1_din, Pid);
			return 0;
		}
		fsp->rdev = dip.dp1.di_rdev;
		break;
	case UFS2:
		if (!KVM_READ(inode.i_din.ffs2_din, &dip,
		    sizeof(struct ufs2_dinode))) {
			dprintf("can't read dinode at %p for pid %d",
			    inode.i_din.ffs2_din, Pid);
			return 0;
		}
		fsp->rdev = dip.dp2.di_rdev;
		break;
	default:
		dprintf("unknown ufs type %ld for pid %d",
			ufsmount.um_fstype, Pid);
		break;
	}
	fsp->fsid = inode.i_dev & 0xffff;
	fsp->fileid = inode.i_number;
	fsp->mode = (mode_t)inode.i_mode;
	fsp->size = inode.i_size;

	return 1;
}

static int
ext2fs_filestat(struct vnode *vp, struct filestat *fsp)
{
	struct inode inode;
	struct ext2fs_dinode dinode;

	if (!KVM_READ(VTOI(vp), &inode, sizeof (inode))) {
		dprintf("can't read inode at %p for pid %d", VTOI(vp), Pid);
		return 0;
	}
	fsp->fsid = inode.i_dev & 0xffff;
	fsp->fileid = inode.i_number;

	if (!KVM_READ(inode.i_din.e2fs_din, &dinode, sizeof dinode)) {
		dprintf("can't read ext2fs_dinode at %p for pid %d",
			inode.i_din.e2fs_din, Pid);
		return 0;
	}
	fsp->mode = dinode.e2di_mode;
	fsp->size = dinode.e2di_size;
	fsp->rdev = dinode.e2di_rdev;

	return 1;
}

static int
nfs_filestat(struct vnode *vp, struct filestat *fsp)
{
	struct nfsnode nfsnode;
	struct vattr va;

	if (!KVM_READ(VTONFS(vp), &nfsnode, sizeof (nfsnode))) {
		dprintf("can't read nfsnode at %p for pid %d", VTONFS(vp),
		    Pid);
		return 0;
	}
	if (!KVM_READ(nfsnode.n_vattr, &va, sizeof(va))) {
		dprintf("can't read vnode attributes at %p for pid %d",
		    nfsnode.n_vattr, Pid);
		return 0;
	}
	fsp->fsid = va.va_fsid;
	fsp->fileid = va.va_fileid;
	fsp->size = nfsnode.n_size;
	fsp->rdev = va.va_rdev;
	fsp->mode = (mode_t)va.va_mode | getftype(vp->v_type);

	return 1;
}

static int
msdosfs_filestat(struct vnode *vp, struct filestat *fsp)
{
	struct denode de;
	struct msdosfsmount mp;

	if (!KVM_READ(VTONFS(vp), &de, sizeof(de))) {
		dprintf("can't read denode at %p for pid %d", VTONFS(vp),
		    Pid);
		return 0;
	}
	if (!KVM_READ(de.de_pmp, &mp, sizeof(mp))) {
		dprintf("can't read mount struct at %p for pid %d", de.de_pmp,
		    Pid);
		return 0;
	}

	fsp->fsid = de.de_dev & 0xffff;
	fsp->fileid = 0; /* XXX see msdosfs_vptofh() for more info */
	fsp->size = de.de_FileSize;
	fsp->rdev = 0;	/* msdosfs doesn't support device files */
	fsp->mode = (0777 & mp.pm_mask) | getftype(vp->v_type);
	return 1;
}

static const char *
layer_filestat(struct vnode *vp, struct filestat *fsp)
{
	struct layer_node layer_node;
	struct mount mount;
	struct vnode vn;
	const char *badtype;

	if (!KVM_READ(VTOLAYER(vp), &layer_node, sizeof(layer_node))) {
		dprintf("can't read layer_node at %p for pid %d",
		    VTOLAYER(vp), Pid);
		return "error";
	}
	if (!KVM_READ(vp->v_mount, &mount, sizeof(struct mount))) {
		dprintf("can't read mount struct at %p for pid %d",
		    vp->v_mount, Pid);
		return "error";
	}
	vp = layer_node.layer_lowervp;
	if (!KVM_READ(vp, &vn, sizeof(struct vnode))) {
		dprintf("can't read vnode at %p for pid %d", vp, Pid);
		return "error";
	}
	if ((badtype = vfilestat(&vn, fsp)) == NULL)
		fsp->fsid = mount.mnt_stat.f_fsidx.__fsid_val[0];
	return badtype;
}

static char *
getmnton(struct mount *m)
{
	static struct mount mount;
	static struct mtab {
		struct mtab *next;
		struct mount *m;
		char mntonname[MNAMELEN];
	} *mhead = NULL;
	struct mtab *mt;

	for (mt = mhead; mt != NULL; mt = mt->next)
		if (m == mt->m)
			return mt->mntonname;
	if (!KVM_READ(m, &mount, sizeof(struct mount))) {
		warnx("can't read mount table at %p", m);
		return NULL;
	}
	if ((mt = malloc(sizeof (struct mtab))) == NULL) {
		err(1, "malloc(%u)", (unsigned int)sizeof(struct mtab));
	}
	mt->m = m;
	(void)memmove(&mt->mntonname[0], &mount.mnt_stat.f_mntonname[0],
	    MNAMELEN);
	mt->next = mhead;
	mhead = mt;
	return mt->mntonname;
}

static const char *
inet_addrstr(char *buf, size_t len, const struct in_addr *a, uint16_t p)
{
	char addr[256], serv[256];
	struct sockaddr_in sin;
	const int niflags = nflg ? (NI_NUMERICHOST|NI_NUMERICSERV) : 0;

	(void)memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_len = sizeof(sin);
	sin.sin_addr = *a;
	sin.sin_port = htons(p);

	serv[0] = '\0';

	if (getnameinfo((struct sockaddr *)&sin, sin.sin_len,
	    addr, sizeof(addr), serv, sizeof(serv), niflags)) {
		if (inet_ntop(AF_INET, a, addr, sizeof(addr)) == NULL)
			strlcpy(addr, "invalid", sizeof(addr));
	}

	if (serv[0] == '\0')
		snprintf(serv, sizeof(serv), "%u", p);

	if (a->s_addr == INADDR_ANY) {
		if (p == 0)
			buf[0] = '\0';
		else
			snprintf(buf, len, "*:%s", serv);
		return buf;
	}

	snprintf(buf, len, "%s:%s", addr, serv);
	return buf;
}

#ifdef INET6
static const char *
inet6_addrstr(char *buf, size_t len, const struct in6_addr *a, uint16_t p)
{
	char addr[256], serv[256];
	struct sockaddr_in6 sin6;
	const int niflags = nflg ? (NI_NUMERICHOST|NI_NUMERICSERV) : 0;

	(void)memset(&sin6, 0, sizeof(sin6));
	sin6.sin6_family = AF_INET6;
	sin6.sin6_len = sizeof(sin6);
	sin6.sin6_addr = *a;
	sin6.sin6_port = htons(p);

	if (IN6_IS_ADDR_LINKLOCAL(a) &&
	    *(u_int16_t *)&sin6.sin6_addr.s6_addr[2] != 0) {
		sin6.sin6_scope_id =
			ntohs(*(uint16_t *)&sin6.sin6_addr.s6_addr[2]);
		sin6.sin6_addr.s6_addr[2] = 0;
		sin6.sin6_addr.s6_addr[3] = 0;
	}

	serv[0] = '\0';

	if (getnameinfo((struct sockaddr *)&sin6, sin6.sin6_len,
	    addr, sizeof(addr), serv, sizeof(serv), niflags)) {
		if (inet_ntop(AF_INET6, a, addr, sizeof(addr)) == NULL)
			strlcpy(addr, "invalid", sizeof(addr));
	}

	if (serv[0] == '\0')
		snprintf(serv, sizeof(serv), "%u", p);

	if (IN6_IS_ADDR_UNSPECIFIED(a)) {
		if (p == 0)
			buf[0] = '\0';
		else
			snprintf(buf, len, "*:%s", serv);
		return buf;
	}

	if (strchr(addr, ':') == NULL)
		snprintf(buf, len, "%s:%s", addr, serv);
	else
		snprintf(buf, len, "[%s]:%s", addr, serv);

	return buf;
}
#endif

static const char *
at_addrstr(char *buf, size_t len, const struct sockaddr_at *sat)
{
	const struct netrange *nr = &sat->sat_range.r_netrange;
	const struct at_addr *at = &sat->sat_addr;
	char addr[64], phase[64], range[64];

	if (sat->sat_port || at->s_net || at->s_node) {
		if (at->s_net || at->s_node)
			snprintf(addr, sizeof(addr), "%u.%u:%u",
			    ntohs(at->s_net), at->s_node, sat->sat_port);
		else
			snprintf(addr, sizeof(addr), "*:%u", sat->sat_port);
	} else
		addr[0] = '\0';

	if (nr->nr_phase)
		snprintf(phase, sizeof(phase), " phase %u", nr->nr_phase);
	else
		phase[0] = '\0';

	if (nr->nr_firstnet || nr->nr_lastnet)
		snprintf(range, sizeof(range), " range [%u-%u]",
		    ntohs(nr->nr_firstnet), ntohs(nr->nr_lastnet));
	else
		range[0] = '\0';

	snprintf(buf, len, "%s%s%s", addr, phase, range);
	return buf;
}

static void
socktrans(struct socket *sock, int i)
{
	static const char *stypename[] = {
		"unused",	/* 0 */
		"stream", 	/* 1 */
		"dgram",	/* 2 */
		"raw",		/* 3 */
		"rdm",		/* 4 */
		"seqpak"	/* 5 */
	};
#define	STYPEMAX 5
	struct socket	so;
	struct protosw	proto;
	struct domain	dom;
	struct inpcb	inpcb;
#ifdef INET6
	struct in6pcb	in6pcb;
#endif
	struct unpcb	unpcb;
	struct ddpcb	ddpcb;
	int len;
	char dname[32];
	char lbuf[512], fbuf[512];
	PREFIX(i);

	/* fill in socket */
	if (!KVM_READ(sock, &so, sizeof(struct socket))) {
		dprintf("can't read sock at %p", sock);
		goto bad;
	}

	/* fill in protosw entry */
	if (!KVM_READ(so.so_proto, &proto, sizeof(struct protosw))) {
		dprintf("can't read protosw at %p", so.so_proto);
		goto bad;
	}

	/* fill in domain */
	if (!KVM_READ(proto.pr_domain, &dom, sizeof(struct domain))) {
		dprintf("can't read domain at %p", proto.pr_domain);
		goto bad;
	}

	if ((len = kvm_read(kd, (u_long)dom.dom_name, dname,
	    sizeof(dname) - 1)) != sizeof(dname) -1) {
		dprintf("can't read domain name at %p", dom.dom_name);
		dname[0] = '\0';
	}
	else
		dname[len] = '\0';

	if ((u_short)so.so_type > STYPEMAX)
		(void)printf("* %s ?%d", dname, so.so_type);
	else
		(void)printf("* %s %s", dname, stypename[so.so_type]);

	/* 
	 * protocol specific formatting
	 *
	 * Try to find interesting things to print.  For TCP, the interesting
	 * thing is the address of the tcpcb, for UDP and others, just the
	 * inpcb (socket pcb).  For UNIX domain, its the address of the socket
	 * pcb and the address of the connected pcb (if connected).  Otherwise
	 * just print the protocol number and address of the socket itself.
	 * The idea is not to duplicate netstat, but to make available enough
	 * information for further analysis.
	 */
	fbuf[0] = '\0';
	lbuf[0] = '\0';
	switch(dom.dom_family) {
	case AF_INET:
		getinetproto(proto.pr_protocol);
		switch (proto.pr_protocol) {
		case IPPROTO_TCP:
		case IPPROTO_UDP:
			if (so.so_pcb == NULL)
				break;
			if (kvm_read(kd, (u_long)so.so_pcb, (char *)&inpcb,
			    sizeof(inpcb)) != sizeof(inpcb)) {
				dprintf("can't read inpcb at %p", so.so_pcb);
				goto bad;
			}
			inet_addrstr(lbuf, sizeof(lbuf), &inpcb.inp_laddr,
			    ntohs(inpcb.inp_lport));
			inet_addrstr(fbuf, sizeof(fbuf), &inpcb.inp_faddr,
			    ntohs(inpcb.inp_fport));
			break;
		default:
			break;
		}
		break;
#ifdef INET6
	case AF_INET6:
		getinetproto(proto.pr_protocol);
		switch (proto.pr_protocol) {
		case IPPROTO_TCP:
		case IPPROTO_UDP:
			if (so.so_pcb == NULL)
				break;
			if (kvm_read(kd, (u_long)so.so_pcb, (char *)&in6pcb,
			    sizeof(in6pcb)) != sizeof(in6pcb)) {
				dprintf("can't read in6pcb at %p", so.so_pcb);
				goto bad;
			}
			inet6_addrstr(lbuf, sizeof(lbuf), &in6pcb.in6p_laddr,
			    ntohs(in6pcb.in6p_lport));
			inet6_addrstr(fbuf, sizeof(fbuf), &in6pcb.in6p_faddr,
			    ntohs(in6pcb.in6p_fport));
			break;
		default:
			break;
		}
		break;
#endif
	case AF_LOCAL:
		/* print address of pcb and connected pcb */
		if (so.so_pcb) {
			char shoconn[4], *cp;
			void *pcb[2];
			size_t p = 0;

			pcb[0] = so.so_pcb;

			cp = shoconn;
			if (!(so.so_state & SS_CANTRCVMORE))
				*cp++ = '<';
			*cp++ = '-';
			if (!(so.so_state & SS_CANTSENDMORE))
				*cp++ = '>';
			*cp = '\0';
again:
			if (kvm_read(kd, (u_long)pcb[p], (char *)&unpcb,
			    sizeof(struct unpcb)) != sizeof(struct unpcb)){
				dprintf("can't read unpcb at %p", so.so_pcb);
				goto bad;
			}

			if (unpcb.unp_addr) {
				struct sockaddr_un *sun = 
					malloc(unpcb.unp_addrlen);
				if (sun == NULL)
				    err(1, "malloc(%zu)",
					unpcb.unp_addrlen);
				if (kvm_read(kd, (u_long)unpcb.unp_addr,
				    sun, unpcb.unp_addrlen) !=
				    (ssize_t)unpcb.unp_addrlen) {
					dprintf("can't read sun at %p",
					    unpcb.unp_addr);
					free(sun);
				} else {
					snprintf(fbuf, sizeof(fbuf), " %s %s %s",
					    shoconn, sun->sun_path,
					    p == 0 ? "[creat]" : "[using]");
					free(sun);
					break;
				}
			}
			if (unpcb.unp_conn) {
				if (p == 0) {
					pcb[++p] = unpcb.unp_conn;
					goto again;
				} else
					snprintf(fbuf, sizeof(fbuf),
					    " %p %s %p", pcb[0], shoconn,
					    pcb[1]);
			}
		}
		break;
	case AF_APPLETALK:
		getatproto(proto.pr_protocol);
		if (so.so_pcb) {
			if (kvm_read(kd, (u_long)so.so_pcb, (char *)&ddpcb,
			    sizeof(ddpcb)) != sizeof(ddpcb)){
				dprintf("can't read ddpcb at %p", so.so_pcb);
				goto bad;
			}
			at_addrstr(fbuf, sizeof(fbuf), &ddpcb.ddp_fsat);
			at_addrstr(lbuf, sizeof(lbuf), &ddpcb.ddp_lsat);
		}
		break;
	default:
		/* print protocol number and socket address */
		snprintf(fbuf, sizeof(fbuf), " %d %jx", proto.pr_protocol,
		    (uintmax_t)(uintptr_t)sock);
		break;
	}
	if (fbuf[0] || lbuf[0])
		printf(" %s%s%s", fbuf, (fbuf[0] && lbuf[0]) ? " <-> " : "",
		    lbuf);
	else if (so.so_pcb)
		printf(" %jx", (uintmax_t)(uintptr_t)so.so_pcb);
	(void)printf("\n");
	return;
bad:
	(void)printf("* error\n");
}

static void
ptrans(struct file *fp, struct pipe *cpipe, int i)
{
	struct pipe cp;

	PREFIX(i);
	
	/* fill in pipe */
	if (!KVM_READ(cpipe, &cp, sizeof(struct pipe))) {
		dprintf("can't read pipe at %p", cpipe);
		goto bad;
	}

	/* pipe descriptor is either read or write, never both */
	(void)printf("* pipe %p %s %p %s%s%s", cpipe,
		(fp->f_flag & FWRITE) ? "->" : "<-",
		cp.pipe_peer,
		(fp->f_flag & FWRITE) ? "w" : "r",
		(fp->f_flag & FNONBLOCK) ? "n" : "",
		(cp.pipe_state & PIPE_ASYNC) ? "a" : "");
	(void)printf("\n");
	return;
bad:
	(void)printf("* error\n");
}

static void
misctrans(struct file *file, int i)
{

	PREFIX(i);
	pmisc(file, dtypes[file->f_type]);
}

/*
 * getinetproto --
 *	print name of protocol number
 */
static void
getinetproto(int number)
{
	const char *cp;

	switch (number) {
	case IPPROTO_IP:
		cp = "ip"; break;
	case IPPROTO_ICMP:
		cp ="icmp"; break;
	case IPPROTO_GGP:
		cp ="ggp"; break;
	case IPPROTO_TCP:
		cp ="tcp"; break;
	case IPPROTO_EGP:
		cp ="egp"; break;
	case IPPROTO_PUP:
		cp ="pup"; break;
	case IPPROTO_UDP:
		cp ="udp"; break;
	case IPPROTO_IDP:
		cp ="idp"; break;
	case IPPROTO_RAW:
		cp ="raw"; break;
	case IPPROTO_ICMPV6:
		cp ="icmp6"; break;
	default:
		(void)printf(" %d", number);
		return;
	}
	(void)printf(" %s", cp);
}

/*
 * getatproto --
 *	print name of protocol number
 */
static void
getatproto(int number)
{
	const char *cp;

	switch (number) {
	case ATPROTO_DDP:
		cp = "ddp"; break;
	case ATPROTO_AARP:
		cp ="aarp"; break;
	default:
		(void)printf(" %d", number);
		return;
	}
	(void)printf(" %s", cp);
}

static int
getfname(const char *filename)
{
	struct stat statbuf;
	DEVS *cur;

	if (stat(filename, &statbuf)) {
		warn("stat(%s)", filename);
		return 0;
	}
	if ((cur = malloc(sizeof(*cur))) == NULL) {
		err(1, "malloc(%zu)", sizeof(*cur));
	}
	cur->next = devs;
	devs = cur;

	cur->ino = statbuf.st_ino;
	cur->fsid = statbuf.st_dev & 0xffff;
	cur->name = filename;
	return 1;
}

mode_t
getftype(enum vtype v_type)
{
	mode_t ftype;

	switch (v_type) {
	case VREG:
		ftype = S_IFREG;
		break;
	case VDIR:
		ftype = S_IFDIR;
		break;
	case VBLK:
		ftype = S_IFBLK;
		break;
	case VCHR:
		ftype = S_IFCHR;
		break;
	case VLNK:
		ftype = S_IFLNK;
		break;
	case VSOCK:
		ftype = S_IFSOCK;
		break;
	case VFIFO:
		ftype = S_IFIFO;
		break;
	default:
		ftype = 0;
		break;
	};

	return ftype;
}

static void
usage(void)
{
	(void)fprintf(stderr, "Usage: %s [-fnv] [-p pid] [-u user] "
	    "[-N system] [-M core] [file ...]\n", getprogname());
	exit(1);
}
