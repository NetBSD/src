/*	$NetBSD: linux32_unistd.c,v 1.35.14.1 2014/08/20 00:03:33 tls Exp $ */

/*-
 * Copyright (c) 2006 Emmanuel Dreyfus, all rights reserved.
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
 *	This product includes software developed by Emmanuel Dreyfus
 * 4. The name of the author may not be used to endorse or promote 
 *    products derived from this software without specific prior written 
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE THE AUTHOR AND CONTRIBUTORS ``AS IS'' 
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS 
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>

__KERNEL_RCSID(0, "$NetBSD: linux32_unistd.c,v 1.35.14.1 2014/08/20 00:03:33 tls Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/fstypes.h>
#include <sys/signal.h>
#include <sys/dirent.h>
#include <sys/kernel.h>
#include <sys/fcntl.h>
#include <sys/select.h>
#include <sys/proc.h>
#include <sys/ucred.h>
#include <sys/swap.h>
#include <sys/kauth.h>
#include <sys/filedesc.h>
#include <sys/vfs_syscalls.h>

#include <machine/types.h>

#include <sys/syscallargs.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_conv.h>

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_signal.h>
#include <compat/linux/common/linux_machdep.h>
#include <compat/linux/common/linux_misc.h>
#include <compat/linux/common/linux_oldolduname.h>
#include <compat/linux/common/linux_ipc.h>
#include <compat/linux/common/linux_sem.h>
#include <compat/linux/common/linux_fcntl.h>
#include <compat/linux/linux_syscallargs.h>

#include <compat/linux32/common/linux32_types.h>
#include <compat/linux32/common/linux32_signal.h>
#include <compat/linux32/common/linux32_machdep.h>
#include <compat/linux32/common/linux32_sched.h>
#include <compat/linux32/common/linux32_sysctl.h>
#include <compat/linux32/common/linux32_socketcall.h>
#include <compat/linux32/linux32_syscallargs.h>

static int linux32_select1(struct lwp *, register_t *, 
    int, fd_set *, fd_set *, fd_set *, struct timeval *);

int
linux32_sys_brk(struct lwp *l, const struct linux32_sys_brk_args *uap, register_t *retval)
{
	/* {
		syscallarg(netbsd32_charp) nsize;
	} */
	struct linux_sys_brk_args ua;

	NETBSD32TOP_UAP(nsize, char);
	return linux_sys_brk(l, &ua, retval);
}

int
linux32_sys_llseek(struct lwp *l, const struct linux32_sys_llseek_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
                syscallarg(u_int32_t) ohigh;
                syscallarg(u_int32_t) olow;
		syscallarg(netbsd32_voidp) res;
		syscallarg(int) whence;
	} */
	struct linux_sys_llseek_args ua;

	NETBSD32TO64_UAP(fd);
	NETBSD32TO64_UAP(ohigh);
	NETBSD32TO64_UAP(olow);
	NETBSD32TOP_UAP(res, void);
	NETBSD32TO64_UAP(whence);

	return linux_sys_llseek(l, &ua, retval);
}

int
linux32_sys_select(struct lwp *l, const struct linux32_sys_select_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) nfds;
		syscallarg(netbsd32_fd_setp_t) readfds;
		syscallarg(netbsd32_fd_setp_t) writefds;
		syscallarg(netbsd32_fd_setp_t) exceptfds;
		syscallarg(netbsd32_timeval50p_t) timeout;
	} */

	return linux32_select1(l, retval, SCARG(uap, nfds), 
	    SCARG_P32(uap, readfds),
	    SCARG_P32(uap, writefds), 
	    SCARG_P32(uap, exceptfds), 
	    SCARG_P32(uap, timeout));
}

int
linux32_sys_oldselect(struct lwp *l, const struct linux32_sys_oldselect_args *uap, register_t *retval)
{
	/* {
		syscallarg(linux32_oldselectp_t) lsp;
	} */
	struct linux32_oldselect lsp32;
	int error;

	if ((error = copyin(SCARG_P32(uap, lsp), &lsp32, sizeof(lsp32))) != 0)
		return error;

	return linux32_select1(l, retval, lsp32.nfds, 
	     NETBSD32PTR64(lsp32.readfds), NETBSD32PTR64(lsp32.writefds),
	     NETBSD32PTR64(lsp32.exceptfds), NETBSD32PTR64(lsp32.timeout));
}

