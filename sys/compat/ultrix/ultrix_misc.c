/*	$NetBSD: ultrix_misc.c,v 1.123 2018/01/09 20:55:43 maya Exp $	*/

/*
 * Copyright (c) 1995, 1997 Jonathan Stone (hereinafter referred to as the author)
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
 *      This product includes software developed by Jonathan Stone for
 *      the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
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
 *
 *	@(#)sun_misc.c	8.1 (Berkeley) 6/18/93
 *
 * from: Header: sun_misc.c,v 1.16 93/04/07 02:46:27 torek Exp
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ultrix_misc.c,v 1.123 2018/01/09 20:55:43 maya Exp $");

#if defined(_KERNEL_OPT)
#include "opt_sysv.h"
#endif

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

/*
 * Ultrix compatibility module.
 *
 * Ultrix system calls that are implemented differently in BSD are
 * handled here.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/dirent.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/kernel.h>
#include <sys/exec.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/mman.h>
#include <sys/mount.h>
#include <sys/resource.h>
#include <sys/resourcevar.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/socket.h>
#include <sys/vnode.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <sys/utsname.h>
#include <sys/unistd.h>
#include <sys/ipc.h>

#include <sys/syscallargs.h>

#include <uvm/uvm_extern.h>

#include <compat/ultrix/ultrix_syscall.h>
#include <compat/ultrix/ultrix_syscallargs.h>
#include <compat/common/compat_util.h>
#include <compat/sys/time.h>

#include <netinet/in.h>

#include <miscfs/specfs/specdev.h>

#include <nfs/rpcv2.h>
#include <nfs/nfsproto.h>
#include <nfs/nfs.h>

#include <sys/socketvar.h>				/* sosetopt() */

#include <compat/ultrix/ultrix_flock.h>

#include <compat/sys/signal.h>
#include <compat/sys/signalvar.h>

#ifdef __mips
#include <mips/cachectl.h>
#include <mips/frame.h>
#endif

static int ultrix_to_bsd_flock(struct ultrix_flock *, struct flock *);
static void bsd_to_ultrix_flock(struct flock *, struct ultrix_flock *);

extern struct sysent ultrix_sysent[];
extern const char * const ultrix_syscallnames[];
extern char ultrix_sigcode[], ultrix_esigcode[];

struct uvm_object *emul_ultrix_object;

#ifndef __HAVE_SYSCALL_INTERN
void	syscall(void);
#endif

struct emul emul_ultrix = {
	.e_name =		"ultrix",
	.e_path =		"/emul/ultrix",
#ifndef __HAVE_MINIMAL_EMUL
	.e_flags =		0,
	.e_errno =		NULL,
	.e_nosys =		ULTRIX_SYS_syscall,
	.e_nsysent =		ULTRIX_SYS_NSYSENT,
#endif
	.e_sysent =		ultrix_sysent,
#ifdef SYSCALL_DEBUG
	.e_syscallnames =	ultrix_syscallnames,
#else
	.e_syscallnames =	NULL,
#endif
#ifdef __mips
	.e_sendsig =		sendsig_sigcontext,
#else /* vax */
	.e_sendsig =		sendsig,
#endif
	.e_trapsignal =		trapsignal,
	.e_tracesig =		NULL,
	.e_sigcode =		ultrix_sigcode,
	.e_esigcode =		ultrix_esigcode,
	.e_sigobject =		&emul_ultrix_object,
	.e_setregs =		setregs,
	.e_proc_exec =		NULL,
	.e_proc_fork =		NULL,
	.e_proc_exit =		NULL,
	.e_lwp_fork =		NULL,
	.e_lwp_exit =		NULL,
#ifdef __HAVE_SYSCALL_INTERN
	.e_syscall_intern =	syscall_intern,
#else
	.e_syscall_intern =	syscall,
#endif
	.e_sysctlovly =		NULL,
	.e_vm_default_addr =	uvm_default_mapaddr,
	.e_usertrap =		NULL,
	.e_ucsize =		0,
	.e_startlwp =		NULL
};

