/*	$NetBSD: linux32_misc.c,v 1.1.8.1 2006/05/24 15:48:27 tron Exp $ */

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

__KERNEL_RCSID(0, "$NetBSD: linux32_misc.c,v 1.1.8.1 2006/05/24 15:48:27 tron Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/fstypes.h>
#include <sys/signal.h>
#include <sys/dirent.h>
#include <sys/kernel.h>
#include <sys/fcntl.h>
#include <sys/select.h>
#include <sys/sa.h>
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
#include <compat/linux/common/linux_limit.h>
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
linux32_sys_uname(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_uname_args /* {
		syscallarg(linux32_utsnamep) up;
	} */ *uap = v;
	struct linux_utsname luts;
	struct linux_utsname *lp;

	strncpy(luts.l_sysname, linux32_sysname, sizeof(luts.l_sysname));
	strncpy(luts.l_nodename, hostname, sizeof(luts.l_nodename));
	strncpy(luts.l_release, linux32_release, sizeof(luts.l_release));
	strncpy(luts.l_version, linux32_version, sizeof(luts.l_version));
#ifdef LINUX_UNAME_ARCH
	strncpy(luts.l_machine, LINUX_UNAME_ARCH, sizeof(luts.l_machine));
#else  
	strncpy(luts.l_machine, machine, sizeof(luts.l_machine));
#endif 
	strncpy(luts.l_domainname, domainname, sizeof(luts.l_domainname));
       
	lp = (struct linux_utsname *)NETBSD32PTR64(SCARG(uap, up));

        return copyout(&luts, lp, sizeof(luts));
}

int
linux32_sys_open(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_open_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(int) flags;
		syscallarg(int) mode;
	} */ *uap = v;
	struct linux_sys_open_args ua;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(flags);
	NETBSD32TO64_UAP(mode);

	return linux_sys_open(l, &ua, retval);
}


int
linux32_sys_brk(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_brk_args /* {
		syscallarg(netbsd32_charp) nsize;
	} */ *uap = v;
	struct linux_sys_brk_args ua;

	NETBSD32TOP_UAP(nsize, char);
	return linux_sys_brk(l, &ua, retval);
}

int
linux32_sys_old_mmap(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_old_mmap_args /* {
		syscallarg(linux32_oldmmapp) lmp;
	} */ *uap = v;
	struct linux_sys_old_mmap_args ua;

	NETBSD32TOP_UAP(lmp, struct linux_oldmmap);
	return linux_sys_old_mmap(l, &ua, retval);

	return 0;
}

int
linux32_sys_mprotect(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{ 
	struct linux32_sys_mprotect_args /* {
		syscallarg(netbsd32_voidp) addr;
		syscallarg(netbsd32_long) len;
		syscallarg(int) prot;
	} */ *uap = v; 
	struct sys_mprotect_args ua;

	NETBSD32TOP_UAP(addr, void);
	NETBSD32TOX_UAP(len, long);
	NETBSD32TO64_UAP(prot);
	return (linux_sys_mprotect(l, &ua, retval));
}

int
linux32_sys_stat64(l, v, retval)  
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_stat64_args /* {  
	        syscallarg(netbsd32_charp) path;
	        syscallarg(linux32_statp) sp;
	} */ *uap = v;
	struct sys___stat30_args ua;
	caddr_t sg = stackgap_init(l->l_proc, 0);
	int error;
	struct stat st;
	struct linux32_stat64 st32;
	struct stat *stp;
	struct linux32_stat64 *st32p;
	
	NETBSD32TOP_UAP(path, const char);

	CHECK_ALT_EXIST(l, &sg, SCARG(&ua, path));

	stp = stackgap_alloc(l->l_proc, &sg, sizeof(*stp));
	SCARG(&ua, ub) = stp;
		
	if ((error = sys___stat30(l, &ua, retval)) != 0)
		return error;

	if ((error = copyin(stp, &st, sizeof(st))) != 0)
		return error;
	
	linux32_from_stat(&st, &st32);

	st32p = (struct linux32_stat64 *)(long)SCARG(uap, sp);

	if ((error = copyout(&st32, st32p, sizeof(*st32p))) != 0)
		return error;

	return 0;
}

int
linux32_sys_lstat64(l, v, retval)  
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_lstat64_args /* {  
	        syscallarg(netbsd32_charp) path;
	        syscallarg(linux32_stat64p) sp;
	} */ *uap = v;
	struct sys___lstat30_args ua;
	caddr_t sg = stackgap_init(l->l_proc, 0);
	int error;
	struct stat st;
	struct linux32_stat64 st32;
	struct stat *stp;
	struct linux32_stat64 *st32p;
	
	NETBSD32TOP_UAP(path, const char);

	CHECK_ALT_EXIST(l, &sg, SCARG(&ua, path));

	stp = stackgap_alloc(l->l_proc, &sg, sizeof(*stp));
	SCARG(&ua, ub) = stp;
		
	if ((error = sys___lstat30(l, &ua, retval)) != 0)
		return error;

	if ((error = copyin(stp, &st, sizeof(st))) != 0)
		return error;
	
	linux32_from_stat(&st, &st32);

	st32p = (struct linux32_stat64 *)(long)SCARG(uap, sp);

	if ((error = copyout(&st32, st32p, sizeof(*st32p))) != 0)
		return error;

	return 0;
}

