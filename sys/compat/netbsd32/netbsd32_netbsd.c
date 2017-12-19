/*	$NetBSD: netbsd32_netbsd.c,v 1.209 2017/12/19 08:48:19 kamil Exp $	*/

/*
 * Copyright (c) 1998, 2001, 2008 Matthew R. Green
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: netbsd32_netbsd.c,v 1.209 2017/12/19 08:48:19 kamil Exp $");

#if defined(_KERNEL_OPT)
#include "opt_ddb.h"
#include "opt_ntp.h"
#include "opt_ktrace.h"
#include "opt_compat_netbsd.h"
#include "opt_compat_43.h"
#include "opt_sysv.h"
#include "opt_syscall_debug.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
//#define msg __msg /* Don't ask me! */
#include <sys/mount.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/socketvar.h>
#include <sys/mbuf.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/swap.h>
#include <sys/time.h>
#include <sys/signalvar.h>
#include <sys/ptrace.h>
#include <sys/ktrace.h>
#include <sys/trace.h>
#include <sys/resourcevar.h>
#include <sys/pool.h>
#include <sys/vnode.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/namei.h>
#include <sys/dirent.h>
#include <sys/quotactl.h>
#include <sys/kauth.h>
#include <sys/vfs_syscalls.h>

#include <uvm/uvm_extern.h>
#include <uvm/uvm_swap.h>

#include <sys/syscallargs.h>
#include <sys/proc.h>
#include <sys/acct.h>
#include <sys/exec.h>

#include <net/if.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_exec.h>
#include <compat/netbsd32/netbsd32_syscall.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>
#include <compat/netbsd32/netbsd32_conv.h>

#include <compat/sys/mman.h>

#if defined(DDB)
#include <ddb/ddbvar.h>
#endif

extern struct sysent netbsd32_sysent[];
#ifdef SYSCALL_DEBUG
extern const char * const netbsd32_syscallnames[];
#endif
#ifdef __HAVE_SYSCALL_INTERN
void netbsd32_syscall_intern(struct proc *);
#else
void syscall(void);
#endif

#define LIMITCHECK(a, b) ((a) != RLIM_INFINITY && (a) > (b))

#ifdef COMPAT_16
extern char netbsd32_sigcode[], netbsd32_esigcode[];
struct uvm_object *emul_netbsd32_object;
#endif

extern struct sysctlnode netbsd32_sysctl_root;

#ifdef MODULAR
#include <compat/netbsd32/netbsd32_syscalls_autoload.c>
#endif

struct emul emul_netbsd32 = {
	.e_name =		"netbsd32",
	.e_path =		"/emul/netbsd32",
#ifndef __HAVE_MINIMAL_EMUL
	.e_flags =		0,
	.e_errno =		NULL,
	.e_nosys =		NETBSD32_SYS_netbsd32_syscall,
	.e_nsysent =		NETBSD32_SYS_NSYSENT,
#endif
	.e_sysent =		netbsd32_sysent,
#ifdef SYSCALL_DEBUG
	.e_syscallnames =	netbsd32_syscallnames,
#else
	.e_syscallnames =	NULL,
#endif
#ifdef MODULAR
	.e_sc_autoload =	netbsd32_syscalls_autoload,
#endif
	.e_sendsig =		netbsd32_sendsig,
	.e_trapsignal =		trapsignal,
	.e_tracesig =		NULL,
#ifdef COMPAT_16
	.e_sigcode =		netbsd32_sigcode,
	.e_esigcode =		netbsd32_esigcode,
	.e_sigobject =		&emul_netbsd32_object,
#else
	.e_sigcode =		NULL,
	.e_esigcode =		NULL,
	.e_sigobject =		NULL,
#endif
	.e_setregs =		netbsd32_setregs,
	.e_proc_exec =		NULL,
	.e_proc_fork =		NULL,
	.e_proc_exit =		NULL,
	.e_lwp_fork =		NULL,
	.e_lwp_exit =		NULL,
#ifdef __HAVE_SYSCALL_INTERN
	.e_syscall_intern =	netbsd32_syscall_intern,
#else
	.e_syscall =		syscall,
#endif
	.e_sysctlovly =		&netbsd32_sysctl_root,
	.e_fault =		NULL,
	.e_vm_default_addr =	netbsd32_vm_default_addr,
	.e_usertrap =		NULL,
	.e_ucsize =		sizeof(ucontext32_t),
	.e_startlwp =		startlwp32,
#ifdef notyet
	.e_ktrpsig =		netbsd32_ktrpsig,
#else
	.e_ktrpsig =		NULL,
#endif
};

/*
 * below are all the standard NetBSD system calls, in the 32bit
 * environment, with the necessary conversions to 64bit before
 * calling the real syscall.  anything that needs special
 * attention is handled elsewhere.
 */

int
netbsd32_exit(struct lwp *l, const struct netbsd32_exit_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) rval;
	} */
	struct sys_exit_args ua;

	NETBSD32TO64_UAP(rval);
	return sys_exit(l, &ua, retval);
}

int
netbsd32_read(struct lwp *l, const struct netbsd32_read_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(netbsd32_voidp) buf;
		syscallarg(netbsd32_size_t) nbyte;
	} */
	struct sys_read_args ua;

	NETBSD32TO64_UAP(fd);
	NETBSD32TOP_UAP(buf, void *);
	NETBSD32TOX_UAP(nbyte, size_t);
	return sys_read(l, &ua, retval);
}

int
netbsd32_write(struct lwp *l, const struct netbsd32_write_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(const netbsd32_voidp) buf;
		syscallarg(netbsd32_size_t) nbyte;
	} */
	struct sys_write_args ua;

	NETBSD32TO64_UAP(fd);
	NETBSD32TOP_UAP(buf, void *);
	NETBSD32TOX_UAP(nbyte, size_t);
	return sys_write(l, &ua, retval);
}

int
netbsd32_close(struct lwp *l, const struct netbsd32_close_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
	} */
	struct sys_close_args ua;

	NETBSD32TO64_UAP(fd);
	return sys_close(l, &ua, retval);
}

int
netbsd32_open(struct lwp *l, const struct netbsd32_open_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(int) flags;
		syscallarg(mode_t) mode;
	} */
	struct sys_open_args ua;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(flags);
	NETBSD32TO64_UAP(mode);

	return sys_open(l, &ua, retval);
}

int
netbsd32_link(struct lwp *l, const struct netbsd32_link_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(const netbsd32_charp) link;
	} */
	struct sys_link_args ua;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TOP_UAP(link, const char);
	return (sys_link(l, &ua, retval));
}

int
netbsd32_unlink(struct lwp *l, const struct netbsd32_unlink_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) path;
	} */
	struct sys_unlink_args ua;

	NETBSD32TOP_UAP(path, const char);

	return (sys_unlink(l, &ua, retval));
}

int
netbsd32_chdir(struct lwp *l, const struct netbsd32_chdir_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) path;
	} */
	struct sys_chdir_args ua;

	NETBSD32TOP_UAP(path, const char);

	return (sys_chdir(l, &ua, retval));
}

int
netbsd32_fchdir(struct lwp *l, const struct netbsd32_fchdir_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
	} */
	struct sys_fchdir_args ua;

	NETBSD32TO64_UAP(fd);

	return (sys_fchdir(l, &ua, retval));
}

int
netbsd32___mknod50(struct lwp *l, const struct netbsd32___mknod50_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(mode_t) mode;
		syscallarg(netbsd32_dev_t) dev;
	} */

	return do_sys_mknod(l, SCARG_P32(uap, path), SCARG(uap, mode),
	    SCARG(uap, dev), retval, UIO_USERSPACE);
}

int
netbsd32_chmod(struct lwp *l, const struct netbsd32_chmod_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(mode_t) mode;
	} */
	struct sys_chmod_args ua;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(mode);

	return (sys_chmod(l, &ua, retval));
}

int
netbsd32_chown(struct lwp *l, const struct netbsd32_chown_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(uid_t) uid;
		syscallarg(gid_t) gid;
	} */
	struct sys_chown_args ua;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(uid);
	NETBSD32TO64_UAP(gid);

	return (sys_chown(l, &ua, retval));
}

int
netbsd32_break(struct lwp *l, const struct netbsd32_break_args *uap, register_t *retval)
{
	/* {
		syscallarg(netbsd32_charp) nsize;
	} */
	struct sys_obreak_args ua;

	NETBSD32TOP_UAP(nsize, char);
	return (sys_obreak(l, &ua, retval));
}

int
netbsd32_mount(struct lwp *l, const struct netbsd32_mount_args *uap, register_t *retval)
{
#ifdef COMPAT_40
	/* {
		syscallarg(const netbsd32_charp) type;
		syscallarg(const netbsd32_charp) path;
		syscallarg(int) flags;
		syscallarg(netbsd32_voidp) data;
	} */
	struct compat_40_sys_mount_args ua;

	NETBSD32TOP_UAP(type, const char);
	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(flags);
	NETBSD32TOP_UAP(data, void);
	return (compat_40_sys_mount(l, &ua, retval));
#else
	return ENOSYS;
#endif
}

int
netbsd32_unmount(struct lwp *l, const struct netbsd32_unmount_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(int) flags;
	} */
	struct sys_unmount_args ua;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(flags);
	return (sys_unmount(l, &ua, retval));
}

int
netbsd32_setuid(struct lwp *l, const struct netbsd32_setuid_args *uap, register_t *retval)
{
	/* {
		syscallarg(uid_t) uid;
	} */
	struct sys_setuid_args ua;

	NETBSD32TO64_UAP(uid);
	return (sys_setuid(l, &ua, retval));
}

int
netbsd32_accept(struct lwp *l, const struct netbsd32_accept_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) s;
		syscallarg(netbsd32_sockaddrp_t) name;
		syscallarg(netbsd32_intp) anamelen;
	} */
	struct sys_accept_args ua;

	NETBSD32TO64_UAP(s);
	NETBSD32TOP_UAP(name, struct sockaddr);
	NETBSD32TOP_UAP(anamelen, socklen_t);
	return (sys_accept(l, &ua, retval));
}