static int
linux32_select1(struct lwp *l, register_t *retval, int nfds,
		fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
		struct timeval *timeout)
{   
	struct timespec ts0, ts1, uts, *ts = NULL;
	struct netbsd32_timeval50 utv32;
	int error;


	/*
	 * Store current time for computation of the amount of
	 * time left.
	 */
	if (timeout) {
		if ((error = copyin(timeout, &utv32, sizeof(utv32))))
			return error;

		uts.tv_sec = utv32.tv_sec;
		uts.tv_nsec = utv32.tv_usec * 1000;

		if (itimespecfix(&uts)) {
			/*
			 * The timeval was invalid.  Convert it to something
			 * valid that will act as it does under Linux.
			 */
			uts.tv_sec += uts.tv_nsec / 1000000000;
			uts.tv_nsec %= 1000000000;
			if (uts.tv_nsec < 0) {
				uts.tv_sec -= 1;
				uts.tv_nsec += 1000000000;
			}
			if (uts.tv_sec < 0)
				timespecclear(&uts);
		}
		nanotime(&ts0);
		ts = &uts;
	} else
		timespecclear(&uts); /* XXX GCC4 */

	error = selcommon(retval, nfds, readfds, writefds, exceptfds, ts, NULL);

	if (error) {
		/*
		 * See fs/select.c in the Linux kernel.  Without this,
		 * Maelstrom doesn't work.
		 */
		if (error == ERESTART)
			error = EINTR;
		return error;
	}

	if (timeout) {
		if (*retval) {
			/*
			 * Compute how much time was left of the timeout,
			 * by subtracting the current time and the time
			 * before we started the call, and subtracting
			 * that result from the user-supplied value.
			 */
			nanotime(&ts1);
			timespecsub(&ts1, &ts0, &ts1);
			timespecsub(&uts, &ts1, &uts);
			if (uts.tv_sec < 0)
				timespecclear(&uts);
		} else {
			timespecclear(&uts);
		}
		
		utv32.tv_sec = uts.tv_sec;
		utv32.tv_usec = uts.tv_nsec / 1000;

		if ((error = copyout(&utv32, timeout, sizeof(utv32))))
			return error;
	}

	return 0;
}

static int
linux32_pipe(struct lwp *l, int *fd, register_t *retval)
{
	/* {
		syscallarg(netbsd32_intp) fd;
	} */
	int error;
	int pfds[2];

	pfds[0] = (int)retval[0];
	pfds[1] = (int)retval[1];

	if ((error = copyout(pfds, fd, 2 * sizeof(*fd))) != 0)
		return error;

	retval[0] = 0;
	retval[1] = 0;

	return 0;
}

int
linux32_sys_pipe(struct lwp *l, const struct linux32_sys_pipe_args *uap,
    register_t *retval)
{
	int error;
	if ((error = pipe1(l, retval, 0)))
		return error;
	return linux32_pipe(l, SCARG_P32(uap, fd), retval);
}

int
linux32_sys_pipe2(struct lwp *l, const struct linux32_sys_pipe2_args *uap,
    register_t *retval)
{
	int flags, error;

	flags = linux_to_bsd_ioflags(SCARG(uap, flags));
	if ((flags & ~(O_CLOEXEC|O_NONBLOCK)) != 0)
		return EINVAL;

	if ((error = pipe1(l, retval, flags)))
		return error;

	return linux32_pipe(l, SCARG_P32(uap, fd), retval);
}

int
linux32_sys_dup3(struct lwp *l, const struct linux32_sys_dup3_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(int) from;
		syscallarg(int) to;
		syscallarg(int) flags;
	} */
	struct linux_sys_dup3_args ua;

	NETBSD32TO64_UAP(from);
	NETBSD32TO64_UAP(to);
	NETBSD32TO64_UAP(flags);

	return linux_sys_dup3(l, &ua, retval);
}


