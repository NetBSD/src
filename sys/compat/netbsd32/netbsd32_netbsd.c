/*	$NetBSD: netbsd32_netbsd.c,v 1.57.2.4 2002/04/01 07:44:33 nathanw Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: netbsd32_netbsd.c,v 1.57.2.4 2002/04/01 07:44:33 nathanw Exp $");

#if defined(_KERNEL_OPT)
#include "opt_ddb.h"
#include "opt_ktrace.h"
#include "opt_ntp.h"
#include "opt_compat_netbsd.h"
#include "opt_compat_43.h"
#include "opt_sysv.h"

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

#include <uvm/uvm_extern.h>

#include <sys/syscallargs.h>
#include <sys/proc.h>
#include <sys/acct.h>
#include <sys/exec.h>

#include <net/if.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_syscall.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>
#include <compat/netbsd32/netbsd32_conv.h>

#include <machine/frame.h>

#if defined(DDB)
#include <ddb/ddbvar.h>
#endif

extern char netbsd32_sigcode[], netbsd32_esigcode[];
extern struct sysent netbsd32_sysent[];
#ifdef SYSCALL_DEBUG
extern const char * const netbsd32_syscallnames[];
#endif
#ifdef __HAVE_SYSCALL_INTERN
void netbsd32_syscall_intern __P((struct proc *));
#else
void syscall __P((void));
#endif

const struct emul emul_netbsd32 = {
	"netbsd32",
	"/emul/netbsd32",
#ifndef __HAVE_MINIMAL_EMUL
	0,
	NULL,
	netbsd32_SYS_syscall,
	netbsd32_SYS_MAXSYSCALL,
#endif
	netbsd32_sysent,
#ifdef SYSCALL_DEBUG
	netbsd32_syscallnames,
#else
	NULL,
#endif
	netbsd32_sendsig,
	trapsignal,
	netbsd32_sigcode,
	netbsd32_esigcode,
	netbsd32_setregs,
	NULL,
	NULL,
	NULL,
#ifdef __HAVE_SYSCALL_INTERN
	netbsd32_syscall_intern,
#else
	syscall,
#endif
};

/*
 * below are all the standard NetBSD system calls, in the 32bit
 * environment, with the necessary conversions to 64bit before
 * calling the real syscall.  anything that needs special
 * attention is handled elsewhere.
 */

int
netbsd32_exit(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_exit_args /* {
		syscallarg(int) rval;
	} */ *uap = v;
	struct sys_exit_args ua;

	NETBSD32TO64_UAP(rval);
	return sys_exit(p, &ua, retval);
}

int
netbsd32_read(p, v, retval)
	struct proc *p;
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
	return sys_read(p, &ua, retval);
}

int
netbsd32_write(p, v, retval)
	struct proc *p;
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
	return sys_write(p, &ua, retval);
}

int
netbsd32_close(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_close_args /* {
		syscallarg(int) fd;
	} */ *uap = v;
	struct sys_close_args ua;

	NETBSD32TO64_UAP(fd);
	return sys_close(p, &ua, retval);
}

int
netbsd32_open(p, v, retval)
	struct proc *p;
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
	sg = stackgap_init(p, 0);
	CHECK_ALT_EXIST(p, &sg, SCARG(&ua, path));

	return (sys_open(p, &ua, retval));
}

int
netbsd32_link(p, v, retval)
	struct proc *p;
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
	return (sys_link(p, &ua, retval));
}

int
netbsd32_unlink(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_unlink_args /* {
		syscallarg(const netbsd32_charp) path;
	} */ *uap = v;
	struct sys_unlink_args ua;

	NETBSD32TOP_UAP(path, const char);

	return (sys_unlink(p, &ua, retval));
}

int
netbsd32_chdir(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_chdir_args /* {
		syscallarg(const netbsd32_charp) path;
	} */ *uap = v;
	struct sys_chdir_args ua;

	NETBSD32TOP_UAP(path, const char);

	return (sys_chdir(p, &ua, retval));
}

int
netbsd32_fchdir(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_fchdir_args /* {
		syscallarg(int) fd;
	} */ *uap = v;
	struct sys_fchdir_args ua;

	NETBSD32TO64_UAP(fd);

	return (sys_fchdir(p, &ua, retval));
}

