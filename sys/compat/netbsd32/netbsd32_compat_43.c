/*	$NetBSD: netbsd32_compat_43.c,v 1.44 2007/06/30 15:31:49 dsl Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: netbsd32_compat_43.c,v 1.44 2007/06/30 15:31:49 dsl Exp $");

#if defined(_KERNEL_OPT)
#include "opt_compat_43.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/fcntl.h>
#include <sys/filedesc.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/mount.h>
#include <sys/namei.h>
#include <sys/socket.h>
#include <sys/proc.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/stat.h>
#include <sys/syscallargs.h>
#include <sys/time.h>
#include <sys/ucred.h>
#include <sys/vfs_syscalls.h>
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

static void
netbsd32_from_stat(const struct stat *sb, struct netbsd32_stat43 *sp32)
{

	sp32->st_dev = sb->st_dev;
	sp32->st_ino = sb->st_ino;
	sp32->st_mode = sb->st_mode;
	sp32->st_nlink = sb->st_nlink;
	sp32->st_uid = sb->st_uid;
	sp32->st_gid = sb->st_gid;
	sp32->st_rdev = sb->st_rdev;
	sp32->st_size = sb->st_size < (quad_t)1 << 32 ? sb->st_size : -2;
	sp32->st_atimespec.tv_sec = sb->st_atimespec.tv_sec;
	sp32->st_atimespec.tv_nsec = sb->st_atimespec.tv_nsec;
	sp32->st_mtimespec.tv_sec = sb->st_mtimespec.tv_sec;
	sp32->st_mtimespec.tv_nsec = sb->st_mtimespec.tv_nsec;
	sp32->st_ctimespec.tv_sec = sb->st_ctimespec.tv_sec;
	sp32->st_ctimespec.tv_nsec = sb->st_ctimespec.tv_nsec;
	sp32->st_blksize = sb->st_blksize;
	sp32->st_blocks = sb->st_blocks;
	sp32->st_flags = sb->st_flags;
	sp32->st_gen = sb->st_gen;
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
	struct compat_43_netbsd32_stat43_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(netbsd32_stat43p_t) ub;
	} */ *uap = v;
	struct stat sb;
	struct netbsd32_stat43 sb32;
	int error;

	error = do_sys_stat(l, SCARG_P32(uap, path), FOLLOW, &sb);
	if (error == 0) {
		netbsd32_from_stat(&sb, &sb32);
		error = copyout(&sb32, SCARG_P32(uap, ub), sizeof(sb32));
	}
	return error;
}

int
compat_43_netbsd32_lstat43(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct compat_43_netbsd32_lstat43_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(netbsd32_stat43p_t) ub;
	} */ *uap = v;
	struct stat sb;
	struct netbsd32_stat43 sb32;
	int error;

	error = do_sys_stat(l, SCARG_P32(uap, path), NOFOLLOW, &sb);
	if (error == 0) {
		netbsd32_from_stat(&sb, &sb32);
		error = copyout(&sb32, SCARG_P32(uap, ub), sizeof(sb32));
	}
	return error;
}

