/*	$NetBSD: netbsd32_compat_43.c,v 1.40 2007/04/22 08:29:58 dsl Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: netbsd32_compat_43.c,v 1.40 2007/04/22 08:29:58 dsl Exp $");

#if defined(_KERNEL_OPT)
#include "opt_compat_43.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/fcntl.h>
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/socket.h>
#include <sys/proc.h>
#include <sys/stat.h>
#include <sys/syscallargs.h>
#include <sys/time.h>
#include <sys/ucred.h>
#include <uvm/uvm_extern.h>
#include <sys/sysctl.h>
#include <sys/swap.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>

#include <compat/sys/stat.h>
#include <compat/sys/signal.h>
#include <compat/sys/signalvar.h>
#include <compat/sys/socket.h>

int compat_43_netbsd32_sethostid __P((struct lwp *, void *, register_t *));
int compat_43_netbsd32_killpg __P((struct lwp *, void *, register_t *retval));
int compat_43_netbsd32_sigblock __P((struct lwp *, void *, register_t *retval));
int compat_43_netbsd32_sigblock __P((struct lwp *, void *, register_t *retval));
int compat_43_netbsd32_sigsetmask __P((struct lwp *, void *, register_t *retval));

void
netbsd32_from_stat43(sp43, sp32)
	struct stat43 *sp43;
	struct netbsd32_stat43 *sp32;
{

	sp32->st_dev = sp43->st_dev;
	sp32->st_ino = sp43->st_ino;
	sp32->st_mode = sp43->st_mode;
	sp32->st_nlink = sp43->st_nlink;
	sp32->st_uid = sp43->st_uid;
	sp32->st_gid = sp43->st_gid;
	sp32->st_rdev = sp43->st_rdev;
	sp32->st_size = sp43->st_size;
	sp32->st_atimespec.tv_sec = sp43->st_atimespec.tv_sec;
	sp32->st_atimespec.tv_nsec = sp43->st_atimespec.tv_nsec;
	sp32->st_mtimespec.tv_sec = sp43->st_mtimespec.tv_sec;
	sp32->st_mtimespec.tv_nsec = sp43->st_mtimespec.tv_nsec;
	sp32->st_ctimespec.tv_sec = sp43->st_ctimespec.tv_sec;
	sp32->st_ctimespec.tv_nsec = sp43->st_ctimespec.tv_nsec;
	sp32->st_blksize = sp43->st_blksize;
	sp32->st_blocks = sp43->st_blocks;
	sp32->st_flags = sp43->st_flags;
	sp32->st_gen = sp43->st_gen;
}

/* file system syscalls */
int
compat_43_netbsd32_ocreat(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct compat_43_netbsd32_ocreat_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(mode_t) mode;
	} */ *uap = v;
	struct sys_open_args  ua;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(mode);
	SCARG(&ua, flags) = O_WRONLY | O_CREAT | O_TRUNC;

	return (sys_open(l, &ua, retval));
}

int
compat_43_netbsd32_olseek(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct compat_43_netbsd32_olseek_args /* {
		syscallarg(int) fd;
		syscallarg(netbsd32_long) offset;
		syscallarg(int) whence;
	} */ *uap = v;
	struct sys_lseek_args ua;
	int rv;
	off_t rt;

	SCARG(&ua, fd) = SCARG(uap, fd);
	NETBSD32TOX_UAP(offset, long);
	NETBSD32TO64_UAP(whence);
	rv = sys_lseek(l, &ua, (register_t *)&rt);
	*retval = rt;

	return (rv);
}

int
compat_43_netbsd32_stat43(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct proc *p = l->l_proc;
	struct compat_43_netbsd32_stat43_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(netbsd32_stat43p_t) ub;
	} */ *uap = v;
	struct stat43 sb43, *sgsbp;
	struct netbsd32_stat43 sb32;
	struct compat_43_sys_stat_args ua;
	void *sg = stackgap_init(p, 0);
	int rv, error;

	NETBSD32TOP_UAP(path, const char);
	SCARG(&ua, ub) = sgsbp = stackgap_alloc(p, &sg, sizeof(sb43));
	rv = compat_43_sys_stat(l, &ua, retval);

	error = copyin(sgsbp, &sb43, sizeof(sb43));
	if (error)
		return error;
	netbsd32_from_stat43(&sb43, &sb32);
	error = copyout(&sb32, SCARG_P32(uap, ub), sizeof(sb32));
	if (error)
		return error;

	return (rv);
}