int
netbsd32_getpeername(struct lwp *l, const struct netbsd32_getpeername_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fdes;
		syscallarg(netbsd32_sockaddrp_t) asa;
		syscallarg(netbsd32_intp) alen;
	} */
	struct sys_getpeername_args ua;

	NETBSD32TO64_UAP(fdes);
	NETBSD32TOP_UAP(asa, struct sockaddr);
	NETBSD32TOP_UAP(alen, socklen_t);
/* NB: do the protocol specific sockaddrs need to be converted? */
	return (sys_getpeername(l, &ua, retval));
}

int
netbsd32_getsockname(struct lwp *l, const struct netbsd32_getsockname_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fdes;
		syscallarg(netbsd32_sockaddrp_t) asa;
		syscallarg(netbsd32_intp) alen;
	} */
	struct sys_getsockname_args ua;

	NETBSD32TO64_UAP(fdes);
	NETBSD32TOP_UAP(asa, struct sockaddr);
	NETBSD32TOP_UAP(alen, socklen_t);
	return (sys_getsockname(l, &ua, retval));
}

int
netbsd32_access(struct lwp *l, const struct netbsd32_access_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(int) flags;
	} */
	struct sys_access_args ua;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(flags);

	return sys_access(l, &ua, retval);
}

int
netbsd32_chflags(struct lwp *l, const struct netbsd32_chflags_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(netbsd32_u_long) flags;
	} */
	struct sys_chflags_args ua;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(flags);

	return (sys_chflags(l, &ua, retval));
}

int
netbsd32_fchflags(struct lwp *l, const struct netbsd32_fchflags_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(netbsd32_u_long) flags;
	} */
	struct sys_fchflags_args ua;

	NETBSD32TO64_UAP(fd);
	NETBSD32TO64_UAP(flags);

	return (sys_fchflags(l, &ua, retval));
}

int
netbsd32_lchflags(struct lwp *l, const struct netbsd32_lchflags_args *uap, register_t *retval)
{
	/* {
		syscallarg(const char *) path;
		syscallarg(netbsd32_u_long) flags;
	} */
	struct sys_lchflags_args ua;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(flags);

	return (sys_lchflags(l, &ua, retval));
}

int
netbsd32_kill(struct lwp *l, const struct netbsd32_kill_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) pid;
		syscallarg(int) signum;
	} */
	struct sys_kill_args ua;

	NETBSD32TO64_UAP(pid);
	NETBSD32TO64_UAP(signum);

	return (sys_kill(l, &ua, retval));
}

int
netbsd32_dup(struct lwp *l, const struct netbsd32_dup_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
	} */
	struct sys_dup_args ua;

	NETBSD32TO64_UAP(fd);

	return (sys_dup(l, &ua, retval));
}

int
netbsd32_profil(struct lwp *l, const struct netbsd32_profil_args *uap, register_t *retval)
{
	/* {
		syscallarg(netbsd32_voidp) samples;
		syscallarg(netbsd32_size_t) size;
		syscallarg(netbsd32_u_long) offset;
		syscallarg(u_int) scale;
	} */
	struct sys_profil_args ua;

	NETBSD32TOP_UAP(samples, void *);
	NETBSD32TOX_UAP(size, size_t);
	NETBSD32TOX_UAP(offset, u_long);
	NETBSD32TO64_UAP(scale);
	return (sys_profil(l, &ua, retval));
}

int
netbsd32_ktrace(struct lwp *l, const struct netbsd32_ktrace_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) fname;
		syscallarg(int) ops;
		syscallarg(int) facs;
		syscallarg(int) pid;
	} */
	struct sys_ktrace_args ua;

	NETBSD32TOP_UAP(fname, const char);
	NETBSD32TO64_UAP(ops);
	NETBSD32TO64_UAP(facs);
	NETBSD32TO64_UAP(pid);
	return (sys_ktrace(l, &ua, retval));
}

int
netbsd32_utrace(struct lwp *l, const struct netbsd32_utrace_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) label;
		syscallarg(netbsd32_voidp) addr;
		syscallarg(netbsd32_size_t) len;
	} */
	struct sys_utrace_args ua;

	NETBSD32TOP_UAP(label, const char);
	NETBSD32TOP_UAP(addr, void);
	NETBSD32TO64_UAP(len);
	return (sys_utrace(l, &ua, retval));
}

int
netbsd32___getlogin(struct lwp *l, const struct netbsd32___getlogin_args *uap, register_t *retval)
{
	/* {
		syscallarg(netbsd32_charp) namebuf;
		syscallarg(u_int) namelen;
	} */
	struct sys___getlogin_args ua;

	NETBSD32TOP_UAP(namebuf, char);
	NETBSD32TO64_UAP(namelen);
	return (sys___getlogin(l, &ua, retval));
}

int
netbsd32_setlogin(struct lwp *l, const struct netbsd32_setlogin_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) namebuf;
	} */
	struct sys___setlogin_args ua;

	NETBSD32TOP_UAP(namebuf, char);
	return (sys___setlogin(l, &ua, retval));
}

int
netbsd32_acct(struct lwp *l, const struct netbsd32_acct_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) path;
	} */
	struct sys_acct_args ua;

	NETBSD32TOP_UAP(path, const char);
	return (sys_acct(l, &ua, retval));
}

int
netbsd32_revoke(struct lwp *l, const struct netbsd32_revoke_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) path;
	} */
	struct sys_revoke_args ua;

	NETBSD32TOP_UAP(path, const char);

	return (sys_revoke(l, &ua, retval));
}

int
netbsd32_symlink(struct lwp *l, const struct netbsd32_symlink_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(const netbsd32_charp) link;
	} */
	struct sys_symlink_args ua;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TOP_UAP(link, const char);

	return (sys_symlink(l, &ua, retval));
}

int
netbsd32_readlink(struct lwp *l, const struct netbsd32_readlink_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(netbsd32_charp) buf;
		syscallarg(netbsd32_size_t) count;
	} */
	struct sys_readlink_args ua;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TOP_UAP(buf, char);
	NETBSD32TOX_UAP(count, size_t);

	return (sys_readlink(l, &ua, retval));
}

int
netbsd32_umask(struct lwp *l, const struct netbsd32_umask_args *uap, register_t *retval)
{
	/* {
		syscallarg(mode_t) newmask;
	} */
	struct sys_umask_args ua;

	NETBSD32TO64_UAP(newmask);
	return (sys_umask(l, &ua, retval));
}

int
netbsd32_chroot(struct lwp *l, const struct netbsd32_chroot_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) path;
	} */
	struct sys_chroot_args ua;

	NETBSD32TOP_UAP(path, const char);
	return (sys_chroot(l, &ua, retval));
}

int
netbsd32_sbrk(struct lwp *l, const struct netbsd32_sbrk_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) incr;
	} */
	struct sys_sbrk_args ua;

	NETBSD32TO64_UAP(incr);
	return (sys_sbrk(l, &ua, retval));
}

int
netbsd32_munmap(struct lwp *l, const struct netbsd32_munmap_args *uap, register_t *retval)
{
	/* {
		syscallarg(netbsd32_voidp) addr;
		syscallarg(netbsd32_size_t) len;
	} */
	struct sys_munmap_args ua;

	NETBSD32TOP_UAP(addr, void);
	NETBSD32TOX_UAP(len, size_t);
	return (sys_munmap(l, &ua, retval));
}

int
netbsd32_mprotect(struct lwp *l, const struct netbsd32_mprotect_args *uap, register_t *retval)
{
	/* {
		syscallarg(netbsd32_voidp) addr;
		syscallarg(netbsd32_size_t) len;
		syscallarg(int) prot;
	} */
	struct sys_mprotect_args ua;

	NETBSD32TOP_UAP(addr, void);
	NETBSD32TOX_UAP(len, size_t);
	NETBSD32TO64_UAP(prot);
	return (sys_mprotect(l, &ua, retval));
}

int
netbsd32_madvise(struct lwp *l, const struct netbsd32_madvise_args *uap, register_t *retval)
{
	/* {
		syscallarg(netbsd32_voidp) addr;
		syscallarg(netbsd32_size_t) len;
		syscallarg(int) behav;
	} */
	struct sys_madvise_args ua;

	NETBSD32TOP_UAP(addr, void);
	NETBSD32TOX_UAP(len, size_t);
	NETBSD32TO64_UAP(behav);
	return (sys_madvise(l, &ua, retval));
}

int
netbsd32_mincore(struct lwp *l, const struct netbsd32_mincore_args *uap, register_t *retval)
{
	/* {
		syscallarg(netbsd32_voidp) addr;
		syscallarg(netbsd32_size_t) len;
		syscallarg(netbsd32_charp) vec;
	} */
	struct sys_mincore_args ua;

	NETBSD32TOP_UAP(addr, void *);
	NETBSD32TOX_UAP(len, size_t);
	NETBSD32TOP_UAP(vec, char);
	return (sys_mincore(l, &ua, retval));
}

/* XXX MOVE ME XXX ? */
int
netbsd32_getgroups(struct lwp *l, const struct netbsd32_getgroups_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) gidsetsize;
		syscallarg(netbsd32_gid_tp) gidset;
	} */
	struct sys_getgroups_args ua;

	/* Since sizeof (gid_t) == sizeof (netbsd32_gid_t) ... */

	NETBSD32TO64_UAP(gidsetsize);
	NETBSD32TOP_UAP(gidset, gid_t);
	return (sys_getgroups(l, &ua, retval));
}

int
netbsd32_setgroups(struct lwp *l, const struct netbsd32_setgroups_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) gidsetsize;
		syscallarg(const netbsd32_gid_tp) gidset;
	} */
	struct sys_setgroups_args ua;

	NETBSD32TO64_UAP(gidsetsize);
	NETBSD32TOP_UAP(gidset, gid_t);
	return (sys_setgroups(l, &ua, retval));
}