#define GSI_PROG_ENV 1

int
ultrix_sys_getsysinfo(struct lwp *l, const struct ultrix_sys_getsysinfo_args *uap, register_t *retval)
{
	static short progenv = 0;

	switch (SCARG(uap, op)) {
		/* operations implemented: */
	case GSI_PROG_ENV:
		if (SCARG(uap, nbytes) < sizeof(short))
			return EINVAL;
		*retval = 1;
		return copyout(&progenv, SCARG(uap, buffer), sizeof(progenv));
	default:
		*retval = 0; /* info unavail */
		return 0;
	}
}

int
ultrix_sys_setsysinfo(struct lwp *l, const struct ultrix_sys_setsysinfo_args *uap, register_t *retval)
{

	*retval = 0;
	return 0;
}

int
ultrix_sys_waitpid(struct lwp *l, const struct ultrix_sys_waitpid_args *uap, register_t *retval)
{
	struct compat_50_sys_wait4_args ap;

	SCARG(&ap, pid) = SCARG(uap, pid);
	SCARG(&ap, status) = SCARG(uap, status);
	SCARG(&ap, options) = SCARG(uap, options);
	SCARG(&ap, rusage) = 0;

	return compat_50_sys_wait4(l, &ap, retval);
}

int
ultrix_sys_wait3(struct lwp *l, const struct ultrix_sys_wait3_args *uap, register_t *retval)
{
	struct compat_50_sys_wait4_args ap;

	SCARG(&ap, pid) = -1;
	SCARG(&ap, status) = SCARG(uap, status);
	SCARG(&ap, options) = SCARG(uap, options);
	SCARG(&ap, rusage) = SCARG(uap, rusage);

	return compat_50_sys_wait4(l, &ap, retval);
}

/*
 * Ultrix binaries pass in FD_MAX as the first arg to select().
 * On Ultrix, FD_MAX is 4096, which is more than the NetBSD sys_select()
 * can handle.
 * Since we can't have more than the (native) FD_MAX descriptors open,
 * limit nfds to at most FD_MAX.
 */
int
ultrix_sys_select(struct lwp *l, const struct ultrix_sys_select_args *uap, register_t *retval)
{
	struct timeval50 atv;
	int error;
	struct compat_50_sys_select_args ap;

	/* Limit number of FDs selected on to the native maximum */

	if (SCARG(uap, nd) > FD_SETSIZE)
		SCARG(&ap, nd) = FD_SETSIZE;
	else
		SCARG(&ap, nd) = SCARG(uap, nd);

	SCARG(&ap, in) = SCARG(uap, in);
	SCARG(&ap, ou) = SCARG(uap, ou);
	SCARG(&ap, ex) = SCARG(uap, ex);
	SCARG(&ap, tv) = SCARG(uap, tv);
	/* Check for negative timeval */
	if (SCARG(&ap, tv)) {
		error = copyin(SCARG(uap, tv), &atv, sizeof(atv));
		if (error)
			goto done;
#ifdef DEBUG
		/* Ultrix clients sometimes give negative timeouts? */
		if (atv.tv_sec < 0 || atv.tv_usec < 0)
			printf("ultrix select( %ld, %ld): negative timeout\n",
			    atv.tv_sec, atv.tv_usec);
#endif

	}
	error = compat_50_sys_select(l, &ap, retval);
	if (error == EINVAL)
		printf("ultrix select: bad args?\n");

done:
	return error;
}


#define	SUN__MAP_NEW	0x80000000	/* if not, old mmap & cannot handle */

int
ultrix_sys_mmap(struct lwp *l, const struct ultrix_sys_mmap_args *uap, register_t *retval)
{
	struct sys_mmap_args ouap;

	/*
	 * Verify the arguments.
	 */
	if (SCARG(uap, prot) & ~(PROT_READ|PROT_WRITE|PROT_EXEC))
		return EINVAL;			/* XXX still needed? */

	if ((SCARG(uap, flags) & SUN__MAP_NEW) == 0)
		return EINVAL;

	SCARG(&ouap, flags) = SCARG(uap, flags) & ~SUN__MAP_NEW;
	SCARG(&ouap, addr) = SCARG(uap, addr);
	SCARG(&ouap, len) = SCARG(uap, len);
	SCARG(&ouap, prot) = SCARG(uap, prot);
	SCARG(&ouap, fd) = SCARG(uap, fd);
	SCARG(&ouap, pos) = SCARG(uap, pos);

	return sys_mmap(l, &ouap, retval);
}

