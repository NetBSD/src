/*	$NetBSD: fstat.c,v 1.74 2006/05/11 01:21:23 mrg Exp $	*/

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
__COPYRIGHT("@(#) Copyright (c) 1988, 1993\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)fstat.c	8.3 (Berkeley) 5/2/95";
#else
__RCSID("$NetBSD: fstat.c,v 1.74 2006/05/11 01:21:23 mrg Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/time.h>
#include <sys/proc.h>
#include <sys/user.h>
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
#define	_KERNEL
#include <sys/file.h>
#include <ufs/ufs/inode.h>
#undef _KERNEL
#define _KERNEL
#include <sys/mount.h>
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

static struct file **ofiles;	/* buffer of pointers to file structures */
static int fstat_maxfiles;
#define ALLOC_OFILES(d)	\
	if ((d) > fstat_maxfiles) { \
		free(ofiles); \
		ofiles = malloc((d) * sizeof(struct file *)); \
		if (ofiles == NULL) { \
			err(1, "malloc(%u)", (d) *	\
					(unsigned int)sizeof(struct file *)); \
		} \
		fstat_maxfiles = (d); \
	}

kvm_t *kd;

static void	dofiles(struct kinfo_proc2 *);
static int	ext2fs_filestat(struct vnode *, struct filestat *);
static int	getfname(const char *);
static void	getinetproto(int);
static char   *getmnton(struct mount *);
static const char   *layer_filestat(struct vnode *, struct filestat *);
static int	msdosfs_filestat(struct vnode *, struct filestat *);
static int	nfs_filestat(struct vnode *, struct filestat *);
#ifdef INET6
static const char *inet6_addrstr(struct in6_addr *);
#endif
static void	socktrans(struct socket *, int);
static void	kqueuetrans(void *, int);
static int	ufs_filestat(struct vnode *, struct filestat *);
static void	usage(void) __attribute__((__noreturn__));
static const char   *vfilestat(struct vnode *, struct filestat *);
static void	vtrans(struct vnode *, int, int);
static void	ftrans(struct file *, int);
static void	ptrans(struct file *, struct pipe *, int);

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

/*
 * print open files attributed to this process
 */
static void
dofiles(struct kinfo_proc2 *p)
{
	int i;
	struct filedesc0 filed0;
#define	filed	filed0.fd_fd
	struct cwdinfo cwdi;

	Uname = user_from_uid(p->p_uid, 0);
	Pid = p->p_pid;
	Comm = p->p_comm;

	if (p->p_fd == 0 || p->p_cwdi == 0)
		return;
	if (!KVM_READ(p->p_fd, &filed0, sizeof (filed0))) {
		warnx("can't read filedesc at %#llx for pid %d", (unsigned long long)p->p_fd, Pid);
		return;
	}
	if (!KVM_READ(p->p_cwdi, &cwdi, sizeof(cwdi))) {
		warnx("can't read cwdinfo at %#llx for pid %d", (unsigned long long)p->p_cwdi, Pid);
		return;
	}
	if (filed.fd_nfiles < 0 || filed.fd_lastfile >= filed.fd_nfiles ||
	    filed.fd_freefile > filed.fd_lastfile + 1) {
		dprintf("filedesc corrupted at %#llx for pid %d", (unsigned long long)p->p_fd, Pid);
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
	/*
	 * ktrace vnode, if one
	 */
	if (p->p_tracep)
		ftrans((struct file *)(intptr_t)p->p_tracep, TRACE);
	/*
	 * open files
	 */
#define FPSIZE	(sizeof (struct file *))
	ALLOC_OFILES(filed.fd_lastfile+1);
	if (filed.fd_nfiles > NDFILE) {
		if (!KVM_READ(filed.fd_ofiles, ofiles,
		    (filed.fd_lastfile+1) * FPSIZE)) {
			dprintf("can't read file structures at %p for pid %d",
			    filed.fd_ofiles, Pid);
			return;
		}
	} else
		(void)memmove(ofiles, filed0.fd_dfiles,
		    (filed.fd_lastfile+1) * FPSIZE);
	for (i = 0; i <= filed.fd_lastfile; i++) {
		if (ofiles[i] == NULL)
			continue;
		ftrans(ofiles[i], i);
	}
}

static void
ftrans(struct file *fp, int i)
{
	struct file file;

	if (!KVM_READ(fp, &file, sizeof (struct file))) {
		dprintf("can't read file %d at %p for pid %d",
		    i, fp, Pid);
		return;
	}
	switch (file.f_type) {
	case DTYPE_VNODE:
		vtrans((struct vnode *)file.f_data, i, file.f_flag);
		break;
	case DTYPE_SOCKET:
		if (checkfile == 0)
			socktrans((struct socket *)file.f_data, i);
		break;
	case DTYPE_PIPE:
		if (checkfile == 0)
			ptrans(&file, (struct pipe *)file.f_data, i);
		break;
	case DTYPE_KQUEUE:
		if (checkfile == 0)
			kqueuetrans((void *)file.f_data, i);
		break;
	default:
		dprintf("unknown file type %d for file %d of pid %d",
		    file.f_type, i, Pid);
		break;
	}
}

static const char *
vfilestat(struct vnode *vp, struct filestat *fsp)
{
	const char *badtype = NULL;

	if (vp->v_type == VNON || vp->v_tag == VT_NON)
		badtype = "none";
	else if (vp->v_type == VBAD)
		badtype = "bad";
	else
		switch (vp->v_tag) {
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
	return (badtype);
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

		if (badtype)
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
	if (badtype) {
		(void)printf(" -         -  %10s    -\n", badtype);
		return;
	}
	if (nflg)
		(void)printf(" %2d,%-2d", major(fst.fsid), minor(fst.fsid));
	else
		(void)printf(" %-8s", getmnton(vn.v_mount));
	if (nflg)
		(void)snprintf(mode, sizeof mode, "%o", fst.mode);
	else
		strmode(fst.mode, mode);
	(void)printf(" %7lu %*s", (unsigned long)fst.fileid, nflg ? 5 : 10, mode);
	switch (vn.v_type) {
	case VBLK:
	case VCHR: {
		char *name;

		if (nflg || ((name = devname(fst.rdev, vn.v_type == VCHR ? 
		    S_IFCHR : S_IFBLK)) == NULL))
			(void)printf("  %2d,%-2d", major(fst.rdev),
			    minor(fst.rdev));
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
	union dinode {
		struct ufs1_dinode dp1;
		struct ufs2_dinode dp2;
	} dip;

	if (!KVM_READ(VTOI(vp), &inode, sizeof (inode))) {
		dprintf("can't read inode at %p for pid %d", VTOI(vp), Pid);
		return 0;
	}

	if (!KVM_READ(inode.i_din.ffs1_din, &dip, sizeof(struct ufs1_dinode))) {
		dprintf("can't read dinode at %p for pid %d",
		    inode.i_din.ffs1_din, Pid);
		return 0;
	}
	if (inode.i_size == dip.dp1.di_size)
		fsp->rdev = dip.dp1.di_rdev;
	else {
		if (!KVM_READ(inode.i_din.ffs1_din, &dip,
		    sizeof(struct ufs2_dinode))) {
			dprintf("can't read dinode at %p for pid %d",
			    inode.i_din.ffs1_din, Pid);
			return 0;
		}
		fsp->rdev = dip.dp2.di_rdev;
	}

	fsp->fsid = inode.i_dev & 0xffff;
	fsp->fileid = (long)inode.i_number;
	fsp->mode = (mode_t)inode.i_mode;
	fsp->size = inode.i_size;

	return 1;
}

static int
ext2fs_filestat(struct vnode *vp, struct filestat *fsp)
{
	struct inode inode;
	u_int16_t mode;
	u_int32_t size;

	if (!KVM_READ(VTOI(vp), &inode, sizeof (inode))) {
		dprintf("can't read inode at %p for pid %d", VTOI(vp), Pid);
		return 0;
	}
	fsp->fsid = inode.i_dev & 0xffff;
	fsp->fileid = (long)inode.i_number;

	if (!KVM_READ(&inode.i_e2fs_mode, &mode, sizeof mode)) {
		dprintf("can't read inode %p's mode at %p for pid %d", VTOI(vp),
			&inode.i_e2fs_mode, Pid);
		return 0;
	}
	fsp->mode = mode;

	if (!KVM_READ(&inode.i_e2fs_size, &size, sizeof size)) {
		dprintf("can't read inode %p's size at %p for pid %d", VTOI(vp),
			&inode.i_e2fs_size, Pid);
		return 0;
	}
	fsp->size = size;
	fsp->rdev = 0;  /* XXX */
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
		return ("error");
	}
	if (!KVM_READ(vp->v_mount, &mount, sizeof(struct mount))) {
		dprintf("can't read mount struct at %p for pid %d",
		    vp->v_mount, Pid);
		return ("error");
	}
	vp = layer_node.layer_lowervp;
	if (!KVM_READ(vp, &vn, sizeof(struct vnode))) {
		dprintf("can't read vnode at %p for pid %d", vp, Pid);
		return ("error");
	}
	if ((badtype = vfilestat(&vn, fsp)) == NULL)
		fsp->fsid = mount.mnt_stat.f_fsidx.__fsid_val[0];
	return (badtype);
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
			return (mt->mntonname);
	if (!KVM_READ(m, &mount, sizeof(struct mount))) {
		warnx("can't read mount table at %p", m);
		return (NULL);
	}
	if ((mt = malloc(sizeof (struct mtab))) == NULL) {
		err(1, "malloc(%u)", (unsigned int)sizeof(struct mtab));
	}
	mt->m = m;
	(void)memmove(&mt->mntonname[0], &mount.mnt_stat.f_mntonname[0],
	    MNAMELEN);
	mt->next = mhead;
	mhead = mt;
	return (mt->mntonname);
}

#ifdef INET6
static const char *
inet6_addrstr(struct in6_addr *p)
{
	struct sockaddr_in6 sin6;
	static char hbuf[NI_MAXHOST];
	const int niflags = NI_NUMERICHOST;

	(void)memset(&sin6, 0, sizeof(sin6));
	sin6.sin6_family = AF_INET6;
	sin6.sin6_len = sizeof(struct sockaddr_in6);
	sin6.sin6_addr = *p;
	if (IN6_IS_ADDR_LINKLOCAL(p) &&
	    *(u_int16_t *)&sin6.sin6_addr.s6_addr[2] != 0) {
		sin6.sin6_scope_id =
			ntohs(*(u_int16_t *)&sin6.sin6_addr.s6_addr[2]);
		sin6.sin6_addr.s6_addr[2] = sin6.sin6_addr.s6_addr[3] = 0;
	}

	if (getnameinfo((struct sockaddr *)&sin6, sin6.sin6_len,
			hbuf, sizeof(hbuf), NULL, 0, niflags))
		return "invalid";

	return hbuf;
}
#endif

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
	int len;
	char dname[32];
#ifdef INET6
	char xaddrbuf[NI_MAXHOST + 2];
#endif

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
	switch(dom.dom_family) {
	case AF_INET:
		getinetproto(proto.pr_protocol);
		if (proto.pr_protocol == IPPROTO_TCP) {
			if (so.so_pcb == NULL)
				break;
			if (kvm_read(kd, (u_long)so.so_pcb, (char *)&inpcb,
			    sizeof(struct inpcb)) != sizeof(struct inpcb)) {
				dprintf("can't read inpcb at %p", so.so_pcb);
				goto bad;
			}
			(void)printf(" %lx", (long)inpcb.inp_ppcb);
			(void)printf(" %s:%d",
			    inpcb.inp_laddr.s_addr == INADDR_ANY ? "*" :
			    inet_ntoa(inpcb.inp_laddr), ntohs(inpcb.inp_lport));
			if (inpcb.inp_fport) {
				(void)printf(" <-> %s:%d",
				    inpcb.inp_faddr.s_addr == INADDR_ANY ? "*" :
				    inet_ntoa(inpcb.inp_faddr),
				    ntohs(inpcb.inp_fport));
			}
		} else if (proto.pr_protocol == IPPROTO_UDP) {
			if (so.so_pcb == NULL)
				break;
			if (kvm_read(kd, (u_long)so.so_pcb, (char *)&inpcb,
			    sizeof(struct inpcb)) != sizeof(struct inpcb)) {
				dprintf("can't read inpcb at %p", so.so_pcb);
				goto bad;
			}
			(void)printf(" %lx", (long)so.so_pcb);
			(void)printf(" %s:%d",
			    inpcb.inp_laddr.s_addr == INADDR_ANY ? "*" :
			    inet_ntoa(inpcb.inp_laddr), ntohs(inpcb.inp_lport));
			if (inpcb.inp_fport)
				(void)printf(" <-> %s:%d",
				    inpcb.inp_faddr.s_addr == INADDR_ANY ? "*" :
				    inet_ntoa(inpcb.inp_faddr),
				    ntohs(inpcb.inp_fport));
		} else if (so.so_pcb)
			(void)printf(" %lx", (long)so.so_pcb);
		break;
#ifdef INET6
	case AF_INET6:
		getinetproto(proto.pr_protocol);
		if (proto.pr_protocol == IPPROTO_TCP) {
			if (so.so_pcb == NULL)
				break;
			if (kvm_read(kd, (u_long)so.so_pcb, (char *)&in6pcb,
			    sizeof(struct in6pcb)) != sizeof(struct in6pcb)) {
				dprintf("can't read in6pcb at %p", so.so_pcb);
				goto bad;
			}
			(void)printf(" %lx", (long)in6pcb.in6p_ppcb);
			(void)snprintf(xaddrbuf, sizeof(xaddrbuf), "[%s]",
			    inet6_addrstr(&in6pcb.in6p_laddr));
			(void)printf(" %s:%d",
			    IN6_IS_ADDR_UNSPECIFIED(&in6pcb.in6p_laddr) ? "*" :
			    xaddrbuf,
			    ntohs(in6pcb.in6p_lport));
			if (in6pcb.in6p_fport) {
				(void)snprintf(xaddrbuf, sizeof(xaddrbuf),
				    "[%s]", inet6_addrstr(&in6pcb.in6p_faddr));
				(void)printf(" <-> %s:%d",
			            IN6_IS_ADDR_UNSPECIFIED(&in6pcb.in6p_faddr) ? "*" :
				    xaddrbuf,
				    ntohs(in6pcb.in6p_fport));
			}
		} else if (proto.pr_protocol == IPPROTO_UDP) {
			if (so.so_pcb == NULL)
				break;
			if (kvm_read(kd, (u_long)so.so_pcb, (char *)&in6pcb,
			    sizeof(struct in6pcb)) != sizeof(struct in6pcb)) {
				dprintf("can't read inpcb at %p", so.so_pcb);
				goto bad;
			}
			(void)printf(" %lx", (long)so.so_pcb);
			(void)snprintf(xaddrbuf, sizeof(xaddrbuf), "[%s]", 
			    inet6_addrstr(&in6pcb.in6p_laddr));
			(void)printf(" %s:%d",
		            IN6_IS_ADDR_UNSPECIFIED(&in6pcb.in6p_laddr) ? "*" :
			    xaddrbuf,
			    ntohs(in6pcb.in6p_lport));
			if (in6pcb.in6p_fport) {
				(void)snprintf(xaddrbuf, sizeof(xaddrbuf), "[%s]", 
				    inet6_addrstr(&in6pcb.in6p_faddr));
				(void)printf(" <-> %s:%d",
			            IN6_IS_ADDR_UNSPECIFIED(&in6pcb.in6p_faddr) ? "*" :
				    xaddrbuf,
				    ntohs(in6pcb.in6p_fport));
			}
		} else if (so.so_pcb)
			(void)printf(" %lx", (long)so.so_pcb);
		break;
#endif
	case AF_LOCAL:
		/* print address of pcb and connected pcb */
		if (so.so_pcb) {
			(void)printf(" %lx", (long)so.so_pcb);
			if (kvm_read(kd, (u_long)so.so_pcb, (char *)&unpcb,
			    sizeof(struct unpcb)) != sizeof(struct unpcb)){
				dprintf("can't read unpcb at %p", so.so_pcb);
				goto bad;
			}
			if (unpcb.unp_conn) {
				char shoconn[4], *cp;

				cp = shoconn;
				if (!(so.so_state & SS_CANTRCVMORE))
					*cp++ = '<';
				*cp++ = '-';
				if (!(so.so_state & SS_CANTSENDMORE))
					*cp++ = '>';
				*cp = '\0';
				(void)printf(" %s %lx", shoconn,
				    (long)unpcb.unp_conn);
			}
		}
		break;
	default:
		/* print protocol number and socket address */
		(void)printf(" %d %lx", proto.pr_protocol, (long)sock);
	}
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
kqueuetrans(void *kq, int i)
{

	PREFIX(i);
	(void)printf("* kqueue %lx", (long)kq);
	(void)printf("\n");
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

static int
getfname(const char *filename)
{
	struct stat statbuf;
	DEVS *cur;

	if (stat(filename, &statbuf)) {
		warn("stat(%s)", filename);
		return 0;
	}
	if ((cur = malloc(sizeof(DEVS))) == NULL) {
		err(1, "malloc(%u)", (unsigned int)sizeof(DEVS));
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
