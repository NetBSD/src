/*	$NetBSD: linux_misc_notalpha.c,v 1.4 1995/03/22 05:24:47 mycroft Exp $	*/

/*
 * Copyright (c) 1995 Frank van der Linden
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed for the NetBSD Project
 *      by Frank van der Linden
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Linux compatibility module. Try to deal with various Linux system calls.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/proc.h>
#include <sys/dir.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/filedesc.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/mman.h>
#include <sys/mount.h>
#include <sys/ptrace.h>
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

#include <sys/syscallargs.h>

#include <vm/vm.h>
#include <vm/vm_param.h>

#include <compat/linux/linux_types.h>
#include <compat/linux/linux_fcntl.h>
#include <compat/linux/linux_mmap.h>
#include <compat/linux/linux_syscallargs.h>
#include <compat/linux/linux_util.h>
#include <compat/linux/linux_dirent.h>

/*
 * The information on a terminated (or stopped) process needs
 * to be converted in order for Linux binaries to get a valid signal
 * number out of it.
 */
static int
bsd_to_linux_wstat(status)
	int *status;
{
	if (WIFSIGNALED(*status))
		*status = (*status & ~0177) |
		    bsd_to_linux_sig(WTERMSIG(*status));
	else if (WIFSTOPPED(*status))
		*status = (*status & ~0xff00) |
		    (bsd_to_linux_sig(WSTOPSIG(*status)) << 8);
}

/*
 * waitpid(2). Passed on to the NetBSD call, surrounded by code to
 * reserve some space for a NetBSD-style wait status, and converting
 * it to what Linux wants.
 */
int
linux_waitpid(p, uap, retval)
	struct proc *p;
	struct linux_waitpid_args /* {
		syscallarg(int) pid;
		syscallarg(int *) status;
		syscallarg(int) options;
	} */ *uap;
	register_t *retval;
{
	struct wait4_args w4a;
	int error, *status, tstat;
	caddr_t sg;

	sg = stackgap_init();
	status = (int *) stackgap_alloc(&sg, sizeof status);

	SCARG(&w4a, pid) = SCARG(uap, pid);
	SCARG(&w4a, status) = status;
	SCARG(&w4a, options) = SCARG(uap, options);
	SCARG(&w4a, rusage) = NULL;

	if ((error = wait4(p, &w4a, retval)))
		return error;

	if ((error = copyin(status, &tstat, sizeof tstat)))
		return error;

	bsd_to_linux_wstat(&tstat);

	return copyout(&tstat, SCARG(uap, status), sizeof tstat);
}

/*
 * This is very much the same as waitpid()
 */
int
linux_wait4(p, uap, retval)
	struct proc *p;
	struct linux_wait4_args /* {
		syscallarg(int) pid;
		syscallarg(int *) status;
		syscallarg(int) options;
		syscallarg(struct rusage *) rusage;
	} */ *uap;
	register_t *retval;
{
	struct wait4_args w4a;
	int error, *status, tstat;
	caddr_t sg;

	sg = stackgap_init();
	status = (int *) stackgap_alloc(&sg, sizeof status);

	SCARG(&w4a, pid) = SCARG(uap, pid);
	SCARG(&w4a, status) = status;
	SCARG(&w4a, options) = SCARG(uap, options);
	SCARG(&w4a, rusage) = SCARG(uap, rusage);

	if ((error = wait4(p, &w4a, retval)))
		return error;

	if ((error = copyin(status, &tstat, sizeof tstat)))
		return error;

	bsd_to_linux_wstat(&tstat);

	return copyout(&tstat, SCARG(uap, status), sizeof tstat);
}

/*
 * This is the old brk(2) call. I don't think anything in the Linux
 * world uses this anymore
 */
int
linux_break(p, uap, retval)
	struct proc *p;
	struct linux_brk_args /* {
		syscallarg(char *) nsize;
	} */ *uap;
	register_t *retval;
{
	return ENOSYS;
}

/*
 * Linux brk(2). The check if the new address is >= the old one is
 * done in the kernel in Linux. NetBSD does it in the library.
 */
