/*
 * Copyright (c) 1994 Scott Bartram
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
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
 * from: Header: sun_misc.c,v 1.16 93/04/07 02:46:27 torek Exp 
 *
 *	@(#)sun_misc.c	8.1 (Berkeley) 6/18/93
 *	$Id: ibcs2_misc.c,v 1.2 1994/09/05 01:29:03 mycroft Exp $
 */

/*
 * IBCS2 compatibility module.
 *
 * IBCS2 system calls that are implemented differently in BSD are
 * handled here.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/dir.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/filedesc.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/exec.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/mman.h>
#include <sys/mount.h>
#include <sys/reboot.h>
#include <sys/resource.h>
#include <sys/resourcevar.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/vnode.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <sys/utsname.h>
#include <sys/unistd.h>

#include <netinet/in.h>

#include <miscfs/specfs/specdev.h>

#include <vm/vm.h>
#include <sys/sysctl.h>		/* must be included after vm.h */

#include <i386/include/reg.h>

#include <compat/ibcs2/ibcs2_types.h>
#include <compat/ibcs2/ibcs2_dirent.h>
#include <compat/ibcs2/ibcs2_fcntl.h>
#include <compat/ibcs2/ibcs2_signal.h>
#include <compat/ibcs2/ibcs2_stat.h>
#include <compat/ibcs2/ibcs2_statfs.h>
#include <compat/ibcs2/ibcs2_time.h>
#include <compat/ibcs2/ibcs2_unistd.h>
#include <compat/ibcs2/ibcs2_ustat.h>
#include <compat/ibcs2/ibcs2_utsname.h>

#ifdef DEBUG_IBCS2
#define DPRINTF(s)      printf s
#else
#define DPRINTF(s)
#endif

#define	szsigcode	(esigcode - sigcode)
#define STACK_ALLOC()	ALIGN(PS_STRINGS - szsigcode - STACKGAPLEN)
#define NOSIG		(-1)

extern char sigcode[], esigcode[];

static int ibcs2bsd_sigtbl[IBCS2_NSIG] = {
	NOSIG,			/* 0 */
	SIGHUP,			/* 1 */
	SIGINT,			/* 2 */
	SIGQUIT,		/* 3 */
	SIGILL,			/* 4 */
	SIGTRAP,		/* 5 */
	SIGABRT,		/* 6 */
	SIGEMT,			/* 7 */
	SIGFPE,			/* 8 */
	SIGKILL,		/* 9 */
	SIGBUS,			/* 10 */
	SIGSEGV,		/* 11 */
	SIGSYS,			/* 12 */
	SIGPIPE,		/* 13 */
	SIGALRM,		/* 14 */
	SIGTERM,		/* 15 */
	SIGUSR1,		/* 16 */
	SIGUSR2,		/* 17 */
	SIGCHLD,		/* 18 */
	NOSIG,			/* 19 - SIGPWR */
	SIGWINCH,		/* 20 */
	NOSIG,			/* 21 */
	NOSIG,			/* 22 - SIGPOLL */
	SIGSTOP,		/* 23 */
	SIGTSTP,		/* 24 */
	SIGCONT,		/* 25 */
	SIGTTIN,		/* 26 */
	SIGTTOU,		/* 27 */
	NOSIG,			/* 28 */
	NOSIG,			/* 29 */
	NOSIG,			/* 30 */
	NOSIG,			/* 31 */
};

static int bsd2ibcs_sigtbl[NSIG] = {
	NOSIG,			/* 0 */
	IBCS2_SIGHUP,		/* 1 */
	IBCS2_SIGINT,		/* 2 */
	IBCS2_SIGQUIT,		/* 3 */
	IBCS2_SIGILL,		/* 4 */
	IBCS2_SIGTRAP,		/* 5 */
	IBCS2_SIGABRT,		/* 6 */
	IBCS2_SIGEMT,		/* 7 */
	IBCS2_SIGFPE,		/* 8 */
	IBCS2_SIGKILL,		/* 9 */
	IBCS2_SIGBUS,		/* 10 */
	IBCS2_SIGSEGV,		/* 11 */
	IBCS2_SIGSYS,		/* 12 */
	IBCS2_SIGPIPE,		/* 13 */
	IBCS2_SIGALRM,		/* 14 */
	IBCS2_SIGTERM,		/* 15 */
	NOSIG,			/* 16 */
	IBCS2_SIGSTOP,		/* 17 */
	IBCS2_SIGTSTP,		/* 18 */
	IBCS2_SIGCONT,		/* 19 */
	IBCS2_SIGCLD,		/* 20 */
	IBCS2_SIGTTIN,		/* 21 */
	IBCS2_SIGTTOU,		/* 22 */
	NOSIG,			/* 23 */
	NOSIG,			/* 24 */
	NOSIG,			/* 25 */
	NOSIG,			/* 26 */
	NOSIG,			/* 27 */
	IBCS2_SIGWINCH,		/* 28 */
	NOSIG,			/* 29 */
	IBCS2_SIGUSR1,		/* 30 */
	IBCS2_SIGUSR2,		/* 31 */
};

static int
ibcs2bsd_sig(sig)
	int sig;
{
	if (sig < 1 || sig >= IBCS2_NSIG)
		return NOSIG;
	else
		return ibcs2bsd_sigtbl[sig];
}

int
bsd2ibcs_sig(sig)
	int sig;
{
	if (sig < 1 || sig >= NSIG)
		return NOSIG;
	else
		return bsd2ibcs_sigtbl[sig];
}

static sigset_t
cvt_sigmask(mask, sigtbl)
	sigset_t mask;
	int sigtbl[];
{
	int i, newmask;

	for (i = 0, newmask = 0; i < NSIG; i++)
		if ((sigtbl[i] != NOSIG) && (mask & (1 << i)))
			newmask |= (1 << (sigtbl[i] - 1));
	return newmask;
}

struct ibcs2_sigsys_args {
	int sig;
	void (*fp)();
};