int
compat_43_netbsd32_lstat43(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct proc *p = l->l_proc;
	struct compat_43_netbsd32_lstat43_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(netbsd32_stat43p_t) ub;
	} */ *uap = v;
	struct stat43 sb43, *sgsbp;
	struct netbsd32_stat43 sb32;
	struct compat_43_sys_lstat_args ua;
	void *sg = stackgap_init(p, 0);
	int rv, error;

	NETBSD32TOP_UAP(path, const char);
	SCARG(&ua, ub) = sgsbp = stackgap_alloc(p, &sg, sizeof(sb43));
	rv = compat_43_sys_stat(l, &ua, retval);

	error = copyin(sgsbp, &sb43, sizeof(sb43));
	if (error)
		return error;
	netbsd32_from_stat43(&sb43, &sb32);
	error = copyout(&sb32, SCARG_P32(uap, ub), sizeof(sb32));
	if (error)
		return error;

	return (rv);
}

int
compat_43_netbsd32_fstat43(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct proc *p = l->l_proc;
	struct compat_43_netbsd32_fstat43_args /* {
		syscallarg(int) fd;
		syscallarg(netbsd32_stat43p_t) sb;
	} */ *uap = v;
	struct stat43 sb43, *sgsbp;
	struct netbsd32_stat43 sb32;
	struct compat_43_sys_fstat_args ua;
	void *sg = stackgap_init(p, 0);
	int rv, error;

	NETBSD32TO64_UAP(fd);
	SCARG(&ua, sb) = sgsbp = stackgap_alloc(p, &sg, sizeof(sb43));
	rv = compat_43_sys_fstat(l, &ua, retval);

	error = copyin(sgsbp, &sb43, sizeof(sb43));
	if (error)
		return error;
	netbsd32_from_stat43(&sb43, &sb32);
	error = copyout(&sb32, SCARG_P32(uap, sb), sizeof(sb32));
	if (error)
		return error;

	return (rv);
}

int
compat_43_netbsd32_otruncate(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct compat_43_netbsd32_otruncate_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(netbsd32_long) length;
	} */ *uap = v;
	struct sys_truncate_args ua;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(length);
	return (sys_ftruncate(l, &ua, retval));
}

int
compat_43_netbsd32_oftruncate(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct compat_43_netbsd32_oftruncate_args /* {
		syscallarg(int) fd;
		syscallarg(netbsd32_long) length;
	} */ *uap = v;
	struct sys_ftruncate_args ua;

	NETBSD32TO64_UAP(fd);
	NETBSD32TO64_UAP(length);
	return (sys_ftruncate(l, &ua, retval));
}

int
compat_43_netbsd32_ogetdirentries(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct compat_43_netbsd32_ogetdirentries_args /* {
		syscallarg(int) fd;
		syscallarg(netbsd32_charp) buf;
		syscallarg(u_int) count;
		syscallarg(netbsd32_longp) basep;
	} */ *uap = v;
	struct compat_43_sys_getdirentries_args ua;

	NETBSD32TO64_UAP(fd);
	NETBSD32TOP_UAP(buf, char);
	NETBSD32TO64_UAP(count);
	NETBSD32TOP_UAP(basep, long);
	return (compat_43_sys_getdirentries(l, &ua, retval));
}

/* kernel syscalls */
int
compat_43_netbsd32_ogetkerninfo(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct compat_43_netbsd32_ogetkerninfo_args /* {
		syscallarg(int) op;
		syscallarg(netbsd32_charp) where;
		syscallarg(netbsd32_intp) size;
		syscallarg(int) arg;
	} */ *uap = v;
	struct compat_43_sys_getkerninfo_args ua;

	NETBSD32TO64_UAP(op);
	NETBSD32TOP_UAP(where, char);
	NETBSD32TOP_UAP(size, int);
	NETBSD32TO64_UAP(arg);
	return (compat_43_sys_getkerninfo(l, &ua, retval));
}

