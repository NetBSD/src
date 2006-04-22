/*	$NetBSD: netbsd32_netbsd.c,v 1.99.6.1 2006/04/22 11:38:17 simonb Exp $	*/

/*
 * Copyright (c) 1998, 2001 Matthew R. Green
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: netbsd32_netbsd.c,v 1.99.6.1 2006/04/22 11:38:17 simonb Exp $");

#if defined(_KERNEL_OPT)
#include "opt_ddb.h"
#include "opt_ktrace.h"
#include "opt_ntp.h"
#include "opt_compat_netbsd.h"
#include "opt_compat_43.h"
#include "opt_sysv.h"
#include "opt_nfsserver.h"
#include "opt_syscall_debug.h"

#include "fs_lfs.h"
#include "fs_nfs.h"
#endif

/*
 * Though COMPAT_OLDSOCK is needed only for COMPAT_43, SunOS, Linux,
 * HP-UX, FreeBSD, Ultrix, OSF1, we define it unconditionally so that
 * this would be LKM-safe.
 */
#define COMPAT_OLDSOCK /* used by <sys/socket.h> */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
//#define msg __msg /* Don't ask me! */
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/socketvar.h>
#include <sys/mbuf.h>
#include <sys/stat.h>
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

#include <uvm/uvm_extern.h>

#include <sys/sa.h>
#include <sys/savar.h>
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
#include <compat/netbsd32/netbsd32_sa.h>

#include <machine/frame.h>

#if defined(DDB)
#include <ddb/ddbvar.h>
#endif

extern struct sysent netbsd32_sysent[];
#ifdef SYSCALL_DEBUG
extern const char * const netbsd32_syscallnames[];
#endif
#ifdef __HAVE_SYSCALL_INTERN
void netbsd32_syscall_intern __P((struct proc *));
#else
void syscall __P((void));
#endif

#ifdef COMPAT_16
extern char netbsd32_sigcode[], netbsd32_esigcode[];
struct uvm_object *emul_netbsd32_object;
#endif

extern struct sysctlnode netbsd32_sysctl_root;

const struct sa_emul saemul_netbsd32 = {
	sizeof(ucontext32_t),
	sizeof(struct netbsd32_sa_t),
	sizeof(netbsd32_sa_tp),
	netbsd32_sacopyout,
	netbsd32_upcallconv,
	netbsd32_cpu_upcall,
	(void (*)(struct lwp *, void *))getucontext32,
	netbsd32_sa_ucsp
};

const struct emul emul_netbsd32 = {
	"netbsd32",
	"/emul/netbsd32",
#ifndef __HAVE_MINIMAL_EMUL
	0,
	NULL,
	netbsd32_SYS_syscall,
	netbsd32_SYS_NSYSENT,
#endif
	netbsd32_sysent,
#ifdef SYSCALL_DEBUG
	netbsd32_syscallnames,
#else
	NULL,
#endif
	netbsd32_sendsig,
	trapsignal,
	NULL,
#ifdef COMPAT_16
	netbsd32_sigcode,
	netbsd32_esigcode,
	&emul_netbsd32_object,
#else
	NULL,
	NULL,
	NULL,
#endif
	netbsd32_setregs,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
#ifdef __HAVE_SYSCALL_INTERN
	netbsd32_syscall_intern,
#else
	syscall,
#endif
	&netbsd32_sysctl_root,
	NULL,

	netbsd32_vm_default_addr,
	NULL,
	&saemul_netbsd32,
};

/*
 * below are all the standard NetBSD system calls, in the 32bit
 * environment, with the necessary conversions to 64bit before
 * calling the real syscall.  anything that needs special
 * attention is handled elsewhere.
 */

int
netbsd32_exit(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_exit_args /* {
		syscallarg(int) rval;
	} */ *uap = v;
	struct sys_exit_args ua;

	NETBSD32TO64_UAP(rval);
	return sys_exit(l, &ua, retval);
}

int
netbsd32_read(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_read_args /* {
		syscallarg(int) fd;
		syscallarg(netbsd32_voidp) buf;
		syscallarg(netbsd32_size_t) nbyte;
	} */ *uap = v;
	struct sys_read_args ua;

	NETBSD32TO64_UAP(fd);
	NETBSD32TOP_UAP(buf, void *);
	NETBSD32TOX_UAP(nbyte, size_t);
	return sys_read(l, &ua, retval);
}

int
netbsd32_write(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_write_args /* {
		syscallarg(int) fd;
		syscallarg(const netbsd32_voidp) buf;
		syscallarg(netbsd32_size_t) nbyte;
	} */ *uap = v;
	struct sys_write_args ua;

	NETBSD32TO64_UAP(fd);
	NETBSD32TOP_UAP(buf, void *);
	NETBSD32TOX_UAP(nbyte, size_t);
	return sys_write(l, &ua, retval);
}

int
netbsd32_close(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_close_args /* {
		syscallarg(int) fd;
	} */ *uap = v;
	struct sys_close_args ua;

	NETBSD32TO64_UAP(fd);
	return sys_close(l, &ua, retval);
}

int
netbsd32_open(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_open_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(int) flags;
		syscallarg(mode_t) mode;
	} */ *uap = v;
	struct sys_open_args ua;
	caddr_t sg;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(flags);
	NETBSD32TO64_UAP(mode);
	sg = stackgap_init(l->l_proc, 0);
	CHECK_ALT_EXIST(l, &sg, SCARG(&ua, path));

	return (sys_open(l, &ua, retval));
}

int
netbsd32_link(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_link_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(const netbsd32_charp) link;
	} */ *uap = v;
	struct sys_link_args ua;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TOP_UAP(link, const char);
	return (sys_link(l, &ua, retval));
}

int
netbsd32_unlink(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_unlink_args /* {
		syscallarg(const netbsd32_charp) path;
	} */ *uap = v;
	struct sys_unlink_args ua;

	NETBSD32TOP_UAP(path, const char);

	return (sys_unlink(l, &ua, retval));
}

int
netbsd32_chdir(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_chdir_args /* {
		syscallarg(const netbsd32_charp) path;
	} */ *uap = v;
	struct sys_chdir_args ua;

	NETBSD32TOP_UAP(path, const char);

	return (sys_chdir(l, &ua, retval));
}

int
netbsd32_fchdir(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_fchdir_args /* {
		syscallarg(int) fd;
	} */ *uap = v;
	struct sys_fchdir_args ua;

	NETBSD32TO64_UAP(fd);

	return (sys_fchdir(l, &ua, retval));
}

int
netbsd32_mknod(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_mknod_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(mode_t) mode;
		syscallarg(dev_t) dev;
	} */ *uap = v;
	struct sys_mknod_args ua;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(dev);
	NETBSD32TO64_UAP(mode);

	return (sys_mknod(l, &ua, retval));
}

int
netbsd32_chmod(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_chmod_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(mode_t) mode;
	} */ *uap = v;
	struct sys_chmod_args ua;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(mode);

	return (sys_chmod(l, &ua, retval));
}

int
netbsd32_chown(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_chown_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(uid_t) uid;
		syscallarg(gid_t) gid;
	} */ *uap = v;
	struct sys_chown_args ua;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(uid);
	NETBSD32TO64_UAP(gid);

	return (sys_chown(l, &ua, retval));
}

int
netbsd32_break(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_break_args /* {
		syscallarg(netbsd32_charp) nsize;
	} */ *uap = v;
	struct sys_obreak_args ua;

	SCARG(&ua, nsize) = (char *)NETBSD32PTR64(SCARG(uap, nsize));
	NETBSD32TOP_UAP(nsize, char);
	return (sys_obreak(l, &ua, retval));
}

int
netbsd32_mount(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_mount_args /* {
		syscallarg(const netbsd32_charp) type;
		syscallarg(const netbsd32_charp) path;
		syscallarg(int) flags;
		syscallarg(netbsd32_voidp) data;
	} */ *uap = v;
	struct sys_mount_args ua;

	NETBSD32TOP_UAP(type, const char);
	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(flags);
	NETBSD32TOP_UAP(data, void);
	return (sys_mount(l, &ua, retval));
}

int
netbsd32_unmount(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_unmount_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(int) flags;
	} */ *uap = v;
	struct sys_unmount_args ua;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(flags);
	return (sys_unmount(l, &ua, retval));
}