int
ibcs2_sigsys(p, uap, retval)
	struct proc *p;
	struct ibcs2_sigsys_args *uap;
	int *retval;
{
	int error;
	int nsig = ibcs2bsd_sig(IBCS2_SIGNO(uap->sig));

	if (nsig  == NOSIG) {
		if (IBCS2_SIGCALL(uap->sig) == IBCS2_SIGNAL_MASK
		    || IBCS2_SIGCALL(uap->sig) == IBCS2_SIGSET_MASK)
			*retval = (int)IBCS2_SIG_ERR;
		return EINVAL;
	}
	
	switch (IBCS2_SIGCALL(uap->sig)) {
/*
 * sigset is identical to signal() except that SIG_HOLD is allowed as
 * an action and we don't set the bit in the ibcs_sigflags field.
 */
	case IBCS2_SIGSET_MASK:
		if (uap->fp == IBCS2_SIG_HOLD) {
			struct sigprocmask_args {
				int     how;
				sigset_t mask;
			} sa;

			sa.how = SIG_BLOCK;
			sa.mask = sigmask(nsig);
			return sigprocmask(p, &sa, retval);
		}
		/* else fall through */

	case IBCS2_SIGNAL_MASK:
		{
			struct sigaction *sap, *osap;
			struct sigaction_args {
				int sig;
				struct sigaction *sa;
				struct sigaction *osa;
			} sa_args;

			sap = (struct sigaction *)STACK_ALLOC();
			osap = &sap[1];
			sa_args.sig = nsig;
			sa_args.sa = sap;
			sa_args.osa = osap;
			sa_args.sa->sa_handler = uap->fp;
			sa_args.sa->sa_mask = (sigset_t)0;
			sa_args.sa->sa_flags = 0;
#if 0
			if (sa_args.sig != SIGALRM)
				sa_args.sa->sa_flags = SA_RESTART;
#endif
			error = sigaction(p, &sa_args, retval);
			if (error) {
				DPRINTF(("signal: sigaction failed: %d\n",
					 error));
				*retval = (int)IBCS2_SIG_ERR;
				return error;
			}
			*retval = (int)sa_args.osa->sa_handler;
			if (IBCS2_SIGCALL(uap->sig) == IBCS2_SIGNAL_MASK)
				p->p_md.ibcs_sigflags |= sigmask(nsig);
			return 0;
		}
		
	case IBCS2_SIGHOLD_MASK:
		{
			struct sigprocmask_args {
				int     how;
				sigset_t mask;
			} sa;

			sa.how = SIG_BLOCK;
			sa.mask = sigmask(nsig);
			return sigprocmask(p, &sa, retval);
		}
		
	case IBCS2_SIGRELSE_MASK:
		{
			struct sigprocmask_args {
				int     how;
				sigset_t mask;
			} sa;

			sa.how = SIG_UNBLOCK;
			sa.mask = sigmask(nsig);
			return sigprocmask(p, &sa, retval);
		}
		
	case IBCS2_SIGIGNORE_MASK:
		{
			struct sigaction *sap;
			struct sigaction_args {
				int sig;
				struct sigaction *sa;
				struct sigaction *osa;
			} sa_args;

			sap = (struct sigaction *)STACK_ALLOC();
			sa_args.sig = nsig;
			sa_args.sa = sap;
			sa_args.osa = NULL;
			sa_args.sa->sa_handler = SIG_IGN;
			sa_args.sa->sa_mask = (sigset_t)0;
			sa_args.sa->sa_flags = 0;
			error = sigaction(p, &sa_args, retval);
			if (error) {
				DPRINTF(("sigignore: sigaction failed\n"));
				return error;
			}
			return 0;
		}
		
	case IBCS2_SIGPAUSE_MASK:
		{
			struct sigsuspend_args {
				sigset_t mask;
			} sa;
			sa.mask = p->p_sigmask &~ sigmask(nsig);
			return sigsuspend(p, &sa, retval);
		}
		
	default:
		return ENOSYS;
	}
}

static void
cvt_flock2iflock(flp, iflp)
	struct flock *flp;
	struct ibcs2_flock *iflp;
{
	switch (flp->l_type) {
	case F_RDLCK:
		iflp->l_type = IBCS2_F_RDLCK;
		break;
	case F_WRLCK:
		iflp->l_type = IBCS2_F_WRLCK;
		break;
	case F_UNLCK:
		iflp->l_type = IBCS2_F_UNLCK;
		break;
	}
	iflp->l_whence = (short)flp->l_whence;
	iflp->l_start = (ibcs2_off_t)flp->l_start;
	iflp->l_len = (ibcs2_off_t)flp->l_len;
	iflp->l_sysid = 0;
	iflp->l_pid = (ibcs2_pid_t)flp->l_pid;
}

static void
cvt_iflock2flock(iflp, flp)
	struct ibcs2_flock *iflp;
	struct flock *flp;
{
	flp->l_start = (off_t)iflp->l_start;
	flp->l_len = (off_t)iflp->l_len;
	flp->l_pid = (pid_t)iflp->l_pid;
	switch (iflp->l_type) {
	case IBCS2_F_RDLCK:
		flp->l_type = F_RDLCK;
		break;
	case IBCS2_F_WRLCK:
		flp->l_type = F_WRLCK;
		break;
	case IBCS2_F_UNLCK:
		flp->l_type = F_UNLCK;
		break;
	}
	flp->l_whence = iflp->l_whence;
}

/* convert iBCS2 mode into NetBSD mode */
int
ioflags2oflags(flags)
	int flags;
{
	int r = 0;
	
	if (flags & IBCS2_O_RDONLY) r |= O_RDONLY;
	if (flags & IBCS2_O_WRONLY) r |= O_WRONLY;
	if (flags & IBCS2_O_RDWR) r |= O_RDWR;
	if (flags & IBCS2_O_NDELAY) r |= O_NONBLOCK;
	if (flags & IBCS2_O_APPEND) r |= O_APPEND;
	if (flags & IBCS2_O_SYNC) r |= O_FSYNC;
	if (flags & IBCS2_O_NONBLOCK) r |= O_NONBLOCK;
	if (flags & IBCS2_O_CREAT) r |= O_CREAT;
	if (flags & IBCS2_O_TRUNC) r |= O_TRUNC;
	if (flags & IBCS2_O_EXCL) r |= O_EXCL;
	if (flags & IBCS2_O_NOCTTY) r |= O_NOCTTY;
	return r;
}

/* convert NetBSD mode into iBCS2 mode */
int
oflags2ioflags(flags)
	int flags;
{
	int r = 0;
	
	if (flags & O_RDONLY) r |= IBCS2_O_RDONLY;
	if (flags & O_WRONLY) r |= IBCS2_O_WRONLY;
	if (flags & O_RDWR) r |= IBCS2_O_RDWR;
	if (flags & O_NDELAY) r |= IBCS2_O_NONBLOCK;
	if (flags & O_APPEND) r |= IBCS2_O_APPEND;
	if (flags & O_FSYNC) r |= IBCS2_O_SYNC;
	if (flags & O_NONBLOCK) r |= IBCS2_O_NONBLOCK;
	if (flags & O_CREAT) r |= IBCS2_O_CREAT;
	if (flags & O_TRUNC) r |= IBCS2_O_TRUNC;
	if (flags & O_EXCL) r |= IBCS2_O_EXCL;
	if (flags & O_NOCTTY) r |= IBCS2_O_NOCTTY;
	return r;
}

struct ibcs2_fcntl_args {
	int fd;
	int cmd;
	char *arg;
};

int
ibcs2_fcntl(p, uap, retval)
	struct proc *p;
	struct ibcs2_fcntl_args *uap;
	int *retval;
{
	int error;
	struct fcntl_args {
		int     fd;
		int     cmd;
		int     arg;
	} fa;
	struct flock *flp;
	struct ibcs2_flock ifl;
	
	switch(uap->cmd) {
	case IBCS2_F_DUPFD:
		fa.fd = uap->fd;
		fa.cmd = F_DUPFD;
		fa.arg = (int)uap->arg;
		return fcntl(p, &fa, retval);
	case IBCS2_F_GETFD:
		fa.fd = uap->fd;
		fa.cmd = F_GETFD;
		fa.arg = (int)uap->arg;
		return fcntl(p, &fa, retval);
	case IBCS2_F_SETFD:
		fa.fd = uap->fd;
		fa.cmd = F_SETFD;
		fa.arg = (int)uap->arg;
		return fcntl(p, &fa, retval);
	case IBCS2_F_GETFL:
		fa.fd = uap->fd;
		fa.cmd = F_GETFL;
		fa.arg = (int)uap->arg;
		error = fcntl(p, &fa, retval);
		if (error)
			return error;
		*retval = oflags2ioflags(*retval);
		return error;
	case IBCS2_F_SETFL:
		fa.fd = uap->fd;
		fa.cmd = F_SETFL;
		fa.arg = ioflags2oflags(uap->arg);
		return fcntl(p, &fa, retval);

	case IBCS2_F_GETLK:
		flp = (struct flock *)STACK_ALLOC();
		error = copyin((caddr_t)uap->arg, (caddr_t)&ifl,
			       ibcs2_flock_len);
		if (error)
			return error;
		cvt_iflock2flock(&ifl, flp);
		fa.fd = uap->fd;
		fa.cmd = F_GETLK;
		fa.arg = (int)flp;
		error = fcntl(p, &fa, retval);
		if (error)
			return error;
		cvt_flock2iflock(flp, &ifl);
		return copyout((caddr_t)&ifl, (caddr_t)uap->arg,
			       ibcs2_flock_len);

	case IBCS2_F_SETLK:
		flp = (struct flock *)STACK_ALLOC();
		error = copyin((caddr_t)uap->arg, (caddr_t)&ifl,
			       ibcs2_flock_len);
		if (error)
			return error;
		cvt_iflock2flock(&ifl, flp);
		fa.fd = uap->fd;
		fa.cmd = F_SETLK;
		fa.arg = (int)flp;
		return fcntl(p, &fa, retval);

	case IBCS2_F_SETLKW:
		flp = (struct flock *)STACK_ALLOC();
		error = copyin((caddr_t)uap->arg, (caddr_t)&ifl,
			       ibcs2_flock_len);
		if (error)
			return error;
		cvt_iflock2flock(&ifl, flp);
		fa.fd = uap->fd;
		fa.cmd = F_SETLKW;
		fa.arg = (int)flp;
		return fcntl(p, &fa, retval);
	}
	return ENOSYS;
}