int
linux32_sys_fstat64(l, v, retval)  
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_fstat64_args /* {  
	        syscallarg(int) fd;
	        syscallarg(linux32_stat64p) sp;
	} */ *uap = v;
	struct sys___fstat30_args ua;
	caddr_t sg = stackgap_init(l->l_proc, 0);
	int error;
	struct stat st;
	struct linux32_stat64 st32;
	struct stat *stp;
	struct linux32_stat64 *st32p;
	
	NETBSD32TO64_UAP(fd);

	stp = stackgap_alloc(l->l_proc, &sg, sizeof(*stp));
	SCARG(&ua, sb) = stp;
		
	if ((error = sys___fstat30(l, &ua, retval)) != 0)
		return error;

	if ((error = copyin(stp, &st, sizeof(st))) != 0)
		return error;
	
	linux32_from_stat(&st, &st32);

	st32p = (struct linux32_stat64 *)(long)SCARG(uap, sp);

	if ((error = copyout(&st32, st32p, sizeof(*st32p))) != 0)
		return error;

	return 0;
}

int
linux32_sys_access(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_access_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(int) flags;
	} */ *uap = v;
	struct sys_access_args ua;
	caddr_t sg;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(flags);

	sg = stackgap_init(l->l_proc, 0);
	CHECK_ALT_EXIST(l, &sg, SCARG(&ua, path));

	return sys_access(l, &ua, retval);
}

int
linux32_sys_getrlimit(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_getrlimit_args /* {
		syscallarg(int) which;
		syscallarg(netbsd32_orlimitp_t) rlp;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	caddr_t sg = stackgap_init(p, 0);
	struct sys_getrlimit_args ap;
	struct orlimit orl;
	struct rlimit rl;
	int error;

	SCARG(&ap, which) = linux_to_bsd_limit(SCARG(uap, which));
	if ((error = SCARG(&ap, which)) < 0)
		return -error;

	SCARG(&ap, rlp) = stackgap_alloc(p, &sg, sizeof rl);

	if ((error = sys_getrlimit(l, &ap, retval)) != 0)
		return error;

	if ((error = copyin(SCARG(&ap, rlp), &rl, sizeof(rl))) != 0)
		return error;

	bsd_to_linux_rlimit(&orl, &rl);

	return copyout(&orl, NETBSD32PTR64(SCARG(uap, rlp)), sizeof(orl));
}

int
linux32_sys_setrlimit(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_setrlimit_args /* {
		syscallarg(int) which;
		syscallarg(netbsd32_orlimitp_t) rlp;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	caddr_t sg = stackgap_init(p, 0);
	struct sys_getrlimit_args ap;
	struct rlimit rl;
	struct orlimit orl;
	int error;

	SCARG(&ap, which) = linux_to_bsd_limit(SCARG(uap, which));
	SCARG(&ap, rlp) = stackgap_alloc(p, &sg, sizeof rl);
	if ((error = SCARG(&ap, which)) < 0)
		return -error;

	if ((error = copyin(NETBSD32PTR64(SCARG(uap, rlp)), 
	    &orl, sizeof(orl))) != 0)
		return error;

	linux_to_bsd_rlimit(&rl, &orl);

	if ((error = copyout(&rl, SCARG(&ap, rlp), sizeof(rl))) != 0)
		return error;

	return sys_setrlimit(l, &ap, retval);
}

#if defined(__amd64__)
int
linux32_sys_ugetrlimit(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	return linux32_sys_getrlimit(l, v, retval);
}
#endif

extern struct timezone linux_sys_tz;
int
linux32_sys_gettimeofday(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_gettimeofday_args /* {
		syscallarg(netbsd32_timevalp_t) tp;
		syscallarg(netbsd32_timezonep_t) tzp;
	} */ *uap = v;
	struct timeval tv;
	struct netbsd32_timeval tv32;
	int error;

	if (NETBSD32PTR64(SCARG(uap, tp)) != NULL) {
		microtime(&tv);
		netbsd32_from_timeval(&tv, &tv32);
		if ((error = copyout(&tv32, 
		    (caddr_t)NETBSD32PTR64(SCARG(uap, tp)), 
		    sizeof(tv32))) != 0)
			return error;
	}

	/* timezone size does not change */
	if (NETBSD32PTR64(SCARG(uap, tzp)) != NULL) {
		if ((error = copyout(&linux_sys_tz,
		    (caddr_t)NETBSD32PTR64(SCARG(uap, tzp)), 
		    sizeof(linux_sys_tz))) != 0)
			return error;
	}

	return 0;
}