int
linux_brk(p, uap, retval)
	struct proc *p;
	struct linux_brk_args /* {
		syscallarg(char *) nsize;
	} */ *uap;
	register_t *retval;
{
	char *nbrk = SCARG(uap, nsize);
	struct obreak_args oba;
	struct vmspace *vm = p->p_vmspace;
	int error = 0;
	caddr_t oldbrk, newbrk;

	oldbrk = vm->vm_daddr + ctob(vm->vm_dsize);
	/*
	 * XXX inconsistent.. Linux always returns at least the old
	 * brk value, but it will be page-aligned if this fails,
	 * and possibly not page aligned if it succeeds (the user
	 * supplied pointer is returned).
	 */
	SCARG(&oba, nsize) = nbrk;

	if ((caddr_t) nbrk > vm->vm_daddr && obreak(p, &oba, retval) == 0)
		retval[0] = (register_t) nbrk;
	else
		retval[0] = (register_t) oldbrk;

	return 0;
}

/*
 * I wonder why Linux has gettimeofday() _and_ time().. Still, we
 * need to deal with it.
 */
int
linux_time(p, uap, retval)
	struct proc *p;
	struct linux_time_args /* {
		linux_time_t *t;
	} */ *uap;
	register_t *retval;
{
	struct timeval atv;
	linux_time_t tt;
	int error;

	microtime(&atv);

	tt = atv.tv_sec;
	if (SCARG(uap, t) && (error = copyout(&tt, SCARG(uap, t), sizeof tt)))
		return error;

	retval[0] = tt;
	return 0;
}

/*
 * Convert BSD statfs structure to Linux statfs structure.
 * The Linux structure has less fields, and it also wants
 * the length of a name in a dir entry in a field, which
 * we fake (probably the wrong way).
 */
static void
bsd_to_linux_statfs(bsp, lsp)
	struct statfs *bsp;
	struct linux_statfs *lsp;
{
	lsp->l_ftype = bsp->f_type;
	lsp->l_fbsize = bsp->f_bsize;
	lsp->l_fblocks = bsp->f_blocks;
	lsp->l_fbfree = bsp->f_bfree;
	lsp->l_fbavail = bsp->f_bavail;
	lsp->l_ffiles = bsp->f_files;
	lsp->l_fffree = bsp->f_ffree;
	lsp->l_ffsid.val[0] = bsp->f_fsid.val[0];
	lsp->l_ffsid.val[1] = bsp->f_fsid.val[1];
	lsp->l_fnamelen = MAXNAMLEN;	/* XXX */
}

/*
 * Implement the fs stat functions. Straightforward.
 */
int
linux_statfs(p, uap, retval)
	struct proc *p;
	struct linux_statfs_args /* {
		syscallarg(char *) path;
		syscallarg(struct linux_statfs *) sp;
	} */ *uap;
	register_t *retval;
{
	struct statfs btmp, *bsp;
	struct linux_statfs ltmp;
	struct statfs_args bsa;
	caddr_t sg;
	int error;

	sg = stackgap_init();
	bsp = (struct statfs *) stackgap_alloc(&sg, sizeof (struct statfs));

	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));

	SCARG(&bsa, path) = SCARG(uap, path);
	SCARG(&bsa, buf) = bsp;

	if ((error = statfs(p, &bsa, retval)))
		return error;

	if ((error = copyin((caddr_t) bsp, (caddr_t) &btmp, sizeof btmp)))
		return error;

	bsd_to_linux_statfs(&btmp, &ltmp);

	return copyout((caddr_t) &ltmp, (caddr_t) SCARG(uap, sp), sizeof ltmp);
}

int
linux_fstatfs(p, uap, retval)
	struct proc *p;
	struct linux_fstatfs_args /* {
		syscallarg(int) fd;
		syscallarg(struct linux_statfs *) sp;
	} */ *uap;
	register_t *retval;
{
	struct statfs btmp, *bsp;
	struct linux_statfs ltmp;
	struct fstatfs_args bsa;
	caddr_t sg;
	int error;

	sg = stackgap_init();
	bsp = (struct statfs *) stackgap_alloc(&sg, sizeof (struct statfs));

	SCARG(&bsa, fd) = SCARG(uap, fd);
	SCARG(&bsa, buf) = bsp;

	if ((error = statfs(p, &bsa, retval)))
		return error;

	if ((error = copyin((caddr_t) bsp, (caddr_t) &btmp, sizeof btmp)))
		return error;

	bsd_to_linux_statfs(&btmp, &ltmp);

	return copyout((caddr_t) &ltmp, (caddr_t) SCARG(uap, sp), sizeof ltmp);
}

/*
 * uname(). Just copy the info from the various strings stored in the
 * kernel, and put it in the Linux utsname structure. That structure
 * is almost the same as the NetBSD one, only it has fields 65 characters
 * long, and an extra domainname field.
 */