int
ultrix_sys_setsockopt(struct lwp *l, const struct ultrix_sys_setsockopt_args *uap, register_t *retval)
{
	struct sockopt sopt;
	struct socket *so;
	int error;
	struct sys_setsockopt_args ap;

	SCARG(&ap, s) = SCARG(uap, s);
	SCARG(&ap, level) = SCARG(uap, level);
	SCARG(&ap, name) = SCARG(uap, name);
	SCARG(&ap, val) = SCARG(uap, val);
	SCARG(&ap, valsize) = SCARG(uap, valsize);

	/* fd_getsock() will use the descriptor for us */
	if ((error = fd_getsock(SCARG(&ap, s), &so))  != 0)
		return error;
#define	SO_DONTLINGER (~SO_LINGER)
	if (SCARG(&ap, name) == SO_DONTLINGER) {
		struct linger lg;

		lg.l_onoff = 0;
		error = so_setsockopt(l, so, SCARG(&ap, level), SO_LINGER,
		    &lg, sizeof(lg));
		goto out;
	}
	if (SCARG(&ap, level) == IPPROTO_IP) {
#define		EMUL_IP_MULTICAST_IF		2
#define		EMUL_IP_MULTICAST_TTL		3
#define		EMUL_IP_MULTICAST_LOOP		4
#define		EMUL_IP_ADD_MEMBERSHIP		5
#define		EMUL_IP_DROP_MEMBERSHIP	6
		static const int ipoptxlat[] = {
			IP_MULTICAST_IF,
			IP_MULTICAST_TTL,
			IP_MULTICAST_LOOP,
			IP_ADD_MEMBERSHIP,
			IP_DROP_MEMBERSHIP
		};
		if (SCARG(&ap, name) >= EMUL_IP_MULTICAST_IF &&
		    SCARG(&ap, name) <= EMUL_IP_DROP_MEMBERSHIP) {
			SCARG(&ap, name) =
			    ipoptxlat[SCARG(&ap, name) - EMUL_IP_MULTICAST_IF];
		}
	}
	if (SCARG(&ap, valsize) > MLEN) {
		error = EINVAL;
		goto out;
	}
	sockopt_init(&sopt, SCARG(&ap, level), SCARG(&ap, name),
	    SCARG(&ap, valsize));
	if (SCARG(&ap, val)) {
		error = copyin(SCARG(&ap, val), sopt.sopt_data,
		    (u_int)SCARG(&ap, valsize));
	}
	if (error == 0)
		error = sosetopt(so, &sopt);
	sockopt_destroy(&sopt);
 out:
 	fd_putfile(SCARG(uap, s));
	return error;
}

#define	ULTRIX__SYS_NMLN	32

struct ultrix_utsname {
	char    sysname[ULTRIX__SYS_NMLN];
	char    nodename[ULTRIX__SYS_NMLN];
	char    release[ULTRIX__SYS_NMLN];
	char    version[ULTRIX__SYS_NMLN];
	char    machine[ULTRIX__SYS_NMLN];
};