int
netbsd32_mknod(p, v, retval)
	struct proc *p;
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

	return (sys_mknod(p, &ua, retval));
}

int
netbsd32_chmod(p, v, retval)
	struct proc *p;
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

	return (sys_chmod(p, &ua, retval));
}

int
netbsd32_chown(p, v, retval)
	struct proc *p;
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

	return (sys_chown(p, &ua, retval));
}

int
netbsd32_break(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_break_args /* {
		syscallarg(netbsd32_charp) nsize;
	} */ *uap = v;
	struct sys_obreak_args ua;

	SCARG(&ua, nsize) = (char *)(u_long)SCARG(uap, nsize);
	NETBSD32TOP_UAP(nsize, char);
	return (sys_obreak(p, &ua, retval));
}

int
netbsd32_mount(p, v, retval)
	struct proc *p;
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
	return (sys_mount(p, &ua, retval));
}

int
netbsd32_unmount(p, v, retval)
	struct proc *p;
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
	return (sys_unmount(p, &ua, retval));
}

int
netbsd32_setuid(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_setuid_args /* {
		syscallarg(uid_t) uid;
	} */ *uap = v;
	struct sys_setuid_args ua;

	NETBSD32TO64_UAP(uid);
	return (sys_setuid(p, &ua, retval));
}

int
netbsd32_ptrace(p, v, retval)
	struct proc *p;
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
	return (sys_ptrace(p, &ua, retval));
}

int
netbsd32_accept(p, v, retval)
	struct proc *p;
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
	return (sys_accept(p, &ua, retval));
}

int
netbsd32_getpeername(p, v, retval)
	struct proc *p;
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
	return (sys_getpeername(p, &ua, retval));
}

int
netbsd32_getsockname(p, v, retval)
	struct proc *p;
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
	return (sys_getsockname(p, &ua, retval));
}

int
netbsd32_access(p, v, retval)
	struct proc *p;
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
	sg = stackgap_init(p, 0);
	CHECK_ALT_EXIST(p, &sg, SCARG(&ua, path));

	return (sys_access(p, &ua, retval));
}

int
netbsd32_chflags(p, v, retval)
	struct proc *p;
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

	return (sys_chflags(p, &ua, retval));
}

int
netbsd32_fchflags(p, v, retval)
	struct proc *p;
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

	return (sys_fchflags(p, &ua, retval));
}

int
netbsd32_lchflags(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_lchflags_args /* {
		syscallarg(int) fd;
		syscallarg(netbsd32_u_long) flags;
	} */ *uap = v;
	struct sys_lchflags_args ua;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(flags);

	return (sys_lchflags(p, &ua, retval));
}

int
netbsd32_kill(p, v, retval)
	struct proc *p;
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

	return (sys_kill(p, &ua, retval));
}

int
netbsd32_dup(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_dup_args /* {
		syscallarg(int) fd;
	} */ *uap = v;
	struct sys_dup_args ua;

	NETBSD32TO64_UAP(fd);

	return (sys_dup(p, &ua, retval));
}

int
netbsd32_profil(p, v, retval)
	struct proc *p;
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
	return (sys_profil(p, &ua, retval));
}

#ifdef KTRACE
int
netbsd32_ktrace(p, v, retval)
	struct proc *p;
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
	return (sys_ktrace(p, &ua, retval));
}
#endif /* KTRACE */

int
netbsd32_utrace(p, v, retval)
	struct proc *p;
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
	return (sys_utrace(p, &ua, retval));
}

int
netbsd32___getlogin(p, v, retval)
	struct proc *p;
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
	return (sys___getlogin(p, &ua, retval));
}

int
netbsd32_setlogin(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_setlogin_args /* {
		syscallarg(const netbsd32_charp) namebuf;
	} */ *uap = v;
	struct sys_setlogin_args ua;

	NETBSD32TOP_UAP(namebuf, char);
	return (sys_setlogin(p, &ua, retval));
}

int
netbsd32_acct(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_acct_args /* {
		syscallarg(const netbsd32_charp) path;
	} */ *uap = v;
	struct sys_acct_args ua;

	NETBSD32TOP_UAP(path, const char);
	return (sys_acct(p, &ua, retval));
}