int
netbsd32_setpgid(struct lwp *l, const struct netbsd32_setpgid_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) pid;
		syscallarg(int) pgid;
	} */
	struct sys_setpgid_args ua;

	NETBSD32TO64_UAP(pid);
	NETBSD32TO64_UAP(pgid);
	return (sys_setpgid(l, &ua, retval));
}

int
netbsd32_fcntl(struct lwp *l, const struct netbsd32_fcntl_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(int) cmd;
		syscallarg(netbsd32_voidp) arg;
	} */
	struct sys_fcntl_args ua;

	NETBSD32TO64_UAP(fd);
	NETBSD32TO64_UAP(cmd);
	NETBSD32TOP_UAP(arg, void);
	/* we can do this because `struct flock' doesn't change */
	return (sys_fcntl(l, &ua, retval));
}

int
netbsd32_dup2(struct lwp *l, const struct netbsd32_dup2_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) from;
		syscallarg(int) to;
	} */
	struct sys_dup2_args ua;

	NETBSD32TO64_UAP(from);
	NETBSD32TO64_UAP(to);
	return (sys_dup2(l, &ua, retval));
}

int
netbsd32_fsync(struct lwp *l, const struct netbsd32_fsync_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
	} */
	struct sys_fsync_args ua;

	NETBSD32TO64_UAP(fd);
	return (sys_fsync(l, &ua, retval));
}

int
netbsd32_setpriority(struct lwp *l, const struct netbsd32_setpriority_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) which;
		syscallarg(int) who;
		syscallarg(int) prio;
	} */
	struct sys_setpriority_args ua;

	NETBSD32TO64_UAP(which);
	NETBSD32TO64_UAP(who);
	NETBSD32TO64_UAP(prio);
	return (sys_setpriority(l, &ua, retval));
}

int
netbsd32___socket30(struct lwp *l, const struct netbsd32___socket30_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) domain;
		syscallarg(int) type;
		syscallarg(int) protocol;
	} */
	struct sys___socket30_args ua;

	NETBSD32TO64_UAP(domain);
	NETBSD32TO64_UAP(type);
	NETBSD32TO64_UAP(protocol);
	return (sys___socket30(l, &ua, retval));
}

int
netbsd32_connect(struct lwp *l, const struct netbsd32_connect_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) s;
		syscallarg(const netbsd32_sockaddrp_t) name;
		syscallarg(int) namelen;
	} */
	struct sys_connect_args ua;

	NETBSD32TO64_UAP(s);
	NETBSD32TOP_UAP(name, struct sockaddr);
	NETBSD32TO64_UAP(namelen);
	return (sys_connect(l, &ua, retval));
}

int
netbsd32_getpriority(struct lwp *l, const struct netbsd32_getpriority_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) which;
		syscallarg(int) who;
	} */
	struct sys_getpriority_args ua;

	NETBSD32TO64_UAP(which);
	NETBSD32TO64_UAP(who);
	return (sys_getpriority(l, &ua, retval));
}

int
netbsd32_bind(struct lwp *l, const struct netbsd32_bind_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) s;
		syscallarg(const netbsd32_sockaddrp_t) name;
		syscallarg(int) namelen;
	} */
	struct sys_bind_args ua;

	NETBSD32TO64_UAP(s);
	NETBSD32TOP_UAP(name, struct sockaddr);
	NETBSD32TO64_UAP(namelen);
	return (sys_bind(l, &ua, retval));
}

int
netbsd32_setsockopt(struct lwp *l, const struct netbsd32_setsockopt_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) s;
		syscallarg(int) level;
		syscallarg(int) name;
		syscallarg(const netbsd32_voidp) val;
		syscallarg(int) valsize;
	} */
	struct sys_setsockopt_args ua;

	NETBSD32TO64_UAP(s);
	NETBSD32TO64_UAP(level);
	NETBSD32TO64_UAP(name);
	NETBSD32TOP_UAP(val, void);
	NETBSD32TO64_UAP(valsize);
	/* may be more efficient to do this inline. */
	return (sys_setsockopt(l, &ua, retval));
}

int
netbsd32_listen(struct lwp *l, const struct netbsd32_listen_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) s;
		syscallarg(int) backlog;
	} */
	struct sys_listen_args ua;

	NETBSD32TO64_UAP(s);
	NETBSD32TO64_UAP(backlog);
	return (sys_listen(l, &ua, retval));
}

int
netbsd32_fchown(struct lwp *l, const struct netbsd32_fchown_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(uid_t) uid;
		syscallarg(gid_t) gid;
	} */
	struct sys_fchown_args ua;

	NETBSD32TO64_UAP(fd);
	NETBSD32TO64_UAP(uid);
	NETBSD32TO64_UAP(gid);
	return (sys_fchown(l, &ua, retval));
}

int
netbsd32_fchmod(struct lwp *l, const struct netbsd32_fchmod_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(mode_t) mode;
	} */
	struct sys_fchmod_args ua;

	NETBSD32TO64_UAP(fd);
	NETBSD32TO64_UAP(mode);
	return (sys_fchmod(l, &ua, retval));
}

int
netbsd32_setreuid(struct lwp *l, const struct netbsd32_setreuid_args *uap, register_t *retval)
{
	/* {
		syscallarg(uid_t) ruid;
		syscallarg(uid_t) euid;
	} */
	struct sys_setreuid_args ua;

	NETBSD32TO64_UAP(ruid);
	NETBSD32TO64_UAP(euid);
	return (sys_setreuid(l, &ua, retval));
}

int
netbsd32_setregid(struct lwp *l, const struct netbsd32_setregid_args *uap, register_t *retval)
{
	/* {
		syscallarg(gid_t) rgid;
		syscallarg(gid_t) egid;
	} */
	struct sys_setregid_args ua;

	NETBSD32TO64_UAP(rgid);
	NETBSD32TO64_UAP(egid);
	return (sys_setregid(l, &ua, retval));
}

int
netbsd32_getsockopt(struct lwp *l, const struct netbsd32_getsockopt_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) s;
		syscallarg(int) level;
		syscallarg(int) name;
		syscallarg(netbsd32_voidp) val;
		syscallarg(netbsd32_intp) avalsize;
	} */
	struct sys_getsockopt_args ua;

	NETBSD32TO64_UAP(s);
	NETBSD32TO64_UAP(level);
	NETBSD32TO64_UAP(name);
	NETBSD32TOP_UAP(val, void);
	NETBSD32TOP_UAP(avalsize, socklen_t);
	return (sys_getsockopt(l, &ua, retval));
}

int
netbsd32_rename(struct lwp *l, const struct netbsd32_rename_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) from;
		syscallarg(const netbsd32_charp) to;
	} */
	struct sys_rename_args ua;

	NETBSD32TOP_UAP(from, const char);
	NETBSD32TOP_UAP(to, const char);

	return (sys_rename(l, &ua, retval));
}

int
netbsd32_flock(struct lwp *l, const struct netbsd32_flock_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(int) how;
	} */
	struct sys_flock_args ua;

	NETBSD32TO64_UAP(fd);
	NETBSD32TO64_UAP(how);

	return (sys_flock(l, &ua, retval));
}

int
netbsd32_mkfifo(struct lwp *l, const struct netbsd32_mkfifo_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(mode_t) mode;
	} */
	struct sys_mkfifo_args ua;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(mode);
	return (sys_mkfifo(l, &ua, retval));
}

int
netbsd32_shutdown(struct lwp *l, const struct netbsd32_shutdown_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) s;
		syscallarg(int) how;
	} */
	struct sys_shutdown_args ua;

	NETBSD32TO64_UAP(s);
	NETBSD32TO64_UAP(how);
	return (sys_shutdown(l, &ua, retval));
}

int
netbsd32_socketpair(struct lwp *l, const struct netbsd32_socketpair_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) domain;
		syscallarg(int) type;
		syscallarg(int) protocol;
		syscallarg(netbsd32_intp) rsv;
	} */
	struct sys_socketpair_args ua;

	NETBSD32TO64_UAP(domain);
	NETBSD32TO64_UAP(type);
	NETBSD32TO64_UAP(protocol);
	NETBSD32TOP_UAP(rsv, int);
	/* Since we're just copying out two `int's we can do this */
	return (sys_socketpair(l, &ua, retval));
}

int
netbsd32_mkdir(struct lwp *l, const struct netbsd32_mkdir_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(mode_t) mode;
	} */
	struct sys_mkdir_args ua;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(mode);
	return (sys_mkdir(l, &ua, retval));
}

int
netbsd32_rmdir(struct lwp *l, const struct netbsd32_rmdir_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) path;
	} */
	struct sys_rmdir_args ua;

	NETBSD32TOP_UAP(path, const char);
	return (sys_rmdir(l, &ua, retval));
}