int
netbsd32_setuid(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_setuid_args /* {
		syscallarg(uid_t) uid;
	} */ *uap = v;
	struct sys_setuid_args ua;

	NETBSD32TO64_UAP(uid);
	return (sys_setuid(l, &ua, retval));
}

int
netbsd32_ptrace(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_ptrace_args /* {
		syscallarg(int) req;
		syscallarg(pid_t) pid;
		syscallarg(netbsd32_caddr_t) addr;
		syscallarg(int) data;
	} */ *uap = v;
	struct sys_ptrace_args ua;

	NETBSD32TO64_UAP(req);
	NETBSD32TO64_UAP(pid);
	NETBSD32TOX64_UAP(addr, caddr_t);
	NETBSD32TO64_UAP(data);
	return (sys_ptrace(l, &ua, retval));
}

int
netbsd32_accept(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_accept_args /* {
		syscallarg(int) s;
		syscallarg(netbsd32_sockaddrp_t) name;
		syscallarg(netbsd32_intp) anamelen;
	} */ *uap = v;
	struct sys_accept_args ua;

	NETBSD32TO64_UAP(s);
	NETBSD32TOP_UAP(name, struct sockaddr);
	NETBSD32TOP_UAP(anamelen, int);
	return (sys_accept(l, &ua, retval));
}

int
netbsd32_getpeername(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_getpeername_args /* {
		syscallarg(int) fdes;
		syscallarg(netbsd32_sockaddrp_t) asa;
		syscallarg(netbsd32_intp) alen;
	} */ *uap = v;
	struct sys_getpeername_args ua;

	NETBSD32TO64_UAP(fdes);
	NETBSD32TOP_UAP(asa, struct sockaddr);
	NETBSD32TOP_UAP(alen, int);
/* NB: do the protocol specific sockaddrs need to be converted? */
	return (sys_getpeername(l, &ua, retval));
}

int
netbsd32_getsockname(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_getsockname_args /* {
		syscallarg(int) fdes;
		syscallarg(netbsd32_sockaddrp_t) asa;
		syscallarg(netbsd32_intp) alen;
	} */ *uap = v;
	struct sys_getsockname_args ua;

	NETBSD32TO64_UAP(fdes);
	NETBSD32TOP_UAP(asa, struct sockaddr);
	NETBSD32TOP_UAP(alen, int);
	return (sys_getsockname(l, &ua, retval));
}

int
netbsd32_access(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_access_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(int) flags;
	} */ *uap = v;
	struct sys_access_args ua;
	caddr_t sg;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(flags);
	sg = stackgap_init(l->l_proc, 0);
	CHECK_ALT_EXIST(l, &sg, SCARG(&ua, path));

	return (sys_access(l, &ua, retval));
}

int
netbsd32_chflags(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_chflags_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(netbsd32_u_long) flags;
	} */ *uap = v;
	struct sys_chflags_args ua;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(flags);

	return (sys_chflags(l, &ua, retval));
}

int
netbsd32_fchflags(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_fchflags_args /* {
		syscallarg(int) fd;
		syscallarg(netbsd32_u_long) flags;
	} */ *uap = v;
	struct sys_fchflags_args ua;

	NETBSD32TO64_UAP(fd);
	NETBSD32TO64_UAP(flags);

	return (sys_fchflags(l, &ua, retval));
}

int
netbsd32_lchflags(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_lchflags_args /* {
		syscallarg(const char *) path;
		syscallarg(netbsd32_u_long) flags;
	} */ *uap = v;
	struct sys_lchflags_args ua;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(flags);

	return (sys_lchflags(l, &ua, retval));
}

int
netbsd32_kill(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_kill_args /* {
		syscallarg(int) pid;
		syscallarg(int) signum;
	} */ *uap = v;
	struct sys_kill_args ua;

	NETBSD32TO64_UAP(pid);
	NETBSD32TO64_UAP(signum);

	return (sys_kill(l, &ua, retval));
}

int
netbsd32_dup(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_dup_args /* {
		syscallarg(int) fd;
	} */ *uap = v;
	struct sys_dup_args ua;

	NETBSD32TO64_UAP(fd);

	return (sys_dup(l, &ua, retval));
}

int
netbsd32_profil(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_profil_args /* {
		syscallarg(netbsd32_caddr_t) samples;
		syscallarg(netbsd32_size_t) size;
		syscallarg(netbsd32_u_long) offset;
		syscallarg(u_int) scale;
	} */ *uap = v;
	struct sys_profil_args ua;

	NETBSD32TOX64_UAP(samples, caddr_t);
	NETBSD32TOX_UAP(size, size_t);
	NETBSD32TOX_UAP(offset, u_long);
	NETBSD32TO64_UAP(scale);
	return (sys_profil(l, &ua, retval));
}

#ifdef KTRACE
int
netbsd32_ktrace(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_ktrace_args /* {
		syscallarg(const netbsd32_charp) fname;
		syscallarg(int) ops;
		syscallarg(int) facs;
		syscallarg(int) pid;
	} */ *uap = v;
	struct sys_ktrace_args ua;

	NETBSD32TOP_UAP(fname, const char);
	NETBSD32TO64_UAP(ops);
	NETBSD32TO64_UAP(facs);
	NETBSD32TO64_UAP(pid);
	return (sys_ktrace(l, &ua, retval));
}
#endif /* KTRACE */

int
netbsd32_utrace(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_utrace_args /* {
		syscallarg(const netbsd32_charp) label;
		syscallarg(netbsd32_voidp) addr;
		syscallarg(netbsd32_size_t) len;
	} */ *uap = v;
	struct sys_utrace_args ua;

	NETBSD32TOP_UAP(label, const char);
	NETBSD32TOP_UAP(addr, void);
	NETBSD32TO64_UAP(len);
	return (sys_utrace(l, &ua, retval));
}

int
netbsd32___getlogin(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32___getlogin_args /* {
		syscallarg(netbsd32_charp) namebuf;
		syscallarg(u_int) namelen;
	} */ *uap = v;
	struct sys___getlogin_args ua;

	NETBSD32TOP_UAP(namebuf, char);
	NETBSD32TO64_UAP(namelen);
	return (sys___getlogin(l, &ua, retval));
}

int
netbsd32_setlogin(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_setlogin_args /* {
		syscallarg(const netbsd32_charp) namebuf;
	} */ *uap = v;
	struct sys___setlogin_args ua;

	NETBSD32TOP_UAP(namebuf, char);
	return (sys___setlogin(l, &ua, retval));
}

int
netbsd32_acct(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_acct_args /* {
		syscallarg(const netbsd32_charp) path;
	} */ *uap = v;
	struct sys_acct_args ua;

	NETBSD32TOP_UAP(path, const char);
	return (sys_acct(l, &ua, retval));
}

int
netbsd32_revoke(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_revoke_args /* {
		syscallarg(const netbsd32_charp) path;
	} */ *uap = v;
	struct sys_revoke_args ua;
	caddr_t sg;

	NETBSD32TOP_UAP(path, const char);
	sg = stackgap_init(l->l_proc, 0);
	CHECK_ALT_EXIST(l, &sg, SCARG(&ua, path));

	return (sys_revoke(l, &ua, retval));
}

int
netbsd32_symlink(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_symlink_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(const netbsd32_charp) link;
	} */ *uap = v;
	struct sys_symlink_args ua;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TOP_UAP(link, const char);

	return (sys_symlink(l, &ua, retval));
}

int
netbsd32_readlink(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_readlink_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(netbsd32_charp) buf;
		syscallarg(netbsd32_size_t) count;
	} */ *uap = v;
	struct sys_readlink_args ua;
	caddr_t sg;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TOP_UAP(buf, char);
	NETBSD32TOX_UAP(count, size_t);
	sg = stackgap_init(l->l_proc, 0);
	CHECK_ALT_SYMLINK(l, &sg, SCARG(&ua, path));

	return (sys_readlink(l, &ua, retval));
}

int
netbsd32_umask(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_umask_args /* {
		syscallarg(mode_t) newmask;
	} */ *uap = v;
	struct sys_umask_args ua;

	NETBSD32TO64_UAP(newmask);
	return (sys_umask(l, &ua, retval));
}

