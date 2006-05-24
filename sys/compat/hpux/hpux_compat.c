/*	$NetBSD: hpux_compat.c,v 1.73.12.1 2006/05/24 15:48:27 tron Exp $	*/

/*
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 * from: Utah $Hdr: hpux_compat.c 1.64 93/08/05$
 *
 *	@(#)hpux_compat.c	8.4 (Berkeley) 2/13/94
 */

/*
 * Copyright (c) 1988 University of Utah.
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
 * from: Utah $Hdr: hpux_compat.c 1.64 93/08/05$
 *
 *	@(#)hpux_compat.c	8.4 (Berkeley) 2/13/94
 */

/*
 * Various HP-UX compatibility routines
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: hpux_compat.c,v 1.73.12.1 2006/05/24 15:48:27 tron Exp $");

#if defined(_KERNEL_OPT)
#include "opt_sysv.h"
#include "opt_compat_43.h"
#endif

#ifndef COMPAT_43
#define COMPAT_43
#endif


#include <sys/param.h>
#include <sys/systm.h>
#include <sys/signalvar.h>
#include <sys/kernel.h>
#include <sys/filedesc.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/wait.h>
#include <sys/exec.h>
#include <sys/file.h>
#include <sys/namei.h>
#include <sys/vnode.h>
#include <sys/ioctl.h>
#include <sys/ptrace.h>
#include <sys/stat.h>
#include <sys/syslog.h>
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/ipc.h>
#include <sys/user.h>
#include <sys/mman.h>
#include <sys/kauth.h>

#include <machine/cpu.h>
#include <machine/reg.h>
#include <machine/psl.h>
#include <machine/vmparam.h>

#include <sys/sa.h>
#include <sys/syscallargs.h>

#include <compat/hpux/hpux.h>
#include <compat/hpux/hpux_sig.h>
#include <compat/hpux/hpux_util.h>
#include <compat/hpux/hpux_termio.h>
#include <compat/hpux/hpux_syscall.h>
#include <compat/hpux/hpux_syscallargs.h>

#include <machine/hpux_machdep.h>

#ifdef DEBUG
int unimpresponse = 0;
#endif

static int	hpuxtobsdioctl __P((u_long));
static int	hpux_scale __P((struct timeval *));

/*
 * HP-UX fork and vfork need to map the EAGAIN return value appropriately.
 */
int
hpux_sys_fork(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	/* struct hpux_sys_fork_args *uap = v; */
	int error;

	error = sys_fork(l, v, retval);
	if (error == EAGAIN)
		error = OEAGAIN;
	return (error);
}

int
hpux_sys_vfork(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	/* struct hpux_sys_vfork_args *uap = v; */
	int error;

	error = sys_vfork(l, v, retval);
	if (error == EAGAIN)
		error = OEAGAIN;
	return (error);
}

/*
 * HP-UX versions of wait and wait3 actually pass the parameters
 * (status pointer, options, rusage) into the kernel rather than
 * handling it in the C library stub.  We also need to map any
 * termination signal from BSD to HP-UX.
 */
int
hpux_sys_wait3(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct hpux_sys_wait3_args *uap = v;

	/* rusage pointer must be zero */
	if (SCARG(uap, rusage))
		return (EINVAL);
#if __mc68k__
	l->l_md.md_regs[PS] = PSL_ALLCC;
	l->l_md.md_regs[R0] = SCARG(uap, options);
	l->l_md.md_regs[R1] = SCARG(uap, rusage);
#endif

	return (hpux_sys_wait(l, uap, retval));
}

int
hpux_sys_wait(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct proc *p = l->l_proc;
	struct hpux_sys_wait_args *uap = v;
	struct sys_wait4_args w4;
	int error;
	int sig;
	size_t sz = sizeof(*SCARG(&w4, status));
	int status;

	SCARG(&w4, rusage) = NULL;
	SCARG(&w4, options) = 0;

	if (SCARG(uap, status) == NULL) {
		caddr_t sg = stackgap_init(p, 0);
		SCARG(&w4, status) = stackgap_alloc(p, &sg, sz);
	}
	else
		SCARG(&w4, status) = SCARG(uap, status);

	SCARG(&w4, pid) = WAIT_ANY;

	error = sys_wait4(l, &w4, retval);
	/*
	 * HP-UX wait always returns EINTR when interrupted by a signal
	 * (well, unless its emulating a BSD process, but we don't bother...)
	 */
	if (error == ERESTART)
		error = EINTR;
	if (error)
		return error;

	if ((error = copyin(SCARG(&w4, status), &status, sizeof(status))) != 0)
		return error;

	sig = status & 0xFF;
	if (sig == WSTOPPED) {
		sig = (status >> 8) & 0xFF;
		retval[1] = (bsdtohpuxsig(sig) << 8) | WSTOPPED;
	} else if (sig)
		retval[1] = (status & 0xFF00) |
			bsdtohpuxsig(sig & 0x7F) | (sig & 0x80);

	if (SCARG(uap, status) == NULL)
		return error;
	else
		return copyout(&retval[1],
			       SCARG(uap, status), sizeof(retval[1]));
}