int
linux_uname(p, uap, retval)
	struct proc *p;
	struct linux_uname_args /* {
		syscallarg(struct linux_utsname *) up;
	} */ *uap;
	register_t *retval;
{
	extern char ostype[], osrelease[], version[], hostname[], domainname[];
	extern char machine[];
	struct linux_utsname tluts;
	int len;
	char *cp;

	strncpy(tluts.l_sysname, ostype, sizeof (tluts.l_sysname));
	strncpy(tluts.l_nodename, hostname, sizeof (tluts.l_nodename));
	strncpy(tluts.l_release, osrelease, sizeof (tluts.l_release));
	strncpy(tluts.l_machine, machine, sizeof (tluts.l_machine));
	strncpy(tluts.l_domainname, domainname, sizeof (tluts.l_domainname));
	strncpy(tluts.l_version, version, sizeof (tluts.l_version));

	/* This part taken from the the uname() in libc */
	len = sizeof (tluts.l_version);
	for (cp = tluts.l_version; len--; ++cp)
		if (*cp == '\n' || *cp == '\t')
			if (len > 1)
				*cp = ' ';
			else
				*cp = '\0';

	return copyout(&tluts, SCARG(uap, up), sizeof tluts);
}

/*
 * Linux wants to pass everything to a syscall in registers. However,
 * mmap() has 6 of them. Oops: out of register error. They just pass
 * everything in a structure.
 */
int
linux_mmap(p, uap, retval)
	struct proc *p;
	struct linux_mmap_args /* {
		syscallarg(struct linux_mmap *) lmp;
	} */ *uap;
	register_t *retval;
{
	struct linux_mmap lmap;
	struct mmap_args cma;
	int error, flags;

	if ((error = copyin(SCARG(uap, lmp), &lmap, sizeof lmap)))
		return error;

	flags = 0;
	flags |= cvtto_bsd_mask(lmap.lm_flags, LINUX_MAP_SHARED, MAP_SHARED);
	flags |= cvtto_bsd_mask(lmap.lm_flags, LINUX_MAP_PRIVATE, MAP_PRIVATE);
	flags |= cvtto_bsd_mask(lmap.lm_flags, LINUX_MAP_FIXED, MAP_FIXED);
	flags |= cvtto_bsd_mask(lmap.lm_flags, LINUX_MAP_ANON, MAP_ANON);

	SCARG(&cma,addr) = lmap.lm_addr;
	SCARG(&cma,len) = lmap.lm_len;
 	SCARG(&cma,prot) = lmap.lm_prot;
	SCARG(&cma,flags) = flags;
	SCARG(&cma,fd) = lmap.lm_fd;
	SCARG(&cma,pad) = 0;
	SCARG(&cma,pos) = lmap.lm_pos;

	return mmap(p, &cma, retval);
}

/*
 * Linux doesn't use the retval[1] value to determine whether
 * we are the child or parent.
 */
int
linux_fork(p, uap, retval)
	struct proc *p;
	void *uap;
	register_t *retval;
{
	int error;

	if ((error = fork(p, uap, retval)))
		return error;

	if (retval[1] == 1)
		retval[0] = 0;

	return 0;
}

/*
 * This code is partly stolen from src/lib/libc/compat-43/times.c
 * XXX - CLK_TCK isn't declared in /sys, just in <time.h>, done here
 */

#define CLK_TCK 100
#define	CONVTCK(r)	(r.tv_sec * CLK_TCK + r.tv_usec / (1000000 / CLK_TCK))

int
linux_times(p, uap, retval)
	struct proc *p;
	struct linux_times_args /* {
		syscallarg(struct times *) tms;
	} */ *uap;
	register_t *retval;
{
	struct timeval t;
	struct linux_tms ltms;
	struct rusage ru;
	int error, s;

	calcru(p, &ru.ru_utime, &ru.ru_stime, NULL);
	ltms.ltms_utime = CONVTCK(ru.ru_utime);
	ltms.ltms_stime = CONVTCK(ru.ru_stime);

	ltms.ltms_cutime = CONVTCK(p->p_stats->p_cru.ru_utime);
	ltms.ltms_cstime = CONVTCK(p->p_stats->p_cru.ru_stime);

	if ((error = copyout(&ltms, SCARG(uap, tms), sizeof ltms)))
		return error;

	s = splclock();
	timersub(&time, &boottime, &t);
	splx(s);

	retval[0] = ((linux_clock_t)(CONVTCK(t)));
	return 0;
}