struct ibcs2_ulimit_args {
	int cmd;
	int newlim;
};

#define IBCS2_GETFSIZE	1
#define IBCS2_SETFSIZE	2
#define IBCS2_GETPSIZE	3

int
ibcs2_ulimit(p, uap, retval)
	struct proc *p;
	struct ibcs2_ulimit_args *uap;
	int *retval;
{
	int error;
	struct rlimit rl;
	struct setrlimit_args {
		int resource;
		struct rlimit *rlp;
	} sra;
	
	switch (uap->cmd) {
	case IBCS2_GETFSIZE:
		*retval = p->p_rlimit[RLIMIT_FSIZE].rlim_cur;
		return 0;
	case IBCS2_SETFSIZE:	/* XXX - fix this */
#ifdef notyet
		rl.rlim_cur = uap->newlim;
		sra.resource = RLIMIT_FSIZE;
		sra.rlp = &rl;
		error = setrlimit(p, &sra, retval);
		if (!error)
			*retval = p->p_rlimit[RLIMIT_FSIZE].rlim_cur;
		else
			DPRINTF(("failed "));
		return error;
#else
		*retval = uap->newlim;
		return 0;
#endif
	case IBCS2_GETPSIZE:
		*retval = p->p_rlimit[RLIMIT_RSS].rlim_cur; /* XXX */
		return 0;
	default:
		return ENOSYS;
	}
}

struct ibcs2_wait_args {
	int	*status;
};

struct ibcs2_waitpid_args {
	int	pid;
	int	*status;
	int	options;
};

struct ibcs2_waitsys_args {
	union {
		struct ibcs2_wait_args wa;
		struct ibcs2_waitpid_args wpa;
	} u;
};

int
ibcs2_waitsys(p, uap, retval)
	struct proc *p;
	struct ibcs2_waitsys_args *uap;
	int *retval;
{
	int error, status;
	struct wait_args {
		int	pid;
		int	*status;
		int	options;
		struct	rusage *rusage;
#ifdef COMPAT_43
		int	compat;
#endif
	} w4;
#define WAITPID_EFLAGS	0x8c4	/* OF, SF, ZF, PF */
	
	if ((p->p_md.md_regs[tEFLAGS] & WAITPID_EFLAGS) == WAITPID_EFLAGS) {
		/* waitpid */
		w4.pid = uap->u.wpa.pid;
		w4.status = uap->u.wpa.status;
		w4.options = uap->u.wpa.options;
		w4.rusage = NULL;
	} else {
		/* wait */
		w4.pid = WAIT_ANY;
		w4.status = uap->u.wa.status;
		w4.options = 0;
		w4.rusage = NULL;
	}
	error = wait4(p, &w4, retval);
	if (error == 0 && w4.status)	/* this is real iBCS brain-damage */
		copyin((caddr_t)w4.status, (caddr_t)&retval[1],
		       sizeof(w4.status));
	return error;
}

struct ibcs2_execv_args {
	char	*fname;
	char	**argp;
	char	**envp;		/* pseudo */
};

ibcs2_execv(p, uap, retval)
	struct proc *p;
	struct ibcs2_execv_args *uap;
	int *retval;
{
	DPRINTF(("ibcs2_execv(%d): %s ", p->p_pid, uap->fname));
	uap->envp = NULL;
	return (execve(p, uap, retval));
}

struct ibcs2_umount_args {
	char	*name;
};

ibcs2_umount(p, uap, retval)
	struct proc *p;
	struct ibcs2_umount_args *uap;
	int *retval;
{
	struct unmount_args {
		char	*name;
		int	flags;
	} um;

	um.name = uap->name;
	um.flags = 0;
	return (unmount(p, &um, retval));
}

struct ibcs2_mount_args {
	char	*special;
	char	*dir;
	int	flags;
	int	fstype;
	char	*data;
	int	len;
};

ibcs2_mount(p, uap, retval)
	struct proc *p;
	struct ibcs2_mount_args *uap;
	int *retval;
{
#ifdef notyet
	int oflags = uap->flags, nflags, error;
	char fsname[MFSNAMELEN];

	if (oflags & (IBCS2_MS_NOSUB | IBCS2_MS_SYS5))
		return (EINVAL);
	if ((oflags & IBCS2_MS_NEWTYPE) == 0)
		return (EINVAL);
	nflags = 0;
	if (oflags & IBCS2_MS_RDONLY)
		nflags |= MNT_RDONLY;
	if (oflags & IBCS2_MS_NOSUID)
		nflags |= MNT_NOSUID;
	if (oflags & IBCS2_MS_REMOUNT)
		nflags |= MNT_UPDATE;
	uap->flags = nflags;

	if (error = copyinstr((caddr_t)uap->type, fsname, sizeof fsname,
			      (u_int *)0))
		return (error);

	if (strcmp(fsname, "4.2") == 0) {
		uap->type = (caddr_t)STACK_ALLOC();
		if (error = copyout("ufs", uap->type, sizeof("ufs")))
			return (error);
	} else if (strcmp(fsname, "nfs") == 0) {
		struct ibcs2_nfs_args sna;
		struct sockaddr_in sain;
		struct nfs_args na;
		struct sockaddr sa;

		if (error = copyin(uap->data, &sna, sizeof sna))
			return (error);
		if (error = copyin(sna.addr, &sain, sizeof sain))
			return (error);
		bcopy(&sain, &sa, sizeof sa);
		sa.sa_len = sizeof(sain);
		uap->data = (caddr_t)STACK_ALLOC();
		na.addr = (struct sockaddr *)((int)uap->data + sizeof na);
		na.sotype = SOCK_DGRAM;
		na.proto = IPPROTO_UDP;
		na.fh = (nfsv2fh_t *)sna.fh;
		na.flags = sna.flags;
		na.wsize = sna.wsize;
		na.rsize = sna.rsize;
		na.timeo = sna.timeo;
		na.retrans = sna.retrans;
		na.hostname = sna.hostname;

		if (error = copyout(&sa, na.addr, sizeof sa))
			return (error);
		if (error = copyout(&na, uap->data, sizeof na))
			return (error);
	}
	return (mount(p, uap, retval));
#else
	return EINVAL;
#endif
}

