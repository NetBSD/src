/*	$NetBSD: ultrix_misc.c,v 1.82 2003/01/28 21:57:47 atatat Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
__KERNEL_RCSID(0, "$NetBSD: ultrix_misc.c,v 1.82 2003/01/28 21:57:47 atatat Exp $");

#if defined(_KERNEL_OPT)
#include "opt_nfsserver.h"
#include "opt_sysv.h"
#endif

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

/*
 * Ultrix compatibility module.
 *
 * Ultrix system calls that are implemented differently in BSD are
 * handled here.
 */

#if defined(_KERNEL_OPT)
#include "fs_nfs.h"
#endif

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

#include <sys/sa.h>
#include <sys/syscallargs.h>

#include <compat/ultrix/ultrix_syscall.h>
#include <compat/ultrix/ultrix_syscallargs.h>
#include <compat/common/compat_util.h>

#include <netinet/in.h>

#include <miscfs/specfs/specdev.h>

#include <nfs/rpcv2.h>
#include <nfs/nfsproto.h>
#include <nfs/nfs.h>

#include <sys/socketvar.h>				/* sosetopt() */

#include <compat/ultrix/ultrix_flock.h>

#ifdef __mips
#include <mips/cachectl.h>
#endif

static int ultrix_to_bsd_flock __P((struct ultrix_flock *, struct flock *));
static void bsd_to_ultrix_flock __P((struct flock *, struct ultrix_flock *));

extern struct sysent ultrix_sysent[];
extern const char * const ultrix_syscallnames[];
extern char ultrix_sigcode[], ultrix_esigcode[];
#ifdef __HAVE_SYSCALL_INTERN
void syscall_intern(struct proc *);
#else
void syscall __P((void));
#endif

const struct emul emul_ultrix = {
	"ultrix",
	"/emul/ultrix",
#ifndef __HAVE_MINIMAL_EMUL
	0,
	NULL,
	ULTRIX_SYS_syscall,
	ULTRIX_SYS_NSYSENT,
#endif
	ultrix_sysent,
	ultrix_syscallnames,
	sendsig,
	trapsignal,
	ultrix_sigcode,
	ultrix_esigcode,
	setregs,
	NULL,
	NULL,
	NULL,
#ifdef __HAVE_SYSCALL_INTERN
	syscall_intern,
#else
	syscall,
#endif
	NULL,
	NULL,
};

#define GSI_PROG_ENV 1

int
ultrix_sys_getsysinfo(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct ultrix_sys_getsysinfo_args *uap = v;
	static short progenv = 0;

	switch (SCARG(uap, op)) {
		/* operations implemented: */
	case GSI_PROG_ENV:
		if (SCARG(uap, nbytes) < sizeof(short))
			return EINVAL;
		*retval = 1;
		return (copyout(&progenv, SCARG(uap, buffer), sizeof(short)));
	default:
		*retval = 0; /* info unavail */
		return 0;
	}
}

