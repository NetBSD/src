/*	$NetBSD: compat_mod.c,v 1.24.14.28 2018/03/30 11:18:34 pgoyette Exp $	*/

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

/*
 * Linkage for the compat module: spaghetti.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: compat_mod.c,v 1.24.14.28 2018/03/30 11:18:34 pgoyette Exp $");

#ifdef _KERNEL_OPT
#include "opt_compat_netbsd.h"
#include "opt_compat_43.h"
#include "opt_ntp.h"
#include "opt_sysv.h"
#include "opt_lfs.h"
#endif

#include <sys/systm.h>
#include <sys/module.h>
#include <sys/rwlock.h>
#include <sys/tty.h>
#include <sys/signalvar.h>
#include <sys/syscall.h>
#include <sys/syscallargs.h>
#include <sys/syscallvar.h>
#include <sys/sysctl.h>
#include <sys/vfs_syscalls.h>

#include <net/if.h>

#include <uvm/uvm_extern.h>
#include <uvm/uvm_object.h>

#include <compat/common/compat_util.h>
#include <compat/common/compat_mod.h>
#include <compat/common/if_43.h>
#include <compat/sys/uvm.h>
#include <compat/sys/cpuio.h>

#if defined(COMPAT_09) || defined(COMPAT_43)
static struct sysctllog *compat_clog = NULL;
#endif

#ifdef COMPAT_70
#include <net/route.h>
#include <compat/net/route.h>
#include <compat/net/route_70.h>
#endif

static const char * const compat_includes[] = {
	"compat_80", "compat_70", "compat_60", "compat_50", "compat_40",
	"compat_30", "compat_20",
	NULL
};

MODULE_WITH_ALIASES(MODULE_CLASS_EXEC, compat, NULL, &compat_includes);


extern krwlock_t ttcompat_lock;

static const struct syscall_package compat_syscalls[] = {
#if defined(COMPAT_43)
	{ SYS_compat_43_fstat43, 0, (sy_call_t *)compat_43_sys_fstat },
	{ SYS_compat_43_lstat43, 0, (sy_call_t *)compat_43_sys_lstat },
	{ SYS_compat_43_oaccept, 0, (sy_call_t *)compat_43_sys_accept },
	{ SYS_compat_43_ocreat, 0, (sy_call_t *)compat_43_sys_creat },
	{ SYS_compat_43_oftruncate, 0, (sy_call_t *)compat_43_sys_ftruncate },
	{ SYS_compat_43_ogetdirentries, 0, (sy_call_t *)compat_43_sys_getdirentries },
	{ SYS_compat_43_ogetdtablesize, 0, (sy_call_t *)compat_43_sys_getdtablesize },
	{ SYS_compat_43_ogethostid, 0, (sy_call_t *)compat_43_sys_gethostid },
	{ SYS_compat_43_ogethostname, 0, (sy_call_t *)compat_43_sys_gethostname },
	{ SYS_compat_43_ogetkerninfo, 0, (sy_call_t *)compat_43_sys_getkerninfo },
	{ SYS_compat_43_ogetpagesize, 0, (sy_call_t *)compat_43_sys_getpagesize },
	{ SYS_compat_43_ogetpeername, 0, (sy_call_t *)compat_43_sys_getpeername },
	{ SYS_compat_43_ogetrlimit, 0, (sy_call_t *)compat_43_sys_getrlimit },
	{ SYS_compat_43_ogetsockname, 0, (sy_call_t *)compat_43_sys_getsockname },
	{ SYS_compat_43_okillpg, 0, (sy_call_t *)compat_43_sys_killpg },
	{ SYS_compat_43_olseek, 0, (sy_call_t *)compat_43_sys_lseek },
	{ SYS_compat_43_ommap, 0, (sy_call_t *)compat_43_sys_mmap },
	{ SYS_compat_43_oquota, 0, (sy_call_t *)compat_43_sys_quota },
	{ SYS_compat_43_orecv, 0, (sy_call_t *)compat_43_sys_recv },
	{ SYS_compat_43_orecvfrom, 0, (sy_call_t *)compat_43_sys_recvfrom },
	{ SYS_compat_43_orecvmsg, 0, (sy_call_t *)compat_43_sys_recvmsg },
	{ SYS_compat_43_osend, 0, (sy_call_t *)compat_43_sys_send },
	{ SYS_compat_43_osendmsg, 0, (sy_call_t *)compat_43_sys_sendmsg },
	{ SYS_compat_43_osethostid, 0, (sy_call_t *)compat_43_sys_sethostid },
	{ SYS_compat_43_osethostname, 0, (sy_call_t *)compat_43_sys_sethostname },
	{ SYS_compat_43_osetrlimit, 0, (sy_call_t *)compat_43_sys_setrlimit },
	{ SYS_compat_43_osigblock, 0, (sy_call_t *)compat_43_sys_sigblock },
	{ SYS_compat_43_osigsetmask, 0, (sy_call_t *)compat_43_sys_sigsetmask },
	{ SYS_compat_43_osigstack, 0, (sy_call_t *)compat_43_sys_sigstack },
	{ SYS_compat_43_osigvec, 0, (sy_call_t *)compat_43_sys_sigvec },
	{ SYS_compat_43_otruncate, 0, (sy_call_t *)compat_43_sys_truncate },
	{ SYS_compat_43_owait, 0, (sy_call_t *)compat_43_sys_wait },
	{ SYS_compat_43_stat43, 0, (sy_call_t *)compat_43_sys_stat },
#endif

#if defined(COMPAT_09)
	{ SYS_compat_09_ogetdomainname, 0, (sy_call_t *)compat_09_sys_getdomainname },
	{ SYS_compat_09_osetdomainname, 0, (sy_call_t *)compat_09_sys_setdomainname },
	{ SYS_compat_09_ouname, 0, (sy_call_t *)compat_09_sys_uname },
#endif

#if defined(COMPAT_12)
	{ SYS_compat_12_fstat12, 0, (sy_call_t *)compat_12_sys_fstat },
	{ SYS_compat_12_getdirentries, 0, (sy_call_t *)compat_12_sys_getdirentries },
	{ SYS_compat_12_lstat12, 0, (sy_call_t *)compat_12_sys_lstat },
	{ SYS_compat_12_msync, 0, (sy_call_t *)compat_12_sys_msync },
	{ SYS_compat_12_oreboot, 0, (sy_call_t *)compat_12_sys_reboot },
	{ SYS_compat_12_oswapon, 0, (sy_call_t *)compat_12_sys_swapon },
	{ SYS_compat_12_stat12, 0, (sy_call_t *)compat_12_sys_stat },
#endif

	{ 0, 0, NULL },
};

struct compat_init_fini {
	int (*init)(void);
	int (*fini)(void);
} init_fini_list[] = {
#ifdef COMPAT_70
	{ compat_70_init, compat_70_fini },
#endif
#ifdef COMPAT_60
	{ compat_60_init, compat_60_fini },
#endif
#ifdef COMPAT_50
	{ compat_50_init, compat_50_fini },
#endif
#ifdef COMPAT_40
	{ compat_40_init, compat_40_fini },
#endif
#ifdef COMPAT_30
	{ compat_30_init, compat_30_fini },
#endif
#ifdef COMPAT_20
	{ compat_20_init, compat_20_fini },
#endif
#ifdef COMPAT_16
	{ compat_16_init, compat_16_fini },
#endif
#ifdef COMPAT_14
	{ compat_14_init, compat_14_fini },
#endif
#ifdef COMPAT_13
	{ compat_13_init, compat_13_fini },
#endif
#if 0	/* NOT YET */
#ifdef COMPAT_12
	{ compat_12_init, compat_12_fini },