int
linux32_sys_settimeofday(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_settimeofday_args /* {
		syscallarg(netbsd32_timevalp_t) tp;
		syscallarg(netbsd32_timezonep_t) tzp;
	} */ *uap = v;
	struct linux_sys_settimeofday_args ua;

	NETBSD32TOP_UAP(tp, struct timeval);
	NETBSD32TOP_UAP(tzp, struct timezone);

	return linux_sys_settimeofday(l, &ua, retval);
}

int
linux32_sys_fcntl64(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_fcntl64_args /* {
		syscallcarg(int) fd;
                syscallarg(int) cmd;
		syscallarg(netbsd32_voidp) arg; 
	} */ *uap = v;
	struct linux_sys_fcntl64_args ua;

	NETBSD32TO64_UAP(fd);
	NETBSD32TO64_UAP(cmd);
	NETBSD32TOP_UAP(arg, void);

	return linux_sys_fcntl64(l, &ua, retval);
}

int
linux32_sys_fcntl(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_fcntl_args /* {
		syscallcarg(int) fd;
                syscallarg(int) cmd;
		syscallarg(netbsd32_voidp) arg; 
	} */ *uap = v;
	struct linux_sys_fcntl_args ua;

	NETBSD32TO64_UAP(fd);
	NETBSD32TO64_UAP(cmd);
	NETBSD32TOP_UAP(arg, void);

	return linux_sys_fcntl(l, &ua, retval);
}

int
linux32_sys_llseek(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_llseek_args /* {
		syscallcarg(int) fd;
                syscallarg(u_int32_t) ohigh;
                syscallarg(u_int32_t) olow;
		syscallarg(netbsd32_caddr_t) res;
		syscallcarg(int) whence;
	} */ *uap = v;
	struct linux_sys_llseek_args ua;

	NETBSD32TO64_UAP(fd);
	NETBSD32TO64_UAP(ohigh);
	NETBSD32TO64_UAP(olow);
	NETBSD32TOP_UAP(res, char);
	NETBSD32TO64_UAP(whence);

	return linux_sys_llseek(l, &ua, retval);
}


int
linux32_sys_time(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_time_args /* {
		syscallcarg(linux32_timep_t) t;
	} */ *uap = v;
	struct linux_sys_time_args ua;

	NETBSD32TOP_UAP(t, linux_time_t);

	return linux_sys_time(l, &ua, retval);
}


int
linux32_sys_getdents(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_getdents_args /* {
		syscallcarg(int) fd;
		syscallcarg(linux32_direntp_t) dent;
		syscallcarg(unsigned int) count;
	} */ *uap = v;
	struct linux_sys_getdents_args ua;

	NETBSD32TO64_UAP(fd);
	NETBSD32TOP_UAP(dent, struct linux_dirent);
	NETBSD32TO64_UAP(count);

	return linux_sys_getdents(l, &ua, retval);
}


int
linux32_sys_getdents64(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_getdents64_args /* {
		syscallcarg(int) fd;
		syscallcarg(linux32_dirent64p_t) dent;
		syscallcarg(unsigned int) count;
	} */ *uap = v;
	struct linux_sys_getdents64_args ua;

	NETBSD32TO64_UAP(fd);
	NETBSD32TOP_UAP(dent, struct linux_dirent64);
	NETBSD32TO64_UAP(count);

	return linux_sys_getdents64(l, &ua, retval);
}

int
linux32_sys_socketpair(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_socketpair_args /* {
		syscallarg(int) domain;
		syscallarg(int) type;
		syscallarg(int) protocol;
		syscallarg(netbsd32_intp) rsv;
	} */ *uap = v;
	struct linux_sys_socketpair_args ua;

	NETBSD32TO64_UAP(domain);
	NETBSD32TO64_UAP(type);
	NETBSD32TO64_UAP(protocol);
	NETBSD32TOP_UAP(rsv, int)

	return linux_sys_socketpair(l, &ua, retval);
}

int
linux32_sys_sendto(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_sendto_args /* {
		syscallarg(int) s;
		syscallarg(netbsd32_voidp) msg; 
		syscallarg(int) len;
		syscallarg(int) flags;
		syscallarg(netbsd32_osockaddrp_t) to;
		syscallarg(int) tolen;
	} */ *uap = v;
	struct linux_sys_sendto_args ua;

	NETBSD32TO64_UAP(s);
	NETBSD32TOP_UAP(msg, void);
	NETBSD32TO64_UAP(len);
	NETBSD32TO64_UAP(flags);
	NETBSD32TOP_UAP(to, struct osockaddr);
	NETBSD32TO64_UAP(tolen);

	return linux_sys_sendto(l, &ua, retval);
}