int
ultrix_sys_uname(struct lwp *l, const struct ultrix_sys_uname_args *uap, register_t *retval)
{
	struct ultrix_utsname sut;
	const char *cp;
	char *dp, *ep;

	memset(&sut, 0, sizeof(sut));

	strncpy(sut.sysname, ostype, sizeof(sut.sysname) - 1);
	strncpy(sut.nodename, hostname, sizeof(sut.nodename) - 1);
	strncpy(sut.release, osrelease, sizeof(sut.release) - 1);
	dp = sut.version;
	ep = &sut.version[sizeof(sut.version) - 1];
	for (cp = version; *cp && *cp != '('; cp++)
		;
	for (cp++; *cp && *cp != ')' && dp < ep; cp++)
		*dp++ = *cp;
	for (; *cp && *cp != '#'; cp++)
		;
	for (; *cp && *cp != ':' && dp < ep; cp++)
		*dp++ = *cp;
	*dp = '\0';
	strncpy(sut.machine, machine, sizeof(sut.machine) - 1);

	return copyout(&sut, SCARG(uap, name), sizeof(sut));
}

int
ultrix_sys_setpgrp(struct lwp *l, const struct ultrix_sys_setpgrp_args *uap, register_t *retval)
{
	struct proc *p = l->l_proc;
	struct sys_setpgid_args ap;

	SCARG(&ap, pid) = SCARG(uap, pid);
	SCARG(&ap, pgid) = SCARG(uap, pgid);
	/*
	 * difference to our setpgid call is to include backwards
	 * compatibility to pre-setsid() binaries. Do setsid()
	 * instead of setpgid() in those cases where the process
	 * tries to create a new session the old way.
	 */
	if (!SCARG(&ap, pgid) &&
	    (!SCARG(&ap, pid) || SCARG(&ap, pid) == p->p_pid))
		return sys_setsid(l, &ap, retval);
	else
		return sys_setpgid(l, &ap, retval);
}

struct ultrix_ustat {
	daddr_t	f_tfree;	/* total free */
	uint32_t f_tinode;	/* total inodes free */
	char	f_fname[6];	/* filsys name */
	char	f_fpack[6];	/* filsys pack name */
};

int
ultrix_sys_ustat(struct lwp *l, const struct ultrix_sys_ustat_args *uap, register_t *retval)
{
	struct ultrix_ustat us;
	int error;

	memset(&us, 0, sizeof us);

	/*
	 * XXX: should set f_tfree and f_tinode at least
	 * How do we translate dev -> fstat? (and then to ultrix_ustat)
	 */

	if ((error = copyout(&us, SCARG(uap, buf), sizeof us)) != 0)
		return error;
	return 0;
}

int
ultrix_sys_quotactl(struct lwp *l, const struct ultrix_sys_quotactl_args *uap, register_t *retval)
{

	return EINVAL;
}

int
ultrix_sys_vhangup(struct lwp *l, const void *uap, register_t *retval)
{

	return 0;
}


/*
 * RISC Ultrix cache control syscalls
 */
#ifdef __mips
int
ultrix_sys_cacheflush(struct lwp *l, const struct ultrix_sys_cacheflush_args *uap, register_t *retval)
{
	/* {
		syscallarg(void *) addr;
		syscallarg(unsigned) nbytes;
		syscallarg(int) flag;
	} */
	struct proc *p = l->l_proc;
	vaddr_t va  = (vaddr_t)SCARG(uap, addr);
	int nbytes     = SCARG(uap, nbytes);
	int whichcache = SCARG(uap, whichcache);

	return mips_user_cacheflush(p, va, nbytes, whichcache);
}


int
ultrix_sys_cachectl(struct lwp *l, const struct ultrix_sys_cachectl_args *uap, register_t *retval)
{
	/* {
		syscallarg(void *) addr;
		syscallarg(int) nbytes;
		syscallarg(int) cacheop;
	} */
	struct proc *p = l->l_proc;
	vaddr_t va  = (vaddr_t)SCARG(uap, addr);
	int nbytes  = SCARG(uap, nbytes);
	int cacheop = SCARG(uap, cacheop);

	return mips_user_cachectl(p, va, nbytes, cacheop);
}

#endif	/* __mips */


int
ultrix_sys_exportfs(struct lwp *l, const struct ultrix_sys_exportfs_args *uap, register_t *retval)
{

	/*
	 * XXX: should perhaps translate into a mount(2)
	 * with MOUNT_EXPORT?
	 */
	return 0;
}