int
netbsd32_revoke(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_revoke_args /* {
		syscallarg(const netbsd32_charp) path;
	} */ *uap = v;
	struct sys_revoke_args ua;
	caddr_t sg;

	NETBSD32TOP_UAP(path, const char);
	sg = stackgap_init(p, 0);
	CHECK_ALT_EXIST(p, &sg, SCARG(&ua, path));

	return (sys_revoke(p, &ua, retval));
}

int
netbsd32_symlink(p, v, retval)
	struct proc *p;
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

	return (sys_symlink(p, &ua, retval));
}

int
netbsd32_readlink(p, v, retval)
	struct proc *p;
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
	sg = stackgap_init(p, 0);
	CHECK_ALT_SYMLINK(p, &sg, SCARG(&ua, path));

	return (sys_readlink(p, &ua, retval));
}

int
netbsd32_umask(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_umask_args /* {
		syscallarg(mode_t) newmask;
	} */ *uap = v;
	struct sys_umask_args ua;

	NETBSD32TO64_UAP(newmask);
	return (sys_umask(p, &ua, retval));
}

int
netbsd32_chroot(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_chroot_args /* {
		syscallarg(const netbsd32_charp) path;
	} */ *uap = v;
	struct sys_chroot_args ua;

	NETBSD32TOP_UAP(path, const char);
	return (sys_chroot(p, &ua, retval));
}

int
netbsd32_sbrk(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_sbrk_args /* {
		syscallarg(int) incr;
	} */ *uap = v;
	struct sys_sbrk_args ua;

	NETBSD32TO64_UAP(incr);
	return (sys_sbrk(p, &ua, retval));
}

int
netbsd32_sstk(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_sstk_args /* {
		syscallarg(int) incr;
	} */ *uap = v;
	struct sys_sstk_args ua;

	NETBSD32TO64_UAP(incr);
	return (sys_sstk(p, &ua, retval));
}

int
netbsd32_munmap(p, v, retval)
	struct proc *p;
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
	return (sys_munmap(p, &ua, retval));
}

int
netbsd32_mprotect(p, v, retval)
	struct proc *p;
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
	return (sys_mprotect(p, &ua, retval));
}

int
netbsd32_madvise(p, v, retval)
	struct proc *p;
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
	return (sys_madvise(p, &ua, retval));
}

int
netbsd32_mincore(p, v, retval)
	struct proc *p;
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
	return (sys_mincore(p, &ua, retval));
}

/* XXX MOVE ME XXX ? */
int
netbsd32_getgroups(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_getgroups_args /* {
		syscallarg(int) gidsetsize;
		syscallarg(netbsd32_gid_tp) gidset;
	} */ *uap = v;
	struct pcred *pc = p->p_cred;
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
			(caddr_t)(u_long)SCARG(uap, gidset), 
			ngrp * sizeof(gid_t));
	if (error)
		return (error);
	*retval = ngrp;
	return (0);
}

int
netbsd32_setgroups(p, v, retval)
	struct proc *p;
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
	return (sys_setgroups(p, &ua, retval));
}

int
netbsd32_setpgid(p, v, retval)
	struct proc *p;
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
	return (sys_setpgid(p, &ua, retval));
}

int
netbsd32_fcntl(p, v, retval)
	struct proc *p;
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
	return (sys_fcntl(p, &ua, retval));
}

int
netbsd32_dup2(p, v, retval)
	struct proc *p;
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
	return (sys_dup2(p, &ua, retval));
}

int
netbsd32_fsync(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_fsync_args /* {
		syscallarg(int) fd;
	} */ *uap = v;
	struct sys_fsync_args ua;

	NETBSD32TO64_UAP(fd);
	return (sys_fsync(p, &ua, retval));
}

int
netbsd32_setpriority(p, v, retval)
	struct proc *p;
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
	return (sys_setpriority(p, &ua, retval));
}

int
netbsd32_socket(p, v, retval)
	struct proc *p;
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
	return (sys_socket(p, &ua, retval));
}

int
netbsd32_connect(p, v, retval)
	struct proc *p;
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
	return (sys_connect(p, &ua, retval));
}

int
netbsd32_getpriority(p, v, retval)
	struct proc *p;
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
	return (sys_getpriority(p, &ua, retval));
}