int
linux32_sys_recvfrom(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_recvfrom_args /* {
		syscallarg(int) s;
		syscallarg(netbsd32_voidp) buf; 
		syscallarg(netbsd32_size_t) len;
		syscallarg(int) flags;
		syscallarg(netbsd32_osockaddrp_t) from;
		syscallarg(netbsd32_intp) fromlenaddr;
	} */ *uap = v;
	struct linux_sys_recvfrom_args ua;

	NETBSD32TO64_UAP(s);
	NETBSD32TOP_UAP(buf, void);
	NETBSD32TO64_UAP(len);
	NETBSD32TO64_UAP(flags);
	NETBSD32TOP_UAP(from, struct osockaddr);
	NETBSD32TOP_UAP(fromlenaddr, unsigned int);

	return linux_sys_recvfrom(l, &ua, retval);
}

int
linux32_sys_setsockopt(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_setsockopt_args /* {
		syscallarg(int) s;
		syscallarg(int) level;
		syscallarg(int) optname;
		syscallarg(netbsd32_voidp) optval;
		syscallarg(int) optlen;
	} */ *uap = v;
	struct linux_sys_setsockopt_args ua;

	NETBSD32TO64_UAP(s);
	NETBSD32TO64_UAP(level);
	NETBSD32TO64_UAP(optname);
	NETBSD32TOP_UAP(optval, void);
	NETBSD32TO64_UAP(optlen);

	return linux_sys_setsockopt(l, &ua, retval);
}


int
linux32_sys_getsockopt(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_getsockopt_args /* {
		syscallarg(int) s;
		syscallarg(int) level;
		syscallarg(int) optname;
		syscallarg(netbsd32_voidp) optval;
		syscallarg(netbsd32_intp) optlen;
	} */ *uap = v;
	struct linux_sys_getsockopt_args ua;

	NETBSD32TO64_UAP(s);
	NETBSD32TO64_UAP(level);
	NETBSD32TO64_UAP(optname);
	NETBSD32TOP_UAP(optval, void);
	NETBSD32TOP_UAP(optlen, int);

	return linux_sys_getsockopt(l, &ua, retval);
}

int
linux32_sys_socket(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_socket_args /* {
		syscallarg(int) domain;
		syscallarg(int) type;
		syscallarg(int) protocol;
	} */ *uap = v;
	struct linux_sys_socket_args ua;

	NETBSD32TO64_UAP(domain);
	NETBSD32TO64_UAP(type);
	NETBSD32TO64_UAP(protocol);

	return linux_sys_socket(l, &ua, retval);
}

int
linux32_sys_bind(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_bind_args /* {
		syscallarg(int) s;
		syscallarg(netbsd32_osockaddrp_t) name;
		syscallarg(int) namelen;
	} */ *uap = v;
	struct linux_sys_bind_args ua;

	NETBSD32TO64_UAP(s);
	NETBSD32TOP_UAP(name, struct osockaddr)
	NETBSD32TO64_UAP(namelen);

	return linux_sys_bind(l, &ua, retval);
}

int
linux32_sys_connect(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_connect_args /* {
		syscallarg(int) s;
		syscallarg(netbsd32_osockaddrp_t) name;
		syscallarg(int) namelen;
	} */ *uap = v;
	struct linux_sys_connect_args ua;

	NETBSD32TO64_UAP(s);
	NETBSD32TOP_UAP(name, struct osockaddr)
	NETBSD32TO64_UAP(namelen);

	printf("linux32_sys_connect: s = %d, name = %p, namelen = %d\n",
		SCARG(&ua, s), SCARG(&ua, name), SCARG(&ua, namelen));

	return linux_sys_connect(l, &ua, retval);
}

int
linux32_sys_accept(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_accept_args /* {
		syscallarg(int) s;
		syscallarg(netbsd32_osockaddrp_t) name;
		syscallarg(netbsd32_intp) anamelen;
	} */ *uap = v;
	struct linux_sys_accept_args ua;

	NETBSD32TO64_UAP(s);
	NETBSD32TOP_UAP(name, struct osockaddr)
	NETBSD32TOP_UAP(anamelen, int);

	return linux_sys_accept(l, &ua, retval);
}

int
linux32_sys_getpeername(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_getpeername_args /* {
		syscallarg(int) fdes;
		syscallarg(netbsd32_sockaddrp_t) asa;
		syscallarg(netbsd32_intp) alen;
	} */ *uap = v;
	struct linux_sys_getpeername_args ua;

	NETBSD32TO64_UAP(fdes);
	NETBSD32TOP_UAP(asa, struct sockaddr)
	NETBSD32TOP_UAP(alen, int);

	return linux_sys_getpeername(l, &ua, retval);
}

int
linux32_sys_getsockname(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_getsockname_args /* {
		syscallarg(int) fdec;
		syscallarg(netbsd32_charp) asa;
		syscallarg(netbsd32_intp) alen;
	} */ *uap = v;
	struct linux_sys_getsockname_args ua;

	NETBSD32TO64_UAP(fdec);
	NETBSD32TOP_UAP(asa, char)
	NETBSD32TOP_UAP(alen, int);

	return linux_sys_getsockname(l, &ua, retval);
}