int
netbsd32___quotactl(struct lwp *l, const struct netbsd32___quotactl_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(netbsd32_voidp) args;
	} */
	struct netbsd32_quotactlargs args32;
	struct quotactl_args args;
	int error;

	error = copyin(SCARG_P32(uap, args), &args32, sizeof(args32));
	if (error) {
		return error;
	}

	args.qc_op = args32.qc_op;
	switch (args.qc_op) {
	    case QUOTACTL_STAT:
		args.u.stat.qc_info = NETBSD32PTR64(args32.u.stat.qc_info);
		break;
	    case QUOTACTL_IDTYPESTAT:
		args.u.idtypestat.qc_idtype = args32.u.idtypestat.qc_idtype;
		args.u.idtypestat.qc_info =
			NETBSD32PTR64(args32.u.idtypestat.qc_info);
		break;
	    case QUOTACTL_OBJTYPESTAT:
		args.u.objtypestat.qc_objtype =
			args32.u.objtypestat.qc_objtype;
		args.u.objtypestat.qc_info =
			NETBSD32PTR64(args32.u.objtypestat.qc_info);
		break;
	    case QUOTACTL_GET:
		args.u.get.qc_key = NETBSD32PTR64(args32.u.get.qc_key);
		args.u.get.qc_val = NETBSD32PTR64(args32.u.get.qc_val);
		break;
	    case QUOTACTL_PUT:
		args.u.put.qc_key = NETBSD32PTR64(args32.u.put.qc_key);
		args.u.put.qc_val = NETBSD32PTR64(args32.u.put.qc_val);
		break;
	    case QUOTACTL_DEL:
		args.u.del.qc_key = NETBSD32PTR64(args32.u.del.qc_key);
		break;
	    case QUOTACTL_CURSOROPEN:
		args.u.cursoropen.qc_cursor =
			NETBSD32PTR64(args32.u.cursoropen.qc_cursor);
		break;
	    case QUOTACTL_CURSORCLOSE:
		args.u.cursorclose.qc_cursor =
			NETBSD32PTR64(args32.u.cursorclose.qc_cursor);
		break;
	    case QUOTACTL_CURSORSKIPIDTYPE:
		args.u.cursorskipidtype.qc_cursor =
			NETBSD32PTR64(args32.u.cursorskipidtype.qc_cursor);
		args.u.cursorskipidtype.qc_idtype =
			args32.u.cursorskipidtype.qc_idtype;
		break;
	    case QUOTACTL_CURSORGET:
		args.u.cursorget.qc_cursor =
			NETBSD32PTR64(args32.u.cursorget.qc_cursor);
		args.u.cursorget.qc_keys =
			NETBSD32PTR64(args32.u.cursorget.qc_keys);
		args.u.cursorget.qc_vals =
			NETBSD32PTR64(args32.u.cursorget.qc_vals);
		args.u.cursorget.qc_maxnum =
			args32.u.cursorget.qc_maxnum;
		args.u.cursorget.qc_ret =
			NETBSD32PTR64(args32.u.cursorget.qc_ret);
		break;
	    case QUOTACTL_CURSORATEND:
		args.u.cursoratend.qc_cursor =
			NETBSD32PTR64(args32.u.cursoratend.qc_cursor);
		args.u.cursoratend.qc_ret =
			NETBSD32PTR64(args32.u.cursoratend.qc_ret);
		break;
	    case QUOTACTL_CURSORREWIND:
		args.u.cursorrewind.qc_cursor =
			NETBSD32PTR64(args32.u.cursorrewind.qc_cursor);
		break;
	    case QUOTACTL_QUOTAON:
		args.u.quotaon.qc_idtype = args32.u.quotaon.qc_idtype;
		args.u.quotaon.qc_quotafile =
			NETBSD32PTR64(args32.u.quotaon.qc_quotafile);
		break;
	    case QUOTACTL_QUOTAOFF:
		args.u.quotaoff.qc_idtype = args32.u.quotaoff.qc_idtype;
		break;
	    default:
		return EINVAL;
	}

	return do_sys_quotactl(SCARG_P32(uap, path), &args);
}

int
netbsd32___getfh30(struct lwp *l, const struct netbsd32___getfh30_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) fname;
		syscallarg(netbsd32_fhandlep_t) fhp;
		syscallarg(netbsd32_size_tp) fh_size;
	} */
	struct vnode *vp;
	fhandle_t *fh;
	int error;
	struct pathbuf *pb;
	struct nameidata nd;
	netbsd32_size_t usz32, sz32;
	size_t sz;

	/*
	 * Must be super user
	 */
	error = kauth_authorize_system(l->l_cred, KAUTH_SYSTEM_FILEHANDLE,
	    0, NULL, NULL, NULL);
	if (error)
		return (error);

	error = pathbuf_copyin(SCARG_P32(uap, fname), &pb);
	if (error) {
		return error;
	}

	NDINIT(&nd, LOOKUP, FOLLOW | LOCKLEAF | TRYEMULROOT, pb);
	error = namei(&nd);
	if (error) {
		pathbuf_destroy(pb);
		return error;
	}
	vp = nd.ni_vp;
	pathbuf_destroy(pb);

	error = vfs_composefh_alloc(vp, &fh);
	vput(vp);
	if (error != 0) {
		return error;
	}
	error = copyin(SCARG_P32(uap, fh_size), &usz32, sizeof(usz32));
	if (error != 0) {
		goto out;
	}
	sz = FHANDLE_SIZE(fh);
	sz32 = sz;

	error = copyout(&sz32, SCARG_P32(uap, fh_size), sizeof(sz32));
	if (error != 0) {
		goto out;
	}
	if (usz32 >= sz32) {
		error = copyout(fh, SCARG_P32(uap, fhp), sz);
	} else {
		error = E2BIG;
	}
out:
	vfs_composefh_free(fh);
	return (error);
}

int
netbsd32_pread(struct lwp *l, const struct netbsd32_pread_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(netbsd32_voidp) buf;
		syscallarg(netbsd32_size_t) nbyte;
		syscallarg(int) PAD;
		syscallarg(netbsd32_off_t) offset;
	} */
	struct sys_pread_args ua;

	NETBSD32TO64_UAP(fd);
	NETBSD32TOP_UAP(buf, void);
	NETBSD32TOX_UAP(nbyte, size_t);
	NETBSD32TO64_UAP(PAD);
	NETBSD32TO64_UAP(offset);
	return sys_pread(l, &ua, retval);
}

int
netbsd32_pwrite(struct lwp *l, const struct netbsd32_pwrite_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(const netbsd32_voidp) buf;
		syscallarg(netbsd32_size_t) nbyte;
		syscallarg(int) PAD;
		syscallarg(netbsd32_off_t) offset;
	} */
	struct sys_pwrite_args ua;

	NETBSD32TO64_UAP(fd);
	NETBSD32TOP_UAP(buf, void);
	NETBSD32TOX_UAP(nbyte, size_t);
	NETBSD32TO64_UAP(PAD);
	NETBSD32TO64_UAP(offset);
	return sys_pwrite(l, &ua, retval);
}

int
netbsd32_setgid(struct lwp *l, const struct netbsd32_setgid_args *uap, register_t *retval)
{
	/* {
		syscallarg(gid_t) gid;
	} */
	struct sys_setgid_args ua;

	NETBSD32TO64_UAP(gid);
	return (sys_setgid(l, &ua, retval));
}

int
netbsd32_setegid(struct lwp *l, const struct netbsd32_setegid_args *uap, register_t *retval)
{
	/* {
		syscallarg(gid_t) egid;
	} */
	struct sys_setegid_args ua;

	NETBSD32TO64_UAP(egid);
	return (sys_setegid(l, &ua, retval));
}

int
netbsd32_seteuid(struct lwp *l, const struct netbsd32_seteuid_args *uap, register_t *retval)
{
	/* {
		syscallarg(gid_t) euid;
	} */
	struct sys_seteuid_args ua;

	NETBSD32TO64_UAP(euid);
	return (sys_seteuid(l, &ua, retval));
}

int
netbsd32_pathconf(struct lwp *l, const struct netbsd32_pathconf_args *uap, register_t *retval)
{
	/* {
		syscallarg(netbsd32_charp) path;
		syscallarg(int) name;
	} */
	struct sys_pathconf_args ua;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(name);
	return sys_pathconf(l, &ua, retval);
}

int
netbsd32_fpathconf(struct lwp *l, const struct netbsd32_fpathconf_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(int) name;
	} */
	struct sys_fpathconf_args ua;

	NETBSD32TO64_UAP(fd);
	NETBSD32TO64_UAP(name);
	return sys_fpathconf(l, &ua, retval);
}

static void
fixlimit(int which, struct rlimit *alim)
{
	switch (which) {
	case RLIMIT_DATA:
		if (LIMITCHECK(alim->rlim_cur, MAXDSIZ32))
			alim->rlim_cur = MAXDSIZ32;
		if (LIMITCHECK(alim->rlim_max, MAXDSIZ32))
			alim->rlim_max = MAXDSIZ32;
		return;
	case RLIMIT_STACK:
		if (LIMITCHECK(alim->rlim_cur, MAXSSIZ32))
			alim->rlim_cur = MAXSSIZ32;
		if (LIMITCHECK(alim->rlim_max, MAXSSIZ32))
			alim->rlim_max = MAXSSIZ32;
		return;
	default:
		return;
	}
}

int
netbsd32_getrlimit(struct lwp *l, const struct netbsd32_getrlimit_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(int) which;
		syscallarg(netbsd32_rlimitp_t) rlp;
	} */
	int which = SCARG(uap, which);
	struct rlimit alim;

	if ((u_int)which >= RLIM_NLIMITS)
		return EINVAL;

	alim = l->l_proc->p_rlimit[which];

	fixlimit(which, &alim);

	return copyout(&alim, SCARG_P32(uap, rlp), sizeof(alim));
}

int
netbsd32_setrlimit(struct lwp *l, const struct netbsd32_setrlimit_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(int) which;
		syscallarg(const netbsd32_rlimitp_t) rlp;
	} */
		int which = SCARG(uap, which);
	struct rlimit alim;
	int error;

	if ((u_int)which >= RLIM_NLIMITS)
		return EINVAL;

	error = copyin(SCARG_P32(uap, rlp), &alim, sizeof(struct rlimit));
	if (error)
		return (error);

	fixlimit(which, &alim);

	return dosetrlimit(l, l->l_proc, which, &alim);
}

int
netbsd32_mmap(struct lwp *l, const struct netbsd32_mmap_args *uap, register_t *retval)
{
	/* {
		syscallarg(netbsd32_voidp) addr;
		syscallarg(netbsd32_size_t) len;
		syscallarg(int) prot;
		syscallarg(int) flags;
		syscallarg(int) fd;
		syscallarg(netbsd32_long) PAD;
		syscallarg(netbsd32_off_t) pos;
	} */
	struct sys_mmap_args ua;
	int error;

	NETBSD32TOP_UAP(addr, void);
	NETBSD32TOX_UAP(len, size_t);
	NETBSD32TO64_UAP(prot);
	NETBSD32TO64_UAP(flags);
#ifdef __x86_64__
	/*
	 * Ancient kernel on x86 did not obey PROT_EXEC on i386 at least
	 * and ld.so did not turn it on!
	 */
	if (SCARG(&ua, flags) & COMPAT_MAP_COPY) {
		SCARG(&ua, flags) = MAP_PRIVATE
		    | (SCARG(&ua, flags) & ~COMPAT_MAP_COPY);
		SCARG(&ua, prot) |= PROT_EXEC;
	}
#endif
	NETBSD32TO64_UAP(fd);
	NETBSD32TOX_UAP(PAD, long);
	NETBSD32TOX_UAP(pos, off_t);
#ifdef DEBUG_MMAP
	printf("mmap(addr=0x%lx, len=0x%lx, prot=0x%lx, flags=0x%lx, "
	    "fd=%ld, pos=0x%lx);\n",
	    (long)SCARG(&ua, addr), (long)SCARG(&ua, len),
	    (long)SCARG(&ua, prot), (long)SCARG(&ua, flags),
	    (long)SCARG(&ua, fd), (long)SCARG(&ua, pos));
#endif
	error = sys_mmap(l, &ua, retval);
	if ((u_long)*retval > (u_long)UINT_MAX) {
		printf("netbsd32_mmap: retval out of range: 0x%lx\n",
		    (u_long)*retval);
		/* Should try to recover and return an error here. */
	}
	return (error);
}