int
ultrix_sys_sigpending(struct lwp *l, const struct ultrix_sys_sigpending_args *uap, register_t *retval)
{
	sigset_t ss;
	int mask;

	sigpending1(l, &ss);
	mask = ss.__bits[0];

	return copyout(&mask, SCARG(uap, mask), sizeof(int));
}

int
ultrix_sys_sigreturn(struct lwp *l, const struct ultrix_sys_sigreturn_args *uap, register_t *retval)
{
	/* struct sigcontext13 is close enough to Ultrix */
	struct compat_13_sys_sigreturn_args ap;

	SCARG(&ap, sigcntxp) = (void *)SCARG(uap, sigcntxp);

	return compat_13_sys_sigreturn(l, &ap, retval);
}

int
ultrix_sys_sigcleanup(struct lwp *l, const struct ultrix_sys_sigcleanup_args *uap, register_t *retval)
{

	/* struct sigcontext13 is close enough to Ultrix */
	struct compat_13_sys_sigreturn_args ap;

	SCARG(&ap, sigcntxp) = (void *)SCARG(uap, sigcntxp);

	return compat_13_sys_sigreturn(l, &ap, retval);
}

int
ultrix_sys_sigsuspend(struct lwp *l, const struct ultrix_sys_sigsuspend_args *uap, register_t *retval)
{
	int mask = SCARG(uap, mask);
	sigset_t ss;

	ss.__bits[0] = mask;
	ss.__bits[1] = 0;
	ss.__bits[2] = 0;
	ss.__bits[3] = 0;

	return sigsuspend1(l, &ss);
}

#define ULTRIX_SV_ONSTACK 0x0001  /* take signal on signal stack */
#define ULTRIX_SV_INTERRUPT 0x0002  /* do not restart system on signal return */
#define ULTRIX_SV_OLDSIG 0x1000  /* Emulate old signal() for POSIX */

int
ultrix_sys_sigvec(struct lwp *l, const struct ultrix_sys_sigvec_args *uap, register_t *retval)
{
	struct sigvec nsv, osv;
	struct sigaction nsa, osa;
	int error;

	if (SCARG(uap, nsv)) {
		error = copyin(SCARG(uap, nsv), &nsv, sizeof(nsv));
		if (error)
			return error;
		nsa.sa_handler = nsv.sv_handler;
#if 0 /* documentation */
		/* ONSTACK is identical */
		nsa.sa_flags = nsv.sv_flags & ULTRIX_SV_ONSTACK;
		if ((nsv.sv_flags & ULTRIX_SV_OLDSIG)
		    /* old signal() - always restart */
		    || (!(nsv.sv_flags & ULTRIX_SV_INTERRUPT))
		    /* inverted meaning (same bit) */
		    )
			nsa.sa_flags |= SA_RESTART;
#else /* optimized - assuming ULTRIX_SV_OLDSIG=>!ULTRIX_SV_INTERRUPT */
		nsa.sa_flags = nsv.sv_flags & ~ULTRIX_SV_OLDSIG;
		nsa.sa_flags ^= SA_RESTART;
#endif
		native_sigset13_to_sigset(&nsv.sv_mask, &nsa.sa_mask);
	}
	error = sigaction1(l, SCARG(uap, signum),
	    SCARG(uap, nsv) ? &nsa : 0, SCARG(uap, osv) ? &osa : 0,
	    NULL, 0);
	if (error)
		return error;
	if (SCARG(uap, osv)) {
		osv.sv_handler = osa.sa_handler;
		osv.sv_flags = osa.sa_flags ^ SA_RESTART;
		osv.sv_flags &= (ULTRIX_SV_ONSTACK | ULTRIX_SV_INTERRUPT);
		native_sigset_to_sigset13(&osa.sa_mask, &osv.sv_mask);
		error = copyout(&osv, SCARG(uap, osv), sizeof(osv));
		if (error)
			return error;
	}
	return 0;
}