int
linux32_sys_sendmsg(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_sendmsg_args /* {
		syscallarg(int) s;
		syscallarg(netbsd32_msghdrp_t) msg;
		syscallarg(int) flags;
	} */ *uap = v;
	struct linux_sys_sendmsg_args ua;

	NETBSD32TO64_UAP(s);
	NETBSD32TOP_UAP(msg, struct msghdr);
	NETBSD32TO64_UAP(flags);

	return linux_sys_sendmsg(l, &ua, retval);
}

int
linux32_sys_recvmsg(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_recvmsg_args /* {
		syscallarg(int) s;
		syscallarg(netbsd32_msghdrp_t) msg;
		syscallarg(int) flags;
	} */ *uap = v;
	struct linux_sys_recvmsg_args ua;

	NETBSD32TO64_UAP(s);
	NETBSD32TOP_UAP(msg, struct msghdr);
	NETBSD32TO64_UAP(flags);

	return linux_sys_recvmsg(l, &ua, retval);
}

int
linux32_sys_send(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_send_args /* {
		syscallarg(int) s;
		syscallarg(netbsd32_voidp) buf;
		syscallarg(int) len;
		syscallarg(int) flags;
	} */ *uap = v;
	struct sys_sendto_args ua;

	NETBSD32TO64_UAP(s);
	NETBSD32TOP_UAP(buf, void);
	NETBSD32TO64_UAP(len);
	NETBSD32TO64_UAP(flags);
	SCARG(&ua, to) = NULL;
	SCARG(&ua, tolen) = 0;

	return sys_sendto(l, &ua, retval);
}

int
linux32_sys_recv(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_recv_args /* {
		syscallarg(int) s;
		syscallarg(netbsd32_voidp) buf;
		syscallarg(int) len;
		syscallarg(int) flags;
	} */ *uap = v;
	struct sys_recvfrom_args ua;

	NETBSD32TO64_UAP(s);
	NETBSD32TOP_UAP(buf, void);
	NETBSD32TO64_UAP(len);
	NETBSD32TO64_UAP(flags);
	SCARG(&ua, from) = NULL;
	SCARG(&ua, fromlenaddr) = NULL;

	return sys_recvfrom(l, &ua, retval);
}

int
linux32_sys_readlink(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_readlink_args /* {
		syscallarg(const netbsd32_charp) name;
		syscallarg(netbsd32_charp) buf;
		syscallarg(int) count;
	} */ *uap = v;
	struct linux_sys_readlink_args ua;

	NETBSD32TOP_UAP(name, const char);
	NETBSD32TOP_UAP(buf, char)
	NETBSD32TO64_UAP(count);

	return linux_sys_readlink(l, &ua, retval);
}


int
linux32_sys_select(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_select_args /* {
		syscallarg(int) nfds;
		syscallarg(netbsd32_fd_setp_t) readfds;
		syscallarg(netbsd32_fd_setp_t) writefds;
		syscallarg(netbsd32_fd_setp_t) exceptfds;
		syscallarg(netbsd32_timevalp_t) timeout;
	} */ *uap = v;

	return linux32_select1(l, retval, SCARG(uap, nfds), 
	    NETBSD32PTR64(SCARG(uap, readfds)),
	    NETBSD32PTR64(SCARG(uap, writefds)), 
	    NETBSD32PTR64(SCARG(uap, exceptfds)), 
	    NETBSD32PTR64(SCARG(uap, timeout)));
}