int
netbsd32_mremap(struct lwp *l, const struct netbsd32_mremap_args *uap, register_t *retval)
{
	/* {
		syscallarg(void *) old_address;
		syscallarg(size_t) old_size;
		syscallarg(void *) new_address;
		syscallarg(size_t) new_size;
		syscallarg(int) flags;
	} */
	struct sys_mremap_args ua;

	NETBSD32TOP_UAP(old_address, void);
	NETBSD32TOX_UAP(old_size, size_t);
	NETBSD32TOP_UAP(new_address, void);
	NETBSD32TOX_UAP(new_size, size_t);
	NETBSD32TO64_UAP(flags);

	return sys_mremap(l, &ua, retval);
}

int
netbsd32_lseek(struct lwp *l, const struct netbsd32_lseek_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(int) PAD;
		syscallarg(netbsd32_off_t) offset;
		syscallarg(int) whence;
	} */
	struct sys_lseek_args ua;
	union {
	    register_t retval64[2];
	    register32_t retval32[4];
	} newpos;
	int rv;

	NETBSD32TO64_UAP(fd);
	NETBSD32TO64_UAP(PAD);
	NETBSD32TO64_UAP(offset);
	NETBSD32TO64_UAP(whence);
	rv = sys_lseek(l, &ua, newpos.retval64);

	/*
	 * We have to split the 64 bit value into 2 halves which will
	 * end up in separate 32 bit registers.
	 * This should DTRT on big and little-endian systems provided that
	 * gcc's 'strict aliasing' tests don't decide that the retval32[]
	 * entries can't have been assigned to, so need not be read!
	 */
	retval[0] = newpos.retval32[0];
	retval[1] = newpos.retval32[1];

	return rv;
}

int
netbsd32_truncate(struct lwp *l, const struct netbsd32_truncate_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(int) PAD;
		syscallarg(netbsd32_off_t) length;
	} */
	struct sys_truncate_args ua;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(PAD);
	NETBSD32TO64_UAP(length);
	return (sys_truncate(l, &ua, retval));
}

int
netbsd32_ftruncate(struct lwp *l, const struct netbsd32_ftruncate_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(int) PAD;
		syscallarg(netbsd32_off_t) length;
	} */
	struct sys_ftruncate_args ua;

	NETBSD32TO64_UAP(fd);
	NETBSD32TO64_UAP(PAD);
	NETBSD32TO64_UAP(length);
	return (sys_ftruncate(l, &ua, retval));
}

int
netbsd32_mlock(struct lwp *l, const struct netbsd32_mlock_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_voidp) addr;
		syscallarg(netbsd32_size_t) len;
	} */
	struct sys_mlock_args ua;

	NETBSD32TOP_UAP(addr, const void);
	NETBSD32TO64_UAP(len);
	return (sys_mlock(l, &ua, retval));
}

int
netbsd32_munlock(struct lwp *l, const struct netbsd32_munlock_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_voidp) addr;
		syscallarg(netbsd32_size_t) len;
	} */
	struct sys_munlock_args ua;

	NETBSD32TOP_UAP(addr, const void);
	NETBSD32TO64_UAP(len);
	return (sys_munlock(l, &ua, retval));
}

int
netbsd32_undelete(struct lwp *l, const struct netbsd32_undelete_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) path;
	} */
	struct sys_undelete_args ua;

	NETBSD32TOP_UAP(path, const char);
	return (sys_undelete(l, &ua, retval));
}

int
netbsd32_getpgid(struct lwp *l, const struct netbsd32_getpgid_args *uap, register_t *retval)
{
	/* {
		syscallarg(pid_t) pid;
	} */
	struct sys_getpgid_args ua;

	NETBSD32TO64_UAP(pid);
	return (sys_getpgid(l, &ua, retval));
}

int
netbsd32_reboot(struct lwp *l, const struct netbsd32_reboot_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) opt;
		syscallarg(netbsd32_charp) bootstr;
	} */
	struct sys_reboot_args ua;

	NETBSD32TO64_UAP(opt);
	NETBSD32TOP_UAP(bootstr, char);
	return (sys_reboot(l, &ua, retval));
}

#include <sys/poll.h>
int
netbsd32_poll(struct lwp *l, const struct netbsd32_poll_args *uap, register_t *retval)
{
	/* {
		syscallarg(netbsd32_pollfdp_t) fds;
		syscallarg(u_int) nfds;
		syscallarg(int) timeout;
	} */
	struct sys_poll_args ua;

	NETBSD32TOP_UAP(fds, struct pollfd);
	NETBSD32TO64_UAP(nfds);
	NETBSD32TO64_UAP(timeout);

	return (sys_poll(l, &ua, retval));
}

int
netbsd32_fdatasync(struct lwp *l, const struct netbsd32_fdatasync_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
	} */
	struct sys_fdatasync_args ua;

	NETBSD32TO64_UAP(fd);
	return (sys_fdatasync(l, &ua, retval));
}

int
netbsd32___posix_rename(struct lwp *l, const struct netbsd32___posix_rename_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) from;
		syscallarg(const netbsd32_charp) to;
	} */
	struct sys___posix_rename_args ua;

	NETBSD32TOP_UAP(from, const char);
	NETBSD32TOP_UAP(to, const char);
	return (sys___posix_rename(l, &ua, retval));
}

static int
netbsd32_swapctl_stats(struct lwp *l, struct sys_swapctl_args *uap, register_t *retval)
{
	struct swapent *ksep;
	struct netbsd32_swapent *usep32;
	struct netbsd32_swapent se32;
	int count = SCARG(uap, misc);
	int i, error = 0;
	size_t ksep_len;

	if (count < 0)
		return EINVAL;

	swapsys_lock(RW_WRITER);

	if ((size_t)count > (size_t)uvmexp.nswapdev)
		count = uvmexp.nswapdev;
	if (count == 0) {
		/* No swap device */
		swapsys_unlock();
		return 0;
	}

	ksep_len = sizeof(*ksep) * count;
	ksep = kmem_alloc(ksep_len, KM_SLEEP);
	usep32 = (struct netbsd32_swapent *)SCARG(uap, arg);

	uvm_swap_stats(SWAP_STATS, ksep, count, retval);
	count = *retval;

	swapsys_unlock();

	for (i = 0; i < count; i++) {
		se32.se_dev = ksep[i].se_dev;
		se32.se_flags = ksep[i].se_flags;
		se32.se_nblks = ksep[i].se_nblks;
		se32.se_inuse = ksep[i].se_inuse;
		se32.se_priority = ksep[i].se_priority;
		memcpy(se32.se_path, ksep[i].se_path,
			sizeof(se32.se_path));

		error = copyout(&se32, usep32 + i, sizeof(se32));
		if (error)
			break;
	}

	kmem_free(ksep, ksep_len);

	return error;
}

int
netbsd32_swapctl(struct lwp *l, const struct netbsd32_swapctl_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) cmd;
		syscallarg(const netbsd32_voidp) arg;
		syscallarg(int) misc;
	} */
	struct sys_swapctl_args ua;

	NETBSD32TO64_UAP(cmd);
	NETBSD32TOP_UAP(arg, void);
	NETBSD32TO64_UAP(misc);

	/* SWAP_STATS50 and SWAP_STATS13 structures need no translation */
	if (SCARG(&ua, cmd) == SWAP_STATS)
		return netbsd32_swapctl_stats(l, &ua, retval);

	return (sys_swapctl(l, &ua, retval));
}

int
netbsd32_minherit(struct lwp *l, const struct netbsd32_minherit_args *uap, register_t *retval)
{
	/* {
		syscallarg(netbsd32_voidp) addr;
		syscallarg(netbsd32_size_t) len;
		syscallarg(int) inherit;
	} */
	struct sys_minherit_args ua;

	NETBSD32TOP_UAP(addr, void);
	NETBSD32TOX_UAP(len, size_t);
	NETBSD32TO64_UAP(inherit);
	return (sys_minherit(l, &ua, retval));
}

int
netbsd32_lchmod(struct lwp *l, const struct netbsd32_lchmod_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(mode_t) mode;
	} */
	struct sys_lchmod_args ua;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(mode);
	return (sys_lchmod(l, &ua, retval));
}

int
netbsd32_lchown(struct lwp *l, const struct netbsd32_lchown_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(uid_t) uid;
		syscallarg(gid_t) gid;
	} */
	struct sys_lchown_args ua;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(uid);
	NETBSD32TO64_UAP(gid);
	return (sys_lchown(l, &ua, retval));
}

int
netbsd32___msync13(struct lwp *l, const struct netbsd32___msync13_args *uap, register_t *retval)
{
	/* {
		syscallarg(netbsd32_voidp) addr;
		syscallarg(netbsd32_size_t) len;
		syscallarg(int) flags;
	} */
	struct sys___msync13_args ua;

	NETBSD32TOP_UAP(addr, void);
	NETBSD32TOX_UAP(len, size_t);
	NETBSD32TO64_UAP(flags);
	return (sys___msync13(l, &ua, retval));
}