int
netbsd32_chroot(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_chroot_args /* {
		syscallarg(const netbsd32_charp) path;
	} */ *uap = v;
	struct sys_chroot_args ua;

	NETBSD32TOP_UAP(path, const char);
	return (sys_chroot(l, &ua, retval));
}

int
netbsd32_sbrk(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_sbrk_args /* {
		syscallarg(int) incr;
	} */ *uap = v;
	struct sys_sbrk_args ua;

	NETBSD32TO64_UAP(incr);
	return (sys_sbrk(l, &ua, retval));
}

int
netbsd32_sstk(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_sstk_args /* {
		syscallarg(int) incr;
	} */ *uap = v;
	struct sys_sstk_args ua;

	NETBSD32TO64_UAP(incr);
	return (sys_sstk(l, &ua, retval));
}

int
netbsd32_munmap(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_munmap_args /* {
		syscallarg(netbsd32_voidp) addr;
		syscallarg(netbsd32_size_t) len;
	} */ *uap = v;
	struct sys_munmap_args ua;

	NETBSD32TOP_UAP(addr, void);
	NETBSD32TOX_UAP(len, size_t);
	return (sys_munmap(l, &ua, retval));
}

int
netbsd32_mprotect(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_mprotect_args /* {
		syscallarg(netbsd32_voidp) addr;
		syscallarg(netbsd32_size_t) len;
		syscallarg(int) prot;
	} */ *uap = v;
	struct sys_mprotect_args ua;

	NETBSD32TOP_UAP(addr, void);
	NETBSD32TOX_UAP(len, size_t);
	NETBSD32TO64_UAP(prot);
	return (sys_mprotect(l, &ua, retval));
}

int
netbsd32_madvise(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_madvise_args /* {
		syscallarg(netbsd32_voidp) addr;
		syscallarg(netbsd32_size_t) len;
		syscallarg(int) behav;
	} */ *uap = v;
	struct sys_madvise_args ua;

	NETBSD32TOP_UAP(addr, void);
	NETBSD32TOX_UAP(len, size_t);
	NETBSD32TO64_UAP(behav);
	return (sys_madvise(l, &ua, retval));
}

int
netbsd32_mincore(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_mincore_args /* {
		syscallarg(netbsd32_caddr_t) addr;
		syscallarg(netbsd32_size_t) len;
		syscallarg(netbsd32_charp) vec;
	} */ *uap = v;
	struct sys_mincore_args ua;

	NETBSD32TOX64_UAP(addr, caddr_t);
	NETBSD32TOX_UAP(len, size_t);
	NETBSD32TOP_UAP(vec, char);
	return (sys_mincore(l, &ua, retval));
}

/* XXX MOVE ME XXX ? */
int
netbsd32_getgroups(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_getgroups_args /* {
		syscallarg(int) gidsetsize;
		syscallarg(netbsd32_gid_tp) gidset;
	} */ *uap = v;
	struct pcred *pc = l->l_proc->p_cred;
	int ngrp;
	int error;

	ngrp = SCARG(uap, gidsetsize);
	if (ngrp == 0) {
		*retval = pc->pc_ucred->cr_ngroups;
		return (0);
	}
	if (ngrp < pc->pc_ucred->cr_ngroups)
		return (EINVAL);
	ngrp = pc->pc_ucred->cr_ngroups;
	/* Should convert gid_t to netbsd32_gid_t, but they're the same */
	error = copyout((caddr_t)pc->pc_ucred->cr_groups,
	    (caddr_t)NETBSD32PTR64(SCARG(uap, gidset)), ngrp * sizeof(gid_t));
	if (error)
		return (error);
	*retval = ngrp;
	return (0);
}

int
netbsd32_setgroups(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_setgroups_args /* {
		syscallarg(int) gidsetsize;
		syscallarg(const netbsd32_gid_tp) gidset;
	} */ *uap = v;
	struct sys_setgroups_args ua;

	NETBSD32TO64_UAP(gidsetsize);
	NETBSD32TOP_UAP(gidset, gid_t);
	return (sys_setgroups(l, &ua, retval));
}

int
netbsd32_setpgid(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_setpgid_args /* {
		syscallarg(int) pid;
		syscallarg(int) pgid;
	} */ *uap = v;
	struct sys_setpgid_args ua;

	NETBSD32TO64_UAP(pid);
	NETBSD32TO64_UAP(pgid);
	return (sys_setpgid(l, &ua, retval));
}

int
netbsd32_fcntl(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_fcntl_args /* {
		syscallarg(int) fd;
		syscallarg(int) cmd;
		syscallarg(netbsd32_voidp) arg;
	} */ *uap = v;
	struct sys_fcntl_args ua;

	NETBSD32TO64_UAP(fd);
	NETBSD32TO64_UAP(cmd);
	NETBSD32TOP_UAP(arg, void);
	/* we can do this because `struct flock' doesn't change */
	return (sys_fcntl(l, &ua, retval));
}

int
netbsd32_dup2(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_dup2_args /* {
		syscallarg(int) from;
		syscallarg(int) to;
	} */ *uap = v;
	struct sys_dup2_args ua;

	NETBSD32TO64_UAP(from);
	NETBSD32TO64_UAP(to);
	return (sys_dup2(l, &ua, retval));
}

int
netbsd32_fsync(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_fsync_args /* {
		syscallarg(int) fd;
	} */ *uap = v;
	struct sys_fsync_args ua;

	NETBSD32TO64_UAP(fd);
	return (sys_fsync(l, &ua, retval));
}

int
netbsd32_setpriority(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_setpriority_args /* {
		syscallarg(int) which;
		syscallarg(int) who;
		syscallarg(int) prio;
	} */ *uap = v;
	struct sys_setpriority_args ua;

	NETBSD32TO64_UAP(which);
	NETBSD32TO64_UAP(who);
	NETBSD32TO64_UAP(prio);
	return (sys_setpriority(l, &ua, retval));
}

int
netbsd32_socket(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_socket_args /* {
		syscallarg(int) domain;
		syscallarg(int) type;
		syscallarg(int) protocol;
	} */ *uap = v;
	struct sys_socket_args ua;

	NETBSD32TO64_UAP(domain);
	NETBSD32TO64_UAP(type);
	NETBSD32TO64_UAP(protocol);
	return (sys_socket(l, &ua, retval));
}

int
netbsd32_connect(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_connect_args /* {
		syscallarg(int) s;
		syscallarg(const netbsd32_sockaddrp_t) name;
		syscallarg(int) namelen;
	} */ *uap = v;
	struct sys_connect_args ua;

	NETBSD32TO64_UAP(s);
	NETBSD32TOP_UAP(name, struct sockaddr);
	NETBSD32TO64_UAP(namelen);
	return (sys_connect(l, &ua, retval));
}

int
netbsd32_getpriority(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_getpriority_args /* {
		syscallarg(int) which;
		syscallarg(int) who;
	} */ *uap = v;
	struct sys_getpriority_args ua;

	NETBSD32TO64_UAP(which);
	NETBSD32TO64_UAP(who);
	return (sys_getpriority(l, &ua, retval));
}

int
netbsd32_bind(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_bind_args /* {
		syscallarg(int) s;
		syscallarg(const netbsd32_sockaddrp_t) name;
		syscallarg(int) namelen;
	} */ *uap = v;
	struct sys_bind_args ua;

	NETBSD32TO64_UAP(s);
	NETBSD32TOP_UAP(name, struct sockaddr);
	NETBSD32TO64_UAP(namelen);
	return (sys_bind(l, &ua, retval));
}