int
compat_43_netbsd32_ogethostname(l, v, retval)
	struct lwp* l;
	void *v;
	register_t *retval;
{
	struct compat_43_netbsd32_ogethostname_args /* {
		syscallarg(netbsd32_charp) hostname;
		syscallarg(u_int) len;
	} */ *uap = v;
	int name[2];
	size_t sz;

	name[0] = CTL_KERN;
	name[1] = KERN_HOSTNAME;
	sz = SCARG(uap, len);
	return (old_sysctl(&name[0], 2,
	    SCARG_P32(uap, hostname), &sz, 0, 0, l));
}

int
compat_43_netbsd32_osethostname(l, v, retval)
	struct lwp* l;
	void *v;
	register_t *retval;
{
	struct compat_43_netbsd32_osethostname_args /* {
		syscallarg(netbsd32_charp) hostname;
		syscallarg(u_int) len;
	} */ *uap = v;
	int name[2];

	name[0] = CTL_KERN;
	name[1] = KERN_HOSTNAME;
	return old_sysctl(&name[0], 2, 0, 0, (char *)SCARG_P32(uap,
	    hostname), SCARG(uap, len), l);
}

int
compat_43_netbsd32_sethostid(l, v, retval)
	struct lwp* l;
	void *v;
	register_t *retval;
{
	struct compat_43_netbsd32_sethostid_args /* {
		syscallarg(int32_t) hostid;
	} */ *uap = v;
	struct compat_43_sys_sethostid_args ua;

	NETBSD32TO64_UAP(hostid);
	return (compat_43_sys_sethostid(l, &ua, retval));
}

int
compat_43_netbsd32_ogetrlimit(l, v, retval)
	struct lwp* l;
	void *v;
	register_t *retval;
{
	struct compat_43_netbsd32_ogetrlimit_args /* {
		syscallarg(int) which;
		syscallarg(netbsd32_orlimitp_t) rlp;
	} */ *uap = v;
	struct compat_43_sys_getrlimit_args ua;

	NETBSD32TO64_UAP(which);
	NETBSD32TOP_UAP(rlp, struct orlimit);
	return (compat_43_sys_getrlimit(l, &ua, retval));
}

int
compat_43_netbsd32_osetrlimit(l, v, retval)
	struct lwp* l;
	void *v;
	register_t *retval;
{
	struct compat_43_netbsd32_osetrlimit_args /* {
		syscallarg(int) which;
		syscallarg(netbsd32_orlimitp_t) rlp;
	} */ *uap = v;
	struct compat_43_sys_setrlimit_args ua;

	NETBSD32TO64_UAP(which);
	NETBSD32TOP_UAP(rlp, struct orlimit);
	return (compat_43_sys_setrlimit(l, &ua, retval));
}

int
compat_43_netbsd32_killpg(l, v, retval)
	struct lwp* l;
	void *v;
	register_t *retval;
{
	struct compat_43_netbsd32_killpg_args /* {
		syscallarg(int) pgid;
		syscallarg(int) signum;
	} */ *uap = v;
	struct compat_43_sys_killpg_args ua;

	NETBSD32TO64_UAP(pgid);
	NETBSD32TO64_UAP(signum);
	return (compat_43_sys_killpg(l, &ua, retval));
}

/* virtual memory syscalls */
int
compat_43_netbsd32_ommap(l, v, retval)
	struct lwp* l;
	void *v;
	register_t *retval;
{
	struct compat_43_netbsd32_ommap_args /* {
		syscallarg(netbsd32_caddr_t) addr;
		syscallarg(netbsd32_size_t) len;
		syscallarg(int) prot;
		syscallarg(int) flags;
		syscallarg(int) fd;
		syscallarg(netbsd32_long) pos;
	} */ *uap = v;
	struct compat_43_sys_mmap_args ua;

	NETBSD32TOP_UAP(addr, void *);
	NETBSD32TOX_UAP(len, size_t);
	NETBSD32TO64_UAP(prot);
	NETBSD32TO64_UAP(flags);
	NETBSD32TO64_UAP(fd);
	NETBSD32TOX_UAP(pos, long);
	return (compat_43_sys_mmap(l, &ua, retval));
}