int
netbsd32___posix_chown(struct lwp *l, const struct netbsd32___posix_chown_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(uid_t) uid;
		syscallarg(gid_t) gid;
	} */
	struct sys___posix_chown_args ua;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(uid);
	NETBSD32TO64_UAP(gid);
	return (sys___posix_chown(l, &ua, retval));
}

int
netbsd32___posix_fchown(struct lwp *l, const struct netbsd32___posix_fchown_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(uid_t) uid;
		syscallarg(gid_t) gid;
	} */
	struct sys___posix_fchown_args ua;

	NETBSD32TO64_UAP(fd);
	NETBSD32TO64_UAP(uid);
	NETBSD32TO64_UAP(gid);
	return (sys___posix_fchown(l, &ua, retval));
}

int
netbsd32___posix_lchown(struct lwp *l, const struct netbsd32___posix_lchown_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(uid_t) uid;
		syscallarg(gid_t) gid;
	} */
	struct sys___posix_lchown_args ua;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(uid);
	NETBSD32TO64_UAP(gid);
	return (sys___posix_lchown(l, &ua, retval));
}

int
netbsd32_getsid(struct lwp *l, const struct netbsd32_getsid_args *uap, register_t *retval)
{
	/* {
		syscallarg(pid_t) pid;
	} */
	struct sys_getsid_args ua;

	NETBSD32TO64_UAP(pid);
	return (sys_getsid(l, &ua, retval));
}

int
netbsd32_fktrace(struct lwp *l, const struct netbsd32_fktrace_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(int) ops;
		syscallarg(int) facs;
		syscallarg(int) pid;
	} */
	struct sys_fktrace_args ua;

	NETBSD32TOX_UAP(fd, int);
	NETBSD32TO64_UAP(ops);
	NETBSD32TO64_UAP(facs);
	NETBSD32TO64_UAP(pid);
	return (sys_fktrace(l, &ua, retval));
}

int
netbsd32___sigpending14(struct lwp *l, const struct netbsd32___sigpending14_args *uap, register_t *retval)
{
	/* {
		syscallarg(sigset_t *) set;
	} */
	struct sys___sigpending14_args ua;

	NETBSD32TOP_UAP(set, sigset_t);
	return (sys___sigpending14(l, &ua, retval));
}

int
netbsd32___sigprocmask14(struct lwp *l, const struct netbsd32___sigprocmask14_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) how;
		syscallarg(const sigset_t *) set;
		syscallarg(sigset_t *) oset;
	} */
	struct sys___sigprocmask14_args ua;

	NETBSD32TO64_UAP(how);
	NETBSD32TOP_UAP(set, sigset_t);
	NETBSD32TOP_UAP(oset, sigset_t);
	return (sys___sigprocmask14(l, &ua, retval));
}

int
netbsd32___sigsuspend14(struct lwp *l, const struct netbsd32___sigsuspend14_args *uap, register_t *retval)
{
	/* {
		syscallarg(const sigset_t *) set;
	} */
	struct sys___sigsuspend14_args ua;

	NETBSD32TOP_UAP(set, sigset_t);
	return (sys___sigsuspend14(l, &ua, retval));
}

int
netbsd32_fchroot(struct lwp *l, const struct netbsd32_fchroot_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
	} */
	struct sys_fchroot_args ua;

	NETBSD32TO64_UAP(fd);
	return (sys_fchroot(l, &ua, retval));
}

/*
 * Open a file given a file handle.
 *
 * Check permissions, allocate an open file structure,
 * and call the device open routine if any.
 */
int
netbsd32___fhopen40(struct lwp *l, const struct netbsd32___fhopen40_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_pointer_t *) fhp;
		syscallarg(netbsd32_size_t) fh_size;
		syscallarg(int) flags;
	} */
	struct sys___fhopen40_args ua;

	NETBSD32TOP_UAP(fhp, fhandle_t);
	NETBSD32TO64_UAP(fh_size);
	NETBSD32TO64_UAP(flags);
	return (sys___fhopen40(l, &ua, retval));
}

/* virtual memory syscalls */
int
netbsd32_ovadvise(struct lwp *l, const struct netbsd32_ovadvise_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) anom;
	} */
	struct sys_ovadvise_args ua;

	NETBSD32TO64_UAP(anom);
	return (sys_ovadvise(l, &ua, retval));
}

void
netbsd32_adjust_limits(struct proc *p)
{
	static const struct {
		int id;
		rlim_t lim;
	} lm[] = {
		{ RLIMIT_DATA,	MAXDSIZ32 },
		{ RLIMIT_STACK, MAXSSIZ32 },
	};
	size_t i;
	struct plimit *lim;
	struct rlimit *rlim;

	/*
	 * We can only reduce the current limits, we cannot stop external
	 * processes from changing them (eg via sysctl) later on.
	 * So there is no point trying to lock out such changes here.
	 *
	 * If we assume that rlim_cur/max are accessed using atomic
	 * operations, we don't need to lock against any other updates
	 * that might happen if the plimit structure is shared writable
	 * between multiple processes.
	 */

	/* Scan to determine is any limits are out of range */
	lim = p->p_limit;
	for (i = 0; ; i++) {
		if (i >= __arraycount(lm))
			/* All in range */
			return;
		rlim = lim->pl_rlimit + lm[i].id;
		if (LIMITCHECK(rlim->rlim_cur, lm[i].lim))
			break;
		if (LIMITCHECK(rlim->rlim_max, lm[i].lim))
			break;
	}

	lim_privatise(p);

	lim = p->p_limit;
	for (i = 0; i < __arraycount(lm); i++) {
		rlim = lim->pl_rlimit + lm[i].id;
		if (LIMITCHECK(rlim->rlim_cur, lm[i].lim))
			rlim->rlim_cur = lm[i].lim;
		if (LIMITCHECK(rlim->rlim_max, lm[i].lim))
			rlim->rlim_max = lm[i].lim;
	}
}

int
netbsd32_uuidgen(struct lwp *l, const struct netbsd32_uuidgen_args *uap, register_t *retval)
{
	/* {
		syscallarg(netbsd32_uuidp_t) store;
		syscallarg(int) count;
	} */
	struct sys_uuidgen_args ua;

	NETBSD32TOP_UAP(store, struct uuid);
	NETBSD32TO64_UAP(count);
	return (sys_uuidgen(l, &ua, retval));
}

int
netbsd32_extattrctl(struct lwp *l, const struct netbsd32_extattrctl_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(int) cmd;
		syscallarg(const netbsd32_charp) filename;
		syscallarg(int) attrnamespace;
		syscallarg(const netbsd32_charp) attrname;
	} */
	struct sys_extattrctl_args ua;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(cmd);
	NETBSD32TOP_UAP(filename, const char);
	NETBSD32TO64_UAP(attrnamespace);
	NETBSD32TOP_UAP(attrname, const char);
	return sys_extattrctl(l, &ua, retval);
}

int
netbsd32_extattr_set_fd(struct lwp *l, const struct netbsd32_extattr_set_fd_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(int) attrnamespace;
		syscallarg(const netbsd32_charp) attrname;
		syscallarg(const netbsd32_voidp) data;
		syscallarg(netbsd32_size_t) nbytes;
	} */
	struct sys_extattr_set_fd_args ua;

	NETBSD32TO64_UAP(fd);
	NETBSD32TO64_UAP(attrnamespace);
	NETBSD32TOP_UAP(attrname, const char);
	NETBSD32TOP_UAP(data, const void);
	NETBSD32TOX_UAP(nbytes, size_t);
	return sys_extattr_set_fd(l, &ua, retval);
}

int
netbsd32_extattr_set_file(struct lwp *l, const struct netbsd32_extattr_set_file_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(int) attrnamespace;
		syscallarg(const netbsd32_charp) attrname;
		syscallarg(const netbsd32_voidp) data;
		syscallarg(netbsd32_size_t) nbytes;
	} */
	struct sys_extattr_set_file_args ua;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(attrnamespace);
	NETBSD32TOP_UAP(attrname, const char);
	NETBSD32TOP_UAP(data, const void);
	NETBSD32TOX_UAP(nbytes, size_t);
	return sys_extattr_set_file(l, &ua, retval);
}

int
netbsd32_extattr_set_link(struct lwp *l, const struct netbsd32_extattr_set_link_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(int) attrnamespace;
		syscallarg(const netbsd32_charp) attrname;
		syscallarg(const netbsd32_voidp) data;
		syscallarg(netbsd32_size_t) nbytes;
	} */
	struct sys_extattr_set_link_args ua;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(attrnamespace);
	NETBSD32TOP_UAP(attrname, const char);
	NETBSD32TOP_UAP(data, const void);
	NETBSD32TOX_UAP(nbytes, size_t);
	return sys_extattr_set_link(l, &ua, retval);
}

int
netbsd32_extattr_get_fd(struct lwp *l, const struct netbsd32_extattr_get_fd_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(int) attrnamespace;
		syscallarg(const netbsd32_charp) attrname;
		syscallarg(netbsd32_voidp) data;
		syscallarg(netbsd32_size_t) nbytes;
	} */
	struct sys_extattr_get_fd_args ua;

	NETBSD32TO64_UAP(fd);
	NETBSD32TO64_UAP(attrnamespace);
	NETBSD32TOP_UAP(attrname, const char);
	NETBSD32TOP_UAP(data, void);
	NETBSD32TOX_UAP(nbytes, size_t);
	return sys_extattr_get_fd(l, &ua, retval);
}

int
netbsd32_extattr_get_file(struct lwp *l, const struct netbsd32_extattr_get_file_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(int) attrnamespace;
		syscallarg(const netbsd32_charp) attrname;
		syscallarg(netbsd32_voidp) data;
		syscallarg(netbsd32_size_t) nbytes;
	} */
	struct sys_extattr_get_file_args ua;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(attrnamespace);
	NETBSD32TOP_UAP(attrname, const char);
	NETBSD32TOP_UAP(data, void);
	NETBSD32TOX_UAP(nbytes, size_t);
	return sys_extattr_get_file(l, &ua, retval);
}