/*
 * Read iBCS2-style directory entries.  We suck them into kernel space so
 * that they can be massaged before being copied out to user code.  Like
 * SunOS, we squish out `empty' entries.
 *
 * This is quite ugly, but what do you expect from compatibility code?
 */

struct ibcs2_getdents_args {
	int	fd;
	char	*buf;
	int	nbytes;
};

ibcs2_getdents(p, uap, retval)
	struct proc *p;
	register struct ibcs2_getdents_args *uap;
	int *retval;
{
	register struct vnode *vp;
	register caddr_t inp, buf;	/* BSD-format */
	register int len, reclen;	/* BSD-format */
	register caddr_t outp;		/* iBCS2-format */
	register int resid;		/* iBCS2-format */
	struct file *fp;
	struct uio auio;
	struct iovec aiov;
	struct ibcs2_dirent idb;
	off_t off;			/* true file offset */
	ibcs2_off_t soff;		/* SYSV file offset */
	int buflen, error, eofflag;
#define	BSD_DIRENT(cp)		((struct dirent *)(cp))
#define IBCS2_DIRENT(cp)	((struct ibcs2_dirent *)(cp))
#define	IBCS2_RECLEN(reclen)	(reclen + sizeof(u_short))

	if ((error = getvnode(p->p_fd, uap->fd, &fp)) != 0)
		return (error);
	if ((fp->f_flag & FREAD) == 0)
		return (EBADF);
	vp = (struct vnode *)fp->f_data;
	if (vp->v_type != VDIR)	/* XXX  vnode readdir op should do this */
		return (EINVAL);
	buflen = min(MAXBSIZE, uap->nbytes);
	buf = malloc(buflen, M_TEMP, M_WAITOK);
	VOP_LOCK(vp);
	off = fp->f_offset;
again:
	aiov.iov_base = buf;
	aiov.iov_len = buflen;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_rw = UIO_READ;
	auio.uio_segflg = UIO_SYSSPACE;
	auio.uio_procp = p;
	auio.uio_resid = buflen;
	auio.uio_offset = off;
	/*
	 * First we read into the malloc'ed buffer, then
	 * we massage it into user space, one record at a time.
	 */
	if (error = VOP_READDIR(vp, &auio, fp->f_cred, &eofflag, (u_long *)0,
	    0))
		goto out;
	inp = buf;
	outp = uap->buf;
	resid = uap->nbytes;
	if ((len = buflen - auio.uio_resid) == 0)
		goto eof;
	for (; len > 0; len -= reclen) {
		reclen = ((struct dirent *)inp)->d_reclen;
		if (reclen & 3)
			panic("ibcs2_getdents");
		off += reclen;		/* each entry points to next */
		if (BSD_DIRENT(inp)->d_fileno == 0) {
			inp += reclen;	/* it is a hole; squish it out */
			continue;
		}
		if (reclen > len || resid < IBCS2_RECLEN(reclen)) {
			/* entry too big for buffer, so just stop */
			outp++;
			break;
		}
		/*
		 * Massage in place to make a iBCS2-shaped dirent (otherwise
		 * we have to worry about touching user memory outside of
		 * the copyout() call).
		 */
		idb.d_ino = (ibcs2_ino_t)BSD_DIRENT(inp)->d_fileno;
		idb.d_off = (ibcs2_off_t)off;
		idb.d_reclen = (u_short)IBCS2_RECLEN(reclen);
		if ((error = copyout((caddr_t)&idb, outp, 10)) != 0 ||
		    (error = copyout(BSD_DIRENT(inp)->d_name, outp + 10,
				     BSD_DIRENT(inp)->d_namlen + 1)) != 0)
			goto out;
		/* advance past this real entry */
		inp += reclen;
		/* advance output past Sun-shaped entry */
		outp += IBCS2_RECLEN(reclen);
		resid -= IBCS2_RECLEN(reclen);
	}
	/* if we squished out the whole block, try again */
	if (outp == uap->buf)
		goto again;
	fp->f_offset = off;		/* update the vnode offset */
eof:
	*retval = uap->nbytes - resid;
out:
	VOP_UNLOCK(vp);
	free(buf, M_TEMP);
	return (error);
}

struct ibcs2_fchroot_args {
	int	fdes;
};
ibcs2_fchroot(p, uap, retval)
	register struct proc *p;
	register struct ibcs2_fchroot_args *uap;
	int *retval;
{
	register struct filedesc *fdp = p->p_fd;
	register struct vnode *vp;
	struct file *fp;
	int error;

	if ((error = suser(p->p_ucred, &p->p_acflag)) != 0)
		return (error);
	if ((error = getvnode(fdp, uap->fdes, &fp)) != 0)
		return (error);
	vp = (struct vnode *)fp->f_data;
	VOP_LOCK(vp);
	if (vp->v_type != VDIR)
		error = ENOTDIR;
	else
		error = VOP_ACCESS(vp, VEXEC, p->p_ucred, p);
	VOP_UNLOCK(vp);
	if (error)
		return (error);
	VREF(vp);
	if (fdp->fd_rdir != NULL)
		vrele(fdp->fd_rdir);
	fdp->fd_rdir = vp;
	return (0);
}

struct ibcs2_ustat_args {
	struct ibcs2_ustat *buf;
	int dev;
};

struct ibcs2_uname_args {
	struct ibcs2_utsname *buf;
	int junk;
};

struct ibcs2_utssys_args {
	union {
		struct ibcs2_ustat_args ustat_args;
		struct ibcs2_uname_args uname_args;
	} u;
	int flag;
};

int
ibcs2_utssys(p, uap, retval)
	struct proc *p;
	struct ibcs2_utssys_args *uap;
	int *retval;
{
	switch (uap->flag) {
	case 0:			/* uname(2) */
	{
		struct ibcs2_utsname sut;
		extern char ostype[], machine[], osrelease[];

		bzero(&sut, ibcs2_utsname_len);

		bcopy(ostype, sut.sysname, sizeof(sut.sysname) - 1);
		bcopy(hostname, sut.nodename, sizeof(sut.nodename));
		sut.nodename[sizeof(sut.nodename)-1] = '\0';
		bcopy(osrelease, sut.release, sizeof(sut.release) - 1);
		bcopy("1", sut.version, sizeof(sut.version) - 1);
		bcopy(machine, sut.machine, sizeof(sut.machine) - 1);

		return copyout((caddr_t)&sut, (caddr_t)uap->u.uname_args.buf,
			       ibcs2_utsname_len);
	}

	case 2:			/* ustat(2) */
		return ENOSYS;	/* XXX - TODO */

	default:
		return ENOSYS;
	}
}

struct ibcs2_open_args {
	char	*fname;
	int	fmode;
	int	crtmode;
};

int
cvt_o_flags(flags)
	int flags;
{
	int r = 0;

        /* convert mode into NetBSD mode */
	if (flags & IBCS2_O_WRONLY) r |= O_WRONLY;
	if (flags & IBCS2_O_RDWR) r |= O_RDWR;
	if (flags & (IBCS2_O_NDELAY | IBCS2_O_NONBLOCK)) r |= O_NONBLOCK;
	if (flags & IBCS2_O_APPEND) r |= O_APPEND;
	if (flags & IBCS2_O_SYNC) r |= O_FSYNC;
	if (flags & IBCS2_O_CREAT) r |= O_CREAT;
	if (flags & IBCS2_O_TRUNC) r |= O_TRUNC;
	if (flags & IBCS2_O_EXCL) r |= O_EXCL;
	return r;
}