int
linux32_sys_openat(struct lwp *l, const struct linux32_sys_openat_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(const netbsd32_charp) path;
		syscallarg(int) flags;
		syscallarg(int) mode;
	} */
	struct linux_sys_openat_args ua;

	NETBSD32TO64_UAP(fd);
	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(flags);
	NETBSD32TO64_UAP(mode);

	return linux_sys_openat(l, &ua, retval);
}

int
linux32_sys_mknodat(struct lwp *l, const struct linux32_sys_mknodat_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(const netbsd32_charp) path;
		syscallarg(linux_umode_t) mode;
		syscallarg(unsigned) dev;
	} */
	struct linux_sys_mknodat_args ua;

	NETBSD32TO64_UAP(fd);
	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(mode);
	NETBSD32TO64_UAP(dev);

	return linux_sys_mknodat(l, &ua, retval);
}

int
linux32_sys_linkat(struct lwp *l, const struct linux32_sys_linkat_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd1;
		syscallarg(netbsd32_charp) name1;
		syscallarg(int) fd2;
		syscallarg(netbsd32_charp) name2;
		syscallarg(int) flags;
	} */
	int fd1 = SCARG(uap, fd1);
	const char *name1 = SCARG_P32(uap, name1);
	int fd2 = SCARG(uap, fd2);
	const char *name2 = SCARG_P32(uap, name2);
	int follow;

	follow = SCARG(uap, flags) & LINUX_AT_SYMLINK_FOLLOW;

	return do_sys_linkat(l, fd1, name1, fd2, name2, follow, retval);
}

int
linux32_sys_unlink(struct lwp *l, const struct linux32_sys_unlink_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) path;
	} */
	struct linux_sys_unlink_args ua;

	NETBSD32TOP_UAP(path, const char);

	return linux_sys_unlink(l, &ua, retval);
}

int
linux32_sys_unlinkat(struct lwp *l, const struct linux32_sys_unlinkat_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(const netbsd32_charp) path;
		syscallarg(int) flag;
	} */
	struct linux_sys_unlinkat_args ua;

	NETBSD32TO64_UAP(fd);
	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(flag);

	return linux_sys_unlinkat(l, &ua, retval);
}

int
linux32_sys_fchmodat(struct lwp *l, const struct linux32_sys_fchmodat_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(netbsd_charp) path;
		syscallarg(linux_umode_t) mode;
	} */

	return do_sys_chmodat(l, SCARG(uap, fd), SCARG_P32(uap, path),
			      SCARG(uap, mode), AT_SYMLINK_FOLLOW);
}

int
linux32_sys_fchownat(struct lwp *l, const struct linux32_sys_fchownat_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(netbsd_charp) path;
		syscallarg(uid_t) owner;
		syscallarg(gid_t) group;
		syscallarg(int) flag;
	} */
	int flag;

	flag = linux_to_bsd_atflags(SCARG(uap, flag));
	return do_sys_chownat(l, SCARG(uap, fd), SCARG_P32(uap, path),
			      SCARG(uap, owner), SCARG(uap, group), flag);
}

int
linux32_sys_faccessat(struct lwp *l, const struct linux32_sys_faccessat_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(netbsd_charp) path;
		syscallarg(int) amode;
	} */

	return do_sys_accessat(l, SCARG(uap, fd), SCARG_P32(uap, path),
	     SCARG(uap, amode), AT_SYMLINK_FOLLOW);
}

int
linux32_sys_utimensat(struct lwp *l, const struct linux32_sys_utimensat_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(const netbsd32_charp) path;
		syscallarg(const linux32_timespecp_t) times;
		syscallarg(int) flags;
	} */
	int error;
	struct linux32_timespec lts[2];
	struct timespec *tsp = NULL, ts[2];

	if (SCARG_P32(uap, times)) {
		error = copyin(SCARG_P32(uap, times), &lts, sizeof(lts));
		if (error != 0)
			return error;
		linux32_to_native_timespec(&ts[0], &lts[0]);
		linux32_to_native_timespec(&ts[1], &lts[1]);
		tsp = ts;
	}

	return linux_do_sys_utimensat(l, SCARG(uap, fd), SCARG_P32(uap, path),
	    tsp, SCARG(uap, flag), retval);
}

