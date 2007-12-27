/*	$NetBSD: linux32_unistd.c,v 1.12.2.2 2007/12/27 00:44:18 mjf Exp $ */

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

__KERNEL_RCSID(0, "$NetBSD: linux32_unistd.c,v 1.12.2.2 2007/12/27 00:44:18 mjf Exp $");

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

#include <machine/types.h>

#include <sys/syscallargs.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_conv.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_signal.h>
#include <compat/linux/common/linux_machdep.h>
#include <compat/linux/common/linux_misc.h>
#include <compat/linux/common/linux_oldolduname.h>
#include <compat/linux/linux_syscallargs.h>

#include <compat/linux32/common/linux32_types.h>
#include <compat/linux32/common/linux32_signal.h>
#include <compat/linux32/common/linux32_machdep.h>
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
		syscallcarg(int) fd;
                syscallarg(u_int32_t) ohigh;
                syscallarg(u_int32_t) olow;
		syscallarg(netbsd32_caddr_t) res;
		syscallcarg(int) whence;
	} */
	struct linux_sys_llseek_args ua;

	NETBSD32TO64_UAP(fd);
	NETBSD32TO64_UAP(ohigh);
	NETBSD32TO64_UAP(olow);
	NETBSD32TOP_UAP(res, char);
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
		syscallarg(netbsd32_timevalp_t) timeout;
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
linux32_select1(l, retval, nfds, readfds, writefds, exceptfds, timeout)
        struct lwp *l;
        register_t *retval;
        int nfds;
        fd_set *readfds, *writefds, *exceptfds;
        struct timeval *timeout;
{   
	struct timeval tv0, tv1, utv, *tv = NULL;
	struct netbsd32_timeval utv32;
	int error;

	timerclear(&utv); /* XXX GCC4 */

	/*
	 * Store current time for computation of the amount of
	 * time left.
	 */
	if (timeout) {
		if ((error = copyin(timeout, &utv32, sizeof(utv32))))
			return error;

		netbsd32_to_timeval(&utv32, &utv);

		if (itimerfix(&utv)) {
			/*
			 * The timeval was invalid.  Convert it to something
			 * valid that will act as it does under Linux.
			 */
			utv.tv_sec += utv.tv_usec / 1000000;
			utv.tv_usec %= 1000000;
			if (utv.tv_usec < 0) {
				utv.tv_sec -= 1;
				utv.tv_usec += 1000000;
			}
			if (utv.tv_sec < 0)
				timerclear(&utv);
		}
		microtime(&tv0);
		tv = &utv;
	}

	error = selcommon(l, retval, nfds, 
	    readfds, writefds, exceptfds, tv, NULL);

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
			microtime(&tv1);
			timersub(&tv1, &tv0, &tv1);
			timersub(&utv, &tv1, &utv);
			if (utv.tv_sec < 0)
				timerclear(&utv);
		} else {
			timerclear(&utv);
		}
		
		netbsd32_from_timeval(&utv, &utv32);

		if ((error = copyout(&utv32, timeout, sizeof(utv32))))
			return error;
	}

	return 0;
}

int
linux32_sys_pipe(struct lwp *l, const struct linux32_sys_pipe_args *uap, register_t *retval)
{
	/* {
		syscallarg(netbsd32_intp) fd;
	} */
	int error;
	int pfds[2];

	if ((error = sys_pipe(l, 0, retval)))
		return error;

	pfds[0] = (int)retval[0];
	pfds[1] = (int)retval[1];

	if ((error = copyout(pfds, SCARG_P32(uap, fd), 2 * sizeof (int))) != 0)
		return error;

	retval[0] = 0;
	retval[1] = 0;

	return 0;
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
linux32_sys_chown16(struct lwp *l, const struct linux32_sys_chown16_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(int) uid;
		syscallarg(int) gid;
	} */
        struct sys___posix_chown_args ua;

	NETBSD32TOP_UAP(path, const char);

        if ((linux32_uid_t)SCARG(uap, uid) == (linux32_uid_t)-1)
        	SCARG(&ua, uid) = (uid_t)-1;
	else
        	SCARG(&ua, uid) = SCARG(uap, uid);

        if ((linux32_gid_t)SCARG(uap, gid) == (linux32_gid_t)-1)
        	SCARG(&ua, gid) = (gid_t)-1;
	else
        	SCARG(&ua, gid) = SCARG(uap, gid);
       
        return sys___posix_chown(l, &ua, retval);
}