/* network syscalls */
int
compat_43_netbsd32_oaccept(l, v, retval)
	struct lwp* l;
	void *v;
	register_t *retval;
{
	struct compat_43_netbsd32_oaccept_args /* {
		syscallarg(int) s;
		syscallarg(netbsd32_caddr_t) name;
		syscallarg(netbsd32_intp) anamelen;
	} */ *uap = v;
	struct compat_43_sys_accept_args ua;

	NETBSD32TOX_UAP(s, int);
	NETBSD32TOP_UAP(name, void *);
	NETBSD32TOP_UAP(anamelen, int);
	return (compat_43_sys_accept(l, &ua, retval));
}

int
compat_43_netbsd32_osend(l, v, retval)
	struct lwp* l;
	void *v;
	register_t *retval;
{
	struct compat_43_netbsd32_osend_args /* {
		syscallarg(int) s;
		syscallarg(netbsd32_caddr_t) buf;
		syscallarg(int) len;
		syscallarg(int) flags;
	} */ *uap = v;
	struct compat_43_sys_send_args ua;

	NETBSD32TO64_UAP(s);
	NETBSD32TOP_UAP(buf, void *);
	NETBSD32TO64_UAP(len);
	NETBSD32TO64_UAP(flags);
	return (compat_43_sys_send(l, &ua, retval));
}

int
compat_43_netbsd32_orecv(l, v, retval)
	struct lwp* l;
	void *v;
	register_t *retval;
{
	struct compat_43_netbsd32_orecv_args /* {
		syscallarg(int) s;
		syscallarg(netbsd32_caddr_t) buf;
		syscallarg(int) len;
		syscallarg(int) flags;
	} */ *uap = v;
	struct compat_43_sys_recv_args ua;

	NETBSD32TO64_UAP(s);
	NETBSD32TOP_UAP(buf, void *);
	NETBSD32TO64_UAP(len);
	NETBSD32TO64_UAP(flags);
	return (compat_43_sys_recv(l, &ua, retval));
}

/*
 * XXX convert these to use a common iovec code to the native
 * netbsd call.
 */
int
compat_43_netbsd32_orecvmsg(l, v, retval)
	struct lwp* l;
	void *v;
	register_t *retval;
{
	struct proc *p = l->l_proc;
	struct compat_43_netbsd32_orecvmsg_args /* {
		syscallarg(int) s;
		syscallarg(netbsd32_omsghdrp_t) msg;
		syscallarg(int) flags;
	} */ *uap = v;
	struct compat_43_sys_recvmsg_args ua;
	struct omsghdr omh, *sgsbp;
	struct netbsd32_omsghdr omh32;
	struct iovec iov, *sgsbp2;
	struct netbsd32_iovec iov32, *iovec32p;
	void *sg = stackgap_init(p, 0);
	int i, error, rv;

	NETBSD32TO64_UAP(s);
	NETBSD32TO64_UAP(flags);

	/*
	 * this is annoying:
	 *	- copyin the msghdr32 struct
	 *	- stackgap_alloc a msghdr struct
	 *	- convert msghdr32 to msghdr:
	 *		- stackgap_alloc enough space for iovec's
	 *		- copy in each iov32, and convert to iov
	 *		- copyout converted iov
	 *	- copyout converted msghdr
	 *	- do real syscall
	 *	- copyin the msghdr struct
	 *	- convert msghdr to msghdr32
	 *		- copyin each iov and convert to iov32
	 *		- copyout converted iov32
	 *	- copyout converted msghdr32
	 */
	error = copyin(SCARG_P32(uap, msg), &omh32, sizeof(omh32));
	if (error)
		return (error);

	SCARG(&ua, msg) = sgsbp = stackgap_alloc(p, &sg, sizeof(omh));
	omh.msg_name = (void *)NETBSD32PTR64(omh32.msg_name);
	omh.msg_namelen = omh32.msg_namelen;
	omh.msg_iovlen = (size_t)omh32.msg_iovlen;
	omh.msg_iov = sgsbp2 = stackgap_alloc(p, &sg, sizeof(struct iovec) * omh.msg_iovlen);
	iovec32p = (struct netbsd32_iovec *)NETBSD32PTR64(omh32.msg_iov);
	for (i = 0; i < omh.msg_iovlen; i++, sgsbp2++, iovec32p++) {
		error = copyin(iovec32p, &iov32, sizeof(iov32));
		if (error)
			return (error);
		iov.iov_base =
		    (struct iovec *)NETBSD32PTR64(iovec32p->iov_base);
		iov.iov_len = (size_t)iovec32p->iov_len;
		error = copyout(&iov, sgsbp2, sizeof(iov));
		if (error)
			return (error);
	}
	omh.msg_accrights = (void *)NETBSD32PTR64(omh32.msg_accrights);
	omh.msg_accrightslen = omh32.msg_accrightslen;
	error = copyout(&omh, sgsbp, sizeof(omh));
	if (error)
		return (error);

	rv = compat_43_sys_recvmsg(l, &ua, retval);

	error = copyin(sgsbp, &omh, sizeof(omh));
	if (error)
		return error;
	NETBSD32PTR32(omh32.msg_name, omh.msg_name);
	omh32.msg_namelen = omh.msg_namelen;
	omh32.msg_iovlen = (netbsd32_size_t)omh.msg_iovlen;
	iovec32p = NETBSD32PTR64(omh32.msg_iov);
	sgsbp2 = omh.msg_iov;
	for (i = 0; i < omh.msg_iovlen; i++, sgsbp2++, iovec32p++) {
		error = copyin(sgsbp2, &iov, sizeof(iov));
		if (error)
			return (error);
		NETBSD32PTR32(iov32.iov_base, iov.iov_base);
		iov32.iov_len = (netbsd32_size_t)iov.iov_len;
		error = copyout(&iov32, iovec32p, sizeof(iov32));
		if (error)
			return (error);
	}
	NETBSD32PTR32(omh32.msg_accrights, omh.msg_accrights);
	omh32.msg_accrightslen = omh.msg_accrightslen;

	error = copyout(&omh32, SCARG_P32(uap, msg), sizeof(omh32));
	if (error)
		return error;

	return (rv);
}