int
linux32_sys_creat(struct lwp *l, const struct linux32_sys_creat_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(int) mode;
	} */
	struct sys_open_args ua;

	NETBSD32TOP_UAP(path, const char);
	SCARG(&ua, flags) = O_CREAT | O_TRUNC | O_WRONLY;
	NETBSD32TO64_UAP(mode);

	return sys_open(l, &ua, retval);
}

int
linux32_sys_mknod(struct lwp *l, const struct linux32_sys_mknod_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(int) mode;
		syscallarg(int) dev;
	} */
	struct linux_sys_mknod_args ua;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(mode);
	NETBSD32TO64_UAP(dev);

	return linux_sys_mknod(l, &ua, retval);
}

int
linux32_sys_break(struct lwp *l, const struct linux32_sys_break_args *uap, register_t *retval)
{
#if 0
	/* {
		syscallarg(const netbsd32_charp) nsize;
	} */
#endif

	return ENOSYS;
}

int
linux32_sys_swapon(struct lwp *l, const struct linux32_sys_swapon_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) name;
	} */
	struct sys_swapctl_args ua;

        SCARG(&ua, cmd) = SWAP_ON;
        SCARG(&ua, arg) = SCARG_P32(uap, name);
        SCARG(&ua, misc) = 0;   /* priority */
        return (sys_swapctl(l, &ua, retval));
}

int
linux32_sys_swapoff(struct lwp *l, const struct linux32_sys_swapoff_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) path;
	} */
	struct sys_swapctl_args ua;

        SCARG(&ua, cmd) = SWAP_OFF;
        SCARG(&ua, arg) = SCARG_P32(uap, path);
        SCARG(&ua, misc) = 0;   /* priority */
        return (sys_swapctl(l, &ua, retval));
}


int
linux32_sys_reboot(struct lwp *l, const struct linux32_sys_reboot_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) magic1;
		syscallarg(int) magic2;
		syscallarg(int) cmd;
		syscallarg(netbsd32_voidp) arg;
	} */
	struct linux_sys_reboot_args ua;

	NETBSD32TO64_UAP(magic1);
	NETBSD32TO64_UAP(magic2);
	NETBSD32TO64_UAP(cmd);
	NETBSD32TOP_UAP(arg, void);
	
	return linux_sys_reboot(l, &ua, retval);
}

int
linux32_sys_setresuid(struct lwp *l, const struct linux32_sys_setresuid_args *uap, register_t *retval)
{
	/* {
		syscallarg(uid_t) ruid;
		syscallarg(uid_t) euid;
		syscallarg(uid_t) suid;
	} */
	struct linux_sys_setresuid_args ua;

	NETBSD32TO64_UAP(ruid);
	NETBSD32TO64_UAP(euid);
	NETBSD32TO64_UAP(suid);

	return linux_sys_setresuid(l, &ua, retval);
}

int
linux32_sys_getresuid(struct lwp *l, const struct linux32_sys_getresuid_args *uap, register_t *retval)
{
	/* {
		syscallarg(linux32_uidp_t) ruid;
		syscallarg(linux32_uidp_t) euid;
		syscallarg(linux32_uidp_t) suid;
	} */
	kauth_cred_t pc = l->l_cred;
	int error;
	uid_t uid;

	uid = kauth_cred_getuid(pc);
	if ((error = copyout(&uid, SCARG_P32(uap, ruid), sizeof(uid_t))) != 0)
		return error;

	uid = kauth_cred_geteuid(pc);
	if ((error = copyout(&uid, SCARG_P32(uap, euid), sizeof(uid_t))) != 0)
		return error;

	uid = kauth_cred_getsvuid(pc);
	return copyout(&uid, SCARG_P32(uap, suid), sizeof(uid_t));
}

int
linux32_sys_setresgid(struct lwp *l, const struct linux32_sys_setresgid_args *uap, register_t *retval)
{
	/* {
		syscallarg(gid_t) rgid;
		syscallarg(gid_t) egid;
		syscallarg(gid_t) sgid;
	} */
	struct linux_sys_setresgid_args ua;

	NETBSD32TO64_UAP(rgid);
	NETBSD32TO64_UAP(egid);
	NETBSD32TO64_UAP(sgid);

	return linux_sys_setresgid(l, &ua, retval);
}

