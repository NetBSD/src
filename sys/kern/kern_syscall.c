/*	$NetBSD: kern_syscall.c,v 1.4.6.3 2010/10/09 03:32:31 yamt Exp $	*/

/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software developed for The NetBSD Foundation
 * by Andrew Doran.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kern_syscall.c,v 1.4.6.3 2010/10/09 03:32:31 yamt Exp $");

#include "opt_modular.h"

/* XXX To get syscall prototypes. */
#define SYSVSHM
#define SYSVSEM
#define SYSVMSG

#include <sys/param.h>
#include <sys/module.h>
#include <sys/sched.h>
#include <sys/syscall.h>
#include <sys/syscallargs.h>
#include <sys/syscallvar.h>
#include <sys/systm.h>
#include <sys/xcall.h>

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
	    { SYS_compat_50_lfs_segwait, "compat" },
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
	 * failed.  Acquiring kernconfig_lock delays us until any unload
	 * has been completed or rolled back.
	 */
	kernconfig_lock();
	sy = l->l_sysent;
	if (sy->sy_call != sys_nomodule) {
		kernconfig_unlock();
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
			kernconfig_unlock();
			return ERESTART;
		}
	}
	kernconfig_unlock();
#endif	/* MODULAR */

	return sys_nosys(l, v, retval);
}

int
syscall_establish(const struct emul *em, const struct syscall_package *sp)
{
	struct sysent *sy;
	int i;

	KASSERT(kernconfig_is_held());

	if (em == NULL) {
		em = &emul_netbsd;
	}
	sy = em->e_sysent;

	/*
	 * Ensure that all preconditions are valid, since this is
	 * an all or nothing deal.  Once a system call is entered,
	 * it can become busy and we could be unable to remove it
	 * on error.
	 */
	for (i = 0; sp[i].sp_call != NULL; i++) {
		if (sy[sp[i].sp_code].sy_call != sys_nomodule) {
#ifdef DIAGNOSTIC
			printf("syscall %d is busy\n", sp[i].sp_code);
#endif
			return EBUSY;
		}
	}
	/* Everything looks good, patch them in. */
	for (i = 0; sp[i].sp_call != NULL; i++) {
		sy[sp[i].sp_code].sy_call = sp[i].sp_call;
	}

	return 0;
}

int
syscall_disestablish(const struct emul *em, const struct syscall_package *sp)
{
	struct sysent *sy;
	uint64_t where;
	lwp_t *l;
	int i;

	KASSERT(kernconfig_is_held());

	if (em == NULL) {
		em = &emul_netbsd;
	}
	sy = em->e_sysent;

	/*
	 * First, patch the system calls to sys_nomodule to gate further
	 * activity.
	 */
	for (i = 0; sp[i].sp_call != NULL; i++) {
		KASSERT(sy[sp[i].sp_code].sy_call == sp[i].sp_call);
		sy[sp[i].sp_code].sy_call = sys_nomodule;
	}

	/*
	 * Run a cross call to cycle through all CPUs.  This does two
	 * things: lock activity provides a barrier and makes our update
	 * of sy_call visible to all CPUs, and upon return we can be sure
	 * that we see pertinent values of l_sysent posted by remote CPUs.
	 */
	where = xc_broadcast(0, (xcfunc_t)nullop, NULL, NULL);
	xc_wait(where);

	/*
	 * Now it's safe to check l_sysent.  Run through all LWPs and see
	 * if anyone is still using the system call.
	 */
	for (i = 0; sp[i].sp_call != NULL; i++) {
		mutex_enter(proc_lock);
		LIST_FOREACH(l, &alllwp, l_list) {
			if (l->l_sysent == &sy[sp[i].sp_code]) {
				break;
			}
		}
		mutex_exit(proc_lock);
		if (l == NULL) {
			continue;
		}
		/*
		 * We lose: one or more calls are still in use.  Put back
		 * the old entrypoints and act like nothing happened.
		 * When we drop kernconfig_lock, any system calls held in
		 * sys_nomodule() will be restarted.
		 */
		for (i = 0; sp[i].sp_call != NULL; i++) {
			sy[sp[i].sp_code].sy_call = sp[i].sp_call;
		}
		return EBUSY;
	}

	return 0;
}