int
netbsd32_setsockopt(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_setsockopt_args /* {
		syscallarg(int) s;
		syscallarg(int) level;
		syscallarg(int) name;
		syscallarg(const netbsd32_voidp) val;
		syscallarg(int) valsize;
	} */ *uap = v;
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
netbsd32_listen(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_listen_args /* {
		syscallarg(int) s;
		syscallarg(int) backlog;
	} */ *uap = v;
	struct sys_listen_args ua;

	NETBSD32TO64_UAP(s);
	NETBSD32TO64_UAP(backlog);
	return (sys_listen(l, &ua, retval));
}

int
netbsd32_fchown(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_fchown_args /* {
		syscallarg(int) fd;
		syscallarg(uid_t) uid;
		syscallarg(gid_t) gid;
	} */ *uap = v;
	struct sys_fchown_args ua;

	NETBSD32TO64_UAP(fd);
	NETBSD32TO64_UAP(uid);
	NETBSD32TO64_UAP(gid);
	return (sys_fchown(l, &ua, retval));
}

int
netbsd32_fchmod(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_fchmod_args /* {
		syscallarg(int) fd;
		syscallarg(mode_t) mode;
	} */ *uap = v;
	struct sys_fchmod_args ua;

	NETBSD32TO64_UAP(fd);
	NETBSD32TO64_UAP(mode);
	return (sys_fchmod(l, &ua, retval));
}

int
netbsd32_setreuid(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_setreuid_args /* {
		syscallarg(uid_t) ruid;
		syscallarg(uid_t) euid;
	} */ *uap = v;
	struct sys_setreuid_args ua;

	NETBSD32TO64_UAP(ruid);
	NETBSD32TO64_UAP(euid);
	return (sys_setreuid(l, &ua, retval));
}

int
netbsd32_setregid(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_setregid_args /* {
		syscallarg(gid_t) rgid;
		syscallarg(gid_t) egid;
	} */ *uap = v;
	struct sys_setregid_args ua;

	NETBSD32TO64_UAP(rgid);
	NETBSD32TO64_UAP(egid);
	return (sys_setregid(l, &ua, retval));
}

int
netbsd32_getsockopt(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_getsockopt_args /* {
		syscallarg(int) s;
		syscallarg(int) level;
		syscallarg(int) name;
		syscallarg(netbsd32_voidp) val;
		syscallarg(netbsd32_intp) avalsize;
	} */ *uap = v;
	struct sys_getsockopt_args ua;

	NETBSD32TO64_UAP(s);
	NETBSD32TO64_UAP(level);
	NETBSD32TO64_UAP(name);
	NETBSD32TOP_UAP(val, void);
	NETBSD32TOP_UAP(avalsize, int);
	return (sys_getsockopt(l, &ua, retval));
}

int
netbsd32_rename(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_rename_args /* {
		syscallarg(const netbsd32_charp) from;
		syscallarg(const netbsd32_charp) to;
	} */ *uap = v;
	struct sys_rename_args ua;

	NETBSD32TOP_UAP(from, const char);
	NETBSD32TOP_UAP(to, const char)

	return (sys_rename(l, &ua, retval));
}

int
netbsd32_flock(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_flock_args /* {
		syscallarg(int) fd;
		syscallarg(int) how;
	} */ *uap = v;
	struct sys_flock_args ua;

	NETBSD32TO64_UAP(fd);
	NETBSD32TO64_UAP(how)

	return (sys_flock(l, &ua, retval));
}

int
netbsd32_mkfifo(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_mkfifo_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(mode_t) mode;
	} */ *uap = v;
	struct sys_mkfifo_args ua;

	NETBSD32TOP_UAP(path, const char)
	NETBSD32TO64_UAP(mode);
	return (sys_mkfifo(l, &ua, retval));
}

int
netbsd32_shutdown(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_shutdown_args /* {
		syscallarg(int) s;
		syscallarg(int) how;
	} */ *uap = v;
	struct sys_shutdown_args ua;

	NETBSD32TO64_UAP(s)
	NETBSD32TO64_UAP(how);
	return (sys_shutdown(l, &ua, retval));
}

int
netbsd32_socketpair(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_socketpair_args /* {
		syscallarg(int) domain;
		syscallarg(int) type;
		syscallarg(int) protocol;
		syscallarg(netbsd32_intp) rsv;
	} */ *uap = v;
	struct sys_socketpair_args ua;

	NETBSD32TO64_UAP(domain);
	NETBSD32TO64_UAP(type);
	NETBSD32TO64_UAP(protocol);
	NETBSD32TOP_UAP(rsv, int);
	/* Since we're just copying out two `int's we can do this */
	return (sys_socketpair(l, &ua, retval));
}

int
netbsd32_mkdir(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_mkdir_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(mode_t) mode;
	} */ *uap = v;
	struct sys_mkdir_args ua;

	NETBSD32TOP_UAP(path, const char)
	NETBSD32TO64_UAP(mode);
	return (sys_mkdir(l, &ua, retval));
}

int
netbsd32_rmdir(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_rmdir_args /* {
		syscallarg(const netbsd32_charp) path;
	} */ *uap = v;
	struct sys_rmdir_args ua;

	NETBSD32TOP_UAP(path, const char);
	return (sys_rmdir(l, &ua, retval));
}

int
netbsd32_quotactl(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_quotactl_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(int) cmd;
		syscallarg(int) uid;
		syscallarg(netbsd32_caddr_t) arg;
	} */ *uap = v;
	struct sys_quotactl_args ua;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(cmd);
	NETBSD32TO64_UAP(uid);
	NETBSD32TOX64_UAP(arg, caddr_t);
	return (sys_quotactl(l, &ua, retval));
}

#if defined(NFS) || defined(NFSSERVER)
int
netbsd32_nfssvc(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
#if 0
	struct netbsd32_nfssvc_args /* {
		syscallarg(int) flag;
		syscallarg(netbsd32_voidp) argp;
	} */ *uap = v;
	struct sys_nfssvc_args ua;

	NETBSD32TO64_UAP(flag);
	NETBSD32TOP_UAP(argp, void);
	return (sys_nfssvc(l, &ua, retval));
#else
	/* Why would we want to support a 32-bit nfsd? */
	return (ENOSYS);
#endif
}
#endif

#if defined(NFS) || defined(NFSSERVER)
int
netbsd32_getfh(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_getfh_args /* {
		syscallarg(const netbsd32_charp) fname;
		syscallarg(netbsd32_fhandlep_t) fhp;
	} */ *uap = v;
	struct sys_getfh_args ua;

	NETBSD32TOP_UAP(fname, const char);
	NETBSD32TOP_UAP(fhp, struct fhandle);
	/* Lucky for us a fhandlep_t doesn't change sizes */
	return (sys_getfh(l, &ua, retval));
}
#endif

int
netbsd32_pread(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_pread_args /* {
		syscallarg(int) fd;
		syscallarg(netbsd32_voidp) buf;
		syscallarg(netbsd32_size_t) nbyte;
		syscallarg(int) pad;
		syscallarg(off_t) offset;
	} */ *uap = v;
	struct sys_pread_args ua;
	ssize_t rt;
	int error;

	NETBSD32TO64_UAP(fd);
	NETBSD32TOP_UAP(buf, void);
	NETBSD32TOX_UAP(nbyte, size_t);
	NETBSD32TO64_UAP(pad);
	NETBSD32TO64_UAP(offset);
	error = sys_pread(l, &ua, (register_t *)&rt);
	*retval = rt;
	return (error);
}

int
netbsd32_pwrite(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_pwrite_args /* {
		syscallarg(int) fd;
		syscallarg(const netbsd32_voidp) buf;
		syscallarg(netbsd32_size_t) nbyte;
		syscallarg(int) pad;
		syscallarg(off_t) offset;
	} */ *uap = v;
	struct sys_pwrite_args ua;
	ssize_t rt;
	int error;

	NETBSD32TO64_UAP(fd);
	NETBSD32TOP_UAP(buf, void);
	NETBSD32TOX_UAP(nbyte, size_t);
	NETBSD32TO64_UAP(pad);
	NETBSD32TO64_UAP(offset);
	error = sys_pwrite(l, &ua, (register_t *)&rt);
	*retval = rt;
	return (error);
}

int
netbsd32_setgid(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_setgid_args /* {
		syscallarg(gid_t) gid;
	} */ *uap = v;
	struct sys_setgid_args ua;

	NETBSD32TO64_UAP(gid);
	return (sys_setgid(l, v, retval));
}

int
netbsd32_setegid(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_setegid_args /* {
		syscallarg(gid_t) egid;
	} */ *uap = v;
	struct sys_setegid_args ua;

	NETBSD32TO64_UAP(egid);
	return (sys_setegid(l, v, retval));
}

int
netbsd32_seteuid(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_seteuid_args /* {
		syscallarg(gid_t) euid;
	} */ *uap = v;
	struct sys_seteuid_args ua;

	NETBSD32TO64_UAP(euid);
	return (sys_seteuid(l, v, retval));
}