int
linux32_sys_lchown16(struct lwp *l, const struct linux32_sys_lchown16_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(int) uid;
		syscallarg(int) gid;
	} */
        struct sys___posix_lchown_args ua;

	NETBSD32TOP_UAP(path, const char);

        if ((linux32_uid_t)SCARG(uap, uid) == (linux32_uid_t)-1)
        	SCARG(&ua, uid) = (uid_t)-1;
	else
        	SCARG(&ua, uid) = SCARG(uap, uid);

        if ((linux32_gid_t)SCARG(uap, gid) == (linux32_gid_t)-1)
        	SCARG(&ua, gid) = (gid_t)-1;
	else
        	SCARG(&ua, gid) = SCARG(uap, gid);
       
        return sys___posix_lchown(l, &ua, retval);
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
linux32_sys_rename(struct lwp *l, const struct linux32_sys_rename_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) from;
		syscallarg(const netbsd32_charp) to;
	} */
	struct sys___posix_rename_args ua;

	NETBSD32TOP_UAP(from, const char);
	NETBSD32TOP_UAP(to, const char);

	return sys___posix_rename(l, &ua, retval);
}

int
linux32_sys_getgroups16(struct lwp *l, const struct linux32_sys_getgroups16_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) gidsetsize;
		syscallarg(linux32_gidp_t) gidset;
	} */
	struct linux_sys_getgroups16_args ua;

	NETBSD32TO64_UAP(gidsetsize);
	NETBSD32TOP_UAP(gidset, linux_gid_t);
	
	return linux_sys_getgroups16(l, &ua, retval);
}

int
linux32_sys_setgroups16(struct lwp *l, const struct linux32_sys_setgroups16_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) gidsetsize;
		syscallarg(linux32_gidp_t) gidset;
	} */
	struct linux_sys_setgroups16_args ua;

	NETBSD32TO64_UAP(gidsetsize);
	NETBSD32TOP_UAP(gidset, linux_gid_t);
	
	return linux_sys_setgroups16(l, &ua, retval);
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
linux32_sys_truncate(struct lwp *l, const struct linux32_sys_truncate_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(netbsd32_charp) buf;
		syscallarg(int) count;
	} */
	struct compat_43_sys_truncate_args ua;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(length);

	return compat_43_sys_truncate(l, &ua, retval);
}

int
linux32_sys_fchown16(struct lwp *l, const struct linux32_sys_fchown16_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(int) uid;
		syscallarg(int) gid;
	} */
        struct sys___posix_fchown_args ua;

	SCARG(&ua, fd) = SCARG(uap, fd);

        if ((linux32_uid_t)SCARG(uap, uid) == (linux32_uid_t)-1)
        	SCARG(&ua, uid) = (uid_t)-1;
	else
        	SCARG(&ua, uid) = SCARG(uap, uid);

        if ((linux32_gid_t)SCARG(uap, gid) == (linux32_gid_t)-1)
        	SCARG(&ua, gid) = (gid_t)-1;
	else
        	SCARG(&ua, gid) = SCARG(uap, gid);
       
        return sys___posix_fchown(l, &ua, retval);
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

	SCARG(&ua, ruid) = (SCARG(uap, ruid) == -1) ? -1 : SCARG(uap, ruid);
	SCARG(&ua, euid) = (SCARG(uap, euid) == -1) ? -1 : SCARG(uap, euid);
	SCARG(&ua, suid) = (SCARG(uap, suid) == -1) ? -1 : SCARG(uap, suid);

	return linux_sys_setresuid(l, &ua, retval);
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

	SCARG(&ua, rgid) = (SCARG(uap, rgid) == -1) ? -1 : SCARG(uap, rgid);
	SCARG(&ua, egid) = (SCARG(uap, egid) == -1) ? -1 : SCARG(uap, egid);
	SCARG(&ua, sgid) = (SCARG(uap, sgid) == -1) ? -1 : SCARG(uap, sgid);

	return linux_sys_setresgid(l, &ua, retval);
}