int
ibcs2_open(p, uap, retval)
	struct proc *p;
	struct ibcs2_open_args *uap;
	int *retval;
{
	int noctty = uap->fmode & IBCS2_O_NOCTTY;
	int ret;

	uap->fmode = cvt_o_flags(uap->fmode);
	ret = open(p, uap, retval);

	if (!ret && !noctty && SESS_LEADER(p) && !(p->p_flag & P_CONTROLT)) {
		struct filedesc *fdp = p->p_fd;
		struct file *fp = fdp->fd_ofiles[*retval];

		/* ignore any error, just give it a try */
		if (fp->f_type == DTYPE_VNODE)
			(fp->f_ops->fo_ioctl)(fp, TIOCSCTTY, (caddr_t) 0, p);
	}
	return ret;
}

static int
ibcs2_cvt_statfs(sp, buf, len)
	struct statfs *sp;
	caddr_t buf;
	int len;
{
	struct ibcs2_statfs ssfs;

	bzero(&ssfs, sizeof ssfs);
	ssfs.f_fstyp = 0;
	ssfs.f_bsize = sp->f_bsize;
	ssfs.f_frsize = 0;
	ssfs.f_blocks = sp->f_blocks;
	ssfs.f_bfree = sp->f_bfree;
	ssfs.f_files = sp->f_files;
	ssfs.f_ffree = sp->f_ffree;
	ssfs.f_fname[0] = 0;
	ssfs.f_fpack[0] = 0;
	return copyout((caddr_t)&ssfs, buf, len);
}	

struct ibcs2_statfs_args {
	char	*path;
	struct	ibcs2_statfs *buf;
	int	len;
	int	fstype;
};

ibcs2_statfs(p, uap, retval)
	struct proc *p;
	struct ibcs2_statfs_args *uap;
	int *retval;
{
	register struct mount *mp;
	register struct statfs *sp;
	int error;
	struct nameidata nd;

	NDINIT(&nd, LOOKUP, FOLLOW, UIO_USERSPACE, uap->path, p);
	if (error = namei(&nd))
		return (error);
	mp = nd.ni_vp->v_mount;
	sp = &mp->mnt_stat;
	vrele(nd.ni_vp);
	if (error = VFS_STATFS(mp, sp, p))
		return (error);
	sp->f_flags = mp->mnt_flag & MNT_VISFLAGMASK;
	return ibcs2_cvt_statfs(sp, (caddr_t)uap->buf, uap->len);
}

struct ibcs2_fstatfs_args {
	int	fd;
	struct	ibcs2_statfs *buf;
	int	len;
	int	fstype;
};

ibcs2_fstatfs(p, uap, retval)
	struct proc *p;
	struct ibcs2_fstatfs_args *uap;
	int *retval;
{
	struct file *fp;
	struct mount *mp;
	register struct statfs *sp;
	int error;

	if (error = getvnode(p->p_fd, uap->fd, &fp))
		return (error);
	mp = ((struct vnode *)fp->f_data)->v_mount;
	sp = &mp->mnt_stat;
	if (error = VFS_STATFS(mp, sp, p))
		return (error);
	sp->f_flags = mp->mnt_flag & MNT_VISFLAGMASK;
	return ibcs2_cvt_statfs(sp, (caddr_t)uap->buf, uap->len);
}

struct ibcs2_mknod_args {
	char	*fname;
	int	fmode;
	int	dev;
};

ibcs2_mknod(p, uap, retval)
	struct proc *p;
	struct ibcs2_mknod_args *uap;
	int *retval;
{
	if (S_ISFIFO(uap->fmode))
		return mkfifo(p, uap, retval);

	return mknod(p, uap, retval);
}

struct ibcs2_xenix_args {
	int callno;
};

int
ibcs2_xenix(p, uap, retval)
	struct proc *p;
	struct ibcs2_xenix_args *uap;
	int *retval;
{
	return EINVAL;
}

struct ibcs2_select_args {
	int nfds;
	fd_set *readfds, *writefds, *exceptfds;
	struct ibcs2_timeval *timeout;
};

/* int select(nfds, readfds, writefds, exceptfds, timeout) */

int
ibcs2_select(p, uap, retval)
	struct proc *p;
	struct ibcs2_select_args *uap;
	int *retval;
{
	return EINVAL;
}

static void
cvt_sa2isa(sap, isap)
	struct sigaction *sap;
	struct ibcs2_sigaction *isap;
{
	sap->sa_handler = isap->sa_handler;
	sap->sa_mask = cvt_sigmask(isap->sa_mask, ibcs2bsd_sigtbl);
	sap->sa_flags = (isap->sa_flags & IBCS2_SA_NOCLDSTOP)
		? SA_NOCLDSTOP : 0;
}

static void
cvt_isa2sa(isap, sap)
	struct ibcs2_sigaction *isap;
	struct sigaction *sap;
{
	isap->sa_handler = sap->sa_handler;
	isap->sa_mask = cvt_sigmask(sap->sa_mask, bsd2ibcs_sigtbl);
	isap->sa_flags = (sap->sa_flags & SA_NOCLDSTOP)
		? IBCS2_SA_NOCLDSTOP : 0;
}

struct ibcs2_sigaction_args {
	int sig;
	struct ibcs2_sigaction *act, *oact;
};

int
ibcs2_sigaction(p, uap, retval)
	struct proc *p;
	struct ibcs2_sigaction_args *uap;
	int *retval;
{
	int error;
	struct sigaction_args {
		int sig;
		struct sigaction *act, *oact;
	} sa;
	struct ibcs2_sigaction *isa, *oisa;

	isa = (struct ibcs2_sigaction *)STACK_ALLOC();
	oisa = &isa[1];
	sa.sig = ibcs2bsd_sig(uap->sig);
	sa.act = (struct sigaction *)&isa[2];
	sa.oact = (struct sigaction *)&isa[3];
	if (error = copyin((caddr_t)uap->act, (caddr_t)isa, sizeof(*isa)))
		return error;
	cvt_isa2sa(isa, sa.act);
	if (error = sigaction(p, &sa, retval))
		return error;
	cvt_sa2isa(sa.oact, oisa);
        return copyout((caddr_t)oisa, (caddr_t)uap->oact, sizeof(*oisa));
}

struct ibcs2_sigprocmask_args {
	int how;
	sigset_t *set, *oset;
};

int
ibcs2_sigprocmask(p, uap, retval)
	struct proc *p;
	struct ibcs2_sigprocmask_args *uap;
	int *retval;
{
	int error;
	sigset_t iset;
	struct sigprocmask_args {
		int how;
		sigset_t mask;
	} sa;

	switch (uap->how) {
	case IBCS2_SIG_BLOCK:
		sa.how = SIG_BLOCK;
		break;
	case IBCS2_SIG_UNBLOCK:
		sa.how = SIG_UNBLOCK;
		break;
	case IBCS2_SIG_SETMASK:
		sa.how = SIG_SETMASK;
		break;
	default:
		return EINVAL;
	}
	if (error = copyin((caddr_t)uap->set, (caddr_t)&iset, sizeof(iset)))
		return error;
	sa.mask = cvt_sigmask(iset, ibcs2bsd_sigtbl);
	if (error = sigprocmask(p, &sa, retval))
		return error;
	iset = cvt_sigmask(*retval, bsd2ibcs_sigtbl);
	return copyout((caddr_t)&iset, (caddr_t)uap->oset, sizeof(iset));
}

struct ibcs2_sigpending_args {
	sigset_t *mask;
};

