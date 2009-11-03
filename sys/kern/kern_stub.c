/*	$NetBSD: kern_stub.c,v 1.21 2009/11/03 05:23:28 dyoung Exp $	*/

/*-
 * Copyright (c) 2007, 2008 The NetBSD Foundation, Inc.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (c) 1982, 1986, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	@(#)subr_xxx.c	8.3 (Berkeley) 3/29/95
 */

/*
 * Stubs for system calls and facilities not included in the system.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kern_stub.c,v 1.21 2009/11/03 05:23:28 dyoung Exp $");

#include "opt_ptrace.h"
#include "opt_ktrace.h"
#include "opt_modular.h"
#include "opt_sa.h"
#ifdef _KERNEL_OPT
#include "fs_lfs.h"
#endif

/* XXX To get syscall prototypes. */
#define SYSVSHM
#define SYSVSEM
#define SYSVMSG

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/fstypes.h>
#include <sys/signalvar.h>
#include <sys/syscall.h>
#include <sys/syscallargs.h>
#include <sys/syscallvar.h>
#include <sys/ktrace.h>
#include <sys/intr.h>
#include <sys/cpu.h>
#include <sys/module.h>

/*
 * Nonexistent system call-- signal process (may want to handle it).  Flag
 * error in case process won't see signal immediately (blocked or ignored).
 */
#ifndef PTRACE
__weak_alias(sys_ptrace,sys_nosys);
#endif	/* PTRACE */

/*
 * ktrace stubs.  ktruser() goes to enosys as we want to fail the syscall,
 * but not kill the process: utrace() is a debugging feature.
 */
#ifndef KTRACE
__weak_alias(ktr_csw,nullop);		/* Probes */
__weak_alias(ktr_emul,nullop);
__weak_alias(ktr_geniov,nullop);
__weak_alias(ktr_genio,nullop);
__weak_alias(ktr_mibio,nullop);
__weak_alias(ktr_namei,nullop);
__weak_alias(ktr_namei2,nullop);
__weak_alias(ktr_psig,nullop);
__weak_alias(ktr_saupcall,nullop);
__weak_alias(ktr_syscall,nullop);
__weak_alias(ktr_sysret,nullop);
__weak_alias(ktr_kuser,nullop);
__weak_alias(ktr_mmsg,nullop);
__weak_alias(ktr_mib,nullop);
__weak_alias(ktr_mool,nullop);
__weak_alias(ktr_execarg,nullop);
__weak_alias(ktr_execenv,nullop);

__weak_alias(sys_fktrace,sys_nosys);	/* Syscalls */
__weak_alias(sys_ktrace,sys_nosys);
__weak_alias(sys_utrace,sys_nosys);

int	ktrace_on;			/* Misc */
__weak_alias(ktruser,enosys);
__weak_alias(ktr_point,nullop);
#endif	/* KTRACE */

__weak_alias(spldebug_start, voidop);
__weak_alias(spldebug_stop, voidop);
__weak_alias(machdep_init,nullop);

#if !defined(KERN_SA)
/*
 * Scheduler activations system calls.  These need to remain, even when
 * KERN_SA isn't defined, until libc's major version is bumped.
 */
__strong_alias(sys_sa_register,sys_nosys);
__strong_alias(sys_sa_stacks,sys_nosys);
__strong_alias(sys_sa_enable,sys_nosys);
__strong_alias(sys_sa_setconcurrency,sys_nosys);
__strong_alias(sys_sa_yield,sys_nosys);
__strong_alias(sys_sa_preempt,sys_nosys);
__strong_alias(sys_sa_unblockyield,sys_nosys);

/*
 * Stubs for compat_netbsd32.
 */
__strong_alias(dosa_register,sys_nosys);
__strong_alias(sa_stacks1,sys_nosys);
#endif

/*
 * Stubs for architectures that do not support kernel preemption.
 */
#ifndef __HAVE_PREEMPTION
bool
cpu_kpreempt_enter(uintptr_t where, int s)
{

	return false;
}

void
cpu_kpreempt_exit(uintptr_t where)
{

}

bool
cpu_kpreempt_disabled(void)
{

	return true;
}
#else
# ifndef MULTIPROCESSOR
#   error __HAVE_PREEMPTION requires MULTIPROCESSOR
# endif
#endif	/* !__HAVE_PREEMPTION */

int
sys_nosys(struct lwp *l, const void *v, register_t *retval)
{

	mutex_enter(proc_lock);
	psignal(l->l_proc, SIGSYS);
	mutex_exit(proc_lock);
	return ENOSYS;
}