int
linux32_sys_nice(struct lwp *l, const struct linux32_sys_nice_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) incr;
	} */
	struct netbsd32_setpriority_args bsa;

	SCARG(&bsa, which) = PRIO_PROCESS;
	SCARG(&bsa, who) = 0;
	SCARG(&bsa, prio) = SCARG(uap, incr);

	return netbsd32_setpriority(l, &bsa, retval);
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

int
linux32_sys_setreuid16(struct lwp *l, const struct linux32_sys_setreuid16_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) ruid;
		syscallarg(int) euid;
	} */
	struct sys_setreuid_args bsa;

	if ((linux32_uid_t)SCARG(uap, ruid) == (linux32_uid_t)-1)
		SCARG(&bsa, ruid) = (uid_t)-1;
	else
		SCARG(&bsa, ruid) = SCARG(uap, ruid);
	if ((linux32_uid_t)SCARG(uap, euid) == (linux32_uid_t)-1)
		SCARG(&bsa, euid) = (uid_t)-1;
	else
		SCARG(&bsa, euid) = SCARG(uap, euid);

	return sys_setreuid(l, &bsa, retval);
}

int
linux32_sys_setregid16(struct lwp *l, const struct linux32_sys_setregid16_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) rgid;
		syscallarg(int) egid;
	} */
	struct sys_setregid_args bsa;

	if ((linux32_gid_t)SCARG(uap, rgid) == (linux32_gid_t)-1)
		SCARG(&bsa, rgid) = (gid_t)-1;
	else
		SCARG(&bsa, rgid) = SCARG(uap, rgid);
	if ((linux32_gid_t)SCARG(uap, egid) == (linux32_gid_t)-1)
		SCARG(&bsa, egid) = (gid_t)-1;
	else
		SCARG(&bsa, egid) = SCARG(uap, egid);

	return sys_setregid(l, &bsa, retval);
}

int
linux32_sys_setresuid16(struct lwp *l, const struct linux32_sys_setresuid16_args *uap, register_t *retval)
{
	/* {
		syscallarg(uid_t) ruid;
		syscallarg(uid_t) euid;
		syscallarg(uid_t) suid;
	} */
	struct linux32_sys_setresuid_args lsa;

	if ((linux32_uid_t)SCARG(uap, ruid) == (linux32_uid_t)-1)
		SCARG(&lsa, ruid) = (uid_t)-1;
	else
		SCARG(&lsa, ruid) = SCARG(uap, ruid);
	if ((linux32_uid_t)SCARG(uap, euid) == (linux32_uid_t)-1)
		SCARG(&lsa, euid) = (uid_t)-1;
	else
		SCARG(&lsa, euid) = SCARG(uap, euid);
	if ((linux32_uid_t)SCARG(uap, suid) == (linux32_uid_t)-1)
		SCARG(&lsa, suid) = (uid_t)-1;
	else
		SCARG(&lsa, suid) = SCARG(uap, euid);

	return linux32_sys_setresuid(l, &lsa, retval);
}

int
linux32_sys_setresgid16(struct lwp *l, const struct linux32_sys_setresgid16_args *uap, register_t *retval)
{
	/* {
		syscallarg(gid_t) rgid;
		syscallarg(gid_t) egid;
		syscallarg(gid_t) sgid;
	} */
	struct linux32_sys_setresgid_args lsa;

	if ((linux32_gid_t)SCARG(uap, rgid) == (linux32_gid_t)-1)
		SCARG(&lsa, rgid) = (gid_t)-1;
	else
		SCARG(&lsa, rgid) = SCARG(uap, rgid);
	if ((linux32_gid_t)SCARG(uap, egid) == (linux32_gid_t)-1)
		SCARG(&lsa, egid) = (gid_t)-1;
	else
		SCARG(&lsa, egid) = SCARG(uap, egid);
	if ((linux32_gid_t)SCARG(uap, sgid) == (linux32_gid_t)-1)
		SCARG(&lsa, sgid) = (gid_t)-1;
	else
		SCARG(&lsa, sgid) = SCARG(uap, sgid);

	return linux32_sys_setresgid(l, &lsa, retval);
}