int
linux32_sys_oldselect(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_oldselect_args /* {
		syscallarg(linux32_oldselectp_t) lsp;
	} */ *uap = v;
	struct linux32_oldselect lsp32;
	int error;

	if ((error = copyin(NETBSD32PTR64(SCARG(uap, lsp)), 
	    &lsp32, sizeof(lsp32))) != 0)
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
	struct timeval tv0, tv1, utv, otv;
	struct netbsd32_timeval utv32;
	int error;

	/*
	 * Store current time for computation of the amount of
	 * time left.
	 */
	if (timeout) {
		if ((error = copyin(timeout, &utv32, sizeof(utv32))))
			return error;

		netbsd32_to_timeval(&utv32, &utv);
		otv = utv;

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
	} else {
		timerclear(&utv);
	}

	error = selcommon(l, retval, nfds, 
	    readfds, writefds, exceptfds, &utv, NULL);

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
			timersub(&otv, &tv1, &utv);
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
linux32_sys_pipe(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_pipe_args /* {
		syscallarg(netbsd32_intp) fd;
	} */ *uap = v;
	int error;
	int pfds[2];

	if ((error = sys_pipe(l, 0, retval)))
		return error;

	pfds[0] = (int)retval[0];
	pfds[1] = (int)retval[1];

	if ((error = copyout(pfds, NETBSD32PTR64(SCARG(uap, fd)), 
	    2 * sizeof (int))) != 0)
		return error;

	retval[0] = 0;
	retval[1] = 0;

	return 0;
}


int
linux32_sys_times(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_times_args /* {
		syscallarg(linux32_tmsp_t) tms;
	} */ *uap = v;
	struct linux32_tms ltms32;
	struct linux_tms ltms;
	struct linux_tms *ltmsp;
	struct linux_sys_times_args ua;
	caddr_t sg = stackgap_init(l->l_proc, 0);
	int error;

	ltmsp = stackgap_alloc(l->l_proc, &sg, sizeof(*ltmsp));
	SCARG(&ua, tms) = (struct times *)ltmsp;

	if ((error = linux_sys_times(l, &ua, retval)) != 0)
		return error;

	if ((error = copyin(ltmsp, &ltms, sizeof(ltms))) != 0)
		return error;

	ltms32.ltms32_utime = (linux32_clock_t)ltms.ltms_utime;
	ltms32.ltms32_stime = (linux32_clock_t)ltms.ltms_stime;
	ltms32.ltms32_cutime = (linux32_clock_t)ltms.ltms_cutime;
	ltms32.ltms32_cstime = (linux32_clock_t)ltms.ltms_cstime;

	if ((error = copyout(&ltms32, 
	    NETBSD32PTR64(SCARG(uap, tms)), sizeof(ltms32))) != 0)
		return error;

	return 0;
}

int
linux32_sys_clone(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_clone_args /* {
		syscallarg(int) flags;
		syscallarg(netbsd32_voidp) stack;
	} */ *uap = v;
	struct linux_sys_clone_args ua;
	
	NETBSD32TO64_UAP(flags);
	NETBSD32TOP_UAP(stack, void *);
#ifdef LINUX_NPTL
	SCARG(&ua, parent_tidptr) = NULL;
	SCARG(&ua, child_tidptr) = NULL;
#endif

	return linux_sys_clone(l, &ua, retval);
}

int
linux32_sys_sched_setscheduler(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_sched_setscheduler_args /* {
		syscallarg(int) pid;
		syscallarg(int) policy;
		syscallarg(const linux32_sched_paramp_t) sp;
	} */ *uap = v;
	struct linux_sys_sched_setscheduler_args ua;

	NETBSD32TO64_UAP(pid);
	NETBSD32TO64_UAP(policy);
	NETBSD32TOP_UAP(sp, const struct linux_sched_param);

	return linux_sys_sched_setscheduler(l, &ua, retval);
}


int
linux32_sys_waitpid(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_waitpid_args /* {
		syscallarg(int) pid;
		syscallarg(netbsd32_intp) status;
		syscallarg(int) options;
	} */ *uap = v;
	struct linux_sys_wait4_args ua;

	SCARG(&ua, pid) = SCARG(uap, pid);
	SCARG(&ua, status) = NETBSD32PTR64(SCARG(uap, status));
	SCARG(&ua, options) = SCARG(uap, options);
	SCARG(&ua, rusage) = NULL;

	return linux_sys_wait4(l, &ua, retval);	
}

int
linux32_sys_wait4(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_wait4_args /* {
		syscallarg(int) pid;
		syscallarg(netbsd32_intp) status;
		syscallarg(int) options;
		syscallarg(netbsd32_rusagep_t) rusage;
	} */ *uap = v;
	struct linux_sys_wait4_args ua;

	NETBSD32TO64_UAP(pid);
	NETBSD32TOP_UAP(status, int);
	NETBSD32TO64_UAP(options);
	NETBSD32TOP_UAP(rusage, struct rusage);

	return linux_sys_wait4(l, &ua, retval);	
}

int
linux32_sys_unlink(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_unlink_args /* {
		syscallarg(const netbsd32_charp) path;
	} */ *uap = v;
	struct linux_sys_unlink_args ua;

	NETBSD32TOP_UAP(path, const char);
	
	return linux_sys_unlink(l, &ua, retval);
}

int
linux32_sys_chdir(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_chdir_args /* {
		syscallarg(const netbsd32_charp) path;
	} */ *uap = v;
	struct sys_chdir_args ua;
	caddr_t sg = stackgap_init(l->l_proc, 0);

	NETBSD32TOP_UAP(path, const char);

	CHECK_ALT_EXIST(l, &sg, SCARG(&ua, path));
	
	return sys_chdir(l, &ua, retval);
}

int
linux32_sys_link(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_link_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(const netbsd32_charp) link;
	} */ *uap = v;
	struct sys_link_args ua;
	caddr_t sg = stackgap_init(l->l_proc, 0);

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TOP_UAP(link, const char);

	CHECK_ALT_EXIST(l, &sg, SCARG(&ua, path));
	CHECK_ALT_CREAT(l, &sg, SCARG(&ua, link));
	
	return sys_link(l, &ua, retval);
}

int
linux32_sys_creat(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_creat_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(int) mode;
	} */ *uap = v;
	struct sys_open_args ua;
	caddr_t sg = stackgap_init(l->l_proc, 0);

	NETBSD32TOP_UAP(path, const char);
	SCARG(&ua, flags) = O_CREAT | O_TRUNC | O_WRONLY;
	NETBSD32TO64_UAP(mode);

	CHECK_ALT_EXIST(l, &sg, SCARG(&ua, path));

	return sys_open(l, &ua, retval);
}