/*
 * NetBSD passes fd[0] in retval[0], and fd[1] in retval[1].
 * Linux directly passes the pointer.
 */
int
linux_pipe(p, uap, retval)
	struct proc *p;
	struct linux_pipe_args /* {
		syscallarg(int *) pfds;
	} */ *uap;
	register_t *retval;
{
	int error;

	if ((error = pipe(p, 0, retval)))
		return error;

	/* Assumes register_t is an int */

	if ((error = copyout(retval, SCARG(uap, pfds), 2 * sizeof (int))))
		return error;

	retval[0] = 0;
	return 0;
}

/*
 * Alarm. This is a libc call which used setitimer(2) in NetBSD.
 * Fiddle with the timers to make it work.
 */
int
linux_alarm(p, uap, retval)
	struct proc *p;
	struct linux_alarm_args /* {
		syscallarg(unsigned int) secs;
	} */ *uap;
	register_t *retval;
{
	int error, s;
	struct itimerval *itp, it;

	itp = &p->p_realtimer;
	s = splclock();
	/*
	 * Clear any pending timer alarms.
	 */
	untimeout(realitexpire, p);
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

	/*
	 * alarm(0) just resets the timer.
	 */
	if (SCARG(uap, secs) == 0) {
		timerclear(&itp->it_value);
		splx(s);
		return 0;
	}

	/*
	 * Check the new alarm time for sanity, and set it.
	 */
	timerclear(&it.it_interval);
	it.it_value.tv_sec = SCARG(uap, secs);
	it.it_value.tv_usec = 0;
	if (itimerfix(&it.it_value) || itimerfix(&it.it_interval)) {
		splx(s);
		return (EINVAL);
	}

	if (timerisset(&it.it_value)) {
		timeradd(&it.it_value, &time, &it.it_value);
		timeout(realitexpire, p, hzto(&it.it_value));
	}
	p->p_realtimer = it;
	splx(s);

	return 0;
}

/*
 * utime(). Do conversion to things that utimes() understands, 
 * and pass it on.
 */
int
linux_utime(p, uap, retval)
	struct proc *p;
	struct linux_utime_args /* {
		syscallarg(char *) path;
		syscallarg(struct linux_utimbuf *)times;
	} */ *uap;
	register_t *retval;
{
	caddr_t sg;
	int error;
	struct utimes_args ua;
	struct timeval tv[2], *tvp;
	struct linux_utimbuf lut;

	sg = stackgap_init();
	CHECK_ALT_EXIST(p, &sg, SCARG(uap, path));

	SCARG(&ua, path) = SCARG(uap, path);

	if (SCARG(uap, times) != NULL) {
		if ((error = copyin(SCARG(uap, times), &lut, sizeof lut)))
			return error;
		tv[0].tv_usec = tv[1].tv_usec = 0;
		tv[0].tv_sec = lut.l_actime;
		tv[1].tv_sec = lut.l_modtime;
		tvp = (struct timeval *) stackgap_alloc(sizeof tv);
		if ((error = copyout(tv, tvp, sizeof tv)))
			return error;
		SCARG(&ua, tptr) = tvp;
	}
	else
		SCARG(&ua, tptr) = NULL;

	return utimes(p, uap, retval);
}

/*
 * Linux 'readdir' call. This code is mostly taken from the
 * SunOS getdents call (see compat/sunos/sunos_misc.c), though
 * an attempt has been made to keep it a little cleaner (failing
 * miserably, because of the cruft needed if count 1 is passed).
 *
 * Read in BSD-style entries, convert them, and copy them out.
 * Note that the Linux d_reclen is actually the name length,
 * and d_off is the reclen.
 *
 * Note that this doesn't handle union-mounted filesystems.
 */