int
ultrix_sys_shmsys(struct lwp *l, const struct ultrix_sys_shmsys_args *uap, register_t *retval)
{

#ifdef SYSVSHM
	/* Ultrix SVSHM weirndess: */
	struct sys_shmat_args shmat_args;
	struct compat_14_sys_shmctl_args shmctl_args;
	struct sys_shmdt_args shmdt_args;
	struct sys_shmget_args shmget_args;


	switch (SCARG(uap, shmop)) {
	case 0:						/* Ultrix shmat() */
		SCARG(&shmat_args, shmid) = SCARG(uap, a2);
		SCARG(&shmat_args, shmaddr) = (void *)SCARG(uap, a3);
		SCARG(&shmat_args, shmflg) = SCARG(uap, a4);
		return sys_shmat(l, &shmat_args, retval);

	case 1:						/* Ultrix shmctl() */
		SCARG(&shmctl_args, shmid) = SCARG(uap, a2);
		SCARG(&shmctl_args, cmd) = SCARG(uap, a3);
		SCARG(&shmctl_args, buf) = (struct shmid_ds14 *)SCARG(uap, a4);
		return compat_14_sys_shmctl(l, &shmctl_args, retval);

	case 2:						/* Ultrix shmdt() */
		SCARG(&shmat_args, shmaddr) = (void *)SCARG(uap, a2);
		return sys_shmdt(l, &shmdt_args, retval);

	case 3:						/* Ultrix shmget() */
		SCARG(&shmget_args, key) = SCARG(uap, a2);
		SCARG(&shmget_args, size) = SCARG(uap, a3);
		SCARG(&shmget_args, shmflg) = SCARG(uap, a4)
		    & (IPC_CREAT|IPC_EXCL|IPC_NOWAIT);
		return sys_shmget(l, &shmget_args, retval);

	default:
		return EINVAL;
	}
#else
	return EOPNOTSUPP;
#endif	/* SYSVSHM */
}

static int
ultrix_to_bsd_flock(struct ultrix_flock *ufl, struct flock *fl)
{

	fl->l_start = ufl->l_start;
	fl->l_len = ufl->l_len;
	fl->l_pid = ufl->l_pid;
	fl->l_whence = ufl->l_whence;

	switch (ufl->l_type) {
	case ULTRIX_F_RDLCK:
		fl->l_type = F_RDLCK;
		break;
	case ULTRIX_F_WRLCK:
		fl->l_type = F_WRLCK;
		break;
	case ULTRIX_F_UNLCK:
		fl->l_type = F_UNLCK;
		break;
	default:
		return EINVAL;
	}

	return 0;
}

static void
bsd_to_ultrix_flock(struct flock *fl, struct ultrix_flock *ufl)
{

	ufl->l_start = fl->l_start;
	ufl->l_len = fl->l_len;
	ufl->l_pid = fl->l_pid;
	ufl->l_whence = fl->l_whence;

	switch (fl->l_type) {
	case F_RDLCK:
		ufl->l_type = ULTRIX_F_RDLCK;
		break;
	case F_WRLCK:
		ufl->l_type = ULTRIX_F_WRLCK;
		break;
	case F_UNLCK:
		ufl->l_type = ULTRIX_F_UNLCK;
		break;
	}
}

int
ultrix_sys_fcntl(struct lwp *l, const struct ultrix_sys_fcntl_args *uap, register_t *retval)
{
	int error;
	struct ultrix_flock ufl;
	struct flock fl;

	switch (SCARG(uap, cmd)) {
	case F_GETLK:
	case F_SETLK:
	case F_SETLKW:
		error = copyin(SCARG(uap, arg), &ufl, sizeof(ufl));
		if (error)
			return error;
		error = ultrix_to_bsd_flock(&ufl, &fl);
		if (error)
			return error;
		error = do_fcntl_lock(SCARG(uap, fd), SCARG(uap, cmd), &fl);
		if (SCARG(uap, cmd) != F_GETLK || error != 0)
			return error;
		bsd_to_ultrix_flock(&fl, &ufl);
		return copyout(&ufl, SCARG(uap, arg), sizeof(ufl));

	default:
		break;
	}

	return sys_fcntl(l, (const void *)uap, retval);
}