int
netbsd32_extattr_get_link(struct lwp *l, const struct netbsd32_extattr_get_link_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(int) attrnamespace;
		syscallarg(const netbsd32_charp) attrname;
		syscallarg(netbsd32_voidp) data;
		syscallarg(netbsd32_size_t) nbytes;
	} */
	struct sys_extattr_get_link_args ua;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(attrnamespace);
	NETBSD32TOP_UAP(attrname, const char);
	NETBSD32TOP_UAP(data, void);
	NETBSD32TOX_UAP(nbytes, size_t);
	return sys_extattr_get_link(l, &ua, retval);
}

int
netbsd32_extattr_delete_fd(struct lwp *l, const struct netbsd32_extattr_delete_fd_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(int) attrnamespace;
		syscallarg(const netbsd32_charp) attrname;
	} */
	struct sys_extattr_delete_fd_args ua;

	NETBSD32TO64_UAP(fd);
	NETBSD32TO64_UAP(attrnamespace);
	NETBSD32TOP_UAP(attrname, const char);
	return sys_extattr_delete_fd(l, &ua, retval);
}

int
netbsd32_extattr_delete_file(struct lwp *l, const struct netbsd32_extattr_delete_file_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(int) attrnamespace;
		syscallarg(const netbsd32_charp) attrname;
	} */
	struct sys_extattr_delete_file_args ua;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(attrnamespace);
	NETBSD32TOP_UAP(attrname, const char);
	return sys_extattr_delete_file(l, &ua, retval);
}

int
netbsd32_extattr_delete_link(struct lwp *l, const struct netbsd32_extattr_delete_link_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(int) attrnamespace;
		syscallarg(const netbsd32_charp) attrname;
	} */
	struct sys_extattr_delete_link_args ua;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(attrnamespace);
	NETBSD32TOP_UAP(attrname, const char);
	return sys_extattr_delete_link(l, &ua, retval);
}

int
netbsd32_extattr_list_fd(struct lwp *l, const struct netbsd32_extattr_list_fd_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(int) attrnamespace;
		syscallarg(netbsd32_voidp) data;
		syscallarg(netbsd32_size_t) nbytes;
	} */
	struct sys_extattr_list_fd_args ua;

	NETBSD32TO64_UAP(fd);
	NETBSD32TO64_UAP(attrnamespace);
	NETBSD32TOP_UAP(data, void);
	NETBSD32TOX_UAP(nbytes, size_t);
	return sys_extattr_list_fd(l, &ua, retval);
}

int
netbsd32_extattr_list_file(struct lwp *l, const struct netbsd32_extattr_list_file_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(int) attrnamespace;
		syscallarg(netbsd32_voidp) data;
		syscallarg(netbsd32_size_t) nbytes;
	} */
	struct sys_extattr_list_file_args ua;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(attrnamespace);
	NETBSD32TOP_UAP(data, void);
	NETBSD32TOX_UAP(nbytes, size_t);
	return sys_extattr_list_file(l, &ua, retval);
}

int
netbsd32_extattr_list_link(struct lwp *l, const struct netbsd32_extattr_list_link_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(int) attrnamespace;
		syscallarg(netbsd32_voidp) data;
		syscallarg(netbsd32_size_t) nbytes;
	} */
	struct sys_extattr_list_link_args ua;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(attrnamespace);
	NETBSD32TOP_UAP(data, void);
	NETBSD32TOX_UAP(nbytes, size_t);
	return sys_extattr_list_link(l, &ua, retval);
}

int
netbsd32_mlockall(struct lwp *l, const struct netbsd32_mlockall_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) flags;
	} */
	struct sys_mlockall_args ua;

	NETBSD32TO64_UAP(flags);
	return (sys_mlockall(l, &ua, retval));
}

int
netbsd32___clone(struct lwp *l, const struct netbsd32___clone_args *uap, register_t *retval)
{
	/*  {
		syscallarg(int) flags;
		syscallarg(netbsd32_voidp) stack;
	} */
	struct sys___clone_args ua;

	NETBSD32TO64_UAP(flags);
	NETBSD32TOP_UAP(stack, void);
	return sys___clone(l, &ua, retval);
}

int
netbsd32_fsync_range(struct lwp *l, const struct netbsd32_fsync_range_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(int) flags;
		syscallarg(off_t) start;
		syscallarg(off_t) length;
	} */
	struct sys_fsync_range_args ua;

	NETBSD32TO64_UAP(fd);
	NETBSD32TO64_UAP(flags);
	NETBSD32TO64_UAP(start);
	NETBSD32TO64_UAP(length);
	return (sys_fsync_range(l, &ua, retval));
}

int
netbsd32_rasctl(struct lwp *l, const struct netbsd32_rasctl_args *uap, register_t *retval)
{
	/* {
		syscallarg(netbsd32_voidp) addr;
		syscallarg(netbsd32_size_t) len;
		syscallarg(int) op;
	} */
	struct sys_rasctl_args ua;

	NETBSD32TOP_UAP(addr, void *);
	NETBSD32TOX_UAP(len, size_t);
	NETBSD32TO64_UAP(op);
	return sys_rasctl(l, &ua, retval);
}

int
netbsd32_setxattr(struct lwp *l, const struct netbsd32_setxattr_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(const netbsd32_charp) name;
		syscallarg(netbsd32_voidp) value;
		syscallarg(netbsd32_size_t) size;
		syscallarg(int) flags;
	} */
	struct sys_setxattr_args ua;
	NETBSD32TOP_UAP(path, const char);
	NETBSD32TOP_UAP(name, const char);
	NETBSD32TOP_UAP(value, void);
	NETBSD32TOX_UAP(size, size_t);
	NETBSD32TO64_UAP(flags);
	return sys_setxattr(l, &ua, retval);
}

int
netbsd32_lsetxattr(struct lwp *l, const struct netbsd32_lsetxattr_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(const netbsd32_charp) name;
		syscallarg(netbsd32_voidp) value;
		syscallarg(netbsd32_size_t) size;
		syscallarg(int) flags;
	} */
	struct sys_lsetxattr_args ua;
	NETBSD32TOP_UAP(path, const char);
	NETBSD32TOP_UAP(name, const char);
	NETBSD32TOP_UAP(value, void);
	NETBSD32TOX_UAP(size, size_t);
	NETBSD32TO64_UAP(flags);
	return sys_lsetxattr(l, &ua, retval);
}

int
netbsd32_fsetxattr(struct lwp *l, const struct netbsd32_fsetxattr_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(const netbsd32_charp) name;
		syscallarg(netbsd32_voidp) value;
		syscallarg(netbsd32_size_t) size;
		syscallarg(int) flags;
	} */
	struct sys_fsetxattr_args ua;
	NETBSD32TO64_UAP(fd);
	NETBSD32TOP_UAP(name, const char);
	NETBSD32TOP_UAP(value, void);
	NETBSD32TOX_UAP(size, size_t);
	NETBSD32TO64_UAP(flags);
	return sys_fsetxattr(l, &ua, retval);
}

int
netbsd32_getxattr(struct lwp *l, const struct netbsd32_getxattr_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(const netbsd32_charp) name;
		syscallarg(netbsd32_voidp) value;
		syscallarg(netbsd32_size_t) size;
	} */
	struct sys_getxattr_args ua;
	NETBSD32TOP_UAP(path, const char);
	NETBSD32TOP_UAP(name, const char);
	NETBSD32TOP_UAP(value, void);
	NETBSD32TOX_UAP(size, size_t);
	return sys_getxattr(l, &ua, retval);
}

int
netbsd32_lgetxattr(struct lwp *l, const struct netbsd32_lgetxattr_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(const netbsd32_charp) name;
		syscallarg(netbsd32_voidp) value;
		syscallarg(netbsd32_size_t) size;
	} */
	struct sys_lgetxattr_args ua;
	NETBSD32TOP_UAP(path, const char);
	NETBSD32TOP_UAP(name, const char);
	NETBSD32TOP_UAP(value, void);
	NETBSD32TOX_UAP(size, size_t);
	return sys_lgetxattr(l, &ua, retval);
}

int
netbsd32_fgetxattr(struct lwp *l, const struct netbsd32_fgetxattr_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(const netbsd32_charp) name;
		syscallarg(netbsd32_voidp) value;
		syscallarg(netbsd32_size_t) size;
	} */
	struct sys_fgetxattr_args ua;
	NETBSD32TO64_UAP(fd);
	NETBSD32TOP_UAP(name, const char);
	NETBSD32TOP_UAP(value, void);
	NETBSD32TOX_UAP(size, size_t);
	return sys_fgetxattr(l, &ua, retval);
}

int
netbsd32_listxattr(struct lwp *l, const struct netbsd32_listxattr_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(netbsd32_charp) list;
		syscallarg(netbsd32_size_t) size;
	} */
	struct sys_listxattr_args ua;
	NETBSD32TOP_UAP(path, const char);
	NETBSD32TOP_UAP(list, char);
	NETBSD32TOX_UAP(size, size_t);
	return sys_listxattr(l, &ua, retval);
}

int
netbsd32_llistxattr(struct lwp *l, const struct netbsd32_llistxattr_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(netbsd32_charp) list;
		syscallarg(netbsd32_size_t) size;
	} */
	struct sys_llistxattr_args ua;
	NETBSD32TOP_UAP(path, const char);
	NETBSD32TOP_UAP(list, char);
	NETBSD32TOX_UAP(size, size_t);
	return sys_llistxattr(l, &ua, retval);
}

int
netbsd32_flistxattr(struct lwp *l, const struct netbsd32_flistxattr_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(netbsd32_charp) list;
		syscallarg(netbsd32_size_t) size;
	} */
	struct sys_flistxattr_args ua;
	NETBSD32TO64_UAP(fd);
	NETBSD32TOP_UAP(list, char);
	NETBSD32TOX_UAP(size, size_t);
	return sys_flistxattr(l, &ua, retval);
}

int
netbsd32_removexattr(struct lwp *l, const struct netbsd32_removexattr_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(const netbsd32_charp) name;
	} */
	struct sys_removexattr_args ua;
	NETBSD32TOP_UAP(path, const char);
	NETBSD32TOP_UAP(name, const char);
	return sys_removexattr(l, &ua, retval);
}