int
netbsd32_bind(p, v, retval)
	struct proc *p;
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
	return (sys_bind(p, &ua, retval));
}

int
netbsd32_setsockopt(p, v, retval)
	struct proc *p;
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
	return (sys_setsockopt(p, &ua, retval));
}

int
netbsd32_listen(p, v, retval)
	struct proc *p;
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
	return (sys_listen(p, &ua, retval));
}

int
netbsd32_fchown(p, v, retval)
	struct proc *p;
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
	return (sys_fchown(p, &ua, retval));
}

int
netbsd32_fchmod(p, v, retval)
	struct proc *p;
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
	return (sys_fchmod(p, &ua, retval));
}

int
netbsd32_setreuid(p, v, retval)
	struct proc *p;
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
	return (sys_setreuid(p, &ua, retval));
}

int
netbsd32_setregid(p, v, retval)
	struct proc *p;
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
	return (sys_setregid(p, &ua, retval));
}

int
netbsd32_getsockopt(p, v, retval)
	struct proc *p;
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
	return (sys_getsockopt(p, &ua, retval));
}

int
netbsd32_rename(p, v, retval)
	struct proc *p;
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

	return (sys_rename(p, &ua, retval));
}

int
netbsd32_flock(p, v, retval)
	struct proc *p;
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

	return (sys_flock(p, &ua, retval));
}

int
netbsd32_mkfifo(p, v, retval)
	struct proc *p;
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
	return (sys_mkfifo(p, &ua, retval));
}

int
netbsd32_shutdown(p, v, retval)
	struct proc *p;
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
	return (sys_shutdown(p, &ua, retval));
}

int
netbsd32_socketpair(p, v, retval)
	struct proc *p;
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
	return (sys_socketpair(p, &ua, retval));
}

int
netbsd32_mkdir(p, v, retval)
	struct proc *p;
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
	return (sys_mkdir(p, &ua, retval));
}

int
netbsd32_rmdir(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_rmdir_args /* {
		syscallarg(const netbsd32_charp) path;
	} */ *uap = v;
	struct sys_rmdir_args ua;

	NETBSD32TOP_UAP(path, const char);
	return (sys_rmdir(p, &ua, retval));
}

int
netbsd32_quotactl(p, v, retval)
	struct proc *p;
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
	return (sys_quotactl(p, &ua, retval));
}

#if defined(NFS) || defined(NFSSERVER)
int
netbsd32_nfssvc(p, v, retval)
	struct proc *p;
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
	return (sys_nfssvc(p, &ua, retval));
#else
	/* Why would we want to support a 32-bit nfsd? */
	return (ENOSYS);
#endif
}
#endif

#if defined(NFS) || defined(NFSSERVER)
int
netbsd32_getfh(p, v, retval)
	struct proc *p;
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
	return (sys_getfh(p, &ua, retval));
}
#endif

int
netbsd32_sysarch(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_sysarch_args /* {
		syscallarg(int) op;
		syscallarg(netbsd32_voidp) parms;
	} */ *uap = v;

	switch (SCARG(uap, op)) {
	default:
		printf("(%s) netbsd32_sysarch(%d)\n", MACHINE, SCARG(uap, op));
		return EINVAL;
	}
}

int
netbsd32_pread(p, v, retval)
	struct proc *p;
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
	error = sys_pread(p, &ua, (register_t *)&rt);
	*retval = rt;
	return (error);
}

int
netbsd32_pwrite(p, v, retval)
	struct proc *p;
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
	error = sys_pwrite(p, &ua, (register_t *)&rt);
	*retval = rt;
	return (error);
}

int
netbsd32_setgid(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_setgid_args /* {
		syscallarg(gid_t) gid;
	} */ *uap = v;
	struct sys_setgid_args ua;

	NETBSD32TO64_UAP(gid);
	return (sys_setgid(p, v, retval));
}

int
netbsd32_setegid(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_setegid_args /* {
		syscallarg(gid_t) egid;
	} */ *uap = v;
	struct sys_setegid_args ua;

	NETBSD32TO64_UAP(egid);
	return (sys_setegid(p, v, retval));
}

int
netbsd32_seteuid(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_seteuid_args /* {
		syscallarg(gid_t) euid;
	} */ *uap = v;
	struct sys_seteuid_args ua;

	NETBSD32TO64_UAP(euid);
	return (sys_seteuid(p, v, retval));
}