int
compat_43_netbsd32_osendmsg(l, v, retval)
	struct lwp* l;
	void *v;
	register_t *retval;
{
	struct proc *p = l->l_proc;
	struct compat_43_netbsd32_osendmsg_args /* {
		syscallarg(int) s;
		syscallarg(netbsd32_caddr_t) msg;
		syscallarg(int) flags;
	} */ *uap = v;
	struct compat_43_sys_recvmsg_args ua;
	struct omsghdr omh, *sgsbp;
	struct netbsd32_omsghdr omh32;
	struct iovec iov, *sgsbp2;
	struct netbsd32_iovec iov32, *iovec32p;
	void *sg = stackgap_init(p, 0);
	int i, error, rv;

	NETBSD32TO64_UAP(s);
	NETBSD32TO64_UAP(flags);

	/*
	 * this is annoying:
	 *	- copyin the msghdr32 struct
	 *	- stackgap_alloc a msghdr struct
	 *	- convert msghdr32 to msghdr:
	 *		- stackgap_alloc enough space for iovec's
	 *		- copy in each iov32, and convert to iov
	 *		- copyout converted iov
	 *	- copyout converted msghdr
	 *	- do real syscall
	 *	- copyin the msghdr struct
	 *	- convert msghdr to msghdr32
	 *		- copyin each iov and convert to iov32
	 *		- copyout converted iov32
	 *	- copyout converted msghdr32
	 */
	error = copyin(SCARG_P32(uap, msg), &omh32, sizeof(omh32));
	if (error)
		return (error);

	SCARG(&ua, msg) = sgsbp = stackgap_alloc(p, &sg, sizeof(omh));
	omh.msg_name = NETBSD32PTR64(omh32.msg_name);
	omh.msg_namelen = omh32.msg_namelen;
	omh.msg_iovlen = (size_t)omh32.msg_iovlen;
	omh.msg_iov = sgsbp2 = stackgap_alloc(p, &sg, sizeof(struct iovec) * omh.msg_iovlen);
	iovec32p = NETBSD32PTR64(omh32.msg_iov);
	for (i = 0; i < omh.msg_iovlen; i++, sgsbp2++, iovec32p++) {
		error = copyin(iovec32p, &iov32, sizeof(iov32));
		if (error)
			return (error);
		iov.iov_base = NETBSD32PTR64(iovec32p->iov_base);
		iov.iov_len = (size_t)iovec32p->iov_len;
		error = copyout(&iov, sgsbp2, sizeof(iov));
		if (error)
			return (error);
	}
	omh.msg_accrights = NETBSD32PTR64(omh32.msg_accrights);
	omh.msg_accrightslen = omh32.msg_accrightslen;
	error = copyout(&omh, sgsbp, sizeof(omh));
	if (error)
		return (error);

	rv = compat_43_sys_sendmsg(l, &ua, retval);

	error = copyin(sgsbp, &omh, sizeof(omh));
	if (error)
		return error;
	NETBSD32PTR32(omh32.msg_name, omh.msg_name);
	omh32.msg_namelen = omh.msg_namelen;
	omh32.msg_iovlen = (netbsd32_size_t)omh.msg_iovlen;
	iovec32p = NETBSD32PTR64(omh32.msg_iov);
	sgsbp2 = omh.msg_iov;
	for (i = 0; i < omh.msg_iovlen; i++, sgsbp2++, iovec32p++) {
		error = copyin(sgsbp2, &iov, sizeof(iov));
		if (error)
			return (error);
		NETBSD32PTR32(iov32.iov_base, iov.iov_base);
		iov32.iov_len = (netbsd32_size_t)iov.iov_len;
		error = copyout(&iov32, iovec32p, sizeof(iov32));
		if (error)
			return (error);
	}
	NETBSD32PTR32(omh32.msg_accrights, omh.msg_accrights);
	omh32.msg_accrightslen = omh.msg_accrightslen;

	error = copyout(&omh32, SCARG_P32(uap, msg), sizeof(omh32));
	if (error)
		return error;

	return (rv);
}