int
ultrix_sys_setsysinfo(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{

#ifdef notyet
	struct ultrix_sys_setsysinfo_args *uap = v;
#endif

	*retval = 0;
	return 0;
}

int
ultrix_sys_waitpid(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct ultrix_sys_waitpid_args *uap = v;
	struct sys_wait4_args ua;

	SCARG(&ua, pid) = SCARG(uap, pid);
	SCARG(&ua, status) = SCARG(uap, status);
	SCARG(&ua, options) = SCARG(uap, options);
	SCARG(&ua, rusage) = 0;

	return (sys_wait4(l, &ua, retval));
}

int
ultrix_sys_wait3(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct ultrix_sys_wait3_args *uap = v;
	struct sys_wait4_args ua;

	SCARG(&ua, pid) = -1;
	SCARG(&ua, status) = SCARG(uap, status);
	SCARG(&ua, options) = SCARG(uap, options);
	SCARG(&ua, rusage) = SCARG(uap, rusage);

	return (sys_wait4(l, &ua, retval));
}

/*
 * Ultrix binaries pass in FD_MAX as the first arg to select().
 * On Ultrix, FD_MAX is 4096, which is more than the NetBSD sys_select()
 * can handle.
 * Since we can't have more than the (native) FD_MAX descriptors open, 
 * limit nfds to at most FD_MAX.
 */
int
ultrix_sys_select(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct sys_select_args *uap = v;
	struct timeval atv;
	int error;

	/* Limit number of FDs selected on to the native maximum */

	if (SCARG(uap, nd) > FD_SETSIZE)
		SCARG(uap, nd) = FD_SETSIZE;

	/* Check for negative timeval */
	if (SCARG(uap, tv)) {
		error = copyin((caddr_t)SCARG(uap, tv), (caddr_t)&atv,
			       sizeof(atv));
		if (error)
			goto done;
#ifdef DEBUG
		/* Ultrix clients sometimes give negative timeouts? */
		if (atv.tv_sec < 0 || atv.tv_usec < 0)
			printf("ultrix select( %ld, %ld): negative timeout\n",
			    atv.tv_sec, atv.tv_usec);
#endif

	}
	error = sys_select(l, (void*) uap, retval);
	if (error == EINVAL)
		printf("ultrix select: bad args?\n");

done:
	return error;
}

#if defined(NFS)
int
async_daemon(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct sys_nfssvc_args ouap;

	SCARG(&ouap, flag) = NFSSVC_BIOD;
	SCARG(&ouap, argp) = NULL;

	return (sys_nfssvc(l, &ouap, retval));
}
#endif /* NFS */


#define	SUN__MAP_NEW	0x80000000	/* if not, old mmap & cannot handle */

int
ultrix_sys_mmap(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct ultrix_sys_mmap_args *uap = v;
	struct proc *p = l->l_proc;
	struct sys_mmap_args ouap;
	struct filedesc *fdp;
	struct file *fp;
	struct vnode *vp;

	/*
	 * Verify the arguments.
	 */
	if (SCARG(uap, prot) & ~(PROT_READ|PROT_WRITE|PROT_EXEC))
		return (EINVAL);			/* XXX still needed? */

	if ((SCARG(uap, flags) & SUN__MAP_NEW) == 0)
		return (EINVAL);

	SCARG(&ouap, flags) = SCARG(uap, flags) & ~SUN__MAP_NEW;
	SCARG(&ouap, addr) = SCARG(uap, addr);
	SCARG(&ouap, len) = SCARG(uap, len);
	SCARG(&ouap, prot) = SCARG(uap, prot);
	SCARG(&ouap, fd) = SCARG(uap, fd);
	SCARG(&ouap, pos) = SCARG(uap, pos);

	return (sys_mmap(l, &ouap, retval));
}

int
ultrix_sys_setsockopt(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct ultrix_sys_setsockopt_args *uap = v;
	struct proc *p = l->l_proc;
	struct file *fp;
	struct mbuf *m = NULL;
	int error;

	/* getsock() will use the descriptor for us */
	if ((error = getsock(p->p_fd, SCARG(uap, s), &fp))  != 0)
		return (error);
#define	SO_DONTLINGER (~SO_LINGER)
	if (SCARG(uap, name) == SO_DONTLINGER) {
		m = m_get(M_WAIT, MT_SOOPTS);
		mtod(m, struct linger *)->l_onoff = 0;
		m->m_len = sizeof(struct linger);
		error = sosetopt((struct socket *)fp->f_data, SCARG(uap, level),
		    SO_LINGER, m);
	}
	if (SCARG(uap, level) == IPPROTO_IP) {
#define		EMUL_IP_MULTICAST_IF		2
#define		EMUL_IP_MULTICAST_TTL		3
#define		EMUL_IP_MULTICAST_LOOP		4
#define		EMUL_IP_ADD_MEMBERSHIP		5
#define		EMUL_IP_DROP_MEMBERSHIP	6
		static int ipoptxlat[] = {
			IP_MULTICAST_IF,
			IP_MULTICAST_TTL,
			IP_MULTICAST_LOOP,
			IP_ADD_MEMBERSHIP,
			IP_DROP_MEMBERSHIP
		};
		if (SCARG(uap, name) >= EMUL_IP_MULTICAST_IF &&
		    SCARG(uap, name) <= EMUL_IP_DROP_MEMBERSHIP) {
			SCARG(uap, name) =
			    ipoptxlat[SCARG(uap, name) - EMUL_IP_MULTICAST_IF];
		}
	}
	if (SCARG(uap, valsize) > MLEN) {
		error = EINVAL;
		goto out;
	}
	if (SCARG(uap, val)) {
		m = m_get(M_WAIT, MT_SOOPTS);
		error = copyin(SCARG(uap, val), mtod(m, caddr_t),
		    (u_int)SCARG(uap, valsize));
		if (error) {
			(void) m_free(m);
			goto out;
		}
		m->m_len = SCARG(uap, valsize);
	}
	error = sosetopt((struct socket *)fp->f_data, SCARG(uap, level),
	    SCARG(uap, name), m);
 out:
	FILE_UNUSE(fp, p);
	return (error);
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
ultrix_sys_uname(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct ultrix_sys_uname_args *uap = v;
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

	return copyout((caddr_t)&sut, (caddr_t)SCARG(uap, name),
	    sizeof(struct ultrix_utsname));
}

int
ultrix_sys_setpgrp(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct ultrix_sys_setpgrp_args *uap = v;
	struct proc *p = l->l_proc;

	/*
	 * difference to our setpgid call is to include backwards
	 * compatibility to pre-setsid() binaries. Do setsid()
	 * instead of setpgid() in those cases where the process
	 * tries to create a new session the old way.
	 */
	if (!SCARG(uap, pgid) &&
	    (!SCARG(uap, pid) || SCARG(uap, pid) == p->p_pid))
		return sys_setsid(l, uap, retval);
	else
		return sys_setpgid(l, uap, retval);
}

#if defined (NFSSERVER)
int
ultrix_sys_nfssvc(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{

#if 0	/* XXX */
	struct ultrix_sys_nfssvc_args *uap = v;
	struct emul *e = p->p_emul;
	struct sys_nfssvc_args outuap;
	struct sockaddr sa;
	int error;
	caddr_t sg = stackgap_init(p, 0);

	memset(&outuap, 0, sizeof outuap);
	SCARG(&outuap, fd) = SCARG(uap, fd);
	SCARG(&outuap, mskval) = stackgap_alloc(p, &sg, sizeof sa);
	SCARG(&outuap, msklen) = sizeof sa;
	SCARG(&outuap, mtchval) = stackgap_alloc(p, &sg, sizeof sa);
	SCARG(&outuap, mtchlen) = sizeof sa;

	memset(&sa, 0, sizeof sa);
	if (error = copyout(&sa, SCARG(&outuap, mskval), SCARG(&outuap, msklen)))
		return (error);
	if (error = copyout(&sa, SCARG(&outuap, mtchval), SCARG(&outuap, mtchlen)))
		return (error);

	return nfssvc(l, &outuap, retval);
#else
	return (ENOSYS);
#endif
}
#endif /* NFSSERVER */

struct ultrix_ustat {
	daddr_t	f_tfree;	/* total free */
	ino_t	f_tinode;	/* total inodes free */
	char	f_fname[6];	/* filsys name */
	char	f_fpack[6];	/* filsys pack name */
};

int
ultrix_sys_ustat(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct ultrix_sys_ustat_args *uap = v;
	struct ultrix_ustat us;
	int error;

	memset(&us, 0, sizeof us);

	/*
	 * XXX: should set f_tfree and f_tinode at least
	 * How do we translate dev -> fstat? (and then to ultrix_ustat)
	 */

	if ((error = copyout(&us, SCARG(uap, buf), sizeof us)) != 0)
		return (error);
	return 0;
}

int
ultrix_sys_quotactl(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{

#ifdef notyet
	struct ultrix_sys_quotactl_args *uap = v;
#endif

	return EINVAL;
}

int
ultrix_sys_vhangup(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{

	return 0;
}


/*
 * RISC Ultrix cache control syscalls
 */
#ifdef __mips
int
ultrix_sys_cacheflush(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct ultrix_sys_cacheflush_args /* {
		syscallarg(void *) addr;
		syscallarg(int) nbytes;
		syscallarg(int) flag;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	vaddr_t va  = (vaddr_t)SCARG(uap, addr);
	int nbytes     = SCARG(uap, nbytes);
	int whichcache = SCARG(uap, whichcache);

	return (mips_user_cacheflush(p, va, nbytes, whichcache));
}


int
ultrix_sys_cachectl(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct ultrix_sys_cachectl_args /* {
		syscallarg(void *) addr;
		syscallarg(int) nbytes;
		syscallarg(int) cacheop;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	vaddr_t va  = (vaddr_t)SCARG(uap, addr);
	int nbytes  = SCARG(uap, nbytes);
	int cacheop = SCARG(uap, cacheop);

	return mips_user_cachectl(p, va, nbytes, cacheop);
}

#endif	/* __mips */


int
ultrix_sys_exportfs(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
#ifdef notyet
	struct ultrix_sys_exportfs_args *uap = v;
#endif

	/*
	 * XXX: should perhaps translate into a mount(2)
	 * with MOUNT_EXPORT?
	 */
	return 0;
}

int
ultrix_sys_sigpending(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct ultrix_sys_sigpending_args *uap = v;
	sigset_t ss;
	int mask;

	sigpending1(l->l_proc, &ss);
	mask = ss.__bits[0];

	return (copyout((caddr_t)&mask, (caddr_t)SCARG(uap, mask), sizeof(int)));
}

int
ultrix_sys_sigreturn(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct ultrix_sys_sigreturn_args *uap = v;

	/* struct sigcontext13 is close enough to Ultrix */

	return (compat_13_sys_sigreturn(l, uap, retval));
}

int
ultrix_sys_sigcleanup(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct ultrix_sys_sigcleanup_args *uap = v;

	/* struct sigcontext13 is close enough to Ultrix */

	return (compat_13_sys_sigreturn(l, uap, retval));
}

int
ultrix_sys_sigsuspend(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct ultrix_sys_sigsuspend_args *uap = v;
	int mask = SCARG(uap, mask);
	sigset_t ss;

	ss.__bits[0] = mask;
	ss.__bits[1] = 0;
	ss.__bits[2] = 0;
	ss.__bits[3] = 0;

	return (sigsuspend1(l->l_proc, &ss));
}

#define ULTRIX_SV_ONSTACK 0x0001  /* take signal on signal stack */
#define ULTRIX_SV_INTERRUPT 0x0002  /* do not restart system on signal return */
#define ULTRIX_SV_OLDSIG 0x1000  /* Emulate old signal() for POSIX */

int
ultrix_sys_sigvec(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct ultrix_sys_sigvec_args *uap = v;
	struct sigvec nsv, osv;
	struct sigaction nsa, osa;
	int error;

	if (SCARG(uap, nsv)) {
		error = copyin(SCARG(uap, nsv), &nsv, sizeof(nsv));
		if (error)
			return (error);
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
	error = sigaction1(l->l_proc, SCARG(uap, signum),
	    SCARG(uap, nsv) ? &nsa : 0, SCARG(uap, osv) ? &osa : 0,
	    NULL, 0);
	if (error)
		return (error);
	if (SCARG(uap, osv)) {
		osv.sv_handler = osa.sa_handler;
		osv.sv_flags = osa.sa_flags ^ SA_RESTART;
		osv.sv_flags &= (ULTRIX_SV_ONSTACK | ULTRIX_SV_INTERRUPT);
		native_sigset_to_sigset13(&osa.sa_mask, &osv.sv_mask);
		error = copyout(&osv, SCARG(uap, osv), sizeof(osv));
		if (error)
			return (error);
	}
	return (0);
}

int
ultrix_sys_shmsys(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{

#ifdef SYSVSHM

	/* Ultrix SVSHM weirndess: */
	struct ultrix_sys_shmsys_args *uap = v;
	struct sys_shmat_args shmat_args;
	struct compat_14_sys_shmctl_args shmctl_args;
	struct sys_shmdt_args shmdt_args;
	struct sys_shmget_args shmget_args;


	switch (SCARG(uap, shmop)) {
	case 0:						/* Ultrix shmat() */
		SCARG(&shmat_args, shmid) = SCARG(uap, a2);
		SCARG(&shmat_args, shmaddr) = (void *)SCARG(uap, a3);
		SCARG(&shmat_args, shmflg) = SCARG(uap, a4);
		return (sys_shmat(l, &shmat_args, retval));

	case 1:						/* Ultrix shmctl() */
		SCARG(&shmctl_args, shmid) = SCARG(uap, a2);
		SCARG(&shmctl_args, cmd) = SCARG(uap, a3);
		SCARG(&shmctl_args, buf) = (struct shmid_ds14 *)SCARG(uap, a4);
		return (compat_14_sys_shmctl(l, &shmctl_args, retval));

	case 2:						/* Ultrix shmdt() */
		SCARG(&shmat_args, shmaddr) = (void *)SCARG(uap, a2);
		return (sys_shmdt(l, &shmdt_args, retval));

	case 3:						/* Ultrix shmget() */
		SCARG(&shmget_args, key) = SCARG(uap, a2);
		SCARG(&shmget_args, size) = SCARG(uap, a3);
		SCARG(&shmget_args, shmflg) = SCARG(uap, a4)
		    & (IPC_CREAT|IPC_EXCL|IPC_NOWAIT);
		return (sys_shmget(l, &shmget_args, retval));

	default:
		return (EINVAL);
	}
#else
	return (EOPNOTSUPP);
#endif	/* SYSVSHM */
}

static int
ultrix_to_bsd_flock(ufl, fl)
	struct ultrix_flock *ufl;
	struct flock *fl;
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
		return (EINVAL);
	}

	return (0);
}

static void
bsd_to_ultrix_flock(fl, ufl)
	struct flock *fl;
	struct ultrix_flock *ufl;
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
ultrix_sys_fcntl(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct ultrix_sys_fcntl_args *uap = v;
	struct proc *p = l->l_proc;
	int error;
	struct ultrix_flock ufl;
	struct flock fl, *flp;
	caddr_t sg;
	struct sys_fcntl_args *args, fca;

	switch (SCARG(uap, cmd)) {
	case F_GETLK:
	case F_SETLK:
	case F_SETLKW:
		error = copyin(SCARG(uap, arg), &ufl, sizeof(ufl));
		if (error)
			return (error);
		error = ultrix_to_bsd_flock(&ufl, &fl);
		if (error)
			return (error);
		sg = stackgap_init(p, 0);
		flp = (struct flock *)stackgap_alloc(p, &sg, sizeof(*flp));
		error = copyout(&fl, flp, sizeof(*flp));
		if (error)
			return (error);

		SCARG(&fca, fd) = SCARG(uap, fd);
		SCARG(&fca, cmd) = SCARG(uap, cmd);
		SCARG(&fca, arg) = flp;
		args = &fca;
		break;
	default:
		args = v;
		break;
	}

	error = sys_fcntl(l, args, retval);
	if (error)
		return (error);

	switch (SCARG(uap, cmd)) {
	case F_GETLK:
		error = copyin(flp, &fl, sizeof(fl));
		if (error)
			return (error);
		bsd_to_ultrix_flock(&fl, &ufl);
		error = copyout(&ufl, SCARG(uap, arg), sizeof(ufl));
		break;
	}

	return (error);
}