int
hpux_sys_waitpid(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct hpux_sys_waitpid_args /* {
		syscallarg(pid_t) pid;
		syscallarg(int *) status;
		syscallarg(int) options;
		syscallarg(struct rusage *) rusage;
	} */ *uap = v;
	int rv, sig, xstat, error;

	SCARG(uap, rusage) = 0;
	error = sys_wait4(l, uap, retval);
	/*
	 * HP-UX wait always returns EINTR when interrupted by a signal
	 * (well, unless its emulating a BSD process, but we don't bother...)
	 */
	if (error == ERESTART)
		error = EINTR;
	if (error)
		return (error);

	if (SCARG(uap, status)) {
		/*
		 * Wait4 already wrote the status out to user space,
		 * pull it back, change the signal portion, and write
		 * it back out.
		 */
		error = copyin(SCARG(uap, status), &rv, sizeof(int));
		if (error)
			return (error);

		if (WIFSTOPPED(rv)) {
			sig = WSTOPSIG(rv);
			rv = W_STOPCODE(bsdtohpuxsig(sig));
		} else if (WIFSIGNALED(rv)) {
			sig = WTERMSIG(rv);
			xstat = WEXITSTATUS(rv);
			rv = W_EXITCODE(xstat, bsdtohpuxsig(sig)) |
				WCOREDUMP(rv);
		}

		error = copyout(&rv, SCARG(uap, status), sizeof(int));
	}
	return (error);
}

/*
 * Read and write calls.  Same as BSD except for non-blocking behavior.
 * There are three types of non-blocking reads/writes in HP-UX checked
 * in the following order:
 *
 *	O_NONBLOCK: return -1 and errno == EAGAIN
 *	O_NDELAY:   return 0
 *	FIOSNBIO:   return -1 and errno == EWOULDBLOCK
 */
int
hpux_sys_read(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct proc *p = l->l_proc;
	struct hpux_sys_read_args *uap = v;
	int error;

	error = sys_read(l, (struct sys_read_args *) uap, retval);
	if (error == EWOULDBLOCK) {
		/* sys_read validates fd before this indexing */
		char *fp = &p->p_fd->fd_ofileflags[SCARG(uap, fd)];

		if (*fp & HPUX_UF_NONBLOCK_ON) {
			*retval = -1;
			error = OEAGAIN;
		} else if (*fp & HPUX_UF_FNDELAY_ON) {
			*retval = 0;
			error = 0;
		}
	}
	return (error);
}

int
hpux_sys_write(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct proc *p = l->l_proc;
	struct hpux_sys_write_args *uap = v;
	int error;

	error = sys_write(l, (struct sys_write_args *) uap, retval);
	if (error == EWOULDBLOCK) {
		/* sys_write validates fd before this indexing */
		char *fp = &p->p_fd->fd_ofileflags[SCARG(uap, fd)];

		if (*fp & HPUX_UF_NONBLOCK_ON) {
			*retval = -1;
			error = OEAGAIN;
		} else if (*fp & HPUX_UF_FNDELAY_ON) {
			*retval = 0;
			error = 0;
		}
	}
	return (error);
}

int
hpux_sys_readv(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct proc *p = l->l_proc;
	struct hpux_sys_readv_args *uap = v;
	int error;

	error = sys_readv(l, (struct sys_readv_args *) uap, retval);
	if (error == EWOULDBLOCK) {
		/* sys_readv validates fd before this indexing */
		char *fp = &p->p_fd->fd_ofileflags[SCARG(uap, fd)];

		if (*fp & HPUX_UF_NONBLOCK_ON) {
			*retval = -1;
			error = OEAGAIN;
		} else if (*fp & HPUX_UF_FNDELAY_ON) {
			*retval = 0;
			error = 0;
		}
	}
	return (error);
}

int
hpux_sys_writev(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct proc *p = l->l_proc;
	struct hpux_sys_writev_args *uap = v;
	int error;

	if (SCARG(uap, fd) < 0)
		return EBADF;

	error = sys_writev(l, (struct sys_writev_args *) uap, retval);
	if (error == EWOULDBLOCK) {
		/* sys_writev validates fd before this indexing */
		char *fp = &p->p_fd->fd_ofileflags[SCARG(uap, fd)];

		if (*fp & HPUX_UF_NONBLOCK_ON) {
			*retval = -1;
			error = OEAGAIN;
		} else if (*fp & HPUX_UF_FNDELAY_ON) {
			*retval = 0;
			error = 0;
		}
	}
	return (error);
}