#ifdef LFS
int
netbsd32_sys_lfs_bmapv(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{

	return (ENOSYS);	/* XXX */
}

int
netbsd32_sys_lfs_markv(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{

	return (ENOSYS);	/* XXX */
}

int
netbsd32_sys_lfs_segclean(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{

	return (ENOSYS);	/* XXX */
}

int
netbsd32_sys_lfs_segwait(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{

	return (ENOSYS);	/* XXX */
}
#endif

int
netbsd32_pathconf(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_pathconf_args /* {
		syscallarg(int) fd;
		syscallarg(int) name;
	} */ *uap = v;
	struct sys_pathconf_args ua;
	long rt;
	int error;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(name);
	error = sys_pathconf(l, &ua, (register_t *)&rt);
	*retval = rt;
	return (error);
}

int
netbsd32_fpathconf(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_fpathconf_args /* {
		syscallarg(int) fd;
		syscallarg(int) name;
	} */ *uap = v;
	struct sys_fpathconf_args ua;
	long rt;
	int error;

	NETBSD32TO64_UAP(fd);
	NETBSD32TO64_UAP(name);
	error = sys_fpathconf(l, &ua, (register_t *)&rt);
	*retval = rt;
	return (error);
}

int
netbsd32_getrlimit(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_getrlimit_args /* {
		syscallarg(int) which;
		syscallarg(netbsd32_rlimitp_t) rlp;
	} */ *uap = v;
	int which = SCARG(uap, which);

	if ((u_int)which >= RLIM_NLIMITS)
		return (EINVAL);
	return (copyout(&l->l_proc->p_rlimit[which],
	    (caddr_t)NETBSD32PTR64(SCARG(uap, rlp)), sizeof(struct rlimit)));
}

int
netbsd32_setrlimit(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_setrlimit_args /* {
		syscallarg(int) which;
		syscallarg(const netbsd32_rlimitp_t) rlp;
	} */ *uap = v;
		int which = SCARG(uap, which);
	struct rlimit alim;
	int error;
	struct proc *p = l->l_proc;

	error = copyin((caddr_t)NETBSD32PTR64(SCARG(uap, rlp)), &alim,
	    sizeof(struct rlimit));
	if (error)
		return (error);

	switch (which) {
	case RLIMIT_DATA:
		if (alim.rlim_cur > MAXDSIZ32)
			alim.rlim_cur = MAXDSIZ32;
		if (alim.rlim_max > MAXDSIZ32)
			alim.rlim_max = MAXDSIZ32;
		break;

	case RLIMIT_STACK:
		if (alim.rlim_cur > MAXSSIZ32)
			alim.rlim_cur = MAXSSIZ32;
		if (alim.rlim_max > MAXSSIZ32)
			alim.rlim_max = MAXSSIZ32;
	default:
		break;
	}

	return (dosetrlimit(p, p->p_cred, which, &alim));
}

int
netbsd32_mmap(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_mmap_args /* {
		syscallarg(netbsd32_voidp) addr;
		syscallarg(netbsd32_size_t) len;
		syscallarg(int) prot;
		syscallarg(int) flags;
		syscallarg(int) fd;
		syscallarg(netbsd32_long) pad;
		syscallarg(off_t) pos;
	} */ *uap = v;
	struct sys_mmap_args ua;
	int error;

	NETBSD32TOP_UAP(addr, void);
	NETBSD32TOX_UAP(len, size_t);
	NETBSD32TO64_UAP(prot);
	NETBSD32TO64_UAP(flags);
	NETBSD32TO64_UAP(fd);
	NETBSD32TOX_UAP(pad, long);
	NETBSD32TOX_UAP(pos, off_t);
	error = sys_mmap(l, &ua, retval);
	if ((u_long)*retval > (u_long)UINT_MAX) {
		printf("netbsd32_mmap: retval out of range: 0x%lx",
		    (u_long)*retval);
		/* Should try to recover and return an error here. */
	}
	return (error);
}

int
netbsd32_lseek(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_lseek_args /* {
		syscallarg(int) fd;
		syscallarg(int) pad;
		syscallarg(off_t) offset;
		syscallarg(int) whence;
	} */ *uap = v;
	struct sys_lseek_args ua;
	int rv;

	NETBSD32TO64_UAP(fd);
	NETBSD32TO64_UAP(pad);
	NETBSD32TO64_UAP(offset);
	NETBSD32TO64_UAP(whence);
	rv = sys_lseek(l, &ua, retval);
#ifdef NETBSD32_OFF_T_RETURN
	if (rv == 0)
		NETBSD32_OFF_T_RETURN(retval);
#endif
	return rv;
}

int
netbsd32_truncate(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_truncate_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(int) pad;
		syscallarg(off_t) length;
	} */ *uap = v;
	struct sys_truncate_args ua;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(pad);
	NETBSD32TO64_UAP(length);
	return (sys_truncate(l, &ua, retval));
}

int
netbsd32_ftruncate(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_ftruncate_args /* {
		syscallarg(int) fd;
		syscallarg(int) pad;
		syscallarg(off_t) length;
	} */ *uap = v;
	struct sys_ftruncate_args ua;

	NETBSD32TO64_UAP(fd);
	NETBSD32TO64_UAP(pad);
	NETBSD32TO64_UAP(length);
	return (sys_ftruncate(l, &ua, retval));
}

int
netbsd32_mlock(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_mlock_args /* {
		syscallarg(const netbsd32_voidp) addr;
		syscallarg(netbsd32_size_t) len;
	} */ *uap = v;
	struct sys_mlock_args ua;

	NETBSD32TOP_UAP(addr, const void);
	NETBSD32TO64_UAP(len);
	return (sys_mlock(l, &ua, retval));
}

int
netbsd32_munlock(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_munlock_args /* {
		syscallarg(const netbsd32_voidp) addr;
		syscallarg(netbsd32_size_t) len;
	} */ *uap = v;
	struct sys_munlock_args ua;

	NETBSD32TOP_UAP(addr, const void);
	NETBSD32TO64_UAP(len);
	return (sys_munlock(l, &ua, retval));
}

int
netbsd32_undelete(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_undelete_args /* {
		syscallarg(const netbsd32_charp) path;
	} */ *uap = v;
	struct sys_undelete_args ua;

	NETBSD32TOP_UAP(path, const char);
	return (sys_undelete(l, &ua, retval));
}

int
netbsd32_getpgid(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_getpgid_args /* {
		syscallarg(pid_t) pid;
	} */ *uap = v;
	struct sys_getpgid_args ua;

	NETBSD32TO64_UAP(pid);
	return (sys_getpgid(l, &ua, retval));
}

int
netbsd32_reboot(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_reboot_args /* {
		syscallarg(int) opt;
		syscallarg(netbsd32_charp) bootstr;
	} */ *uap = v;
	struct sys_reboot_args ua;

	NETBSD32TO64_UAP(opt);
	NETBSD32TOP_UAP(bootstr, char);
	return (sys_reboot(l, &ua, retval));
}

#include <sys/poll.h>
int
netbsd32_poll(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_poll_args /* {
		syscallarg(netbsd32_pollfdp_t) fds;
		syscallarg(u_int) nfds;
		syscallarg(int) timeout;
	} */ *uap = v;
	struct sys_poll_args ua;

	NETBSD32TOP_UAP(fds, struct pollfd);
	NETBSD32TO64_UAP(nfds);
	NETBSD32TO64_UAP(timeout);

	return (sys_poll(l, &ua, retval));
}

int
netbsd32_fdatasync(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_fdatasync_args /* {
		syscallarg(int) fd;
	} */ *uap = v;
	struct sys_fdatasync_args ua;

	NETBSD32TO64_UAP(fd);
	return (sys_fdatasync(l, &ua, retval));
}

int
netbsd32___posix_rename(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32___posix_rename_args /* {
		syscallarg(const netbsd32_charp) from;
		syscallarg(const netbsd32_charp) to;
	} */ *uap = v;
	struct sys___posix_rename_args ua;

	NETBSD32TOP_UAP(from, const char);
	NETBSD32TOP_UAP(to, const char);
	return (sys___posix_rename(l, &ua, retval));
}

int
netbsd32_swapctl(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_swapctl_args /* {
		syscallarg(int) cmd;
		syscallarg(const netbsd32_voidp) arg;
		syscallarg(int) misc;
	} */ *uap = v;
	struct sys_swapctl_args ua;

	NETBSD32TO64_UAP(cmd);
	NETBSD32TOP_UAP(arg, void);
	NETBSD32TO64_UAP(misc);
	return (sys_swapctl(l, &ua, retval));
}

int
netbsd32_minherit(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_minherit_args /* {
		syscallarg(netbsd32_voidp) addr;
		syscallarg(netbsd32_size_t) len;
		syscallarg(int) inherit;
	} */ *uap = v;
	struct sys_minherit_args ua;

	NETBSD32TOP_UAP(addr, void);
	NETBSD32TOX_UAP(len, size_t);
	NETBSD32TO64_UAP(inherit);
	return (sys_minherit(l, &ua, retval));
}

int
netbsd32_lchmod(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_lchmod_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(mode_t) mode;
	} */ *uap = v;
	struct sys_lchmod_args ua;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(mode);
	return (sys_lchmod(l, &ua, retval));
}

int
netbsd32_lchown(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_lchown_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(uid_t) uid;
		syscallarg(gid_t) gid;
	} */ *uap = v;
	struct sys_lchown_args ua;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(uid);
	NETBSD32TO64_UAP(gid);
	return (sys_lchown(l, &ua, retval));
}

int
netbsd32___msync13(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32___msync13_args /* {
		syscallarg(netbsd32_voidp) addr;
		syscallarg(netbsd32_size_t) len;
		syscallarg(int) flags;
	} */ *uap = v;
	struct sys___msync13_args ua;

	NETBSD32TOP_UAP(addr, void);
	NETBSD32TOX_UAP(len, size_t);
	NETBSD32TO64_UAP(flags);
	return (sys___msync13(l, &ua, retval));
}

int
netbsd32___posix_chown(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32___posix_chown_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(uid_t) uid;
		syscallarg(gid_t) gid;
	} */ *uap = v;
	struct sys___posix_chown_args ua;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(uid);
	NETBSD32TO64_UAP(gid);
	return (sys___posix_chown(l, &ua, retval));
}