int
sys_nomodule(struct lwp *l, const void *v, register_t *retval)
{
#ifdef MODULAR
	static struct {
		u_int		al_code;
		const char	*al_module;
	} const autoload[] = {
	    { SYS_aio_cancel, "aio" },
	    { SYS_aio_error, "aio" },
	    { SYS_aio_fsync, "aio" },
	    { SYS_aio_read, "aio" },
	    { SYS_aio_return, "aio" },
	    { SYS___aio_suspend50, "aio" },
	    { SYS_aio_write, "aio" },
	    { SYS_lio_listio, "aio" },
	    { SYS_mq_open, "mqueue" },
	    { SYS_mq_close, "mqueue" },
	    { SYS_mq_unlink, "mqueue" },
	    { SYS_mq_getattr, "mqueue" },
	    { SYS_mq_setattr, "mqueue" },
	    { SYS_mq_notify, "mqueue" },
	    { SYS_mq_send, "mqueue" },
	    { SYS_mq_receive, "mqueue" },
	    { SYS___mq_timedsend50, "mqueue" },
	    { SYS___mq_timedreceive50, "mqueue" },
	    { SYS_compat_43_fstat43, "compat" },
	    { SYS_compat_43_lstat43, "compat" },
	    { SYS_compat_43_oaccept, "compat" },
	    { SYS_compat_43_ocreat, "compat" },
	    { SYS_compat_43_oftruncate, "compat" },
	    { SYS_compat_43_ogetdirentries, "compat" },
	    { SYS_compat_43_ogetdtablesize, "compat" },
	    { SYS_compat_43_ogethostid, "compat" },
	    { SYS_compat_43_ogethostname, "compat" },
	    { SYS_compat_43_ogetkerninfo, "compat" },
	    { SYS_compat_43_ogetpagesize, "compat" },
	    { SYS_compat_43_ogetpeername, "compat" },
	    { SYS_compat_43_ogetrlimit, "compat" },
	    { SYS_compat_43_ogetsockname, "compat" },
	    { SYS_compat_43_okillpg, "compat" },
	    { SYS_compat_43_olseek, "compat" },
	    { SYS_compat_43_ommap, "compat" },
	    { SYS_compat_43_oquota, "compat" },
	    { SYS_compat_43_orecv, "compat" },
	    { SYS_compat_43_orecvfrom, "compat" },
	    { SYS_compat_43_orecvmsg, "compat" },
	    { SYS_compat_43_osend, "compat" },
	    { SYS_compat_43_osendmsg, "compat" },
	    { SYS_compat_43_osethostid, "compat" },
	    { SYS_compat_43_osethostname, "compat" },
	    { SYS_compat_43_osetrlimit, "compat" },
	    { SYS_compat_43_osigblock, "compat" },
	    { SYS_compat_43_osigsetmask, "compat" },
	    { SYS_compat_43_osigstack, "compat" },
	    { SYS_compat_43_osigvec, "compat" },
	    { SYS_compat_43_otruncate, "compat" },
	    { SYS_compat_43_owait, "compat" },
	    { SYS_compat_43_stat43, "compat" },
	    { SYS_compat_09_ogetdomainname, "compat" },
	    { SYS_compat_09_osetdomainname, "compat" },
	    { SYS_compat_09_ouname, "compat" },
#ifndef _LP64
	    { SYS_compat_10_omsgsys, "compat" },
	    { SYS_compat_10_osemsys, "compat" },
	    { SYS_compat_10_oshmsys, "compat" },
#endif
	    { SYS_compat_12_fstat12, "compat" },
	    { SYS_compat_12_getdirentries, "compat" },
	    { SYS_compat_12_lstat12, "compat" },
	    { SYS_compat_12_msync, "compat" },
	    { SYS_compat_12_oreboot, "compat" },
	    { SYS_compat_12_oswapon, "compat" },
	    { SYS_compat_12_stat12, "compat" },
	    { SYS_compat_13_sigaction13, "compat" },
	    { SYS_compat_13_sigaltstack13, "compat" },
	    { SYS_compat_13_sigpending13, "compat" },
	    { SYS_compat_13_sigprocmask13, "compat" },
	    { SYS_compat_13_sigreturn13, "compat" },
	    { SYS_compat_13_sigsuspend13, "compat" },
	    { SYS_compat_14___semctl, "compat" },
	    { SYS_compat_14_msgctl, "compat" },
	    { SYS_compat_14_shmctl, "compat" },
	    { SYS_compat_16___sigaction14, "compat" },
	    { SYS_compat_16___sigreturn14, "compat" },
	    { SYS_compat_20_fhstatfs, "compat" },
	    { SYS_compat_20_fstatfs, "compat" },
	    { SYS_compat_20_getfsstat, "compat" },
	    { SYS_compat_20_statfs, "compat" },
	    { SYS_compat_30___fhstat30, "compat" },
	    { SYS_compat_30___fstat13, "compat" },
	    { SYS_compat_30___lstat13, "compat" },
	    { SYS_compat_30___stat13, "compat" },
	    { SYS_compat_30_fhopen, "compat" },
	    { SYS_compat_30_fhstat, "compat" },
	    { SYS_compat_30_fhstatvfs1, "compat" },
	    { SYS_compat_30_getdents, "compat" },
	    { SYS_compat_30_getfh, "compat" },
	    { SYS_compat_30_socket, "compat" },
	    { SYS_compat_40_mount, "compat" },
	    { SYS_compat_50_wait4, "compat" },
	    { SYS_compat_50_mknod, "compat" },
	    { SYS_compat_50_setitimer, "compat" },
	    { SYS_compat_50_getitimer, "compat" },
	    { SYS_compat_50_select, "compat" },
	    { SYS_compat_50_gettimeofday, "compat" },
	    { SYS_compat_50_getrusage, "compat" },
	    { SYS_compat_50_settimeofday, "compat" },
	    { SYS_compat_50_utimes, "compat" },
	    { SYS_compat_50_adjtime, "compat" },
#ifdef LFS
	    { SYS_compat_50_lfs_segwait, "compat" },
#endif
	    { SYS_compat_50_futimes, "compat" },
	    { SYS_compat_50_clock_gettime, "compat" },
	    { SYS_compat_50_clock_settime, "compat" },
	    { SYS_compat_50_clock_getres, "compat" },
	    { SYS_compat_50_timer_settime, "compat" },
	    { SYS_compat_50_timer_gettime, "compat" },
	    { SYS_compat_50_nanosleep, "compat" },
	    { SYS_compat_50___sigtimedwait, "compat" },
	    { SYS_compat_50_mq_timedsend, "compat" },
	    { SYS_compat_50_mq_timedreceive, "compat" },
	    { SYS_compat_50_lutimes, "compat" },
	    { SYS_compat_50_____semctl13, "compat" },
	    { SYS_compat_50___msgctl13, "compat" },
	    { SYS_compat_50___shmctl13, "compat" },
	    { SYS_compat_50__lwp_park, "compat" },
	    { SYS_compat_50_kevent, "compat" },
	    { SYS_compat_50_pselect, "compat" },
	    { SYS_compat_50_pollts, "compat" },
	    { SYS_compat_50___stat30, "compat" },
	    { SYS_compat_50___fstat30, "compat" },
	    { SYS_compat_50___lstat30, "compat" },
	    { SYS_compat_50___ntp_gettime30, "compat" },
	    { SYS_compat_50___fhstat40, "compat" },
	    { SYS_compat_50_aio_suspend, "compat" },
	    { SYS__ksem_init, "ksem" },
	    { SYS__ksem_open, "ksem" },
	    { SYS__ksem_unlink, "ksem" },
	    { SYS__ksem_close, "ksem" },
	    { SYS__ksem_post, "ksem" },
	    { SYS__ksem_wait, "ksem" },
	    { SYS__ksem_trywait, "ksem" },
	    { SYS__ksem_getvalue, "ksem" },
	    { SYS__ksem_destroy, "ksem" },
	    { SYS_nfssvc, "nfsserver" },
	};
	const struct sysent *sy;
	const struct emul *em;
	int code, i;

	/*
	 * Restart the syscall if we interrupted a module unload that
	 * failed.  Acquiring module_lock delays us until any unload
	 * has been completed or rolled back.
	 */
	mutex_enter(&module_lock);
	sy = l->l_sysent;
	if (sy->sy_call != sys_nomodule) {
		mutex_exit(&module_lock);
		return ERESTART;
	}
	/*
	 * Try to autoload a module to satisfy the request.  If it 
	 * works, retry the request.
	 */
	em = l->l_proc->p_emul;
	if (em == &emul_netbsd) {
		code = sy - em->e_sysent;
		for (i = 0; i < __arraycount(autoload); i++) {
			if (autoload[i].al_code != code) {
				continue;
			}
			if (module_autoload(autoload[i].al_module,
			    MODULE_CLASS_ANY) != 0 ||
			    sy->sy_call == sys_nomodule) {
			    	break;
			}
			mutex_exit(&module_lock);
			return ERESTART;
		}
	}
	mutex_exit(&module_lock);
#endif	/* MODULAR */

	return sys_nosys(l, v, retval);
}

/*
 * Unsupported device function (e.g. writing to read-only device).
 */
int
enodev(void)
{

	return (ENODEV);
}

/*
 * Unconfigured device function; driver not configured.
 */
int
enxio(void)
{

	return (ENXIO);
}

/*
 * Unsupported ioctl function.
 */
int
enoioctl(void)
{

	return (ENOTTY);
}

/*
 * Unsupported system function.
 * This is used for an otherwise-reasonable operation
 * that is not supported by the current system binary.
 */
int
enosys(void)
{

	return (ENOSYS);
}

/*
 * Return error for operation not supported
 * on a specific object or file type.
 */
int
eopnotsupp(void)
{

	return (EOPNOTSUPP);
}

/*
 * Generic null operation, void return value.
 */
void
voidop(void)
{
}

/*
 * Generic null operation, always returns success.
 */
/*ARGSUSED*/
int
nullop(void *v)
{

	return (0);
}