#ifdef LFS
int
netbsd32_sys_lfs_bmapv(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{

	return (ENOSYS);	/* XXX */
}

int
netbsd32_sys_lfs_markv(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{

	return (ENOSYS);	/* XXX */
}

int
netbsd32_sys_lfs_segclean(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{

	return (ENOSYS);	/* XXX */
}

int
netbsd32_sys_lfs_segwait(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{

	return (ENOSYS);	/* XXX */
}
#endif

int
netbsd32_pathconf(p, v, retval)
	struct proc *p;
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
	error = sys_pathconf(p, &ua, (register_t *)&rt);
	*retval = rt;
	return (error);
}

int
netbsd32_fpathconf(p, v, retval)
	struct proc *p;
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
	error = sys_fpathconf(p, &ua, (register_t *)&rt);
	*retval = rt;
	return (error);
}

int
netbsd32_getrlimit(p, v, retval)
	struct proc *p;
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
	return (copyout(&p->p_rlimit[which], (caddr_t)(u_long)SCARG(uap, rlp),
	    sizeof(struct rlimit)));
}

int
netbsd32_setrlimit(p, v, retval)
	struct proc *p;
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

	error = copyin((caddr_t)(u_long)SCARG(uap, rlp), &alim, sizeof(struct rlimit));
	if (error)
		return (error);
	return (dosetrlimit(p, p->p_cred, which, &alim));
}

int
netbsd32_mmap(p, v, retval)
	struct proc *p;
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
	void *rt;
	int error;

	NETBSD32TOP_UAP(addr, void);
	NETBSD32TOX_UAP(len, size_t);
	NETBSD32TO64_UAP(prot);
	NETBSD32TO64_UAP(flags);
	NETBSD32TO64_UAP(fd);
	NETBSD32TOX_UAP(pad, long);
	NETBSD32TOX_UAP(pos, off_t);
	error = sys_mmap(p, &ua, (register_t *)&rt);
	if ((u_long)rt > (u_long)UINT_MAX) {
		printf("netbsd32_mmap: retval out of range: %p", rt);
		/* Should try to recover and return an error here. */
	}
	*retval = (netbsd32_voidp)(u_long)rt;
	return (error);
}

int
netbsd32_lseek(p, v, retval)
	struct proc *p;
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

	NETBSD32TO64_UAP(fd);
	NETBSD32TO64_UAP(pad);
	NETBSD32TO64_UAP(offset);
	NETBSD32TO64_UAP(whence);
	return (sys_lseek(p, &ua, retval));
}

int
netbsd32_truncate(p, v, retval)
	struct proc *p;
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
	return (sys_truncate(p, &ua, retval));
}

int
netbsd32_ftruncate(p, v, retval)
	struct proc *p;
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
	return (sys_ftruncate(p, &ua, retval));
}

int
netbsd32_mlock(p, v, retval)
	struct proc *p;
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
	return (sys_mlock(p, &ua, retval));
}

int
netbsd32_munlock(p, v, retval)
	struct proc *p;
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
	return (sys_munlock(p, &ua, retval));
}

int
netbsd32_undelete(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_undelete_args /* {
		syscallarg(const netbsd32_charp) path;
	} */ *uap = v;
	struct sys_undelete_args ua;

	NETBSD32TOP_UAP(path, const char);
	return (sys_undelete(p, &ua, retval));
}

int
netbsd32_getpgid(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_getpgid_args /* {
		syscallarg(pid_t) pid;
	} */ *uap = v;
	struct sys_getpgid_args ua;

	NETBSD32TO64_UAP(pid);
	return (sys_getpgid(p, &ua, retval));
}

int
netbsd32_reboot(p, v, retval)
	struct proc *p;
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
	return (sys_reboot(p, &ua, retval));
}

int
netbsd32_poll(p, v, retval)
	struct proc *p;
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
	return (sys_poll(p, &ua, retval));
}

int
netbsd32_fdatasync(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_fdatasync_args /* {
		syscallarg(int) fd;
	} */ *uap = v;
	struct sys_fdatasync_args ua;

	NETBSD32TO64_UAP(fd);
	return (sys_fdatasync(p, &ua, retval));
}