int
compat_43_netbsd32_orecvfrom(l, v, retval)
	struct lwp* l;
	void *v;
	register_t *retval;
{
	struct compat_43_netbsd32_orecvfrom_args /* {
		syscallarg(int) s;
		syscallarg(netbsd32_caddr_t) buf;
		syscallarg(netbsd32_size_t) len;
		syscallarg(int) flags;
		syscallarg(netbsd32_caddr_t) from;
		syscallarg(netbsd32_intp) fromlenaddr;
	} */ *uap = v;
	struct compat_43_sys_recvfrom_args ua;

	NETBSD32TO64_UAP(s);
	NETBSD32TOP_UAP(buf, void *);
	NETBSD32TOX_UAP(len, size_t);
	NETBSD32TO64_UAP(flags);
	NETBSD32TOP_UAP(from, void *);
	NETBSD32TOP_UAP(fromlenaddr, int);
	return (compat_43_sys_recvfrom(l, &ua, retval));
}

int
compat_43_netbsd32_ogetsockname(l, v, retval)
	struct lwp* l;
	void *v;
	register_t *retval;
{
	struct compat_43_netbsd32_ogetsockname_args /* {
		syscallarg(int) fdec;
		syscallarg(netbsd32_caddr_t) asa;
		syscallarg(netbsd32_intp) alen;
	} */ *uap = v;
	struct compat_43_sys_getsockname_args ua;

	NETBSD32TO64_UAP(fdec);
	NETBSD32TOP_UAP(asa, void *);
	NETBSD32TOP_UAP(alen, int *);
	return (compat_43_sys_getsockname(l, &ua, retval));
}

int
compat_43_netbsd32_ogetpeername(l, v, retval)
	struct lwp* l;
	void *v;
	register_t *retval;
{
	struct compat_43_netbsd32_ogetpeername_args /* {
		syscallarg(int) fdes;
		syscallarg(netbsd32_caddr_t) asa;
		syscallarg(netbsd32_intp) alen;
	} */ *uap = v;
	struct compat_43_sys_getpeername_args ua;

	NETBSD32TO64_UAP(fdes);
	NETBSD32TOP_UAP(asa, void *);
	NETBSD32TOP_UAP(alen, int *);
	return (compat_43_sys_getpeername(l, &ua, retval));
}