int
ibcs2_sigpending(p, uap, retval)
	struct proc *p;
	struct ibcs2_sigpending_args *uap;
	int *retval;
{
	int mask = cvt_sigmask(p->p_siglist & p->p_sigmask, bsd2ibcs_sigtbl);

	return (copyout((caddr_t)&mask, (caddr_t)uap->mask, sizeof(int)));
}

struct ibcs2_sigsuspend_args {
	sigset_t *mask;
};

int
ibcs2_sigsuspend(p, uap, retval)
	struct proc *p;
	struct ibcs2_sigsuspend_args *uap;
	int *retval;
{
	int error;
	struct sigsuspend_args {
		sigset_t mask;
	} sa;

	if (error = copyin((caddr_t)uap->mask, (caddr_t)sa.mask,
			   sizeof(sa.mask)))
		return error;
	return sigsuspend(p, &sa, retval);
}

struct ibcs2_getgroups_args {
	int gidsetsize;
	ibcs2_gid_t *gidset;
};

int
ibcs2_getgroups(p, uap, retval)
	struct proc *p;
	struct ibcs2_getgroups_args *uap;
	int *retval;
{
	int error, i;
	ibcs2_gid_t igid, *iset;
	struct getgroups_args {
		u_int	gidsetsize;
		gid_t	*gidset;
	} sa;

	sa.gidsetsize = uap->gidsetsize;
	sa.gidset = (gid_t *)STACK_ALLOC();
	iset = (ibcs2_gid_t *)&sa.gidset[NGROUPS_MAX];
	if (error = getgroups(p, &sa, retval))
		return error;
	for (i = 0; i < retval[0]; i++)
		iset[i] = (ibcs2_gid_t)sa.gidset[i];
	if (retval[0] && (error = copyout((caddr_t)iset, (caddr_t)uap->gidset,
					  sizeof(ibcs2_gid_t) * retval[0])))
		return error;
        return 0;
}

struct ibcs2_setgroups_args {
	int gidsetsize;
	ibcs2_gid_t *gidset;
};

int
ibcs2_setgroups(p, uap, retval)
	struct proc *p;
	struct ibcs2_setgroups_args *uap;
	int *retval;
{
	int error, i;
	ibcs2_gid_t igid, *iset;
	struct setgroups_args {
		u_int	gidsetsize;
		gid_t	*gidset;
	} sa;

	sa.gidsetsize = uap->gidsetsize;
	sa.gidset = (gid_t *)STACK_ALLOC();
	iset = (ibcs2_gid_t *)&sa.gidset[NGROUPS_MAX];
	if (sa.gidsetsize && (error = copyin((caddr_t)uap->gidset,
					     (caddr_t)iset, 
					     sizeof(ibcs2_gid_t) *
					     sa.gidsetsize)))
		return error;
	for (i = 0; i < sa.gidsetsize; i++)
		sa.gidset[i] = (gid_t)iset[i];
	return setgroups(p, &sa, retval);
}

struct ibcs2_setuid_args {
	int uid;
};

int
ibcs2_setuid(p, uap, retval)
	struct proc *p;
	struct ibcs2_setuid_args *uap;
	int *retval;
{
	struct setuid_args {
		uid_t   uid;
	} sa;

	sa.uid = (uid_t) uap->uid;
	return setuid(p, &sa, retval);
}

struct ibcs2_setgid_args {
	int gid;
};

int
ibcs2_setgid(p, uap, retval)
	struct proc *p;
	struct ibcs2_setgid_args *uap;
	int *retval;
{
	struct setgid_args {
		gid_t   gid;
	} sa;

	sa.gid = (gid_t) uap->gid;
	return setgid(p, &sa, retval);
}

struct ibcs2_timeb {
	ibcs2_time_t time;
	unsigned short millitm;
	short timezone;
	short dstflag;
};
#define ibcs2_timeb_len	10	/* packed struct */

struct ibcs2_ftime_args {
	struct ibcs2_timeb *tp;
};

int
ibcs2_ftime(p, uap, retval)
	struct proc *p;
	struct ibcs2_ftime_args *uap;
	int *retval;
{
	struct timeval tv;
	extern struct timezone tz;
	struct ibcs2_timeb itb;

	microtime(&tv);
	itb.time = tv.tv_sec;
	itb.millitm = (tv.tv_usec / 1000);
	itb.timezone = tz.tz_minuteswest;
	itb.dstflag = tz.tz_dsttime;
	return copyout((caddr_t)&itb, (caddr_t)uap->tp, ibcs2_timeb_len);
}

struct ibcs2_time_args {
	ibcs2_time_t *tp;
};

int
ibcs2_time(p, uap, retval)
	struct proc *p;
	struct ibcs2_time_args *uap;
	int *retval;
{
	struct timeval tv;

	microtime(&tv);
	*retval = tv.tv_sec;
	if (uap->tp)
		return copyout((caddr_t)&tv.tv_sec, (caddr_t)uap->tp,
			       sizeof(ibcs2_time_t));
	else
		return 0;
}

struct ibcs2_pathconf_args {
	char	*path;
	int	name;
};

int
ibcs2_pathconf(p, uap, retval)
	struct proc *p;
	struct ibcs2_pathconf_args *uap;
	int *retval;
{
	uap->name++;		/* iBCS2 _PC_* defines are offset by one */
        return pathconf(p, uap, retval);
}

struct ibcs2_fpathconf_args {
	int	fd;
	int	name;
};

int
ibcs2_fpathconf(p, uap, retval)
	struct proc *p;
	struct ibcs2_fpathconf_args *uap;
	int *retval;
{
	uap->name++;		/* iBCS2 _PC_* defines are offset by one */
        return fpathconf(p, uap, retval);
}

struct ibcs2_sysconf_args {
	int	name;
};

int
ibcs2_sysconf(p, uap, retval)
	struct proc *p;
	struct ibcs2_sysconf_args *uap;
	int *retval;
{
	int mib[2], value, len, error;
	struct __sysctl_args {
		int     *name;
		u_int   namelen;
		void    *old;
		size_t  *oldlenp;
		void    *new;
		size_t  newlen;
	} sa;
	struct getrlimit_args {
		u_int   which;
		struct  rlimit *rlp;
	} ga;

	switch(uap->name) {
	case IBCS2_SC_ARG_MAX:
		mib[1] = KERN_ARGMAX;
		break;

	case IBCS2_SC_CHILD_MAX:
		ga.which = RLIMIT_NPROC;
		ga.rlp = (struct rlimit *)STACK_ALLOC();
		if (error = getrlimit(p, &ga, retval))
			return error;
		*retval = ga.rlp->rlim_cur;
		return 0;

	case IBCS2_SC_CLK_TCK:
		*retval = hz;
		return 0;

	case IBCS2_SC_NGROUPS_MAX:
		mib[1] = KERN_NGROUPS;
		break;

	case IBCS2_SC_OPEN_MAX:
		ga.which = RLIMIT_NOFILE;
		ga.rlp = (struct rlimit *)STACK_ALLOC();
		if (error = getrlimit(p, &ga, retval))
			return error;
		*retval = ga.rlp->rlim_cur;
		return 0;
		
	case IBCS2_SC_JOB_CONTROL:
		mib[1] = KERN_JOB_CONTROL;
		break;
		
	case IBCS2_SC_SAVED_IDS:
		mib[1] = KERN_SAVED_IDS;
		break;
		
	case IBCS2_SC_VERSION:
		mib[1] = KERN_POSIX1;
		break;
		
	case IBCS2_SC_PASS_MAX:
		*retval = 128;		/* XXX - should we create PASS_MAX ? */
		return 0;

	case IBCS2_SC_XOPEN_VERSION:
		*retval = 2;		/* XXX: What should that be? */
		return 0;
		
	default:
		return EINVAL;
	}

	mib[0] = CTL_KERN;
	len = sizeof(value);
	sa.name = mib;
	sa.namelen = 2;
	sa.old = &value;
	sa.oldlenp = &len;
	sa.new = NULL;
	sa.newlen = 0;
	if (error = __sysctl(p, &sa, retval))
		return error;
	*retval = value;
	return 0;
}