int
linux32_sys_mknod(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_mknod_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(int) mode;
		syscallarg(int) dev;
	} */ *uap = v;
	struct linux_sys_mknod_args ua;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(mode);
	NETBSD32TO64_UAP(dev);

	return linux_sys_mknod(l, &ua, retval);
}

int
linux32_sys_chmod(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_chmod_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(int) mode;
	} */ *uap = v;
	struct sys_chmod_args ua;
	caddr_t sg = stackgap_init(l->l_proc, 0);

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(mode);

	CHECK_ALT_EXIST(l, &sg, SCARG(&ua, path));

	return sys_chmod(l, &ua, retval);
}

int
linux32_sys_lchown16(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_lchown16_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(int) uid;
		syscallarg(int) gid;
	} */ *uap = v;
        struct sys___posix_lchown_args ua;
        caddr_t sg = stackgap_init(l->l_proc, 0);

	NETBSD32TOP_UAP(path, const char);
        CHECK_ALT_SYMLINK(l, &sg, SCARG(&ua, path));

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
linux32_sys_break(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
#if 0
	struct linux32_sys_break_args /* {
		syscallarg(const netbsd32_charp) nsize;
	} */ *uap = v;
#endif

	return ENOSYS;
}

int
linux32_sys_stime(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_stime_args /* {
		syscallarg(linux32_timep_t) t;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct timespec ts;
	linux32_time_t tt32;
	int error;
	
	if ((error = kauth_authorize_generic(p->p_cred, KAUTH_GENERIC_ISSUSER, &p->p_acflag)) != 0)
		return error;

	if ((error = copyin(&tt32, 
	    NETBSD32PTR64(SCARG(uap, t)), 
	    sizeof tt32)) != 0)

	ts.tv_sec = (long)tt32;
	ts.tv_nsec = 0;

	return settime(p, &ts);
}

int
linux32_sys_utime(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_utime_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(linux32_utimbufp_t) times;
	} */ *uap = v;
	struct proc *p = l->l_proc;
        caddr_t sg = stackgap_init(p, 0);
        struct sys_utimes_args ua;
        struct timeval tv[2], *tvp;
        struct linux32_utimbuf lut;
        int error;

	NETBSD32TOP_UAP(path, const char);
        CHECK_ALT_EXIST(l, &sg, SCARG(&ua, path));


        if (NETBSD32PTR64(SCARG(uap, times)) != NULL) {
                if ((error = copyin(NETBSD32PTR64(SCARG(uap, times)), 
		    &lut, sizeof lut)))
                        return error;

                tv[0].tv_sec = (long)lut.l_actime;
                tv[0].tv_usec = 0;
                tv[1].tv_sec = (long)lut.l_modtime;
		tv[1].tv_usec = 0;

	        tvp = (struct timeval *) stackgap_alloc(p, &sg, sizeof(tv));

                if ((error = copyout(tv, tvp, sizeof(tv))))
                        return error;
                SCARG(&ua, tptr) = tvp;
        } else {
               SCARG(&ua, tptr) = NULL;
	}
                     
        return sys_utimes(l, &ua, retval);
} 

int
linux32_sys_rename(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_rename_args /* {
		syscallarg(const netbsd32_charp) from;
		syscallarg(const netbsd32_charp) to;
	} */ *uap = v;
	struct sys_rename_args ua;
	caddr_t sg = stackgap_init(l->l_proc, 0);

	NETBSD32TOP_UAP(from, const char);
	NETBSD32TOP_UAP(to, const char);

	CHECK_ALT_EXIST(l, &sg, SCARG(&ua, from));
	CHECK_ALT_CREAT(l, &sg, SCARG(&ua, to));
	
	return sys___posix_rename(l, &ua, retval);
}

int
linux32_sys_mkdir(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_mkdir_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(int) mode;
	} */ *uap = v;
	struct sys_mkdir_args ua;
	caddr_t sg = stackgap_init(l->l_proc, 0);

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(mode);

	CHECK_ALT_CREAT(l, &sg, SCARG(&ua, path));
	
	return sys_mkdir(l, &ua, retval);
}

int
linux32_sys_rmdir(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_rmdir_args /* {
		syscallarg(const netbsd32_charp) path;
	} */ *uap = v;
	struct sys_rmdir_args ua;
	caddr_t sg = stackgap_init(l->l_proc, 0);

	NETBSD32TOP_UAP(path, const char);

	CHECK_ALT_EXIST(l, &sg, SCARG(&ua, path));
	
	return sys_rmdir(l, &ua, retval);
}