int
netbsd32_lremovexattr(struct lwp *l, const struct netbsd32_lremovexattr_args *uap, register_t *retval)
{
	/* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(const netbsd32_charp) name;
	} */
	struct sys_lremovexattr_args ua;
	NETBSD32TOP_UAP(path, const char);
	NETBSD32TOP_UAP(name, const char);
	return sys_lremovexattr(l, &ua, retval);
}

int
netbsd32_fremovexattr(struct lwp *l, const struct netbsd32_fremovexattr_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(const netbsd32_charp) name;
	} */
	struct sys_fremovexattr_args ua;
	NETBSD32TO64_UAP(fd);
	NETBSD32TOP_UAP(name, const char);
	return sys_fremovexattr(l, &ua, retval);
}

int
netbsd32___posix_fadvise50(struct lwp *l,
	const struct netbsd32___posix_fadvise50_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(int) PAD;
		syscallarg(netbsd32_off_t) offset;
		syscallarg(netbsd32_off_t) len;
		syscallarg(int) advice;
	} */

	*retval = do_posix_fadvise(SCARG(uap, fd), SCARG(uap, offset),
	    SCARG(uap, len), SCARG(uap, advice));

	return 0;
}

int
netbsd32__sched_setparam(struct lwp *l,
			 const struct netbsd32__sched_setparam_args *uap,
			 register_t *retval)
{
	/* {
		syscallarg(pid_t) pid;
		syscallarg(lwpid_t) lid;
		syscallarg(int) policy;
		syscallarg(const netbsd32_sched_paramp_t) params;
	} */
	struct sys__sched_setparam_args ua;

	NETBSD32TO64_UAP(pid);
	NETBSD32TO64_UAP(lid);
	NETBSD32TO64_UAP(policy);
	NETBSD32TOP_UAP(params, const struct sched_param *);
	return sys__sched_setparam(l, &ua, retval);
}

int
netbsd32__sched_getparam(struct lwp *l,
			 const struct netbsd32__sched_getparam_args *uap,
			 register_t *retval)
{
	/* {
		syscallarg(pid_t) pid;
		syscallarg(lwpid_t) lid;
		syscallarg(netbsd32_intp) policy;
		syscallarg(netbsd32_sched_paramp_t) params;
	} */
	struct sys__sched_getparam_args ua;

	NETBSD32TO64_UAP(pid);
	NETBSD32TO64_UAP(lid);
	NETBSD32TOP_UAP(policy, int *);
	NETBSD32TOP_UAP(params, struct sched_param *);
	return sys__sched_getparam(l, &ua, retval);
}

int
netbsd32__sched_setaffinity(struct lwp *l,
			    const struct netbsd32__sched_setaffinity_args *uap,
			    register_t *retval)
{
	/* {
		syscallarg(pid_t) pid;
		syscallarg(lwpid_t) lid;
		syscallarg(netbsd_size_t) size;
		syscallarg(const netbsd32_cpusetp_t) cpuset;
	} */
	struct sys__sched_setaffinity_args ua;

	NETBSD32TO64_UAP(pid);
	NETBSD32TO64_UAP(lid);
	NETBSD32TOX_UAP(size, size_t);
	NETBSD32TOP_UAP(cpuset, const cpuset_t *);
	return sys__sched_setaffinity(l, &ua, retval);
}

int
netbsd32__sched_getaffinity(struct lwp *l,
			    const struct netbsd32__sched_getaffinity_args *uap,
			    register_t *retval)
{
	/* {
		syscallarg(pid_t) pid;
		syscallarg(lwpid_t) lid;
		syscallarg(netbsd_size_t) size;
		syscallarg(netbsd32_cpusetp_t) cpuset;
	} */
	struct sys__sched_getaffinity_args ua;

	NETBSD32TO64_UAP(pid);
	NETBSD32TO64_UAP(lid);
	NETBSD32TOX_UAP(size, size_t);
	NETBSD32TOP_UAP(cpuset, cpuset_t *);
	return sys__sched_getaffinity(l, &ua, retval);
}

int
netbsd32__sched_protect(struct lwp *l,
			const struct netbsd32__sched_protect_args *uap,
			register_t *retval)
{
	/* {
		syscallarg(int) priority;
	} */
	struct sys__sched_protect_args ua;

	NETBSD32TO64_UAP(priority);
	return sys__sched_protect(l, &ua, retval);
}

int
netbsd32_pipe2(struct lwp *l, const struct netbsd32_pipe2_args *uap,
	       register_t *retval)
{
	/* {
		syscallarg(netbsd32_intp) fildes;
		syscallarg(int) flags;
	} */
	int fd[2], error;

	error = pipe1(l, retval, SCARG(uap, flags));
	if (error != 0)
		return error;

	fd[0] = retval[0];
	fd[1] = retval[1];

	error = copyout(fd, SCARG_P32(uap, fildes), sizeof(fd));
	if (error != 0)
		return error;

	retval[0] = 0;
	return 0;
}

int
netbsd32_dup3(struct lwp *l, const struct netbsd32_dup3_args *uap,
	      register_t *retval)
{
	/* {
		syscallarg(int) from;
		syscallarg(int) to;
		syscallarg(int) flags;
	} */
	struct sys_dup3_args ua;

	NETBSD32TO64_UAP(from);
	NETBSD32TO64_UAP(to);
	NETBSD32TO64_UAP(flags);

	return sys_dup3(l, &ua, retval);
}

int
netbsd32_kqueue1(struct lwp *l, const struct netbsd32_kqueue1_args *uap,
		 register_t *retval)
{
	/* {
		syscallarg(int) flags;
	} */
	struct sys_kqueue1_args ua;

	NETBSD32TO64_UAP(flags);

	return sys_kqueue1(l, &ua, retval);
}

int
netbsd32_paccept(struct lwp *l, const struct netbsd32_paccept_args *uap,
		 register_t *retval)
{
	/* {
		syscallarg(int) s;
		syscallarg(netbsd32_sockaddrp_t) name;
		syscallarg(netbsd32_socklenp_t) anamelen;
		syscallarg(const netbsd32_sigsetp_t) mask;
		syscallarg(int) flags;
	} */
	struct sys_paccept_args ua;

	NETBSD32TO64_UAP(s);
	NETBSD32TOP_UAP(name, struct sockaddr *);
	NETBSD32TOP_UAP(anamelen, socklen_t *);
	NETBSD32TOP_UAP(mask, const sigset_t *);
	NETBSD32TO64_UAP(flags);

	return sys_paccept(l, &ua, retval);
}

int
netbsd32_fdiscard(struct lwp *l, const struct netbsd32_fdiscard_args *uap,
	register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(netbsd32_off_t) pos;
		syscallarg(netbsd32_off_t) len;
	} */
	struct sys_fdiscard_args ua;

	NETBSD32TO64_UAP(fd);
	NETBSD32TO64_UAP(pos);
	NETBSD32TO64_UAP(len);

	return sys_fdiscard(l, &ua, retval);
}

int
netbsd32_posix_fallocate(struct lwp *l, const struct netbsd32_posix_fallocate_args *uap,
	register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(netbsd32_off_t) pos;
		syscallarg(netbsd32_off_t) len;
	} */
	struct sys_posix_fallocate_args ua;

	NETBSD32TO64_UAP(fd);
	NETBSD32TO64_UAP(pos);
	NETBSD32TO64_UAP(len);

	return sys_posix_fallocate(l, &ua, retval);
}

int
netbsd32_pset_create(struct lwp *l,
    const struct netbsd32_pset_create_args *uap, register_t *retval)
{
	/* {
		syscallarg(netbsd32_psetidp_t) psid;
	}; */

	return sys_pset_create(l, (const void *)uap, retval);
}

int
netbsd32_pset_destroy(struct lwp *l,
     const struct netbsd32_pset_destroy_args *uap, register_t *retval)
{
	/* {
		syscallarg(psetid_t) psid;
	}; */

	return sys_pset_destroy(l, (const void *)uap, retval);
}

int
netbsd32_pset_assign(struct lwp *l,
     const struct netbsd32_pset_assign_args *uap, register_t *retval)
{
	/* {
		syscallarg(psetid_t) psid;
		syscallarg(cpuid_t) cpuid;
		syscallarg(netbsd32_psetidp_t) opsid;
	}; */
	struct sys_pset_assign_args ua;

	SCARG(&ua, psid) = SCARG(uap, psid);
	NETBSD32TO64_UAP(cpuid);
	NETBSD32TOP_UAP(opsid, psetid_t);

	return sys_pset_assign(l, &ua, retval);
}

int
netbsd32__pset_bind(struct lwp *l,
     const struct netbsd32__pset_bind_args *uap, register_t *retval)
{
	/* {
		syscallarg(idtype_t) idtype;
		syscallarg(id_t) first_id;
		syscallarg(id_t) second_id;
		syscallarg(psetid_t) psid;
		syscallarg(netbsd32_psetidp_t) opsid;
	}; */
	struct sys__pset_bind_args ua;

	SCARG(&ua, idtype) = SCARG(uap, idtype);
	SCARG(&ua, first_id) = SCARG(uap, first_id);
	SCARG(&ua, second_id) = SCARG(uap, second_id);
	SCARG(&ua, psid) = SCARG(uap, psid);
	NETBSD32TOP_UAP(opsid, psetid_t);

	return sys__pset_bind(l, &ua, retval);
}


/*
 * MI indirect system call support.
 * Only used if the MD netbsd32_syscall.c doesn't intercept the calls.
 */

#define NETBSD32_SYSCALL
#undef SYS_NSYSENT
#define SYS_NSYSENT NETBSD32_SYS_NSYSENT

#define SYS_SYSCALL netbsd32_syscall
#include "../../kern/sys_syscall.c"
#undef SYS_SYSCALL

#define SYS_SYSCALL netbsd32____syscall
#include "../../kern/sys_syscall.c"
#undef SYS_SYSCALL