static void
ibcs2_statcvt(st, st4)
	struct ostat *st;
	struct ibcs2_stat *st4;
{
	bzero(st4, sizeof(*st4));
	st4->st_dev = (ibcs2_dev_t)st->st_dev;
	st4->st_ino = (ibcs2_ino_t)st->st_ino;
	st4->st_mode = (ibcs2_mode_t)st->st_mode;
	st4->st_nlink = (ibcs2_nlink_t)st->st_nlink;
	st4->st_uid = (ibcs2_uid_t)st->st_uid;
	st4->st_gid = (ibcs2_gid_t)st->st_gid;
	st4->st_rdev = (ibcs2_dev_t)st->st_rdev;
	st4->st_size = (ibcs2_off_t)st->st_size;
	st4->st_atim = (ibcs2_time_t)st->st_atime;
	st4->st_mtim = (ibcs2_time_t)st->st_mtime;
	st4->st_ctim = (ibcs2_time_t)st->st_ctime;
}

struct ostat_args {
	char	*fname;
	struct ostat *st;
};

struct ibcs2_stat_args {
	char	*fname;
	struct ibcs2_stat *st;
};

ibcs2_stat(p, uap, retval)
	struct proc *p;
	struct ibcs2_stat_args *uap;
	int *retval;
{
	struct ostat st;
	struct ibcs2_stat ibcs2_st;
	struct ostat_args cup;
	int error;

	cup.fname = uap->fname;
	cup.st = (struct ostat *)STACK_ALLOC();
	if (error = ostat(p, &cup, retval))
		return (error);
	if (error = copyin(cup.st, &st, sizeof st))
		return (error);
	ibcs2_statcvt(&st, &ibcs2_st);
	return copyout((caddr_t)&ibcs2_st, (caddr_t)uap->st, ibcs2_stat_len);
}

struct olstat_args {
	char	*fname;
	struct ostat *st;
};

struct ibcs2_lstat_args {
	char	*fname;
	struct ibcs2_stat *st;
};

ibcs2_lstat(p, uap, retval)
	struct proc *p;
	struct ibcs2_lstat_args *uap;
	int *retval;
{
	struct ostat st;
	struct ibcs2_stat ibcs2_st;
	struct olstat_args cup;
	int error;

	cup.fname = uap->fname;
	cup.st = (struct ostat *)STACK_ALLOC();
	if (error = olstat(p, &cup, retval))
		return (error);
	if (error = copyin(cup.st, &st, sizeof st))
		return (error);
	ibcs2_statcvt(&st, &ibcs2_st);
	return copyout((caddr_t)&ibcs2_st, (caddr_t)uap->st, ibcs2_stat_len);
}

struct ofstat_args {
	int fd;
	struct ostat *st;
};

struct ibcs2_fstat_args {
	int fd;
	struct ibcs2_stat *st;
};

ibcs2_fstat(p, uap, retval)
	struct proc *p;
	struct ibcs2_fstat_args *uap;
	int *retval;
{
	struct ostat st;
	struct ibcs2_stat ibcs2_st;
	struct ofstat_args cup;
	int error;

	cup.fd = uap->fd;
	cup.st = (struct ostat *)STACK_ALLOC();
	if (error = ofstat(p, &cup, retval))
		return (error);
	if (error = copyin(cup.st, &st, sizeof st))
		return (error);
	ibcs2_statcvt(&st, &ibcs2_st);
	return copyout((caddr_t)&ibcs2_st, (caddr_t)uap->st, ibcs2_stat_len);
}

struct ibcs2_alarm_args {
	unsigned int secs;
};

ibcs2_alarm(p, uap, retval)
	struct proc *p;
	struct ibcs2_alarm_args *uap;
	int *retval;
{
	int error;
        struct itimerval *itp, *oitp;
	struct setitimer_args {
		u_int   which;
		struct  itimerval *itv, *oitv;
	} sa;

        itp = (struct itimerval *)STACK_ALLOC();
	oitp = &itp[1];
        timerclear(&itp->it_interval);
        itp->it_value.tv_sec = uap->secs;
        itp->it_value.tv_usec = 0;

	sa.which = ITIMER_REAL;
	sa.itv = itp;
	sa.oitv = oitp;
        error = setitimer(p, &sa, retval);
	if (error)
		return error;
        if (oitp->it_value.tv_usec)
                oitp->it_value.tv_sec++;
        *retval = oitp->it_value.tv_sec;
        return 0;
}

int
ibcs2_pause(p, uap, retval)
	struct proc *p;
	void *uap;
	int *retval;
{
	int error;
	struct sigsuspend_args {
		sigset_t mask;
	} sa;

	sa.mask = p->p_sigmask;
        error = sigsuspend(p, &sa, retval);
	return error;
}

int
ibcs2_getmsg(p, uap, retval)
	struct proc *p;
	void *uap;
	int *retval;
{
	return 0;
}

int
ibcs2_putmsg(p, uap, retval)
	struct proc *p;
	void *uap;
	int *retval;
{
	return 0;
}

struct ibcs2_times_arg {
	struct tms *tp;
};

int
ibcs2_times(p, uap, retval)
	struct proc *p;
	struct ibcs2_times_arg *uap;
	int *retval;
{
	int error;
	struct getrusage_args {
		int     who;
		struct  rusage *rusage;
	} ga;
	struct tms tms;
        struct rusage *ru = (struct rusage *)STACK_ALLOC();
        struct timeval t;
#define CONVTCK(r)      (r.tv_sec * hz + r.tv_usec / (1000000 / hz))

	ga.who = RUSAGE_SELF;
	ga.rusage = ru;
	error = getrusage(p, &ga, retval);
	if (error)
                return error;
        tms.tms_utime = CONVTCK(ru->ru_utime);
        tms.tms_stime = CONVTCK(ru->ru_stime);

	ga.who = RUSAGE_CHILDREN;
        error = getrusage(p, &ga, retval);
	if (error)
		return error;
        tms.tms_cutime = CONVTCK(ru->ru_utime);
        tms.tms_cstime = CONVTCK(ru->ru_stime);

	microtime(&t);
        *retval = CONVTCK(t);
	
	return copyout(&tms, uap->tp, sizeof(tms));
}

struct ibcs2_stime_args {
	long *timep;
};

int
ibcs2_stime(p, uap, retval)
	struct proc *p;
	struct ibcs2_stime_args *uap;
	int *retval;
{
	int error;
	struct settimeofday_args {
		struct  timeval *tv;
		struct  timezone *tzp;
	} sa;

	sa.tv = STACK_ALLOC();
	sa.tzp = NULL;
	if (error = copyin((caddr_t)uap->timep, &sa.tv->tv_sec, sizeof(long)))
		return error;
	sa.tv->tv_usec = 0;
	if (error = settimeofday(p, &sa, retval))
		return EPERM;
	return 0;
}

int
ibcs2_ptrace(p, uap, retval)
	struct proc *p;
	void *uap;
	int *retval;
{
	return EINVAL;		/* XXX - TODO */
}