int
compat_43_netbsd32_fstat43(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct compat_43_netbsd32_fstat43_args /* {
		syscallarg(int) fd;
		syscallarg(netbsd32_stat43p_t) sb;
	} */ *uap = v;
	struct stat sb;
	struct netbsd32_stat43 sb32;
	int error;

	error = do_sys_fstat(l, SCARG(uap, fd), &sb);
	if (error == 0) {
		netbsd32_from_stat(&sb, &sb32);
		error = copyout(&sb32, SCARG_P32(uap, sb), sizeof(sb32));
	}
	return error;
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
 * This is a brutal clone of compat_43_sys_recvmsg().
 */
int
compat_43_netbsd32_orecvmsg(l, v, retval)
	struct lwp* l;
	void *v;
	register_t *retval;
{
	struct compat_43_netbsd32_orecvmsg_args /* {
		syscallarg(int) s;
		syscallarg(netbsd32_omsghdrp_t) msg;
		syscallarg(int) flags;
	} */ *uap = v;
	struct netbsd32_omsghdr omsg;
	struct msghdr msg;
	struct mbuf *from, *control;
	struct iovec *iov, aiov[UIO_SMALLIOV];
	int error;

	error = copyin(SCARG_P32(uap, msg), &omsg, sizeof (struct omsghdr));
	if (error)
		return (error);

	if (NETBSD32PTR64(omsg.msg_accrights) == NULL)
		omsg.msg_accrightslen = 0;
	/* it was this way in 4.4BSD */
	if (omsg.msg_accrightslen > MLEN)
		return EINVAL;

	iov = netbsd32_get_iov(NETBSD32PTR64(omsg.msg_iov), omsg.msg_iovlen,
	    aiov, __arraycount(aiov));
	if (iov == NULL)
		return EFAULT;

	msg.msg_name	= NETBSD32PTR64(omsg.msg_name);
	msg.msg_namelen = omsg.msg_namelen;
	msg.msg_iovlen	= omsg.msg_iovlen;
	msg.msg_iov	= iov;
	msg.msg_flags	= SCARG(uap, flags) & MSG_USERFLAGS;

	error = do_sys_recvmsg(l, SCARG(uap, s), &msg, &from,
	    NETBSD32PTR64(omsg.msg_accrights) != NULL ? &control : NULL,
	    retval);
	if (error != 0)
		return error;

	/*
	 * If there is any control information and it's SCM_RIGHTS,
	 * pass it back to the program.
	 * XXX: maybe there can be more than one chunk of control data?
	 */
	if (NETBSD32PTR64(omsg.msg_accrights) != NULL && control != NULL) {
		struct cmsghdr *cmsg = mtod(control, void *);

		if (cmsg->cmsg_level == SOL_SOCKET
		    && cmsg->cmsg_type == SCM_RIGHTS
		    && cmsg->cmsg_len < omsg.msg_accrightslen
		    && copyout(CMSG_DATA(cmsg),
			    NETBSD32PTR64(omsg.msg_accrights),
			    cmsg->cmsg_len) == 0) {
			omsg.msg_accrightslen = cmsg->cmsg_len;
			free_control_mbuf(l, control, control->m_next);
		} else {
			omsg.msg_accrightslen = 0;
			free_control_mbuf(l, control, control);
		}
	} else
		omsg.msg_accrightslen = 0;

	if (from != NULL)
		/* convert from sockaddr sa_family to osockaddr one here */
		mtod(from, struct osockaddr *)->sa_family =
				    mtod(from, struct sockaddr *)->sa_family;

	error = copyout_sockname(NETBSD32PTR64(omsg.msg_name),
	    &omsg.msg_namelen, 0, from);
	if (from != NULL)
		m_free(from);

	if (error != 0)
		 error = copyout(&omsg, SCARG_P32(uap, msg), sizeof(omsg));

	return error;
}

int
compat_43_netbsd32_osendmsg(l, v, retval)
	struct lwp* l;
	void *v;
	register_t *retval;
{
	struct compat_43_netbsd32_osendmsg_args /* {
		syscallarg(int) s;
		syscallarg(netbsd32_caddr_t) msg;
		syscallarg(int) flags;
	} */ *uap = v;
	struct iovec *iov, aiov[UIO_SMALLIOV];
	struct netbsd32_omsghdr omsg;
	struct msghdr msg;
	int error;
	struct mbuf *nam;
	struct osockaddr *osa;
	struct sockaddr *sa;

	error = copyin(SCARG_P32(uap, msg), &omsg, sizeof (struct omsghdr));
	if (error != 0)
		return (error);

	iov = netbsd32_get_iov(NETBSD32PTR64(omsg.msg_iov), omsg.msg_iovlen,
	    aiov, __arraycount(aiov));
	if (iov == NULL)
		return EFAULT;

	msg.msg_iovlen = omsg.msg_iovlen;
	msg.msg_iov = iov;
	msg.msg_flags = MSG_NAMEMBUF;

	error = sockargs(&nam, NETBSD32PTR64(omsg.msg_name), omsg.msg_namelen,
	    MT_SONAME);
	if (error != 0)
		goto out;

	sa = mtod(nam, void *);
	osa = mtod(nam, void *);
	sa->sa_family = osa->sa_family;
	sa->sa_len = omsg.msg_namelen;

	msg.msg_name = nam;
	msg.msg_namelen = omsg.msg_namelen;
	error = compat43_set_accrights(&msg, NETBSD32PTR64(omsg.msg_accrights),
	    omsg.msg_accrightslen);
	if (error != 0) {
		m_free(nam);
		goto out;
	}

	error = do_sys_sendmsg(l, SCARG(uap, s), &msg, SCARG(uap, flags), retval);

    out:
	if (iov != aiov)
		free(iov, M_TEMP);
	return (error);
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
	struct compat_43_netbsd32_osigvec_args /* {
		syscallarg(int) signum;
		syscallarg(netbsd32_sigvecp_t) nsv;
		syscallarg(netbsd32_sigvecp_t) osv;
	} */ *uap = v;
	struct netbsd32_sigvec sv32;
	struct sigaction nsa, osa;
	int error;

	if (SCARG(uap, signum) >= 32)
		return EINVAL;

	if (SCARG_P32(uap, nsv)) {
		error = copyin(SCARG_P32(uap, nsv), &sv32, sizeof(sv32));
		if (error)
			return error;
		nsa.sa_handler = NETBSD32PTR64(sv32.sv_handler);
		nsa.sa_mask.__bits[0] = sv32.sv_mask;
		nsa.sa_mask.__bits[1] = 0;
		nsa.sa_mask.__bits[2] = 0;
		nsa.sa_mask.__bits[3] = 0;
		nsa.sa_flags = sv32.sv_flags ^ SA_RESTART;
		error = sigaction1(l, SCARG(uap, signum), &nsa, &osa, NULL, 0);
	} else
		error = sigaction1(l, SCARG(uap, signum), NULL, &osa, NULL, 0);
	if (error)
		return error;

	if (SCARG_P32(uap, osv)) {
		NETBSD32PTR32(sv32.sv_handler, osa.sa_handler);
		sv32.sv_mask = osa.sa_mask.__bits[0];
		sv32.sv_flags = osa.sa_flags ^ SA_RESTART;
		error = copyout(&sv32, SCARG_P32(uap, osv), sizeof(sv32));
	}

	return error;
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
	struct compat_43_netbsd32_osigstack_args /* {
		syscallarg(netbsd32_sigstackp_t) nss;
		syscallarg(netbsd32_sigstackp_t) oss;
	} */ *uap = v;
	struct netbsd32_sigstack ss32;
	struct sigaltstack nsa, osa;
	int error;

	if (SCARG_P32(uap, nss)) {
		error = copyin(SCARG_P32(uap, nss), &ss32, sizeof(ss32));
		if (error)
			return error;
		nsa.ss_sp = NETBSD32PTR64(ss32.ss_sp);
		nsa.ss_size = SIGSTKSZ; /* Use the recommended size */
		nsa.ss_flags = ss32.ss_onstack ? SS_ONSTACK : 0;
		error = sigaltstack1(l, &nsa, &osa);
	} else
		error = sigaltstack1(l, NULL, &osa);
	if (error)
		return error;

	if (SCARG_P32(uap, oss)) {
		NETBSD32PTR32(ss32.ss_sp, osa.ss_sp);
		ss32.ss_onstack = (osa.ss_flags & SS_ONSTACK) != 0;
		error = copyout(&ss32, SCARG_P32(uap, oss), sizeof(ss32));
	}

	return error;
}
