/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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
 *
 *	from: Utah Hdr: hpux_compat.c 1.41 91/04/06
 *	from: @(#)hpux_compat.c	7.16 (Berkeley) 5/30/91
 *	$Id: hpux_compat.c,v 1.4 1994/01/07 00:43:46 mycroft Exp $
 */

/*
 * Various HPUX compatibility routines
 */

#ifdef HPUXCOMPAT

#include "param.h"
#include "systm.h"
#include "signalvar.h"
#include "kernel.h"
#include "filedesc.h"
#include "proc.h"
#include "buf.h"
#include "wait.h"
#include "file.h"
#include "namei.h"
#include "vnode.h"
#include "ioctl.h"
#include "ptrace.h"
#include "stat.h"
#include "syslog.h"
#include "malloc.h"
#include "mount.h"
#include "ipc.h"
#include "user.h"

#include "machine/cpu.h"
#include "machine/reg.h"
#include "machine/psl.h"
#include "machine/vmparam.h"
#include "hpux.h"
#include "hpux_termio.h"

#ifdef DEBUG
int unimpresponse = 0;
#endif

/* SYS5 style UTSNAME info */
struct hpux_utsname protoutsname = {
	"4.4bsd", "", "2.0", "B", "9000/3?0", ""
};

/* 6.0 and later style context */
#ifdef FPCOPROC
char hpux_context[] =
	"standalone HP-MC68881 HP-MC68020 HP-MC68010 localroot default";
#else
char hpux_context[] =
	"standalone HP-MC68020 HP-MC68010 localroot default";
#endif

#define NERR	79
#define BERR	1000

/* indexed by BSD errno */
short bsdtohpuxerrnomap[NERR] = {
/*00*/	  0,   1,   2,   3,   4,   5,   6,   7,   8,   9,
/*10*/	 10,  45,  12,  13,  14,  15,  16,  17,  18,  19,
/*20*/	 20,  21,  22,  23,  24,  25,  26,  27,  28,  29,
/*30*/	 30,  31,  32,  33,  34, 246, 245, 244, 216, 217,
/*40*/	218, 219, 220, 221, 222, 223, 224, 225, 226, 227,
/*50*/	228, 229, 230, 231, 232, 233, 234, 235, 236, 237,
/*60*/	238, 239, 249, 248, 241, 242, 247,BERR,BERR,BERR,
/*70*/   70,  71,BERR,BERR,BERR,BERR,BERR,  46,BERR
};

notimp(p, uap, retval, code, nargs)
	struct proc *p;
	int *uap, *retval;
	int code, nargs;
{
	int error = 0;
#ifdef DEBUG
	register int *argp = uap;
	extern char *hpux_syscallnames[];

	printf("HPUX %s(", hpux_syscallnames[code]);
	if (nargs)
		while (nargs--)
			printf("%x%c", *argp++, nargs? ',' : ')');
	else
		printf(")");
	printf("\n");
	switch (unimpresponse) {
	case 0:
		error = nosys(p, uap, retval);
		break;
	case 1:
		error = EINVAL;
		break;
	}
#else
	error = nosys(p, uap, retval);
#endif
	uprintf("HP-UX system call %d not implemented\n", code);
	return (error);
}

struct hpux_execv_args {
	char	*fname;
	char	**argp;
	char	**envp;
};

hpux_execv(p, uap, retval)
	struct proc *p;
	struct hpux_execv_args *uap;
	int *retval;
{
	extern int execve();

	uap->envp = NULL;
	return (execve(p, uap, retval));
}

/*
 * HPUX versions of wait and wait3 actually pass the parameters
 * (status pointer, options, rusage) into the kernel rather than
 * handling it in the C library stub.  We also need to map any
 * termination signal from BSD to HPUX.
 */
struct hpux_wait3_args {
	int	*status;
	int	options;
	int	rusage;
};

hpux_wait3(p, uap, retval)
	struct proc *p;
	struct hpux_wait3_args *uap;
	int *retval;
{
	/* rusage pointer must be zero */
	if (uap->rusage)
		return (EINVAL);
	p->p_regs[PS] = PSL_ALLCC;
	p->p_regs[R0] = uap->options;
	p->p_regs[R1] = uap->rusage;
	return (hpux_wait(p, uap, retval));
}

struct hpux_wait_args {
	int	*status;
};

hpux_wait(p, uap, retval)
	struct proc *p;
	struct hpux_wait_args *uap;
	int *retval;
{
	int sig, *statp, error;

	statp = uap->status;	/* owait clobbers first arg */
	error = owait(p, uap, retval);
	/*
	 * HP-UX wait always returns EINTR when interrupted by a signal
	 * (well, unless its emulating a BSD process, but we don't bother...)
	 */
	if (error == ERESTART)
		error = EINTR;
	if (error)
		return (error);
	sig = retval[1] & 0xFF;
	if (sig == WSTOPPED) {
		sig = (retval[1] >> 8) & 0xFF;
		retval[1] = (bsdtohpuxsig(sig) << 8) | WSTOPPED;
	} else if (sig)
		retval[1] = (retval[1] & 0xFF00) |
			bsdtohpuxsig(sig & 0x7F) | (sig & 0x80);
	if (statp)
		if (suword((caddr_t)statp, retval[1]))
			error = EFAULT;
	return (error);
}

struct hpux_waitpid_args {
	int	pid;
	int	*status;
	int	options;
	struct	rusage *rusage;	/* wait4 arg */
};

hpux_waitpid(p, uap, retval)
	struct proc *p;
	struct hpux_waitpid_args *uap;
	int *retval;
{
	int sig, *statp, error;

	uap->rusage = 0;
	error = wait4(p, uap, retval);
	/*
	 * HP-UX wait always returns EINTR when interrupted by a signal
	 * (well, unless its emulating a BSD process, but we don't bother...)
	 */
	if (error == ERESTART)
		error = EINTR;
	if (error)
		return (error);
	sig = retval[1] & 0xFF;
	if (sig == WSTOPPED) {
		sig = (retval[1] >> 8) & 0xFF;
		retval[1] = (bsdtohpuxsig(sig) << 8) | WSTOPPED;
	} else if (sig)
		retval[1] = (retval[1] & 0xFF00) |
			bsdtohpuxsig(sig & 0x7F) | (sig & 0x80);
	if (statp)
		if (suword((caddr_t)statp, retval[1]))
			error = EFAULT;
	return (error);
}

/*
 * Must remap some bits in the mode mask.
 * O_CREAT, O_TRUNC, and O_EXCL must be remapped,
 * O_SYNCIO (0100000) is removed entirely.
 */
struct hpux_open_args {
	char	*fname;
	int	mode;
	int	crtmode;
};

hpux_open(p, uap, retval)
	struct proc *p;
	register struct hpux_open_args *uap;
	int *retval;
{
	int mode;

	mode = uap->mode;
	uap->mode &= ~(HPUXFSYNCIO|HPUXFEXCL|HPUXFTRUNC|HPUXFCREAT);
	if (mode & HPUXFCREAT) {
		/*
		 * simulate the pre-NFS behavior that opening a
		 * file for READ+CREATE ignores the CREATE (unless
		 * EXCL is set in which case we will return the
		 * proper error).
		 */
		if ((mode & HPUXFEXCL) || (FFLAGS(mode) & FWRITE))
			uap->mode |= O_CREAT;
	}
	if (mode & HPUXFTRUNC)
		uap->mode |= O_TRUNC;
	if (mode & HPUXFEXCL)
		uap->mode |= O_EXCL;
	return (open(p, uap, retval));
}

/* XXX */
#define	UF_FNDELAY_ON	0x20
#define	UF_FIONBIO_ON	0x40
/* XXX */

struct hpux_fcntl_args {
	int	fdes;
	int	cmd;
	int	arg;
};

hpux_fcntl(p, uap, retval)
	struct proc *p;
	register struct hpux_fcntl_args *uap;
	int *retval;
{
	int mode, error;
	char *fp;

	if (uap->cmd == F_GETFL || uap->cmd == F_SETFL) {
		if ((unsigned)uap->fdes >= p->p_fd->fd_nfiles ||
		    p->p_fd->fd_ofiles[uap->fdes] == NULL)
			return (EBADF);
		fp = &p->p_fd->fd_ofileflags[uap->fdes];
	}
	switch (uap->cmd) {
	case F_SETFL:
		if (uap->arg & FNONBLOCK)
			*fp |= UF_FNDELAY_ON;
		else {
			*fp &= ~UF_FNDELAY_ON;
			if (*fp & UF_FIONBIO_ON)
				uap->arg |= FNONBLOCK;
		}
		uap->arg &= ~(HPUXFSYNCIO|HPUXFREMOTE|FUSECACHE);
		break;
	case F_GETFL:
	case F_DUPFD:
	case F_GETFD:
	case F_SETFD:
		break;
	default:
		return (EINVAL);
	}
	error = fcntl(p, uap, retval);
	if (error == 0 && uap->cmd == F_GETFL) {
		mode = *retval;
		*retval &= ~(O_CREAT|O_TRUNC|O_EXCL|FUSECACHE);
		if ((mode & FNONBLOCK) && (*fp & UF_FNDELAY_ON) == 0)
			*retval &= ~FNONBLOCK;
		if (mode & O_CREAT)
			*retval |= HPUXFCREAT;
		if (mode & O_TRUNC)
			*retval |= HPUXFTRUNC;
		if (mode & O_EXCL)
			*retval |= HPUXFEXCL;
	}
	return (error);
}

/*
 * Read and write should return a 0 count when an operation
 * on a VNODE would block, not an error.
 *
 * In 6.2 and 6.5 sockets appear to return EWOULDBLOCK.
 * In 7.0 the behavior for sockets depends on whether FNONBLOCK is in effect.
 */
struct hpux_read_args {
	int	fd;
};

hpux_read(p, uap, retval)
	struct proc *p;
	struct hpux_read_args *uap;
	int *retval;
{
	int error;

	error = read(p, uap, retval);
	if (error == EWOULDBLOCK &&
	    (p->p_fd->fd_ofiles[uap->fd]->f_type == DTYPE_VNODE ||
	     p->p_fd->fd_ofileflags[uap->fd] & UF_FNDELAY_ON)) {
		error = 0;
		*retval = 0;
	}
	return (error);
}

struct hpux_write_args {
	int	fd;
};

hpux_write(p, uap, retval)
	struct proc *p;
	struct hpux_write_args *uap;
	int *retval;
{
	int error;

	error = write(p, uap, retval);
	if (error == EWOULDBLOCK &&
	    (p->p_fd->fd_ofiles[uap->fd]->f_type == DTYPE_VNODE ||
	     p->p_fd->fd_ofileflags[uap->fd] & UF_FNDELAY_ON)) {
		error = 0;
		*retval = 0;
	}
	return (error);
}

struct hpux_readv_args {
	int	fd;
};

hpux_readv(p, uap, retval)
	struct proc *p;
	struct hpux_readv_args *uap;
	int *retval;
{
	int error;

	error = readv(p, uap, retval);
	if (error == EWOULDBLOCK &&
	    (p->p_fd->fd_ofiles[uap->fd]->f_type == DTYPE_VNODE ||
	     p->p_fd->fd_ofileflags[uap->fd] & UF_FNDELAY_ON)) {
		error = 0;
		*retval = 0;
	}
	return (error);
}

struct hpux_writev_args {
	int	fd;
};

hpux_writev(p, uap, retval)
	struct proc *p;
	struct hpux_writev_args *uap;
	int *retval;
{
	int error;

	error = writev(p, uap, retval);
	if (error == EWOULDBLOCK &&
	    (p->p_fd->fd_ofiles[uap->fd]->f_type == DTYPE_VNODE ||
	     p->p_fd->fd_ofileflags[uap->fd] & UF_FNDELAY_ON)) {
		error = 0;
		*retval = 0;
	}
	return (error);
}

/*
 * 4.3bsd dup allows dup2 to come in on the same syscall entry
 * and hence allows two arguments.  HPUX dup has only one arg.
 */
struct hpux_dup_args {
	int	i;
};

hpux_dup(p, uap, retval)
	struct proc *p;
	register struct hpux_dup_args *uap;
	int *retval;
{
	register struct filedesc *fdp = p->p_fd;
	struct file *fp;
	int fd, error;

	if (((unsigned)uap->i) >= fdp->fd_nfiles ||
	    (fp = fdp->fd_ofiles[uap->i]) == NULL)
		return (EBADF);
	if (error = fdalloc(p, 0, &fd))
		return (error);
	fdp->fd_ofiles[fd] = fp;
	fdp->fd_ofileflags[fd] = fdp->fd_ofileflags[uap->i] &~ UF_EXCLOSE;
	fp->f_count++;
	if (fd > fdp->fd_lastfile)
		fdp->fd_lastfile = fd;
	*retval = fd;
	return (0);
}

struct hpux_utssys_args {
	struct hpux_utsname *uts;
	int dev;
	int request;
};

hpux_utssys(p, uap, retval)
	struct proc *p;
	register struct hpux_utssys_args *uap;
	int *retval;
{
	register int i;
	int error;

	switch (uap->request) {
	/* uname */
	case 0:
		/* fill in machine type */
		switch (machineid) {
		case HP_320:
			protoutsname.machine[6] = '2';
			break;
		/* includes 318 and 319 */
		case HP_330:
			protoutsname.machine[6] = '3';
			break;
		case HP_340:
			protoutsname.machine[6] = '4';
			break;
		case HP_350:
			protoutsname.machine[6] = '5';
			break;
		case HP_360:
			protoutsname.machine[6] = '6';
			break;
		case HP_370:
			protoutsname.machine[6] = '7';
			break;
		/* includes 345 */
		case HP_375:
			protoutsname.machine[6] = '7';
			protoutsname.machine[7] = '5';
			break;
		}
		/* copy hostname (sans domain) to nodename */
		for (i = 0; i < 8 && hostname[i] != '.'; i++)
			protoutsname.nodename[i] = hostname[i];
		protoutsname.nodename[i] = '\0';
		error = copyout((caddr_t)&protoutsname, (caddr_t)uap->uts,
				sizeof(struct hpux_utsname));
		break;

	/* gethostname */
	case 5:
		/* uap->dev is length */
		if (uap->dev > hostnamelen + 1)
			uap->dev = hostnamelen + 1;
		error = copyout((caddr_t)hostname, (caddr_t)uap->uts,
				uap->dev);
		break;

	case 1:	/* ?? */
	case 2:	/* ustat */
	case 3:	/* ?? */
	case 4:	/* sethostname */
	default:
		error = EINVAL;
		break;
	}
	return (error);
}

struct hpux_stat_args {
	char	*fname;
	struct hpux_stat *hsb;
};

hpux_stat(p, uap, retval)
	struct proc *p;
	struct hpux_stat_args *uap;
	int *retval;
{
	return (hpux_stat1(uap->fname, uap->hsb, FOLLOW));
}

struct hpux_lstat_args {
	char	*fname;
	struct hpux_stat *hsb;
};

hpux_lstat(p, uap, retval)
	struct proc *p;
	struct hpux_lstat_args *uap;
	int *retval;
{
	return (hpux_stat1(uap->fname, uap->hsb, NOFOLLOW));
}

struct hpux_fstat_args {
	int	fdes;
	struct	hpux_stat *hsb;
};

hpux_fstat(p, uap, retval)
	struct proc *p;
	register struct hpux_fstat_args *uap;
	int *retval;
{
	register struct filedesc *fdp = p->p_fd;
	register struct file *fp;
	struct stat sb;
	int error;

	if (((unsigned)uap->fdes) >= fdp->fd_nfiles ||
	    (fp = fdp->fd_ofiles[uap->fdes]) == NULL)
		return (EBADF);

	switch (fp->f_type) {

	case DTYPE_VNODE:
		error = vn_stat((struct vnode *)fp->f_data, &sb);
		break;

	case DTYPE_SOCKET:
		error = soo_stat((struct socket *)fp->f_data, &sb);
		break;

	default:
		panic("fstat");
		/*NOTREACHED*/
	}
	/* is this right for sockets?? */
	if (error == 0)
		error = bsdtohpuxstat(&sb, uap->hsb);
	return (error);
}

struct hpux_ulimit_args {
	int	cmd;
	long	newlimit;
};

hpux_ulimit(p, uap, retval)
	struct proc *p;
	register struct hpux_ulimit_args *uap;
	off_t *retval;
{
	struct rlimit *limp;
	int error = 0;

	limp = &p->p_rlimit[RLIMIT_FSIZE];
	switch (uap->cmd) {
	case 2:
		uap->newlimit *= 512;
		if (uap->newlimit > limp->rlim_max &&
		    (error = suser(p->p_ucred, &p->p_acflag)))
			break;
		limp->rlim_cur = limp->rlim_max = uap->newlimit;
		/* else fall into... */

	case 1:
		*retval = limp->rlim_max / 512;
		break;

	case 3:
		limp = &p->p_rlimit[RLIMIT_DATA];
		*retval = ctob(p->p_vmspace->vm_tsize) + limp->rlim_max;
		break;

	default:
		error = EINVAL;
		break;
	}
	return (error);
}

/*
 * Map "real time" priorities 0 (high) thru 127 (low) into nice
 * values -16 (high) thru -1 (low).
 */
struct hpux_rtprio_args {
	int pid;
	int prio;
};

hpux_rtprio(cp, uap, retval)
	struct proc *cp;
	register struct hpux_rtprio_args *uap;
	int *retval;
{
	struct proc *p;
	int nice, error;

	if (uap->prio < RTPRIO_MIN && uap->prio > RTPRIO_MAX &&
	    uap->prio != RTPRIO_NOCHG && uap->prio != RTPRIO_RTOFF)
		return (EINVAL);
	if (uap->pid == 0)
		p = cp;
	else if ((p = pfind(uap->pid)) == 0)
		return (ESRCH);
	nice = p->p_nice;
	if (nice < NZERO)
		*retval = (nice + 16) << 3;
	else
		*retval = RTPRIO_RTOFF;
	switch (uap->prio) {

	case RTPRIO_NOCHG:
		return (0);

	case RTPRIO_RTOFF:
		if (nice >= NZERO)
			return (0);
		nice = NZERO;
		break;

	default:
		nice = (uap->prio >> 3) - 16;
		break;
	}
	error = donice(cp, p, nice);
	if (error == EACCES)
		error = EPERM;
	return (error);
}

struct hpux_advise_args {
	int	arg;
};

hpux_advise(p, uap, retval)
	struct proc *p;
	struct hpux_advise_args *uap;
	int *retval;
{
	int error = 0;

	switch (uap->arg) {
	case 0:
		p->p_addr->u_pcb.pcb_flags |= PCB_HPUXMMAP;
		break;
	case 1:
		ICIA();
		break;
	case 2:
		DCIA();
		break;
	default:
		error = EINVAL;
		break;
	}
	return (error);
}

struct hpux_ptrace_args {
	int	req;
	int	pid;
	int	*addr;
	int	data;
};

hpux_ptrace(p, uap, retval)
	struct proc *p;
	struct hpux_ptrace_args *uap;
	int *retval;
{
	int error;

	if (uap->req == PT_STEP || uap->req == PT_CONTINUE) {
		if (uap->data) {
			uap->data = hpuxtobsdsig(uap->data);
			if (uap->data == 0)
				uap->data = NSIG;
		}
	}
	error = ptrace(p, uap, retval);
	return (error);
}

/* convert from BSD to HPUX errno */
bsdtohpuxerrno(err)
	int err;
{
	if (err < 0 || err >= NERR)
		return(BERR);
	return((int)bsdtohpuxerrnomap[err]);
}

hpux_stat1(fname, hsb, follow)
	char *fname;
	struct hpux_stat *hsb;
	int follow;
{
	register struct nameidata *ndp;
	int error;
	struct stat sb;
	struct nameidata nd;

	ndp = &nd;
	ndp->ni_nameiop = LOOKUP | LOCKLEAF | follow;
	ndp->ni_segflg = UIO_USERSPACE;
	ndp->ni_dirp = fname;
	if (error = namei(ndp, curproc))
		return (error);
	error = vn_stat(ndp->ni_vp, &sb);
	vput(ndp->ni_vp);
	if (error == 0)
		error = bsdtohpuxstat(&sb, hsb);
	return (error);
}

#include "grf.h"

bsdtohpuxstat(sb, hsb)
	struct stat *sb;
	struct hpux_stat *hsb;
{
	struct hpux_stat ds;

	bzero((caddr_t)&ds, sizeof(ds));
	ds.hst_dev = sb->st_dev;
	ds.hst_ino = (u_long)sb->st_ino;
	ds.hst_mode = sb->st_mode;
	ds.hst_nlink = sb->st_nlink;
	ds.hst_uid = (u_short)sb->st_uid;
	ds.hst_gid = (u_short)sb->st_gid;
#if NGRF > 0
	/* XXX: I don't want to talk about it... */
	if ((sb->st_mode & S_IFMT) == S_IFCHR && major(sb->st_rdev) == 10)
		ds.hst_rdev = grfdevno(sb->st_rdev);
	else
#endif
		ds.hst_rdev = bsdtohpuxdev(sb->st_rdev);
	ds.hst_size = sb->st_size;
	ds.hst_atime = sb->st_atime;
	ds.hst_mtime = sb->st_mtime;
	ds.hst_ctime = sb->st_ctime;
	ds.hst_blksize = sb->st_blksize;
	ds.hst_blocks = sb->st_blocks;
	return(copyout((caddr_t)&ds, (caddr_t)hsb, sizeof(ds)));
}

hpuxtobsdioctl(com)
	int com;
{
	switch (com) {
	case HPUXTIOCSLTC:
		com = TIOCSLTC; break;
	case HPUXTIOCGLTC:
		com = TIOCGLTC; break;
	case HPUXTIOCSPGRP:
		com = TIOCSPGRP; break;
	case HPUXTIOCGPGRP:
		com = TIOCGPGRP; break;
	case HPUXTIOCLBIS:
		com = TIOCLBIS; break;
	case HPUXTIOCLBIC:
		com = TIOCLBIC; break;
	case HPUXTIOCLSET:
		com = TIOCLSET; break;
	case HPUXTIOCLGET:
		com = TIOCLGET; break;
	}
	return(com);
}

/*
 * HPUX ioctl system call.  The differences here are:
 *	IOC_IN also means IOC_VOID if the size portion is zero.
 *	no FIOCLEX/FIONCLEX/FIOASYNC/FIOGETOWN/FIOSETOWN
 *	the sgttyb struct is 2 bytes longer
 */
struct hpux_ioctl_args {
	int	fdes;
	int	cmd;
	caddr_t	cmarg;
};

hpux_ioctl(p, uap, retval)
	struct proc *p;
	register struct hpux_ioctl_args *uap;
	int *retval;
{
	register struct filedesc *fdp = p->p_fd;
	register struct file *fp;
	register int com, error;
	register u_int size;
	caddr_t memp = 0;
#define STK_PARAMS	128
	char stkbuf[STK_PARAMS];
	caddr_t data = stkbuf;

	com = uap->cmd;

	/* XXX */
	if (com == HPUXTIOCGETP || com == HPUXTIOCSETP)
		return (getsettty(p, uap->fdes, com, uap->cmarg));

	if (((unsigned)uap->fdes) >= fdp->fd_nfiles ||
	    (fp = fdp->fd_ofiles[uap->fdes]) == NULL)
		return (EBADF);
	if ((fp->f_flag & (FREAD|FWRITE)) == 0)
		return (EBADF);

	/*
	 * Interpret high order word to find
	 * amount of data to be copied to/from the
	 * user's address space.
	 */
	size = IOCPARM_LEN(com);
	if (size > IOCPARM_MAX)
		return (ENOTTY);
	if (size > sizeof (stkbuf)) {
		memp = (caddr_t)malloc((u_long)size, M_IOCTLOPS, M_WAITOK);
		data = memp;
	}
	if (com&IOC_IN) {
		if (size) {
			error = copyin(uap->cmarg, data, (u_int)size);
			if (error) {
				if (memp)
					free(memp, M_IOCTLOPS);
				return (error);
			}
		} else
			*(caddr_t *)data = uap->cmarg;
	} else if ((com&IOC_OUT) && size)
		/*
		 * Zero the buffer so the user always
		 * gets back something deterministic.
		 */
		bzero(data, size);
	else if (com&IOC_VOID)
		*(caddr_t *)data = uap->cmarg;

	switch (com) {

	case HPUXFIOSNBIO:
	{
		char *ofp = &fdp->fd_ofileflags[uap->fdes];
		int tmp;

		if (*(int *)data)
			*ofp |= UF_FIONBIO_ON;
		else
			*ofp &= ~UF_FIONBIO_ON;
		/*
		 * Only set/clear if FNONBLOCK not in effect
		 */
		if ((*ofp & UF_FNDELAY_ON) == 0) {
			tmp = fp->f_flag & FNONBLOCK;
			error = (*fp->f_ops->fo_ioctl)(fp, FIONBIO,
						       (caddr_t)&tmp, p);
		}
		break;
	}

	case HPUXTIOCCONS:
		*(int *)data = 1;
		error = (*fp->f_ops->fo_ioctl)(fp, TIOCCONS, data, p);
		break;

	/* BSD-style job control ioctls */
	case HPUXTIOCLBIS:
	case HPUXTIOCLBIC:
	case HPUXTIOCLSET:
		*(int *)data &= HPUXLTOSTOP;
		if (*(int *)data & HPUXLTOSTOP)
			*(int *)data = LTOSTOP;
		/* fall into */
	case HPUXTIOCLGET:
	case HPUXTIOCSLTC:
	case HPUXTIOCGLTC:
	case HPUXTIOCSPGRP:
	case HPUXTIOCGPGRP:
		error = (*fp->f_ops->fo_ioctl)
			(fp, hpuxtobsdioctl(com), data, p);
		if (error == 0 && com == HPUXTIOCLGET) {
			*(int *)data &= LTOSTOP;
			if (*(int *)data & LTOSTOP)
				*(int *)data = HPUXLTOSTOP;
		}
		break;

	/* SYS 5 termio */
	case HPUXTCGETA:
	case HPUXTCSETA:
	case HPUXTCSETAW:
	case HPUXTCSETAF:
		error = hpux_termio(fp, com, data, p);
		break;

	default:
		error = (*fp->f_ops->fo_ioctl)(fp, com, data, p);
		break;
	}
	/*
	 * Copy any data to user, size was
	 * already set and checked above.
	 */
	if (error == 0 && (com&IOC_OUT) && size)
		error = copyout(data, uap->cmarg, (u_int)size);
	if (memp)
		free(memp, M_IOCTLOPS);
	return (error);
}

/*
 * Man page lies, behaviour here is based on observed behaviour.
 */
struct hpux_getcontext_args {
	char *buf;
	int len;
};

hpux_getcontext(p, uap, retval)
	struct proc *p;
	struct hpux_getcontext_args *uap;
	int *retval;
{
	int error = 0;
	register int len;

	len = MIN(uap->len, sizeof(hpux_context));
	if (len)
		error = copyout(hpux_context, uap->buf, (u_int)len);
	if (error == 0)
		*retval = sizeof(hpux_context);
	return (error);
}

/*
 * This is the equivalent of BSD getpgrp but with more restrictions.
 * Note we do not check the real uid or "saved" uid.
 */
struct hpux_getpgrp2_args {
	int pid;
};

hpux_getpgrp2(cp, uap, retval)
	struct proc *cp;
	register struct hpux_getpgrp2_args *uap;
	int *retval;
{
	register struct proc *p;

	if (uap->pid == 0)
		uap->pid = cp->p_pid;
	p = pfind(uap->pid);
	if (p == 0)
		return (ESRCH);
	if (cp->p_ucred->cr_uid && p->p_ucred->cr_uid != cp->p_ucred->cr_uid &&
	    !inferior(p))
		return (EPERM);
	*retval = p->p_pgid;
	return (0);
}

/*
 * This is the equivalent of BSD setpgrp but with more restrictions.
 * Note we do not check the real uid or "saved" uid or pgrp.
 */
struct hpux_setpgrp2_args {
	int	pid;
	int	pgrp;
};

hpux_setpgrp2(p, uap, retval)
	struct proc *p;
	struct hpux_setpgrp2_args *uap;
	int *retval;
{
	/* empirically determined */
	if (uap->pgrp < 0 || uap->pgrp >= 30000)
		return (EINVAL);
	return (setpgid(p, uap, retval));
}

/*
 * XXX Same as BSD setre[ug]id right now.  Need to consider saved ids.
 */
struct hpux_setresuid_args {
	int	ruid;
	int	euid;
	int	suid;
};

hpux_setresuid(p, uap, retval)
	struct proc *p;
	struct hpux_setresuid_args *uap;
	int *retval;
{
	return (osetreuid(p, uap, retval));
}

struct hpux_setresgid_args {
	int	rgid;
	int	egid;
	int	sgid;
};

hpux_setresgid(p, uap, retval)
	struct proc *p;
	struct hpux_setresgid_args *uap;
	int *retval;
{
	return (osetregid(p, uap, retval));
}

/*
 * XXX: simple recognition hack to see if we can make grmd work.
 */
struct hpux_lockf_args {
	int fd;
	int func;
	long size;
};

hpux_lockf(p, uap, retval)
	struct proc *p;
	struct hpux_lockf_args *uap;
	int *retval;
{
#ifdef DEBUG
	log(LOG_DEBUG, "%d: lockf(%d, %d, %d)\n",
	    p->p_pid, uap->fd, uap->func, uap->size);
#endif
	return (0);
}

struct hpux_getaccess_args {
	char	*path;
	int	uid;
	int	ngroups;
	int	*gidset;
	void	*label;
	void	*privs;
};

hpux_getaccess(p, uap, retval)
	register struct proc *p;
	register struct hpux_getaccess_args *uap;
	int *retval;
{
	struct nameidata *ndp;
	int lgroups[NGROUPS];
	int error = 0;
	register struct ucred *cred;
	register struct vnode *vp;

	/*
	 * Build an appropriate credential structure
	 */
	cred = crdup(p->p_ucred);
	switch (uap->uid) {
	case 65502:	/* UID_EUID */
		break;
	case 65503:	/* UID_RUID */
		cred->cr_uid = p->p_cred->p_ruid;
		break;
	case 65504:	/* UID_SUID */
		error = EINVAL;
		break;
	default:
		if (uap->uid > 65504)
			error = EINVAL;
		cred->cr_uid = uap->uid;
		break;
	}
	switch (uap->ngroups) {
	case -1:	/* NGROUPS_EGID */
		cred->cr_ngroups = 1;
		break;
	case -5:	/* NGROUPS_EGID_SUPP */
		break;
	case -2:	/* NGROUPS_RGID */
		cred->cr_ngroups = 1;
		cred->cr_gid = p->p_cred->p_rgid;
		break;
	case -6:	/* NGROUPS_RGID_SUPP */
		cred->cr_gid = p->p_cred->p_rgid;
		break;
	case -3:	/* NGROUPS_SGID */
	case -7:	/* NGROUPS_SGID_SUPP */
		error = EINVAL;
		break;
	case -4:	/* NGROUPS_SUPP */
		if (cred->cr_ngroups > 1)
			cred->cr_gid = cred->cr_groups[1];
		else
			error = EINVAL;
		break;
	default:
		if (uap->ngroups > 0 && uap->ngroups <= NGROUPS)
			error = copyin((caddr_t)uap->gidset,
				       (caddr_t)&lgroups[0],
				       uap->ngroups * sizeof(lgroups[0]));
		else
			error = EINVAL;
		if (error == 0) {
			int gid;

			for (gid = 0; gid < uap->ngroups; gid++)
				cred->cr_groups[gid] = lgroups[gid];
			cred->cr_ngroups = uap->ngroups;
		}
		break;
	}
	/*
	 * Lookup file using caller's effective IDs.
	 */
	if (error == 0) {
		ndp->ni_nameiop = LOOKUP | FOLLOW | LOCKLEAF;
		ndp->ni_segflg = UIO_USERSPACE;
		ndp->ni_dirp = uap->path;
		error = namei(ndp, p);
	}
	if (error) {
		crfree(cred);
		return (error);
	}
	/*
	 * Use the constructed credentials for access checks.
	 */
	vp = ndp->ni_vp;
	*retval = 0;
	if (VOP_ACCESS(vp, VREAD, cred, p) == 0)
		*retval |= R_OK;
	if (vn_writechk(vp) == 0 && VOP_ACCESS(vp, VWRITE, cred, p) == 0)
		*retval |= W_OK;
	/* XXX we return X_OK for root on VREG even if not */
	if (VOP_ACCESS(vp, VEXEC, cred, p) == 0)
		*retval |= X_OK;
	vput(vp);
	crfree(cred);
	return (error);
}

/*
 * Brutal hack!  Map HPUX u-area offsets into BSD u offsets.
 * No apologies offered, if you don't like it, rewrite it!
 */

extern char kstack[];
#define UOFF(f)		((int)&((struct user *)0)->f)
#define HPUOFF(f)	((int)&((struct hpux_user *)0)->f)

/* simplified FP structure */
struct bsdfp {
	int save[54];
	int reg[24];
	int ctrl[3];
};

hpuxtobsduoff(off)
	int *off;
{
	register int *ar0 = curproc->p_regs;
	struct hpuxfp *hp;
	struct bsdfp *bp;
	register u_int raddr;

	/* u_ar0 field; procxmt puts in U_ar0 */
	if ((int)off == HPUOFF(hpuxu_ar0))
		return(UOFF(U_ar0));

#ifdef FPCOPROC
	/* 68881 registers from PCB */
	hp = (struct hpuxfp *)HPUOFF(hpuxu_fp);
	bp = (struct bsdfp *)UOFF(u_pcb.pcb_fpregs);
	if (off >= hp->hpfp_ctrl && off < &hp->hpfp_ctrl[3])
		return((int)&bp->ctrl[off - hp->hpfp_ctrl]);
	if (off >= hp->hpfp_reg && off < &hp->hpfp_reg[24])
		return((int)&bp->reg[off - hp->hpfp_reg]);
#endif

	/*
	 * Everything else we recognize comes from the kernel stack,
	 * so we convert off to an absolute address (if not already)
	 * for simplicity.
	 */
	if (off < (int *)ctob(UPAGES))
		off = (int *)((u_int)off + (u_int)kstack);

	/*
	 * 68020 registers.
	 * We know that the HPUX registers are in the same order as ours.
	 * The only difference is that their PS is 2 bytes instead of a
	 * padded 4 like ours throwing the alignment off.
	 */
	if (off >= ar0 && off < &ar0[18]) {
		/*
		 * PS: return low word and high word of PC as HP-UX would
		 * (e.g. &u.u_ar0[16.5]).
		 */
		if (off == &ar0[PS])
			raddr = (u_int) &((short *)ar0)[PS*2+1];
		/*
		 * PC: off will be &u.u_ar0[16.5]
		 */
		else if (off == (int *)&(((short *)ar0)[PS*2+1]))
			raddr = (u_int) &ar0[PC];
		/*
		 * D0-D7, A0-A7: easy
		 */
		else
			raddr = (u_int) &ar0[(int)(off - ar0)];
		return((int)(raddr - (u_int)kstack));
	}

	/* everything else */
	return(-1);
}

/*
 * Kludge up a uarea dump so that HPUX debuggers can find out
 * what they need.  IMPORTANT NOTE: we do not EVEN attempt to
 * convert the entire user struct.
 */
hpuxdumpu(vp, cred)
	struct vnode *vp;
	struct ucred *cred;
{
	struct proc *p = curproc;
	int error;
	struct hpux_user *faku;
	struct bsdfp *bp;
	short *foop;

	faku = (struct hpux_user *)malloc((u_long)ctob(1), M_TEMP, M_WAITOK);
	/*
	 * Make sure there is no mistake about this
	 * being a real user structure.
	 */
	bzero((caddr_t)faku, ctob(1));
	/*
	 * Fill in the process sizes.
	 */
	faku->hpuxu_tsize = p->p_vmspace->vm_tsize;
	faku->hpuxu_dsize = p->p_vmspace->vm_dsize;
	faku->hpuxu_ssize = p->p_vmspace->vm_ssize;
	/*
	 * Fill in the exec header for CDB.
	 * This was saved back in exec().  As far as I can tell CDB
	 * only uses this information to verify that a particular
	 * core file goes with a particular binary.
	 */
	bcopy((caddr_t)p->p_addr->u_pcb.pcb_exec,
	      (caddr_t)&faku->hpuxu_exdata, sizeof (struct hpux_exec));
	/*
	 * Adjust user's saved registers (on kernel stack) to reflect
	 * HPUX order.  Note that HPUX saves the SR as 2 bytes not 4
	 * so we have to move it up.
	 */
	faku->hpuxu_ar0 = p->p_regs;
	foop = (short *) p->p_regs;
	foop[32] = foop[33];
	foop[33] = foop[34];
	foop[34] = foop[35];
#ifdef FPCOPROC
	/*
	 * Copy 68881 registers from our PCB format to HPUX format
	 */
	bp = (struct bsdfp *) &p->p_addr->u_pcb.pcb_fpregs;
	bcopy((caddr_t)bp->save, (caddr_t)faku->hpuxu_fp.hpfp_save,
	      sizeof(bp->save));
	bcopy((caddr_t)bp->ctrl, (caddr_t)faku->hpuxu_fp.hpfp_ctrl,
	      sizeof(bp->ctrl));
	bcopy((caddr_t)bp->reg, (caddr_t)faku->hpuxu_fp.hpfp_reg,
	      sizeof(bp->reg));
#endif
	/*
	 * Slay the dragon
	 */
	faku->hpuxu_dragon = -1;
	/*
	 * Dump this artfully constructed page in place of the
	 * user struct page.
	 */
	error = vn_rdwr(UIO_WRITE, vp, (caddr_t)faku, ctob(1), (off_t)0,
			UIO_SYSSPACE, IO_NODELOCKED|IO_UNIT, cred,
			(int *)NULL, p);
	/*
	 * Dump the remaining UPAGES-1 pages normally
	 */
	if (!error) 
		error = vn_rdwr(UIO_WRITE, vp, kstack + ctob(1),
				ctob(UPAGES-1), (off_t)ctob(1), UIO_SYSSPACE,
				IO_NODELOCKED|IO_UNIT, cred, (int *)NULL, p);
	free((caddr_t)faku, M_TEMP);
	return(error);
}

/*
 * The remaining routines are essentially the same as those in kern_xxx.c
 * and vfs_xxx.c as defined under "#ifdef COMPAT".  We replicate them here
 * to avoid HPUXCOMPAT dependencies in those files and to make sure that
 * HP-UX compatibility still works even when COMPAT is not defined.
 */
/* #ifdef COMPAT */

#define HPUX_HZ	50

#include "sys/times.h"

/* from old timeb.h */
struct hpux_timeb {
	time_t	time;
	u_short	millitm;
	short	timezone;
	short	dstflag;
};

/* ye ole stat structure */
struct	ohpux_stat {
	dev_t	ohst_dev;
	u_short	ohst_ino;
	u_short ohst_mode;
	short  	ohst_nlink;
	short  	ohst_uid;
	short  	ohst_gid;
	dev_t	ohst_rdev;
	int	ohst_size;
	int	ohst_atime;
	int	ohst_mtime;
	int	ohst_ctime;
};

/*
 * SYS V style setpgrp()
 */
ohpux_setpgrp(p, uap, retval)
	register struct proc *p;
	int *uap, *retval;
{
	if (p->p_pid != p->p_pgid)
		enterpgrp(p, p->p_pid, 0);
	*retval = p->p_pgid;
	return (0);
}

struct ohpux_time_args {
	long	*tp;
};

ohpux_time(p, uap, retval)
	struct proc *p;
	register struct ohpux_time_args *uap;
	time_t *retval;
{
	int error = 0;

	if (uap->tp)
		error = copyout((caddr_t)&time.tv_sec, (caddr_t)uap->tp,
				sizeof (long));
	*retval = time.tv_sec;
	return (error);
}

struct ohpux_stime_args {
	int	time;
};

ohpux_stime(p, uap, retval)
	struct proc *p;
	register struct ohpux_stime_args *uap;
	int *retval;
{
	struct timeval tv;
	int s, error;

	tv.tv_sec = uap->time;
	tv.tv_usec = 0;
	if (error = suser(p->p_ucred, &p->p_acflag))
		return (error);

	/* WHAT DO WE DO ABOUT PENDING REAL-TIME TIMEOUTS??? */
	boottime.tv_sec += tv.tv_sec - time.tv_sec;
	s = splhigh(); time = tv; splx(s);
	resettodr();
	return (0);
}

struct ohpux_ftime_args {
	struct	hpux_timeb *tp;
};

ohpux_ftime(p, uap, retval)
	struct proc *p;
	register struct ohpux_ftime_args *uap;
	int *retval;
{
	struct hpux_timeb tb;
	int s;

	s = splhigh();
	tb.time = time.tv_sec;
	tb.millitm = time.tv_usec / 1000;
	splx(s);
	tb.timezone = tz.tz_minuteswest;
	tb.dstflag = tz.tz_dsttime;
	return (copyout((caddr_t)&tb, (caddr_t)uap->tp, sizeof (tb)));
}

struct ohpux_alarm_args {
	int	deltat;
};

ohpux_alarm(p, uap, retval)
	register struct proc *p;
	register struct ohpux_alarm_args *uap;
	int *retval;
{
	int s = splhigh();

	untimeout(realitexpire, (caddr_t)p);
	timerclear(&p->p_realtimer.it_interval);
	*retval = 0;
	if (timerisset(&p->p_realtimer.it_value) &&
	    timercmp(&p->p_realtimer.it_value, &time, >))
		*retval = p->p_realtimer.it_value.tv_sec - time.tv_sec;
	if (uap->deltat == 0) {
		timerclear(&p->p_realtimer.it_value);
		splx(s);
		return (0);
	}
	p->p_realtimer.it_value = time;
	p->p_realtimer.it_value.tv_sec += uap->deltat;
	timeout(realitexpire, (caddr_t)p, hzto(&p->p_realtimer.it_value));
	splx(s);
	return (0);
}

struct ohpux_nice_args {
	int	niceness;
};

ohpux_nice(p, uap, retval)
	register struct proc *p;
	register struct ohpux_nice_args *uap;
	int *retval;
{
	int error;

	error = donice(p, p, (p->p_nice-NZERO)+uap->niceness);
	if (error == 0)
		*retval = p->p_nice - NZERO;
	return (error);
}

struct ohpux_times_args {
	struct	tms *tmsb;
};

ohpux_times(p, uap, retval)
	struct proc *p;
	register struct ohpux_times_args *uap;
	time_t *retval;
{
	struct tms atms;
	int error;

	atms.tms_utime = hpux_scale(&p->p_utime);
	atms.tms_stime = hpux_scale(&p->p_stime);
	atms.tms_cutime = hpux_scale(&p->p_stats->p_cru.ru_utime);
	atms.tms_cstime = hpux_scale(&p->p_stats->p_cru.ru_stime);
	error = copyout((caddr_t)&atms, (caddr_t)uap->tmsb, sizeof (atms));
	if (error == 0)
		*retval = hpux_scale(&time) - hpux_scale(&boottime);
	return (error);
}

/*
 * Doesn't exactly do what the documentation says.
 * What we really do is return 1/HPUX_HZ-th of a second since that
 * is what HP-UX returns.
 */
hpux_scale(tvp)
	register struct timeval *tvp;
{
	return (tvp->tv_sec * HPUX_HZ + tvp->tv_usec * HPUX_HZ / 1000000);
}

/*
 * Set IUPD and IACC times on file.
 * Can't set ICHG.
 */
struct ohpux_utime_args {
	char	*fname;
	time_t	*tptr;
};

ohpux_utime(p, uap, retval)
	struct proc *p;
	register struct ohpux_utime_args *uap;
	int *retval;
{
	register struct vnode *vp;
	register struct nameidata *ndp;
	struct vattr vattr;
	time_t tv[2];
	int error;
	struct nameidata nd;

	ndp = &nd;
	if (uap->tptr) {
		error = copyin((caddr_t)uap->tptr, (caddr_t)tv, sizeof (tv));
		if (error)
			return (error);
	} else
		tv[0] = tv[1] = time.tv_sec;
	ndp->ni_nameiop = LOOKUP | FOLLOW | LOCKLEAF;
	ndp->ni_segflg = UIO_USERSPACE;
	ndp->ni_dirp = uap->fname;
	vattr_null(&vattr);
	vattr.va_atime.tv_sec = tv[0];
	vattr.va_atime.tv_usec = 0;
	vattr.va_mtime.tv_sec = tv[1];
	vattr.va_mtime.tv_usec = 0;
	if (error = namei(ndp, p))
		return (error);
	vp = ndp->ni_vp;
	if (vp->v_mount->mnt_flag & MNT_RDONLY)
		error = EROFS;
	else
		error = VOP_SETATTR(vp, &vattr, ndp->ni_cred, p);
	vput(vp);
	return (error);
}

ohpux_pause(p, uap, retval)
	struct proc *p;
	int *uap, *retval;
{
	(void) tsleep(kstack, PPAUSE | PCATCH, "pause", 0);
	/* always return EINTR rather than ERESTART... */
	return (EINTR);
}

/*
 * The old fstat system call.
 */
struct ohpux_fstat_args {
	int	fd;
	struct ohpux_stat *sb;
};

ohpux_fstat(p, uap, retval)
	struct proc *p;
	register struct ohpux_fstat_args *uap;
	int *retval;
{
	register struct filedesc *fdp = p->p_fd;
	struct file *fp;

	if (((unsigned)uap->fd) >= fdp->fd_nfiles ||
	    (fp = fdp->fd_ofiles[uap->fd]) == NULL)
		return (EBADF);
	if (fp->f_type != DTYPE_VNODE)
		return (EINVAL);
	return (ohpux_stat1((struct vnode *)fp->f_data, uap->sb));
}

/*
 * Old stat system call.  This version follows links.
 */
struct ohpux_stat_args {
	char	*fname;
	struct ohpux_stat *sb;
};

ohpux_stat(p, uap, retval)
	struct proc *p;
	register struct ohpux_stat_args *uap;
	int *retval;
{
	register struct nameidata *ndp;
	int error;
	struct nameidata nd;

	ndp = &nd;
	ndp->ni_nameiop = LOOKUP | LOCKLEAF | FOLLOW;
	ndp->ni_segflg = UIO_USERSPACE;
	ndp->ni_dirp = uap->fname;
	if (error = namei(ndp, p))
		return (error);
	error = ohpux_stat1(ndp->ni_vp, uap->sb);
	vput(ndp->ni_vp);
	return (error);
}

int
ohpux_stat1(vp, ub)
	register struct vnode *vp;
	struct ohpux_stat *ub;
{
	struct ohpux_stat ds;
	struct vattr vattr;
	register int error;

	error = VOP_GETATTR(vp, &vattr, curproc->p_ucred, curproc);
	if (error)
		return(error);
	/*
	 * Copy from inode table
	 */
	ds.ohst_dev = vattr.va_fsid;
	ds.ohst_ino = (short)vattr.va_fileid;
	ds.ohst_mode = (u_short)vattr.va_mode;
	ds.ohst_nlink = vattr.va_nlink;
	ds.ohst_uid = (short)vattr.va_uid;
	ds.ohst_gid = (short)vattr.va_gid;
	ds.ohst_rdev = (dev_t)vattr.va_rdev;
	ds.ohst_size = (int)vattr.va_size;
	ds.ohst_atime = (int)vattr.va_atime.tv_sec;
	ds.ohst_mtime = (int)vattr.va_mtime.tv_sec;
	ds.ohst_ctime = (int)vattr.va_ctime.tv_sec;
	return (copyout((caddr_t)&ds, (caddr_t)ub, sizeof(ds)));
}
/* #endif */

#endif