int
hpux_sys_utssys(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct hpux_sys_utssys_args *uap = v;
	int i;
	int error;
	struct hpux_utsname	ut;

	switch (SCARG(uap, request)) {
	/* uname */
	case 0:
		memset(&ut, 0, sizeof(ut));

		strncpy(ut.sysname, ostype, sizeof(ut.sysname));
		ut.sysname[sizeof(ut.sysname) - 1] = '\0';

		/* copy hostname (sans domain) to nodename */
		for (i = 0; i < 8 && hostname[i] != '.'; i++)
			ut.nodename[i] = hostname[i];
		ut.nodename[i] = '\0';

		strncpy(ut.release, osrelease, sizeof(ut.release));
		ut.release[sizeof(ut.release) - 1] = '\0';

		strncpy(ut.version, version, sizeof(ut.version));
		ut.version[sizeof(ut.version) - 1] = '\0';

		strncpy(ut.machine, machine, sizeof(ut.machine));
		ut.machine[sizeof(ut.machine) - 1] = '\0';

		error = copyout((caddr_t)&ut,
		    (caddr_t)SCARG(uap, uts), sizeof(ut));
		break;

	/* gethostname */
	case 5:
		/* SCARG(uap, dev) is length */
		i = SCARG(uap, dev);
		if (i < 0) {
			error = EINVAL;
			break;
		} else if (i > hostnamelen + 1)
			i = hostnamelen + 1;
		error = copyout((caddr_t)hostname, (caddr_t)SCARG(uap, uts), i);
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

int
hpux_sys_sysconf(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct hpux_sys_sysconf_args *uap = v;

	switch (SCARG(uap, name)) {
	case HPUX_SYSCONF_ARGMAX:
		*retval = ARG_MAX;
		break;
	case HPUX_SYSCONF_CHILDMAX:
		*retval = maxproc;
		break;
	case HPUX_SYSCONF_CLKTICK:
		*retval = hz;
		break;
	case HPUX_SYSCONF_NGRPMAX:
		*retval = NGROUPS_MAX;
		break;
	case HPUX_SYSCONF_OPENMAX:
		*retval = maxfiles;
		break;
	case HPUX_SYSCONF_JOBCNTRL:
		*retval = 1;
		break;
	case HPUX_SYSCONF_SAVEDIDS:
#ifdef _POSIX_SAVED_IDS
		*retval = 1;
#else
		*retval = 0;
#endif
		break;
	case HPUX_SYSCONF_VERSION:
		*retval = 198808;
		break;
	/* architecture */
	case HPUX_SYSCONF_CPUTYPE:
		*retval = hpux_cpu_sysconf_arch();
		break;
	default:
		/* XXX */
		uprintf("HP-UX sysconf(%d) not implemented\n",
		    SCARG(uap, name));
		return (EINVAL);
	}
	return (0);
}

int
hpux_sys_ulimit(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct proc *p = l->l_proc;
	struct hpux_sys_ulimit_args *uap = v;
	struct rlimit *limp;
	int error = 0;

	limp = &p->p_rlimit[RLIMIT_FSIZE];
	switch (SCARG(uap, cmd)) {
	case 2:
		SCARG(uap, newlimit) *= 512;
		if (SCARG(uap, newlimit) > limp->rlim_max &&
		    (error = kauth_authorize_generic(p->p_cred, KAUTH_GENERIC_ISSUSER, &p->p_acflag)))
			break;
		limp->rlim_cur = limp->rlim_max = SCARG(uap, newlimit);
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
int
hpux_sys_rtprio(lp, v, retval)
	struct lwp *lp;
	void *v;
	register_t *retval;
{
	struct hpux_sys_rtprio_args *uap = v;
	struct proc *p;
	int nice, error;

	if (SCARG(uap, prio) < RTPRIO_MIN && SCARG(uap, prio) > RTPRIO_MAX &&
	    SCARG(uap, prio) != RTPRIO_NOCHG &&
	    SCARG(uap, prio) != RTPRIO_RTOFF)
		return (EINVAL);
	if (SCARG(uap, pid) == 0)
		p = lp->l_proc;
	else if ((p = pfind(SCARG(uap, pid))) == 0)
		return (ESRCH);
	nice = p->p_nice - NZERO;
	if (nice < 0)
		*retval = (nice + 16) << 3;
	else
		*retval = RTPRIO_RTOFF;
	switch (SCARG(uap, prio)) {

	case RTPRIO_NOCHG:
		return (0);

	case RTPRIO_RTOFF:
		if (nice >= 0)
			return (0);
		nice = 0;
		break;

	default:
		nice = (SCARG(uap, prio) >> 3) - 16;
		break;
	}
	error = donice(lp->l_proc, p, nice);
	if (error == EACCES)
		error = EPERM;
	return (error);
}

/* hpux_sys_advise() is found in hpux_machdep.c */

#if 0 /* XXX - This really, really doesn't work anymore. --scottr */
int
hpux_sys_ptrace(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct hpux_sys_ptrace_args *uap = v;
	int error;
#if defined(PT_READ_U) || defined(PT_WRITE_U)
	int isps = 0;
	struct lwp *lp;
#endif

	switch (SCARG(uap, req)) {
	/* map signal */
#if defined(PT_STEP) || defined(PT_CONTINUE)
# ifdef PT_STEP
	case PT_STEP:
# endif
# ifdef PT_CONTINUE
	case PT_CONTINUE:
# endif
		if (SCARG(uap, data)) {
			SCARG(uap, data) = hpuxtobsdsig(SCARG(uap, data));
			if (SCARG(uap, data) == 0)
				SCARG(uap, data) = NSIG;
		}
		break;
#endif
	/* map u-area offset */
#if defined(PT_READ_U) || defined(PT_WRITE_U)
# ifdef PT_READ_U
	case PT_READ_U:
# endif
# ifdef PT_WRITE_U
	case PT_WRITE_U:
# endif
		/*
		 * Big, cheezy hack: hpux_to_bsd_uoff is really intended
		 * to be called in the child context (procxmt) but we
		 * do it here in the parent context to avoid hacks in
		 * the MI sys_process.c file.  This works only because
		 * we can access the child's md_regs pointer and it
		 * has the correct value (the child has already trapped
		 * into the kernel).
		 */
		if ((cp = pfind(SCARG(uap, pid))) == 0)
			return (ESRCH);
		SCARG(uap, addr) =
		    (int *)hpux_to_bsd_uoff(SCARG(uap, addr), &isps, cp);

		/*
		 * Since HP-UX PS is only 16-bits in ar0, requests
		 * to write PS actually contain the PS in the high word
		 * and the high half of the PC (the following register)
		 * in the low word.  Move the PS value to where BSD
		 * expects it.
		 */
		if (isps && SCARG(uap, req) == PT_WRITE_U)
			SCARG(uap, data) >>= 16;
		break;
#endif
	}

	error = sys_ptrace(l, uap, retval);
	/*
	 * Align PS as HP-UX expects it (see WRITE_U comment above).
	 * Note that we do not return the high part of PC like HP-UX
	 * would, but the HP-UX debuggers don't require it.
	 */
#ifdef PT_READ_U
	if (isps && error == 0 && SCARG(uap, req) == PT_READ_U)
		*retval <<= 16;
#endif
	return (error);
}
#endif

/*
 * HP-UX mmap() emulation (mainly for shared library support).
 */
int
hpux_sys_mmap(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct hpux_sys_mmap_args *uap = v;
	struct sys_mmap_args /* {
		syscallarg(caddr_t) addr;
		syscallarg(size_t) len;
		syscallarg(int) prot;
		syscallarg(int) flags;
		syscallarg(int) fd;
		syscallarg(long) pad;
		syscallarg(off_t) pos;
	} */ nargs;

	SCARG(&nargs, addr) = SCARG(uap, addr);
	SCARG(&nargs, len) = SCARG(uap, len);
	SCARG(&nargs, prot) = SCARG(uap, prot);
	SCARG(&nargs, flags) = SCARG(uap, flags) &
		~(HPUXMAP_FIXED|HPUXMAP_REPLACE|HPUXMAP_ANON);
	if (SCARG(uap, flags) & HPUXMAP_FIXED)
		SCARG(&nargs, flags) |= MAP_FIXED;
	if (SCARG(uap, flags) & HPUXMAP_ANON)
		SCARG(&nargs, flags) |= MAP_ANON;
	SCARG(&nargs, fd) = (SCARG(&nargs, flags) & MAP_ANON) ? -1 : SCARG(uap, fd);
	SCARG(&nargs, pos) = SCARG(uap, pos);

	return (sys_mmap(l, &nargs, retval));
}

static int
hpuxtobsdioctl(com)
	u_long com;
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
	case HPUXTIOCGWINSZ:
		com = TIOCGWINSZ; break;
	case HPUXTIOCSWINSZ:
		com = TIOCSWINSZ; break;
	}
	return(com);
}

/*
 * HP-UX ioctl system call.  The differences here are:
 *	IOC_IN also means IOC_VOID if the size portion is zero.
 *	no FIOCLEX/FIONCLEX/FIOASYNC/FIOGETOWN/FIOSETOWN
 *	the sgttyb struct is 2 bytes longer
 */
int
hpux_sys_ioctl(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct hpux_sys_ioctl_args /* {
		syscallarg(int) fd;
		syscallarg(int) com;
		syscallarg(caddr_t) data;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct filedesc *fdp = p->p_fd;
	struct file *fp;
	int com, error = 0;
	u_int size;
	caddr_t memp = 0;
#define STK_PARAMS	128
	char stkbuf[STK_PARAMS];
	caddr_t dt = stkbuf;

	com = SCARG(uap, com);

	/* XXX */
	if (com == HPUXTIOCGETP || com == HPUXTIOCSETP)
		return (getsettty(l, SCARG(uap, fd), com, SCARG(uap, data)));

	/*
	 * Interpret high order word to find
	 * amount of data to be copied to/from the
	 * user's address space.
	 */
	size = IOCPARM_LEN(com);
	if (size > IOCPARM_MAX)
		return (ENOTTY);

	if ((fp = fd_getfile(fdp, SCARG(uap, fd))) == NULL)
		return (EBADF);

	FILE_USE(fp);

	if ((fp->f_flag & (FREAD|FWRITE)) == 0) {
		error = EBADF;
		goto out;
	}

	if (size > sizeof (stkbuf)) {
		memp = (caddr_t)malloc((u_long)size, M_IOCTLOPS, M_WAITOK);
		dt = memp;
	}

	if (com&IOC_IN) {
		if (size) {
			error = copyin(SCARG(uap, data), dt, (u_int)size);
			if (error)
				goto out;
		} else
			*(caddr_t *)dt = SCARG(uap, data);
	} else if ((com&IOC_OUT) && size)
		/*
		 * Zero the buffer so the user always
		 * gets back something deterministic.
		 */
		memset(dt, 0, size);
	else if (com&IOC_VOID)
		*(caddr_t *)dt = SCARG(uap, data);

	switch (com) {

	case HPUXFIOSNBIO:
	{
		/* This array index is validated by fd_getfile */
		char *ofp = &fdp->fd_ofileflags[SCARG(uap, fd)];
		int tmp;

		if (*(int *)dt)
			*ofp |= HPUX_UF_FIONBIO_ON;
		else
			*ofp &= ~HPUX_UF_FIONBIO_ON;
		/*
		 * Only set/clear if O_NONBLOCK/FNDELAY not in effect
		 */
		if ((*ofp & (HPUX_UF_NONBLOCK_ON|HPUX_UF_FNDELAY_ON)) == 0) {
			tmp = *ofp & HPUX_UF_FIONBIO_ON;
			error = (*fp->f_ops->fo_ioctl)(fp, FIONBIO,
						       (caddr_t)&tmp, l);
		}
		break;
	}

	case HPUXTIOCCONS:
		*(int *)dt = 1;
		error = (*fp->f_ops->fo_ioctl)(fp, TIOCCONS, dt, l);
		break;

	/* BSD-style job control ioctls */
	case HPUXTIOCLBIS:
	case HPUXTIOCLBIC:
	case HPUXTIOCLSET:
		*(int *)dt &= HPUXLTOSTOP;
		if (*(int *)dt & HPUXLTOSTOP)
			*(int *)dt = LTOSTOP;
		/* fall into */

	/* simple mapping cases */
	case HPUXTIOCLGET:
	case HPUXTIOCSLTC:
	case HPUXTIOCGLTC:
	case HPUXTIOCSPGRP:
	case HPUXTIOCGPGRP:
	case HPUXTIOCGWINSZ:
	case HPUXTIOCSWINSZ:
		error = (*fp->f_ops->fo_ioctl)
			(fp, hpuxtobsdioctl(com), dt, l);
		if (error == 0 && com == HPUXTIOCLGET) {
			*(int *)dt &= LTOSTOP;
			if (*(int *)dt & LTOSTOP)
				*(int *)dt = HPUXLTOSTOP;
		}
		break;

	/* SYS 5 termio and POSIX termios */
	case HPUXTCGETA:
	case HPUXTCSETA:
	case HPUXTCSETAW:
	case HPUXTCSETAF:
	case HPUXTCGETATTR:
	case HPUXTCSETATTR:
	case HPUXTCSETATTRD:
	case HPUXTCSETATTRF:
		error = hpux_termio(SCARG(uap, fd), com, dt, l);
		break;

	default:
		error = (*fp->f_ops->fo_ioctl)(fp, com, dt, l);
		break;
	}
	/*
	 * Copy any data to user, size was
	 * already set and checked above.
	 */
	if (error == 0 && (com&IOC_OUT) && size)
		error = copyout(dt, SCARG(uap, data), (u_int)size);
out:
	FILE_UNUSE(fp, l);
	if (memp)
		free(memp, M_IOCTLOPS);
	return (error);
}

/* hpux_sys_getcontext() is found in hpux_machdep.c */

/*
 * This is the equivalent of BSD getpgrp but with more restrictions.
 * Note we do not check the real uid or "saved" uid.
 */
int
hpux_sys_getpgrp2(lp, v, retval)
	struct lwp *lp;
	void *v;
	register_t *retval;
{
	struct hpux_sys_getpgrp2_args *uap = v;
	struct proc *cp = lp->l_proc;
	struct proc *p;

	if (SCARG(uap, pid) == 0)
		SCARG(uap, pid) = cp->p_pid;
	p = pfind(SCARG(uap, pid));
	if (p == 0)
		return (ESRCH);
	if (kauth_cred_geteuid(cp->p_cred) &&
	    kauth_cred_geteuid(p->p_cred) != kauth_cred_geteuid(cp->p_cred) &&
	    !inferior(p, cp))
		return (EPERM);
	*retval = p->p_pgid;
	return (0);
}

/*
 * This is the equivalent of BSD setpgrp but with more restrictions.
 * Note we do not check the real uid or "saved" uid or pgrp.
 */
int
hpux_sys_setpgrp2(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct hpux_sys_setpgrp2_args *uap = v;

	/* empirically determined */
	if (SCARG(uap, pgid) < 0 || SCARG(uap, pgid) >= 30000)
		return (EINVAL);
	return (sys_setpgid(l, uap, retval));
}

/*
 * XXX Same as BSD setre[ug]id right now.  Need to consider saved ids.
 */
int
hpux_sys_setresuid(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct hpux_sys_setresuid_args *uap = v;

	return (sys_setreuid(l, uap, retval));
}

int
hpux_sys_setresgid(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct hpux_sys_setresgid_args *uap = v;

	return (sys_setregid(l, uap, retval));
}

int
hpux_sys_getrlimit(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct hpux_sys_getrlimit_args *uap = v;
	struct compat_43_sys_getrlimit_args ap;

	if (SCARG(uap, which) > HPUXRLIMIT_NOFILE)
		return (EINVAL);
	if (SCARG(uap, which) == HPUXRLIMIT_NOFILE)
		SCARG(uap, which) = RLIMIT_NOFILE;

	SCARG(&ap, which) = SCARG(uap, which);
	SCARG(&ap, rlp) = SCARG(uap, rlp);

	return (compat_43_sys_getrlimit(l, uap, retval));
}

int
hpux_sys_setrlimit(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct hpux_sys_setrlimit_args *uap = v;
	struct compat_43_sys_setrlimit_args ap;

	if (SCARG(uap, which) > HPUXRLIMIT_NOFILE)
		return (EINVAL);
	if (SCARG(uap, which) == HPUXRLIMIT_NOFILE)
		SCARG(uap, which) = RLIMIT_NOFILE;

	SCARG(&ap, which) = SCARG(uap, which);
	SCARG(&ap, rlp) = SCARG(uap, rlp);

	return (compat_43_sys_setrlimit(l, uap, retval));
}

/*
 * XXX: simple recognition hack to see if we can make grmd work.
 */
int
hpux_sys_lockf(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	/* struct hpux_sys_lockf_args *uap = v; */

	return (0);
}

int
hpux_sys_getaccess(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct proc *p = l->l_proc;
	struct hpux_sys_getaccess_args *uap = v;
	int lgroups[NGROUPS];
	int error = 0;
	kauth_cred_t cred;
	struct vnode *vp;
	struct nameidata nd;
	gid_t gid;

	/*
	 * Build an appropriate credential structure
	 */
	cred = kauth_cred_dup(p->p_cred);
	switch (SCARG(uap, uid)) {
	case 65502:	/* UID_EUID */
		break;
	case 65503:	/* UID_RUID */
		kauth_cred_seteuid(cred, kauth_cred_getuid(p->p_cred));
		break;
	case 65504:	/* UID_SUID */
		error = EINVAL;
		break;
	default:
		if (SCARG(uap, uid) > 65504)
			error = EINVAL;
		kauth_cred_seteuid(cred, SCARG(uap, uid));
		break;
	}
	switch (SCARG(uap, ngroups)) {
	case -1:	/* NGROUPS_EGID */
		gid = kauth_cred_getegid(cred);
		kauth_cred_setgroups(cred, &gid, 1, -1);
		break;
	case -5:	/* NGROUPS_EGID_SUPP */
		break;
	case -2:	/* NGROUPS_RGID */
		kauth_cred_setegid(cred, kauth_cred_getgid(p->p_cred));
		gid = kauth_cred_geteuid(p->p_cred);
		kauth_cred_setgroups(cred, &gid, 1, -1);
		break;
	case -6:	/* NGROUPS_RGID_SUPP */
		kauth_cred_setegid(cred, kauth_cred_getgid(p->p_cred));
		break;
	case -3:	/* NGROUPS_SGID */
	case -7:	/* NGROUPS_SGID_SUPP */
		error = EINVAL;
		break;
	case -4:	/* NGROUPS_SUPP */
		if (kauth_cred_ngroups(cred) > 1)
			kauth_cred_setegid(cred, kauth_cred_group(cred, 1));
		else
			error = EINVAL;
		break;
	default:
		if (SCARG(uap, ngroups) > 0 && SCARG(uap, ngroups) <= NGROUPS)
			error = copyin(SCARG(uap, gidset), &lgroups[0],
			    SCARG(uap, ngroups) * sizeof(lgroups[0]));
		else
			error = EINVAL;
		if (error == 0)
			kauth_cred_setgroups(cred, lgroups,
			    SCARG(uap, ngroups), -1);
		break;
	}
	/*
	 * Lookup file using caller's effective IDs.
	 */
	if (error == 0) {
		NDINIT(&nd, LOOKUP, FOLLOW | LOCKLEAF, UIO_USERSPACE,
			SCARG(uap, path), l);
		error = namei(&nd);
	}
	if (error) {
		kauth_cred_free(cred);
		return (error);
	}
	/*
	 * Use the constructed credentials for access checks.
	 */
	vp = nd.ni_vp;
	*retval = 0;
	if (VOP_ACCESS(vp, VREAD, cred, l) == 0)
		*retval |= R_OK;
	if (vn_writechk(vp) == 0 && VOP_ACCESS(vp, VWRITE, cred, l) == 0)
		*retval |= W_OK;
	if (VOP_ACCESS(vp, VEXEC, cred, l) == 0)
		*retval |= X_OK;
	vput(vp);
	kauth_cred_free(cred);
	return (error);
}

/* hpux_to_bsd_uoff() is found in hpux_machdep.c */

/*
 * Ancient HP-UX system calls.  Some 9.x executables even use them!
 */
#define HPUX_HZ	50

#include <sys/times.h>


/*
 * SYS V style setpgrp()
 */
int
hpux_sys_setpgrp_6x(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct proc *p = l->l_proc;

	if (p->p_pid != p->p_pgid)
		enterpgrp(p, p->p_pid, 0);
	*retval = p->p_pgid;
	return (0);
}

int
hpux_sys_time_6x(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct hpux_sys_time_6x_args /* {
		syscallarg(time_t *) t;
	} */ *uap = v;
	int error = 0;
	struct timeval tv;

	microtime(&tv);
        if (SCARG(uap, t) != NULL)
		error = copyout(&tv.tv_sec, SCARG(uap, t), sizeof(time_t));

	*retval = (register_t)tv.tv_sec;
	return (error);
}

int
hpux_sys_stime_6x(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct hpux_sys_stime_6x_args /* {
		syscallarg(int) time;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct timeval tv;
	int s, error;

	tv.tv_sec = SCARG(uap, time);
	tv.tv_usec = 0;
	if ((error = kauth_authorize_generic(p->p_cred, KAUTH_GENERIC_ISSUSER, &p->p_acflag)))
		return (error);

	/* WHAT DO WE DO ABOUT PENDING REAL-TIME TIMEOUTS??? */
	boottime.tv_sec += tv.tv_sec - time.tv_sec;
	s = splclock(); time = tv; splx(s);
	resettodr();
	return (0);
}

int
hpux_sys_ftime_6x(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct hpux_sys_ftime_6x_args /* {
		syscallarg(struct hpux_timeb *) tp;
	} */ *uap = v;
	struct hpux_otimeb tb;
	int s;

	s = splclock();
	tb.time = time.tv_sec;
	tb.millitm = time.tv_usec / 1000;
	splx(s);
	/* NetBSD has no kernel notion of timezone -- fake it. */
	tb.timezone = 0;
	tb.dstflag = 0;
	return (copyout((caddr_t)&tb, (caddr_t)SCARG(uap, tp), sizeof (tb)));
}

int
hpux_sys_alarm_6x(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct hpux_sys_alarm_6x_args /* {
		syscallarg(int) deltat;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	int s;
	struct itimerval *itp, it;
	struct ptimer *ptp;

 	if (p->p_timers && p->p_timers->pts_timers[ITIMER_REAL]) {
		ptp = p->p_timers->pts_timers[ITIMER_REAL];
		itp = &ptp->pt_time;
	} else
		itp = NULL;

	s = splhigh();

	/*
	 * Clear any pending timer alarms.
	 */
	if (itp) {
		callout_stop(&p->p_timers->pts_timers[ITIMER_REAL]->pt_ch);
		timerclear(&itp->it_interval);
		if (timerisset(&itp->it_value) &&
		    timercmp(&itp->it_value, &time, >))
			timersub(&itp->it_value, &time, &itp->it_value);
		/*
		 * Return how many seconds were left (rounded up)
		 */
		retval[0] = itp->it_value.tv_sec;
		if (itp->it_value.tv_usec)
			retval[0]++;
	} else {
		retval[0] = 0;
	}

	/*
	 * alarm(0) just resets the timer.
	 */
	if (SCARG(uap, deltat) == 0) {
		if (itp)
			timerclear(&itp->it_value);
		splx(s);
		return (0);
	}

	/*
	 * Check the new alarm time for sanity, and set it.
	 */
	timerclear(&it.it_interval);
	it.it_value.tv_sec = SCARG(uap, deltat);
	it.it_value.tv_usec = 0;
	if (itimerfix(&it.it_value) || itimerfix(&it.it_interval)) {
		splx(s);
		return (EINVAL);
	}
	if (p->p_timers == NULL)
		timers_alloc(p);
	ptp = p->p_timers->pts_timers[ITIMER_REAL];
	if (ptp == NULL) {
		ptp = pool_get(&ptimer_pool, PR_WAITOK);
		ptp->pt_ev.sigev_notify = SIGEV_SIGNAL;
		ptp->pt_ev.sigev_signo = SIGALRM;
		ptp->pt_overruns = 0;
		ptp->pt_proc = p;
		ptp->pt_type = CLOCK_REALTIME;
		ptp->pt_entry = CLOCK_REALTIME;
		p->p_timers->pts_timers[ITIMER_REAL] = ptp;
		callout_init(&ptp->pt_ch);
	}

	if (timerisset(&it.it_value)) {
		/*
		 * We don't need to check the hzto() return value, here.
		 * callout_reset() does it for us.
		 */
		timeradd(&it.it_value, &time, &it.it_value);
		callout_reset(&ptp->pt_ch, hzto(&ptp->pt_time.it_value),
		    realtimerexpire, ptp);
	}
	ptp->pt_time = it;
	splx(s);

	return (0);
}

int
hpux_sys_nice_6x(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct hpux_sys_nice_6x_args /* {
		syscallarg(int) nval;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	int error;

	error = donice(p, p, (p->p_nice - NZERO) + SCARG(uap, nval));
	if (error == 0)
		*retval = p->p_nice - NZERO;
	return (error);
}

int
hpux_sys_times_6x(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct hpux_sys_times_6x_args /* {
		syscallarg(struct tms *) tms;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct timeval ru, rs, tv;
	struct tms atms;
	int error;

	calcru(p, &ru, &rs, NULL);
	atms.tms_utime = hpux_scale(&ru);
	atms.tms_stime = hpux_scale(&rs);
	atms.tms_cutime = hpux_scale(&p->p_stats->p_cru.ru_utime);
	atms.tms_cstime = hpux_scale(&p->p_stats->p_cru.ru_stime);
	error = copyout((caddr_t)&atms, (caddr_t)SCARG(uap, tms),
	    sizeof (atms));
	if (error == 0) {
		tv = time;
		*(time_t *)retval = hpux_scale(&tv) -
		    hpux_scale(&boottime);
	}
	return (error);
}

/*
 * Doesn't exactly do what the documentation says.
 * What we really do is return 1/HPUX_HZ-th of a second since that
 * is what HP-UX returns.
 */
static int
hpux_scale(tvp)
	struct timeval *tvp;
{
	return (tvp->tv_sec * HPUX_HZ + tvp->tv_usec * HPUX_HZ / 1000000);
}

/*
 * Set IUPD and IACC times on file.
 * Can't set ICHG.
 */
int
hpux_sys_utime_6x(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct hpux_sys_utime_6x_args /* {
		syscallarg(char *) fname;
		syscallarg(time_t *) tptr;
	} */ *uap = v;
	struct vnode *vp;
	struct vattr vattr;
	time_t tv[2];
	int error;
	struct nameidata nd;

	if (SCARG(uap, tptr)) {
		error = copyin((caddr_t)SCARG(uap, tptr), (caddr_t)tv,
		    sizeof (tv));
		if (error)
			return (error);
	} else
		tv[0] = tv[1] = time.tv_sec;
	vattr_null(&vattr);
	vattr.va_atime.tv_sec = tv[0];
	vattr.va_atime.tv_nsec = 0;
	vattr.va_mtime.tv_sec = tv[1];
	vattr.va_mtime.tv_nsec = 0;
	NDINIT(&nd, LOOKUP, FOLLOW | LOCKLEAF, UIO_USERSPACE,
	    SCARG(uap, fname), l);
	if ((error = namei(&nd)))
		return (error);
	vp = nd.ni_vp;
	if (vp->v_mount->mnt_flag & MNT_RDONLY)
		error = EROFS;
	else
		error = VOP_SETATTR(vp, &vattr, nd.ni_cnd.cn_cred, l);
	vput(vp);
	return (error);
}

int
hpux_sys_pause_6x(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct proc *p = l->l_proc;

	return (sigsuspend1(p, &p->p_sigctx.ps_sigmask));
}