int
netbsd32___posix_fchown(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32___posix_fchown_args /* {
		syscallarg(int) fd;
		syscallarg(uid_t) uid;
		syscallarg(gid_t) gid;
	} */ *uap = v;
	struct sys___posix_fchown_args ua;

	NETBSD32TO64_UAP(fd);
	NETBSD32TO64_UAP(uid);
	NETBSD32TO64_UAP(gid);
	return (sys___posix_fchown(l, &ua, retval));
}

int
netbsd32___posix_lchown(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32___posix_lchown_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(uid_t) uid;
		syscallarg(gid_t) gid;
	} */ *uap = v;
	struct sys___posix_lchown_args ua;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(uid);
	NETBSD32TO64_UAP(gid);
	return (sys___posix_lchown(l, &ua, retval));
}

int
netbsd32_getsid(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_getsid_args /* {
		syscallarg(pid_t) pid;
	} */ *uap = v;
	struct sys_getsid_args ua;

	NETBSD32TO64_UAP(pid);
	return (sys_getsid(l, &ua, retval));
}

#ifdef KTRACE
int
netbsd32_fktrace(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_fktrace_args /* {
		syscallarg(const int) fd;
		syscallarg(int) ops;
		syscallarg(int) facs;
		syscallarg(int) pid;
	} */ *uap = v;
#if 0
	struct sys_fktrace_args ua;
#else
	/* XXXX */
	struct sys_fktrace_noconst_args {
		syscallarg(int) fd;
		syscallarg(int) ops;
		syscallarg(int) facs;
		syscallarg(int) pid;
	} ua;
#endif

	NETBSD32TOX_UAP(fd, int);
	NETBSD32TO64_UAP(ops);
	NETBSD32TO64_UAP(facs);
	NETBSD32TO64_UAP(pid);
	return (sys_fktrace(l, &ua, retval));
}
#endif /* KTRACE */

int netbsd32___sigpending14(l, v, retval)
	struct lwp *l;
	void   *v;
	register_t *retval;
{
	struct netbsd32___sigpending14_args /* {
		syscallarg(sigset_t *) set;
	} */ *uap = v;
	struct sys___sigpending14_args ua;

	NETBSD32TOP_UAP(set, sigset_t);
	return (sys___sigpending14(l, &ua, retval));
}

int netbsd32___sigprocmask14(l, v, retval)
	struct lwp *l;
	void   *v;
	register_t *retval;
{
	struct netbsd32___sigprocmask14_args /* {
		syscallarg(int) how;
		syscallarg(const sigset_t *) set;
		syscallarg(sigset_t *) oset;
	} */ *uap = v;
	struct sys___sigprocmask14_args ua;

	NETBSD32TO64_UAP(how);
	NETBSD32TOP_UAP(set, sigset_t);
	NETBSD32TOP_UAP(oset, sigset_t);
	return (sys___sigprocmask14(l, &ua, retval));
}

int netbsd32___sigsuspend14(l, v, retval)
	struct lwp *l;
	void   *v;
	register_t *retval;
{
	struct netbsd32___sigsuspend14_args /* {
		syscallarg(const sigset_t *) set;
	} */ *uap = v;
	struct sys___sigsuspend14_args ua;

	NETBSD32TOP_UAP(set, sigset_t);
	return (sys___sigsuspend14(l, &ua, retval));
};

int netbsd32_fchroot(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_fchroot_args /* {
		syscallarg(int) fd;
	} */ *uap = v;
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
netbsd32_fhopen(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_fhopen_args /* {
		syscallarg(const fhandle_t *) fhp;
		syscallarg(int) flags;
	} */ *uap = v;
	struct sys_fhopen_args ua;

	NETBSD32TOP_UAP(fhp, fhandle_t);
	NETBSD32TO64_UAP(flags);
	return (sys_fhopen(l, &ua, retval));
}

int netbsd32_fhstat(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_fhstat_args /* {
		syscallarg(const netbsd32_fhandlep_t) fhp;
		syscallarg(struct stat *) sb;
	} */ *uap = v;
	struct sys_fhstat_args ua;

	NETBSD32TOP_UAP(fhp, const fhandle_t);
	NETBSD32TOP_UAP(sb, struct stat);
	return (sys_fhstat(l, &ua, retval));
}

/* virtual memory syscalls */
int
netbsd32_ovadvise(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_ovadvise_args /* {
		syscallarg(int) anom;
	} */ *uap = v;
	struct sys_ovadvise_args ua;

	NETBSD32TO64_UAP(anom);
	return (sys_ovadvise(l, &ua, retval));
}

void
netbsd32_adjust_limits(struct proc *p)
{
	rlim_t *valp;

	valp = &p->p_rlimit[RLIMIT_DATA].rlim_cur;
	if (*valp != RLIM_INFINITY && *valp > MAXDSIZ32)
		*valp = MAXDSIZ32;
	valp = &p->p_rlimit[RLIMIT_DATA].rlim_max;
	if (*valp != RLIM_INFINITY && *valp > MAXDSIZ32)
		*valp = MAXDSIZ32;

	valp = &p->p_rlimit[RLIMIT_STACK].rlim_cur;
	if (*valp != RLIM_INFINITY && *valp > MAXSSIZ32)
		*valp = MAXSSIZ32;
	valp = &p->p_rlimit[RLIMIT_STACK].rlim_max;
	if (*valp != RLIM_INFINITY && *valp > MAXSSIZ32)
		*valp = MAXSSIZ32;
}

int
netbsd32_uuidgen(struct lwp *l, void *v, register_t *retval)
{
	struct netbsd32_uuidgen_args /* {
		syscallarg(netbsd32_uuidp_t) store;
		syscallarg(int) count;
	} */ *uap = v;
	struct sys_uuidgen_args ua;

	NETBSD32TOP_UAP(store, struct uuid);
	NETBSD32TO64_UAP(count);
	return (sys_uuidgen(l, &ua, retval));
}

int
netbsd32_extattrctl(struct lwp *l, void *v, register_t *retval)
{
	struct netbsd32_extattrctl_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(int) cmd;
		syscallarg(const netbsd32_charp) filename;
		syscallarg(int) attrnamespace;
		syscallarg(const netbsd32_charp) attrname;
	} */ *uap = v;
	struct sys_extattrctl_args ua;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(cmd);
	NETBSD32TOP_UAP(filename, const char);
	NETBSD32TO64_UAP(attrnamespace);
	NETBSD32TOP_UAP(attrname, const char);
	return sys_extattrctl(l, &ua, retval);
}