/* signal syscalls */
int
compat_43_netbsd32_osigvec(l, v, retval)
	struct lwp* l;
	void *v;
	register_t *retval;
{
	struct proc *p = l->l_proc;
	struct compat_43_netbsd32_osigvec_args /* {
		syscallarg(int) signum;
		syscallarg(netbsd32_sigvecp_t) nsv;
		syscallarg(netbsd32_sigvecp_t) osv;
	} */ *uap = v;
	struct compat_43_sys_sigvec_args ua;
	struct netbsd32_sigvec sv32;
	struct sigvec sv;
	void *sg = stackgap_init(p, 0);
	int rv, error;

	NETBSD32TO64_UAP(signum);
	if (SCARG_P32(uap, osv))
		SCARG(&ua, osv) = stackgap_alloc(p, &sg, sizeof(sv));
	else
		SCARG(&ua, osv) = NULL;
	if (SCARG_P32(uap, nsv)) {
		SCARG(&ua, nsv) = stackgap_alloc(p, &sg, sizeof(sv));
		error = copyin(SCARG_P32(uap, nsv), &sv32, sizeof(sv32));
		if (error)
			return (error);
		sv.sv_handler = (void *)NETBSD32PTR64(sv32.sv_handler);
		sv.sv_mask = sv32.sv_mask;
		sv.sv_flags = sv32.sv_flags;
		error = copyout(&sv, SCARG(&ua, nsv), sizeof(sv));
		if (error)
			return (error);
	} else
		SCARG(&ua, nsv) = NULL;
	rv = compat_43_sys_sigvec(l, &ua, retval);
	if (rv)
		return (rv);

	if (SCARG_P32(uap, osv)) {
		error = copyin(SCARG(&ua, osv), &sv, sizeof(sv));
		if (error)
			return (error);
		NETBSD32PTR32(sv32.sv_handler, sv.sv_handler);
		sv32.sv_mask = sv.sv_mask;
		sv32.sv_flags = sv.sv_flags;
		error = copyout(&sv32, SCARG_P32(uap, osv), sizeof(sv32));
		if (error)
			return (error);
	}

	return (0);
}

int
compat_43_netbsd32_sigblock(l, v, retval)
	struct lwp* l;
	void *v;
	register_t *retval;
{
	struct compat_43_netbsd32_sigblock_args /* {
		syscallarg(int) mask;
	} */ *uap = v;
	struct compat_43_sys_sigblock_args ua;

	NETBSD32TO64_UAP(mask);
	return (compat_43_sys_sigblock(l, &ua, retval));
}

int
compat_43_netbsd32_sigsetmask(l, v, retval)
	struct lwp* l;
	void *v;
	register_t *retval;
{
	struct compat_43_netbsd32_sigsetmask_args /* {
		syscallarg(int) mask;
	} */ *uap = v;
	struct compat_43_sys_sigsetmask_args ua;

	NETBSD32TO64_UAP(mask);
	return (compat_43_sys_sigsetmask(l, &ua, retval));
}

int
compat_43_netbsd32_osigstack(l, v, retval)
	struct lwp* l;
	void *v;
	register_t *retval;
{
	struct proc *p = l->l_proc;
	struct compat_43_netbsd32_osigstack_args /* {
		syscallarg(netbsd32_sigstackp_t) nss;
		syscallarg(netbsd32_sigstackp_t) oss;
	} */ *uap = v;
	struct compat_43_sys_sigstack_args ua;
	struct netbsd32_sigstack ss32;
	struct sigstack ss;
	void *sg = stackgap_init(p, 0);
	int error, rv;

	if (SCARG_P32(uap, oss))
		SCARG(&ua, oss) = stackgap_alloc(p, &sg, sizeof(ss));
	else
		SCARG(&ua, oss) = NULL;
	if (SCARG_P32(uap, nss)) {
		SCARG(&ua, nss) = stackgap_alloc(p, &sg, sizeof(ss));
		error = copyin(SCARG_P32(uap, nss), &ss32, sizeof(ss32));
		if (error)
			return (error);
		ss.ss_sp = NETBSD32PTR64(ss32.ss_sp);
		ss.ss_onstack = ss32.ss_onstack;
		error = copyout(&ss, SCARG(&ua, nss), sizeof(ss));
		if (error)
			return (error);
	} else
		SCARG(&ua, nss) = NULL;

	rv = compat_43_sys_sigstack(l, &ua, retval);
	if (rv)
		return (rv);

	if (SCARG_P32(uap, oss)) {
		error = copyin(SCARG(&ua, oss), &ss, sizeof(ss));
		if (error)
			return (error);
		NETBSD32PTR32(ss32.ss_sp, ss.ss_sp);
		ss32.ss_onstack = ss.ss_onstack;
		error = copyout(&ss32, SCARG_P32(uap, oss), sizeof(ss32));
		if (error)
			return (error);
	}

	return (0);
}