#endif
#ifdef COMPAT_10
	{ compat_10_init, compat_10_fini },
#endif
#ifdef COMPAT_09
	{ compat_09_init, compat_09_fini },
#endif
#ifdef COMPAT_43
	{ compat_43_init, compat_43_fini },
#endif
#endif /* 0 NOT YET */
};

static int
compat_modcmd(modcmd_t cmd, void *arg)
{
	int error;
	int i, j;

	switch (cmd) {
	case MODULE_CMD_INIT:
		/*
		 * Call the init() routine for all components;  if
		 * any component fails, disable (via fini() routine)
		 * those which had already been disabled before we
		 * return to prevent partial module initialization.
		 */
		for (i = 0; i < __arraycount(init_fini_list); i++) {
			error = (*init_fini_list[i].init)();
			if (error != 0) {
				for (j = i - 1; j >= 0; j--) {
					(*init_fini_list[j].fini)();
				}
				return error;
			}
		}

		error = syscall_establish(NULL, compat_syscalls);
		if (error != 0) {
			return error;
		}
#ifdef NOTYET /* XXX */
#ifdef COMPAT_43
/* XXX
 * XXX This would mean that compat_43 and compat_60 are mutually
 * XXX exclusive.  Rather we should save the original vector
 * XXX value, and retore it if we unload.  Note that compat_43
 * XXX should "require" compat_60 to provide a definitive order
 * XXX of initialization (ie, compat_60 first, then compat_43).
 * XXX
		KASSERT(ttcompatvec == NULL);
		ttcompatvec = ttcompat;
		if_43_init();
 * XXX */
#endif
#endif /* XXX NOTYET */
#ifdef COMPAT_10
		vfs_syscalls_10_init();
#endif
		compat_sysctl_init();
		return 0;

	case MODULE_CMD_FINI:
#ifdef COMPAT_10
		vfs_syscalls_10_fini();
#endif
		/* Unlink the system calls. */
		error = syscall_disestablish(NULL, compat_syscalls);
		if (error != 0) {
			return error;
		}
#ifdef NOTYET /* XXX */
#ifdef COMPAT_43
		/* Unlink ttcompatvec. */
		if (rw_tryenter(&ttcompat_lock, RW_WRITER)) {
			ttcompatvec = NULL;
			rw_exit(&ttcompat_lock);
		} else {
			error = syscall_establish(NULL, compat_syscalls);
			KASSERT(error == 0);
			return EBUSY;
		}
#endif
#endif /* NOTYET XXX */
		/*
		 * Disable included components in reverse order;
		 * if any component fails to fini(), re-init those
		 * components which had already been disabled
		 */
		compat_sysctl_fini();
		for (i = __arraycount(init_fini_list) - 1; i >= 0; i--) {
			error = (*init_fini_list[i].fini)();
			if (error != 0) {
				for (j = i + 1;
				     j < __arraycount(init_fini_list); j++) {
					(*init_fini_list[j].init)();
				}
				return error;
			}
		}

		return 0;

	default:
		return ENOTTY;
	}
}

void
compat_sysctl_init(void)
{

#if defined(COMPAT_09) || defined(COMPAT_43)
	compat_sysctl_vfs(&compat_clog);
#endif
}

void
compat_sysctl_fini(void)
{
 
#if defined(COMPAT_09) || defined(COMPAT_43)
        sysctl_teardown(&compat_clog);
#endif
#if defined(COMPAT_43)
	if_43_fini();
#endif
}