int
netbsd32_extattr_set_fd(struct lwp *l, void *v, register_t *retval)
{
	struct netbsd32_extattr_set_fd_args /* {
		syscallarg(int) fd;
		syscallarg(int) attrnamespace;
		syscallarg(const netbsd32_charp) attrname;
		syscallarg(const netbsd32_voidp) data;
		syscallarg(netbsd32_size_t) nbytes;
	} */ *uap = v;
	struct sys_extattr_set_fd_args ua;

	NETBSD32TO64_UAP(fd);
	NETBSD32TO64_UAP(attrnamespace);
	NETBSD32TOP_UAP(attrname, const char);
	NETBSD32TOP_UAP(data, const void);
	NETBSD32TOX_UAP(nbytes, size_t);
	return sys_extattr_set_fd(l, &ua, retval);
}

int
netbsd32_extattr_set_file(struct lwp *l, void *v, register_t *retval)
{
	struct netbsd32_extattr_set_file_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(int) attrnamespace;
		syscallarg(const netbsd32_charp) attrname;
		syscallarg(const netbsd32_voidp) data;
		syscallarg(netbsd32_size_t) nbytes;
	} */ *uap = v;
	struct sys_extattr_set_file_args ua;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(attrnamespace);
	NETBSD32TOP_UAP(attrname, const char);
	NETBSD32TOP_UAP(data, const void);
	NETBSD32TOX_UAP(nbytes, size_t);
	return sys_extattr_set_file(l, &ua, retval);
}

int
netbsd32_extattr_set_link(struct lwp *l, void *v, register_t *retval)
{
	struct netbsd32_extattr_set_link_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(int) attrnamespace;
		syscallarg(const netbsd32_charp) attrname;
		syscallarg(const netbsd32_voidp) data;
		syscallarg(netbsd32_size_t) nbytes;
	} */ *uap = v;
	struct sys_extattr_set_link_args ua;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(attrnamespace);
	NETBSD32TOP_UAP(attrname, const char);
	NETBSD32TOP_UAP(data, const void);
	NETBSD32TOX_UAP(nbytes, size_t);
	return sys_extattr_set_link(l, &ua, retval);
}

int
netbsd32_extattr_get_fd(struct lwp *l, void *v, register_t *retval)
{
	struct netbsd32_extattr_get_fd_args /* {
		syscallarg(int) fd;
		syscallarg(int) attrnamespace;
		syscallarg(const netbsd32_charp) attrname;
		syscallarg(netbsd32_voidp) data;
		syscallarg(netbsd32_size_t) nbytes;
	} */ *uap = v;
	struct sys_extattr_get_fd_args ua;

	NETBSD32TO64_UAP(fd);
	NETBSD32TO64_UAP(attrnamespace);
	NETBSD32TOP_UAP(attrname, const char);
	NETBSD32TOP_UAP(data, void);
	NETBSD32TOX_UAP(nbytes, size_t);
	return sys_extattr_get_fd(l, &ua, retval);
}

int
netbsd32_extattr_get_file(struct lwp *l, void *v, register_t *retval)
{
	struct netbsd32_extattr_get_file_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(int) attrnamespace;
		syscallarg(const netbsd32_charp) attrname;
		syscallarg(netbsd32_voidp) data;
		syscallarg(netbsd32_size_t) nbytes;
	} */ *uap = v;
	struct sys_extattr_get_file_args ua;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(attrnamespace);
	NETBSD32TOP_UAP(attrname, const char);
	NETBSD32TOP_UAP(data, void);
	NETBSD32TOX_UAP(nbytes, size_t);
	return sys_extattr_get_file(l, &ua, retval);
}

int
netbsd32_extattr_get_link(struct lwp *l, void *v, register_t *retval)
{
	struct netbsd32_extattr_get_link_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(int) attrnamespace;
		syscallarg(const netbsd32_charp) attrname;
		syscallarg(netbsd32_voidp) data;
		syscallarg(netbsd32_size_t) nbytes;
	} */ *uap = v;
	struct sys_extattr_get_link_args ua;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(attrnamespace);
	NETBSD32TOP_UAP(attrname, const char);
	NETBSD32TOP_UAP(data, void);
	NETBSD32TOX_UAP(nbytes, size_t);
	return sys_extattr_get_link(l, &ua, retval);
}

int
netbsd32_extattr_delete_fd(struct lwp *l, void *v, register_t *retval)
{
	struct netbsd32_extattr_delete_fd_args /* {
		syscallarg(int) fd;
		syscallarg(int) attrnamespace;
		syscallarg(const netbsd32_charp) attrname;
	} */ *uap = v;
	struct sys_extattr_delete_fd_args ua;

	NETBSD32TO64_UAP(fd);
	NETBSD32TO64_UAP(attrnamespace);
	NETBSD32TOP_UAP(attrname, const char);
	return sys_extattr_delete_fd(l, &ua, retval);
}

int
netbsd32_extattr_delete_file(struct lwp *l, void *v, register_t *retval)
{
	struct netbsd32_extattr_delete_file_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(int) attrnamespace;
		syscallarg(const netbsd32_charp) attrname;
	} */ *uap = v;
	struct sys_extattr_delete_file_args ua;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(attrnamespace);
	NETBSD32TOP_UAP(attrname, const char);
	return sys_extattr_delete_file(l, &ua, retval);
}

int
netbsd32_extattr_delete_link(struct lwp *l, void *v, register_t *retval)
{
	struct netbsd32_extattr_delete_link_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(int) attrnamespace;
		syscallarg(const netbsd32_charp) attrname;
	} */ *uap = v;
	struct sys_extattr_delete_link_args ua;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(attrnamespace);
	NETBSD32TOP_UAP(attrname, const char);
	return sys_extattr_delete_link(l, &ua, retval);
}

int
netbsd32_extattr_list_fd(struct lwp *l, void *v, register_t *retval)
{
	struct netbsd32_extattr_list_fd_args /* {
		syscallarg(int) fd;
		syscallarg(int) attrnamespace;
		syscallarg(netbsd32_voidp) data;
		syscallarg(netbsd32_size_t) nbytes;
	} */ *uap = v;
	struct sys_extattr_list_fd_args ua;

	NETBSD32TO64_UAP(fd);
	NETBSD32TO64_UAP(attrnamespace);
	NETBSD32TOP_UAP(data, void);
	NETBSD32TOX_UAP(nbytes, size_t);
	return sys_extattr_list_fd(l, &ua, retval);
}

int
netbsd32_extattr_list_file(struct lwp *l, void *v, register_t *retval)
{
	struct netbsd32_extattr_list_file_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(int) attrnamespace;
		syscallarg(netbsd32_voidp) data;
		syscallarg(netbsd32_size_t) nbytes;
	} */ *uap = v;
	struct sys_extattr_list_file_args ua;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(attrnamespace);
	NETBSD32TOP_UAP(data, void);
	NETBSD32TOX_UAP(nbytes, size_t);
	return sys_extattr_list_file(l, &ua, retval);
}

int
netbsd32_extattr_list_link(struct lwp *l, void *v, register_t *retval)
{
	struct netbsd32_extattr_list_link_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(int) attrnamespace;
		syscallarg(netbsd32_voidp) data;
		syscallarg(netbsd32_size_t) nbytes;
	} */ *uap = v;
	struct sys_extattr_list_link_args ua;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(attrnamespace);
	NETBSD32TOP_UAP(data, void);
	NETBSD32TOX_UAP(nbytes, size_t);
	return sys_extattr_list_link(l, &ua, retval);
}

int
netbsd32_mlockall(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct netbsd32_mlockall_args /* {
		syscallarg(int) flags;
	} */ *uap = v;
	struct sys_mlockall_args ua;

	NETBSD32TO64_UAP(flags);
	return (sys_mlockall(l, &ua, retval));
}