int
linux32_sys_signal(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_signal_args /* {
		syscallarg(int) signum;
		syscallarg(linux32_handler_t) handler;
	} */ *uap = v;
        struct proc *p = l->l_proc;
        struct sigaction nbsa, obsa;
        int error, sig;

        *retval = -1;

        sig = SCARG(uap, signum);
        if (sig < 0 || sig >= LINUX32__NSIG)
                return EINVAL;

        nbsa.sa_handler = NETBSD32PTR64(SCARG(uap, handler));
        sigemptyset(&nbsa.sa_mask);
        nbsa.sa_flags = SA_RESETHAND | SA_NODEFER;

        if ((error = sigaction1(p, linux32_to_native_signo[sig],
            &nbsa, &obsa, NULL, 0)) != 0)
		return error;

        *retval = (int)(long)obsa.sa_handler;
        return 0;
}

int   
linux32_sys_oldolduname(l, v, retval)
        struct lwp *l;
        void *v;
        register_t *retval;
{
        struct linux32_sys_uname_args /* {
                syscallarg(linux32_oldoldutsnamep_t) up;
        } */ *uap = v; 
        struct linux_oldoldutsname luts;
 
        strncpy(luts.l_sysname, linux32_sysname, sizeof(luts.l_sysname));
        strncpy(luts.l_nodename, hostname, sizeof(luts.l_nodename));
        strncpy(luts.l_release, linux32_release, sizeof(luts.l_release));
        strncpy(luts.l_version, linux32_version, sizeof(luts.l_version));
        strncpy(luts.l_machine, machine, sizeof(luts.l_machine));
 
        return copyout(&luts, NETBSD32PTR64(SCARG(uap, up)), sizeof(luts));
}

int
linux32_sys_getgroups16(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_getgroups16_args /* {
		syscallarg(int) gidsetsize;
		syscallarg(linux32_gidp_t) gidset;
	} */ *uap = v;
	struct linux_sys_getgroups16_args ua;

	NETBSD32TO64_UAP(gidsetsize);
	NETBSD32TOP_UAP(gidset, linux32_gid_t);
	
	return linux_sys_getgroups16(l, &ua, retval);
}

int
linux32_sys_setgroups16(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_setgroups16_args /* {
		syscallarg(int) gidsetsize;
		syscallarg(linux32_gidp_t) gidset;
	} */ *uap = v;
	struct linux_sys_setgroups16_args ua;

	NETBSD32TO64_UAP(gidsetsize);
	NETBSD32TOP_UAP(gidset, linux32_gid_t);
	
	return linux_sys_setgroups16(l, &ua, retval);
}

int
linux32_sys_symlink(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_symlink_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(const netbsd32_charp) link;
	} */ *uap = v;
	struct sys_symlink_args ua;
	caddr_t sg = stackgap_init(l->l_proc, 0);

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TOP_UAP(link, const char);

	CHECK_ALT_EXIST(l, &sg, SCARG(&ua, path));
	CHECK_ALT_CREAT(l, &sg, SCARG(&ua, link));
	
	return sys_symlink(l, &ua, retval);
}


int
linux32_sys_swapon(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_swapon_args /* {
		syscallarg(const netbsd32_charp) name;
	} */ *uap = v;
	struct sys_swapctl_args ua;

        SCARG(&ua, cmd) = SWAP_ON;
        SCARG(&ua, arg) = (void *)__UNCONST(NETBSD32PTR64(SCARG(uap, name)));
        SCARG(&ua, misc) = 0;   /* priority */
        return (sys_swapctl(l, &ua, retval));
}

int
linux32_sys_swapoff(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_swapoff_args /* {
		syscallarg(const netbsd32_charp) path;
	} */ *uap = v;
	struct sys_swapctl_args ua;

        SCARG(&ua, cmd) = SWAP_OFF;
        SCARG(&ua, arg) = (void *)__UNCONST(NETBSD32PTR64(SCARG(uap, path)));
        SCARG(&ua, misc) = 0;   /* priority */
        return (sys_swapctl(l, &ua, retval));
}


int
linux32_sys_reboot(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_reboot_args /* {
		syscallarg(int) magic1;
		syscallarg(int) magic2;
		syscallarg(int) cmd;
		syscallarg(netbsd32_voidp) arg;
	} */ *uap = v;
	struct linux_sys_reboot_args ua;

	NETBSD32TO64_UAP(magic1);
	NETBSD32TO64_UAP(magic2);
	NETBSD32TO64_UAP(cmd);
	NETBSD32TOP_UAP(arg, void);
	
	return linux_sys_reboot(l, &ua, retval);
}

int
linux32_sys_truncate(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_truncate_args /* {
		syscallarg(const netbsd32_charp) path;
		syscallarg(netbsd32_charp) buf;
		syscallarg(int) count;
	} */ *uap = v;
	struct compat_43_sys_truncate_args ua;

	NETBSD32TOP_UAP(path, const char);
	NETBSD32TO64_UAP(length);

	return compat_43_sys_truncate(l, &ua, retval);
}

int
linux32_sys_fchown16(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_fchown16_args /* {
		syscallarg(int) fd;
		syscallarg(int) uid;
		syscallarg(int) gid;
	} */ *uap = v;
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