struct ibcs2_utimbuf {
	time_t actime;
	time_t modtime;
};

struct ibcs2_utime_args {
	char *path;
	struct ibcs2_utimbuf *buf;
};

int
ibcs2_utime(p, uap, retval)
	struct proc *p;
	struct ibcs2_utime_args *uap;
	int *retval;
{
	int error;
	struct utimes_args {
		char    *path;
		struct  timeval *tptr;
	} sa;

	sa.path = uap->path;
	if (uap->buf) {
		struct ibcs2_utimbuf ubuf;

		if (error = copyin((caddr_t)uap->buf, (caddr_t)&ubuf,
				   sizeof(ubuf)))
			return error;
		sa.tptr = STACK_ALLOC();
		sa.tptr[0].tv_sec = ubuf.actime;
		sa.tptr[0].tv_usec = 0;
		sa.tptr[1].tv_sec = ubuf.modtime;
		sa.tptr[1].tv_usec = 0;
	} else
		sa.tptr = NULL;
	return utimes(p, &sa, retval);
}

struct ibcs2_nice_args {
	int incr;
};

int
ibcs2_nice(p, uap, retval)
	struct proc *p;
	struct ibcs2_nice_args *uap;
	int *retval;
{
	int error, cur_nice = p->p_nice;
	struct setpriority_args {
		int     which;
		int     who;
		int     prio;
	} sa;

	sa.which = PRIO_PROCESS;
	sa.who = 0;
	sa.prio = p->p_nice + uap->incr;
	if (error = setpriority(p, &sa, retval))
		return EPERM;
	*retval = p->p_nice;
	return 0;
}

/*
 * iBCS2 getpgrp, setpgrp, setsid, and setpgid
 */

struct ibcs2_pgrpsys_args {
	int	type;
	caddr_t	dummy;
	int	pid;
	int	pgid;
};

int
ibcs2_pgrpsys(p, uap, retval)
	struct proc *p;
	struct ibcs2_pgrpsys_args *uap;
	int *retval;
{
	switch (uap->type) {
	case 0:			/* getpgrp */
		*retval = p->p_pgrp->pg_id;
		return 0;

	case 1:			/* setpgrp */
		{
			struct setpgid_args {
				int     pid;
				int     pgid;
			} sa;

			sa.pid = 0;
			sa.pgid = 0;
			setpgid(p, &sa, retval);
			*retval = p->p_pgrp->pg_id;
			return 0;
		}

	case 2:			/* setpgid */
		{
			struct setpgid_args {
				int     pid;
				int     pgid;
			} sa;

			sa.pid = uap->pid;
			sa.pgid = uap->pgid;
			return setpgid(p, &sa, retval);
		}

	case 3:			/* setsid */
		return setsid(p, NULL, retval);

	default:
		return EINVAL;
	}
}

struct ibcs2_plock_args {
	int cmd;
};

#define IBCS2_UNLOCK	0
#define IBCS2_PROCLOCK	1
#define IBCS2_TEXTLOCK	2
#define IBCS2_DATALOCK	4

/*
 * XXX - need to check for nested calls
 */

int
ibcs2_plock(p, uap, retval)
	struct proc *p;
	struct ibcs2_plock_args *uap;
	int *retval;
{
	int error;
	
        if (error = suser(p->p_ucred, &p->p_acflag))
                return EPERM;
	switch(uap->cmd) {
	case IBCS2_UNLOCK:
	case IBCS2_PROCLOCK:
	case IBCS2_TEXTLOCK:
	case IBCS2_DATALOCK:
		return 0;	/* XXX - TODO */
	}
	return EINVAL;
}

#define SCO_A_REBOOT        1
#define SCO_A_SHUTDOWN      2
#define SCO_A_REMOUNT       4
#define SCO_A_CLOCK         8
#define SCO_A_SETCONFIG     128
#define SCO_A_GETDEV        130

#define SCO_AD_HALT         0
#define SCO_AD_BOOT         1
#define SCO_AD_IBOOT        2
#define SCO_AD_PWRDOWN      3
#define SCO_AD_PWRNAP       4

#define SCO_AD_PANICBOOT    1

#define SCO_AD_GETBMAJ      0
#define SCO_AD_GETCMAJ      1

struct ibcs2_uadmin_args {
	int cmd;
	int func;
	caddr_t data;
};

int
ibcs2_uadmin(p, uap, retval)
	struct proc *p;
	struct ibcs2_uadmin_args *uap;
	int *retval;
{
	switch(uap->cmd) {
	case SCO_A_REBOOT:
	case SCO_A_SHUTDOWN:
		switch(uap->func) {
		case SCO_AD_HALT:
		case SCO_AD_PWRDOWN:
		case SCO_AD_PWRNAP:
			reboot(RB_HALT);
		case SCO_AD_BOOT:
		case SCO_AD_IBOOT:
			reboot(RB_AUTOBOOT);
		}
		return EINVAL;
	case SCO_A_REMOUNT:
	case SCO_A_CLOCK:
	case SCO_A_SETCONFIG:
		return 0;
	case SCO_A_GETDEV:
		return EINVAL;	/* XXX - TODO */
	}
	return EINVAL;
}

struct ibcs2_sysfs_args {
	int cmd;
	caddr_t d1;
	char *buf;
};

#define IBCS2_GETFSIND        1
#define IBCS2_GETFSTYP        2
#define IBCS2_GETNFSTYP       3

int
ibcs2_sysfs(p, uap, retval)
	struct proc *p;
	struct ibcs2_sysfs_args *uap;
	int *retval;
{
	switch(uap->cmd) {
	case IBCS2_GETFSIND:
	case IBCS2_GETFSTYP:
	case IBCS2_GETNFSTYP:
	}
	return EINVAL;		/* XXX - TODO */
}

int
ibcs2_poll(p, uap, retval)
	struct proc *p;
	void *uap;
	int *retval;
{
	return EINVAL;		/* XXX - TODO */
}

struct ibcs2_kill_args {
	int pid;
	int signo;
};

int
ibcs2_kill(p, uap, retval)
	struct proc *p;
	struct ibcs2_kill_args *uap;
	int *retval;
{

	uap->signo = ibcs2bsd_sig(uap->signo);
	if (uap->signo == NOSIG)
		return EINVAL;
	return kill(p, uap, retval);
}

struct xenix_rdchk_args {
	int fd;
};

int
xenix_rdchk(p, uap, retval)
	struct proc *p;
	struct xenix_rdchk_args *uap;
	int *retval;
{
	int error;
	struct ioctl_args {
		int     fd;
		int     com;
		caddr_t data;
	} sa;

	sa.fd = uap->fd;
	sa.com = FIONREAD;
	sa.data = STACK_ALLOC();
	if (error = ioctl(p, &sa, retval))
		return error;
	*retval = (*((int*)sa.data)) ? 1 : 0;
	return 0;
}

struct xenix_chsize_args {
	int fd;
	long size;
};

int
xenix_chsize(p, uap, retval)
	struct proc *p;
	struct xenix_chsize_args *uap;
	int *retval;
{
	struct ftruncate_args {
		int     fd;
		int     pad;
		off_t   length;
	} sa;

	sa.fd = uap->fd;
	sa.pad = 0;
	sa.length = uap->size;
	return ftruncate(p, &sa, retval);
}

struct xenix_nap_args {
	int millisec;
};

int
xenix_nap(p, uap, retval)
	struct proc *p;
	struct xenix_nap_args *uap;
	int *retval;
{
	return ENOSYS;
}