int
netbsd32___clone(struct lwp *l, void *v, register_t *retval)
{
	struct netbsd32___clone_args /*  {
		syscallarg(int) flags;
		syscallarg(netbsd32_voidp) stack;
	} */ *uap = v;
	struct sys___clone_args ua;

	NETBSD32TO64_UAP(flags);
	NETBSD32TOP_UAP(stack, void);
	return sys___clone(l, &ua, retval);
}

int
netbsd32_fsync_range(struct lwp *l, void *v, register_t *retval)
{
	struct netbsd32_fsync_range_args /* {
		syscallarg(int) fd;
		syscallarg(int) flags;
		syscallarg(off_t) start;
		syscallarg(off_t) length;
	} */ *uap = v;
	struct sys_fsync_range_args ua;

	NETBSD32TO64_UAP(fd);
	NETBSD32TO64_UAP(flags);
	NETBSD32TO64_UAP(start);
	NETBSD32TO64_UAP(length);
	return (sys_fsync_range(l, &ua, retval));
}

int
netbsd32_rasctl(struct lwp *l, void *v, register_t *retval)
{
	struct netbsd32_rasctl_args /* {
		syscallarg(netbsd32_caddr_t) addr;
		syscallarg(netbsd32_size_t) len;
		syscallarg(int) op;
	} */ *uap = v;
	struct sys_rasctl_args ua;

	NETBSD32TOX64_UAP(addr, caddr_t);
	NETBSD32TOX_UAP(len, size_t);
	NETBSD32TO64_UAP(op);
	return sys_rasctl(l, &ua, retval);
}

int
netbsd32_setxattr(struct lwp *l, void *v, register_t *retval)
{
	struct netbsd32_setxattr_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(const netbsd32_charp) name;
		syscallarg(netbsd32_voidp) value;
		syscallarg(netbsd32_size_t) size;
		syscallarg(int) flags;
	} */ *uap = v;
	struct sys_setxattr_args ua;
	NETBSD32TOP_UAP(path, const char);
	NETBSD32TOP_UAP(name, const char);
	NETBSD32TOP_UAP(value, void);
	NETBSD32TOX_UAP(size, size_t);
	NETBSD32TO64_UAP(flags);
	return sys_setxattr(l, &ua, retval);
}

int
netbsd32_lsetxattr(struct lwp *l, void *v, register_t *retval)
{
	struct netbsd32_lsetxattr_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(const netbsd32_charp) name;
		syscallarg(netbsd32_voidp) value;
		syscallarg(netbsd32_size_t) size;
		syscallarg(int) flags;
	} */ *uap = v;
	struct sys_lsetxattr_args ua;
	NETBSD32TOP_UAP(path, const char);
	NETBSD32TOP_UAP(name, const char);
	NETBSD32TOP_UAP(value, void);
	NETBSD32TOX_UAP(size, size_t);
	NETBSD32TO64_UAP(flags);
	return sys_lsetxattr(l, &ua, retval);
}

int
netbsd32_fsetxattr(struct lwp *l, void *v, register_t *retval)
{
	struct netbsd32_fsetxattr_args /* {
		syscallarg(int) fd;
		syscallarg(const netbsd32_charp) name;
		syscallarg(netbsd32_voidp) value;
		syscallarg(netbsd32_size_t) size;
		syscallarg(int) flags;
	} */ *uap = v;
	struct sys_fsetxattr_args ua;
	NETBSD32TO64_UAP(fd);
	NETBSD32TOP_UAP(name, const char);
	NETBSD32TOP_UAP(value, void);
	NETBSD32TOX_UAP(size, size_t);
	NETBSD32TO64_UAP(flags);
	return sys_fsetxattr(l, &ua, retval);
}

int
netbsd32_getxattr(struct lwp *l, void *v, register_t *retval)
{
	struct netbsd32_getxattr_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(const netbsd32_charp) name;
		syscallarg(netbsd32_voidp) value;
		syscallarg(netbsd32_size_t) size;
	} */ *uap = v;
	struct sys_getxattr_args ua;
	NETBSD32TOP_UAP(path, const char);
	NETBSD32TOP_UAP(name, const char);
	NETBSD32TOP_UAP(value, void);
	NETBSD32TOX_UAP(size, size_t);
	return sys_getxattr(l, &ua, retval);
}

int
netbsd32_lgetxattr(struct lwp *l, void *v, register_t *retval)
{
	struct netbsd32_lgetxattr_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(const netbsd32_charp) name;
		syscallarg(netbsd32_voidp) value;
		syscallarg(netbsd32_size_t) size;
	} */ *uap = v;
	struct sys_lgetxattr_args ua;
	NETBSD32TOP_UAP(path, const char);
	NETBSD32TOP_UAP(name, const char);
	NETBSD32TOP_UAP(value, void);
	NETBSD32TOX_UAP(size, size_t);
	return sys_lgetxattr(l, &ua, retval);
}

int
netbsd32_fgetxattr(struct lwp *l, void *v, register_t *retval)
{
	struct netbsd32_fgetxattr_args /* {
		syscallarg(int) fd;
		syscallarg(const netbsd32_charp) name;
		syscallarg(netbsd32_voidp) value;
		syscallarg(netbsd32_size_t) size;
	} */ *uap = v;
	struct sys_fgetxattr_args ua;
	NETBSD32TO64_UAP(fd);
	NETBSD32TOP_UAP(name, const char);
	NETBSD32TOP_UAP(value, void);
	NETBSD32TOX_UAP(size, size_t);
	return sys_fgetxattr(l, &ua, retval);
}

int
netbsd32_listxattr(struct lwp *l, void *v, register_t *retval)
{
	struct netbsd32_listxattr_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(netbsd32_charp) list;
		syscallarg(netbsd32_size_t) size;
	} */ *uap = v;
	struct sys_listxattr_args ua;
	NETBSD32TOP_UAP(path, const char);
	NETBSD32TOP_UAP(list, char);
	NETBSD32TOX_UAP(size, size_t);
	return sys_listxattr(l, &ua, retval);
}

int
netbsd32_llistxattr(struct lwp *l, void *v, register_t *retval)
{
	struct netbsd32_llistxattr_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(netbsd32_charp) list;
		syscallarg(netbsd32_size_t) size;
	} */ *uap = v;
	struct sys_llistxattr_args ua;
	NETBSD32TOP_UAP(path, const char);
	NETBSD32TOP_UAP(list, char);
	NETBSD32TOX_UAP(size, size_t);
	return sys_llistxattr(l, &ua, retval);
}

int
netbsd32_flistxattr(struct lwp *l, void *v, register_t *retval)
{
	struct netbsd32_flistxattr_args /* {
		syscallarg(int) fd;
		syscallarg(netbsd32_charp) list;
		syscallarg(netbsd32_size_t) size;
	} */ *uap = v;
	struct sys_flistxattr_args ua;
	NETBSD32TO64_UAP(fd);
	NETBSD32TOP_UAP(list, char);
	NETBSD32TOX_UAP(size, size_t);
	return sys_flistxattr(l, &ua, retval);
}

int
netbsd32_removexattr(struct lwp *l, void *v, register_t *retval)
{
	struct netbsd32_removexattr_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(const netbsd32_charp) name;
	} */ *uap = v;
	struct sys_removexattr_args ua;
	NETBSD32TOP_UAP(path, const char);
	NETBSD32TOP_UAP(name, const char);
	return sys_removexattr(l, &ua, retval);
}

int
netbsd32_lremovexattr(struct lwp *l, void *v, register_t *retval)
{
	struct netbsd32_lremovexattr_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(const netbsd32_charp) name;
	} */ *uap = v;
	struct sys_lremovexattr_args ua;
	NETBSD32TOP_UAP(path, const char);
	NETBSD32TOP_UAP(name, const char);
	return sys_lremovexattr(l, &ua, retval);
}

int
netbsd32_fremovexattr(struct lwp *l, void *v, register_t *retval)
{
	struct netbsd32_fremovexattr_args /* {
		syscallarg(int) fd;
		syscallarg(const netbsd32_charp) name;
	} */ *uap = v;
	struct sys_fremovexattr_args ua;
	NETBSD32TO64_UAP(fd);
	NETBSD32TOP_UAP(name, const char);
	return sys_fremovexattr(l, &ua, retval);
}