int
linux32_sys_getresgid(struct lwp *l, const struct linux32_sys_getresgid_args *uap, register_t *retval)
{
	/* {
		syscallarg(linux32_gidp_t) rgid;
		syscallarg(linux32_gidp_t) egid;
		syscallarg(linux32_gidp_t) sgid;
	} */
	kauth_cred_t pc = l->l_cred;
	int error;
	gid_t gid;

	gid = kauth_cred_getgid(pc);
	if ((error = copyout(&gid, SCARG_P32(uap, rgid), sizeof(gid_t))) != 0)
		return error;

	gid = kauth_cred_getegid(pc);
	if ((error = copyout(&gid, SCARG_P32(uap, egid), sizeof(gid_t))) != 0)
		return error;

	gid = kauth_cred_getsvgid(pc);
	return copyout(&gid, SCARG_P32(uap, sgid), sizeof(gid_t));
}

int
linux32_sys_nice(struct lwp *l, const struct linux32_sys_nice_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) incr;
	} */
	struct proc *p = l->l_proc;
	struct sys_setpriority_args bsa;
	int error;

	SCARG(&bsa, which) = PRIO_PROCESS;
	SCARG(&bsa, who) = 0;
	SCARG(&bsa, prio) = p->p_nice - NZERO + SCARG(uap, incr);

	error = sys_setpriority(l, &bsa, retval);
	return (error) ? EPERM : 0;
}

int
linux32_sys_alarm(struct lwp *l, const struct linux32_sys_alarm_args *uap, register_t *retval)
{
	/* {
		syscallarg(unsigned int) secs;
	} */
	struct linux_sys_alarm_args ua;

	NETBSD32TO64_UAP(secs);

	return linux_sys_alarm(l, &ua, retval);
}

int
linux32_sys_fdatasync(struct lwp *l, const struct linux32_sys_fdatasync_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
	} */
	struct linux_sys_fdatasync_args ua;

	NETBSD32TO64_UAP(fd);

	return linux_sys_fdatasync(l, &ua, retval);
}

int
linux32_sys_setfsuid(struct lwp *l, const struct linux32_sys_setfsuid_args *uap, register_t *retval)
{
	/* {
		syscallarg(uid_t) uid;
	} */
	struct linux_sys_setfsuid_args ua;

	NETBSD32TO64_UAP(uid);

	return linux_sys_setfsuid(l, &ua, retval);
}

int
linux32_sys_setfsgid(struct lwp *l, const struct linux32_sys_setfsgid_args *uap, register_t *retval)
{
	/* {
		syscallarg(gid_t) gid;
	} */
	struct linux_sys_setfsgid_args ua;

	NETBSD32TO64_UAP(gid);

	return linux_sys_setfsgid(l, &ua, retval);
}

/*
 * pread(2).
 */
int
linux32_sys_pread(struct lwp *l,
    const struct linux32_sys_pread_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(netbsd32_voidp) buf;
		syscallarg(netbsd32_size_t) nbyte;
		syscallarg(netbsd32_off_t) offset;
	} */
	struct sys_pread_args pra;

	SCARG(&pra, fd) = SCARG(uap, fd);
	SCARG(&pra, buf) = SCARG_P32(uap, buf);
	SCARG(&pra, nbyte) = SCARG(uap, nbyte);
	SCARG(&pra, PAD) = 0;
	SCARG(&pra, offset) = SCARG(uap, offset);

	return sys_pread(l, &pra, retval);
}

/*
 * pwrite(2).
 */
int
linux32_sys_pwrite(struct lwp *l,
    const struct linux32_sys_pwrite_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(const netbsd32_voidp) buf;
		syscallarg(netbsd32_size_t) nbyte;
		syscallarg(netbsd32_off_t) offset;
	} */
	struct sys_pwrite_args pra;

	SCARG(&pra, fd) = SCARG(uap, fd);
	SCARG(&pra, buf) = SCARG_P32(uap, buf);
	SCARG(&pra, nbyte) = SCARG(uap, nbyte);
	SCARG(&pra, PAD) = 0;
	SCARG(&pra, offset) = SCARG(uap, offset);

	return sys_pwrite(l, &pra, retval);
}