int
netbsd32___posix_rename(p, v, retval)
	struct proc *p;
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
	return (sys___posix_rename(p, &ua, retval));
}

int
netbsd32_swapctl(p, v, retval)
	struct proc *p;
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
	NETBSD32TOP_UAP(arg, const void);
	NETBSD32TO64_UAP(misc);
	return (sys_swapctl(p, &ua, retval));
}

int
netbsd32_minherit(p, v, retval)
	struct proc *p;
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
	return (sys_minherit(p, &ua, retval));
}

int
netbsd32_lchmod(p, v, retval)
	struct proc *p;
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
	return (sys_lchmod(p, &ua, retval));
}

int
netbsd32_lchown(p, v, retval)
	struct proc *p;
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
	return (sys_lchown(p, &ua, retval));
}

int
netbsd32___msync13(p, v, retval)
	struct proc *p;
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
	return (sys___msync13(p, &ua, retval));
}

int
netbsd32___posix_chown(p, v, retval)
	struct proc *p;
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
	return (sys___posix_chown(p, &ua, retval));
}

int
netbsd32___posix_fchown(p, v, retval)
	struct proc *p;
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
	return (sys___posix_fchown(p, &ua, retval));
}

int
netbsd32___posix_lchown(p, v, retval)
	struct proc *p;
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
	return (sys___posix_lchown(p, &ua, retval));
}

int
netbsd32_getsid(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_getsid_args /* {
		syscallarg(pid_t) pid;
	} */ *uap = v;
	struct sys_getsid_args ua;

	NETBSD32TO64_UAP(pid);
	return (sys_getsid(p, &ua, retval));
}

#ifdef KTRACE
int
netbsd32_fktrace(p, v, retval)
	struct proc *p;
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
	return (sys_fktrace(p, &ua, retval));
}
#endif /* KTRACE */

int netbsd32___sigpending14(p, v, retval) 
	struct proc *p;
	void   *v;
	register_t *retval;
{
	struct netbsd32___sigpending14_args /* {
		syscallarg(sigset_t *) set;
	} */ *uap = v;
	struct sys___sigpending14_args ua;

	NETBSD32TOP_UAP(set, sigset_t);
	return (sys___sigpending14(p, &ua, retval));
}

int netbsd32___sigprocmask14(p, v, retval) 
	struct proc *p;
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
	return (sys___sigprocmask14(p, &ua, retval));
}

int netbsd32___sigsuspend14(p, v, retval) 
	struct proc *p;
	void   *v;
	register_t *retval;
{
	struct netbsd32___sigsuspend14_args /* {
		syscallarg(const sigset_t *) set;
	} */ *uap = v;
	struct sys___sigsuspend14_args ua;

	NETBSD32TOP_UAP(set, sigset_t);
	return (sys___sigsuspend14(p, &ua, retval));
};

int netbsd32_fchroot(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_fchroot_args /* {
		syscallarg(int) fd;
	} */ *uap = v;
	struct sys_fchroot_args ua;
	
	NETBSD32TO64_UAP(fd);
	return (sys_fchroot(p, &ua, retval));
}

/*
 * Open a file given a file handle.
 *
 * Check permissions, allocate an open file structure,
 * and call the device open routine if any.
 */
int
netbsd32_fhopen(p, v, retval)
	struct proc *p;
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
	return (sys_fhopen(p, &ua, retval));
}

int netbsd32_fhstat(p, v, retval)
	struct proc *p;
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
	return (sys_fhstat(p, &ua, retval));
}

int netbsd32_fhstatfs(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_fhstatfs_args /* {
		syscallarg(const netbsd32_fhandlep_t) fhp;
		syscallarg(struct statfs *) buf;
	} */ *uap = v;
	struct sys_fhstatfs_args ua;

	NETBSD32TOP_UAP(fhp, const fhandle_t);
	NETBSD32TOP_UAP(buf, struct statfs);
	return (sys_fhstatfs(p, &ua, retval));
}

/* virtual memory syscalls */
int
netbsd32_ovadvise(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct netbsd32_ovadvise_args /* {
		syscallarg(int) anom;
	} */ *uap = v;
	struct sys_ovadvise_args ua;

	NETBSD32TO64_UAP(anom);
	return (sys_ovadvise(p, &ua, retval));
}