int
linux_readdir(p, uap, retval)
	struct proc *p;
	struct linux_readdir_args /* {
		syscallarg(int) fd;
		syscallarg(struct linux_dirent *) dent;
		syscallarg(unsigned int) count;
	} */ *uap;
	register_t *retval;
{
	register struct dirent *bdp;
	struct vnode *vp;
	caddr_t	inp, buf;	/* BSD-format */
	int len, reclen;	/* BSD-format */
	caddr_t outp;		/* Linux-format */
	int resid, linuxreclen;	/* Linux-format */
	struct file *fp;
	struct uio auio;
	struct iovec aiov;
	struct linux_dirent idb;
	off_t off;		/* true file offset */
	linux_off_t soff;	/* Linux file offset */
	int buflen, error, eofflag, nbytes, justone;
	struct vattr va;

	if ((error = getvnode(p->p_fd, SCARG(uap, fd), &fp)) != 0)
		return (error);

	if ((fp->f_flag & FREAD) == 0)
		return (EBADF);

	vp = (struct vnode *) fp->f_data;

	if (vp->v_type != VDIR)	/* XXX  vnode readdir op should do this */
		return (EINVAL);

	if ((error = VOP_GETATTR(vp, &va, p->p_ucred, p)))
		return error;

	nbytes = SCARG(uap, count);
	if (nbytes == 1) {	/* Need this for older Linux libs, apparently */
		nbytes = sizeof (struct linux_dirent);
		justone = 1;
	}
	else
		justone = 0;

	buflen = max(va.va_blocksize, nbytes);
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
	error = VOP_READDIR(vp, &auio, fp->f_cred, &eofflag, (u_long *) 0, 0);
	if (error)
		goto out;

	inp = buf;
	outp = (caddr_t) SCARG(uap, dent);
	resid = nbytes;
	if ((len = buflen - auio.uio_resid) == 0)
		goto eof;

	for (; len > 0; len -= reclen) {
		reclen = ((struct dirent *) inp)->d_reclen;
		if (reclen & 3)
			panic("linux_readdir");
		off += reclen;	/* each entry points to next */
		bdp = (struct dirent *) inp;
		if (bdp->d_fileno == 0) {
			inp += reclen;	/* it is a hole; squish it out */
			continue;
		}
		linuxreclen = LINUX_RECLEN(&idb, bdp->d_namlen);
		if (reclen > len || resid < linuxreclen) {
			/* entry too big for buffer, so just stop */
			outp++;
			break;
		}
		/*
		 * Massage in place to make a Linux-shaped dirent (otherwise
		 * we have to worry about touching user memory outside of
		 * the copyout() call).
		 */
		idb.l_dino = (long) bdp->d_fileno;
		idb.l_doff = (linux_off_t) linuxreclen;
		idb.l_dreclen = (u_short) bdp->d_namlen;	/* sigh */
		strcpy(idb.l_dname, bdp->d_name);
		if ((error = copyout((caddr_t)&idb, outp, linuxreclen)))
			goto out;
		/* advance past this real entry */
		inp += reclen;
		/* advance output past Linux-shaped entry */
		outp += linuxreclen;
		resid -= linuxreclen;
		if (justone)
			break;
	}

	/* if we squished out the whole block, try again */
	if (outp == (caddr_t) SCARG(uap, dent))
		goto again;
	fp->f_offset = off;	/* update the vnode offset */

	if (justone)
		nbytes = resid + linuxreclen;

eof:
	*retval = nbytes - resid;
out:
	VOP_UNLOCK(vp);
	free(buf, M_TEMP);
	return error;
}

/*
 * Out of register error once more.. Apart from that, no difference.
 */
int
linux_select(p, uap, retval)
	struct proc *p;
	struct linux_select_args /* {
		syscallarg(struct linux_select *) lsp;
	} */ *uap;
	register_t *retval;
{
	struct linux_select ls;
	struct select_args bsa;
	int error;

	if ((error = copyin(SCARG(uap, lsp), (caddr_t) &ls, sizeof ls)))
		return error;

	SCARG(&bsa, nd) = ls.nfds;
	SCARG(&bsa, in) = ls.readfds;
	SCARG(&bsa, ou) = ls.writefds;
	SCARG(&bsa, ex) = ls.exceptfds;
	SCARG(&bsa, tv) = ls.timeout;

	return select(p, &bsa, retval);
}

/*
 * Get the process group of a certain process. Look it up
 * and return the value.
 */
int
linux_getpgid(p, uap, retval)
	struct proc *p;
	struct linux_getpgid_args /* {
		syscallarg(int) pid;
	} */ *uap;
	register_t *retval;
{
	struct proc *targp;

	if (SCARG(uap, pid) != 0 && SCARG(uap, pid) != p->p_pid)
		if ((targp = pfind(SCARG(uap, pid))) == 0)
			return ESRCH;
	else
		targp = p;

	retval[0] = targp->p_pgid;
	return 0;
}
