/*	$NetBSD: systrace.c,v 1.9.12.1 2018/06/25 07:25:15 pgoyette Exp $	*/

/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 *
 * Portions Copyright 2006-2008 John Birrell jb@freebsd.org
 *
 */

/*
 * Copyright 2006 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#include <sys/cdefs.h>
/* __FBSDID("$FreeBSD: head/sys/cddl/dev/systrace/systrace.c 306220 2016-09-22 23:22:53Z markj $"); */

#include <sys/proc.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/cpuvar.h>
#include <sys/fcntl.h>
#include <sys/filio.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/kthread.h>
#include <sys/syslimits.h>
#include <sys/linker.h>
#include <sys/lock.h>
#include <sys/malloc.h>
#include <sys/module.h>
#include <sys/mutex.h>
#include <sys/poll.h>
#include <sys/proc.h>
#include <sys/selinfo.h>
#include <sys/syscallargs.h>
#include <sys/uio.h>
#include <sys/unistd.h>

#include <sys/dtrace.h>
#include "dtrace_cddl.h"

#include "emultrace.h"

#define	CONCAT(x,y)	__CONCAT(x,y)
#define	STRING(s)	__STRING(s)

#ifdef __FreeBSD__
#ifdef LINUX_SYSTRACE
#if defined(__amd64__)
#include <amd64/linux/linux.h>
#include <amd64/linux/linux_proto.h>
#include <amd64/linux/linux_syscalls.c>
#include <amd64/linux/linux_systrace_args.c>
#elif defined(__i386__)
#include <i386/linux/linux.h>
#include <i386/linux/linux_proto.h>
#include <i386/linux/linux_syscalls.c>
#include <i386/linux/linux_systrace_args.c>
#else
#error Only i386 and amd64 are supported.
#endif
#define	MODNAME		"linux"
extern struct sysent linux_sysent[];
#define	MAXSYSCALL	LINUX_SYS_MAXSYSCALL
#define	SYSCALLNAMES	linux_syscallnames
#define	SYSENT		linux_sysent
#elif defined(LINUX32_SYSTRACE)
#if defined(__amd64__)
#include <amd64/linux32/linux.h>
#include <amd64/linux32/linux32_proto.h>
#include <amd64/linux32/linux32_syscalls.c>
#include <amd64/linux32/linux32_systrace_args.c>
#else
#error Only amd64 is supported.
#endif
#define	MODNAME		"linux32"
extern struct sysent linux32_sysent[];
#define	MAXSYSCALL	LINUX32_SYS_MAXSYSCALL
#define	SYSCALLNAMES	linux32_syscallnames
#define	SYSENT		linux32_sysent
#elif defined(FREEBSD32_SYSTRACE)
/*
 * The syscall arguments are processed into a DTrace argument array
 * using a generated function. See sys/kern/makesyscalls.sh.
 */
#include <compat/freebsd32/freebsd32_proto.h>
#include <compat/freebsd32/freebsd32_util.h>
#include <compat/freebsd32/freebsd32_syscall.h>
#include <compat/freebsd32/freebsd32_systrace_args.c>
extern const char *freebsd32_syscallnames[];
#define	MODNAME		"freebsd32"
#define	MAXSYSCALL	FREEBSD32_SYS_MAXSYSCALL
#define	SYSCALLNAMES	freebsd32_syscallnames
#define	SYSENT		freebsd32_sysent
#else
/*
 * The syscall arguments are processed into a DTrace argument array
 * using a generated function. See sys/kern/makesyscalls.sh.
 */
#include <sys/syscall.h>
#include <kern/systrace_args.c>
#define	MODNAME		"freebsd"
#define	MAXSYSCALL	SYS_MAXSYSCALL
#define	SYSCALLNAMES	syscallnames
#define	SYSENT		sysent
#define	NATIVE_ABI
#endif

#define	PROVNAME	"syscall"
#define	DEVNAME	        "dtrace/systrace/" MODNAME
#endif /* __FreeBSD__ */

#ifdef __NetBSD__
#include <sys/syscallargs.h>

#ifndef NATIVE
extern const char	* const CONCAT(emulname,_syscallnames)[];
extern const char	* const CONCAT(alt,CONCAT(emulname,_syscallnames))[];
extern 	struct sysent 	CONCAT(emulname,_sysent)[];
#define	MODNAME		CONCAT(dtrace_syscall_,emulname)
#define	MODDEP		"dtrace_syscall,compat_" STRING(emulname)
#define	MAXSYSCALL	CONCAT(EMULNAME,_SYS_MAXSYSCALL)
#define	SYSCALLNAMES	CONCAT(emulname,_syscallnames)
#define	ALTSYSCALLNAMES	CONCAT(alt,CONCAT(emulname,_syscallnames))
#define	SYSENT		CONCAT(emulname,_sysent)
#define	PROVNAME	STRING(emulname) "_syscall"
#else
extern const char	* const syscallnames[];
extern const char	* const altsyscallnames[];
#define	MODNAME		dtrace_syscall
#define	MODDEP		"dtrace"
#define	MAXSYSCALL	SYS_MAXSYSCALL
#define	SYSCALLNAMES	syscallnames
#define	ALTSYSCALLNAMES	altsyscallnames
#define	SYSENT		sysent
#define	PROVNAME	"syscall"
#endif

#define	MODCMD		CONCAT(MODNAME,_modcmd)
#define EMUL		CONCAT(emul_,emulname)
extern struct emul 	EMUL;
#define curthread	curlwp
#endif /* __NetBSD__ */

#define	SYSTRACE_ARTIFICIAL_FRAMES	1

#define	SYSTRACE_SHIFT			16
#define	SYSTRACE_ISENTRY(x)		((int)(x) >> SYSTRACE_SHIFT)
#define	SYSTRACE_SYSNUM(x)		((int)(x) & ((1 << SYSTRACE_SHIFT) - 1))
#define	SYSTRACE_ENTRY(id)		((1 << SYSTRACE_SHIFT) | (id))
#define	SYSTRACE_RETURN(id)		(id)

#if ((1 << SYSTRACE_SHIFT) <= MAXSYSCALL)
#error 1 << SYSTRACE_SHIFT must exceed number of system calls
#endif

static int	systrace_unload(void);
static void	systrace_getargdesc(void *, dtrace_id_t, void *, dtrace_argdesc_t *);
static uint64_t	systrace_getargval(void *, dtrace_id_t, void *, int, int);
static void	systrace_provide(void *, dtrace_probedesc_t *);
static void	systrace_destroy(void *, dtrace_id_t, void *);
static int	systrace_enable(void *, dtrace_id_t, void *);
static void	systrace_disable(void *, dtrace_id_t, void *);
static void	systrace_load(void *);

#ifdef __FreeBSD__
static union {
	const char	**p_constnames;
	char		**pp_syscallnames;
} uglyhack = { SYSCALLNAMES };
#endif

static dtrace_pattr_t systrace_attr = {
{ DTRACE_STABILITY_EVOLVING, DTRACE_STABILITY_EVOLVING, DTRACE_CLASS_COMMON },
{ DTRACE_STABILITY_PRIVATE, DTRACE_STABILITY_PRIVATE, DTRACE_CLASS_UNKNOWN },
{ DTRACE_STABILITY_PRIVATE, DTRACE_STABILITY_PRIVATE, DTRACE_CLASS_ISA },
{ DTRACE_STABILITY_EVOLVING, DTRACE_STABILITY_EVOLVING, DTRACE_CLASS_COMMON },
{ DTRACE_STABILITY_PRIVATE, DTRACE_STABILITY_PRIVATE, DTRACE_CLASS_ISA },
};

static dtrace_pops_t systrace_pops = {
	systrace_provide,
	NULL,
	systrace_enable,
	systrace_disable,
	NULL,
	NULL,
	systrace_getargdesc,
	systrace_getargval,
	NULL,
	systrace_destroy
};

static dtrace_provider_id_t	systrace_id;

/*
 * Probe callback function.
 *
 * Note: This function is called for _all_ syscalls, regardless of which sysent
 *       array the syscall comes from. It could be a standard syscall or a
 *       compat syscall from something like Linux.
 */
#ifdef __FreeBSD__
#ifdef NATIVE_ABI
static void
systrace_probe(struct syscall_args *sa, enum systrace_probe_t type, int retval)
{
	uint64_t uargs[nitems(sa->args)];
	dtrace_id_t id;
	int n_args, sysnum;

	sysnum = sa->code;
	memset(uargs, 0, sizeof(uargs));

	if (type == SYSTRACE_ENTRY) {
		if ((id = sa->callp->sy_entry) == DTRACE_IDNONE)
			return;

		if (sa->callp->sy_systrace_args_func != NULL)
			/*
			 * Convert the syscall parameters using the registered
			 * function.
			 */
			(*sa->callp->sy_systrace_args_func)(sysnum, sa->args,
			    uargs, &n_args);
		else
			/*
			 * Use the built-in system call argument conversion
			 * function to translate the syscall structure fields
			 * into the array of 64-bit values that DTrace expects.
			 */
			systrace_args(sysnum, sa->args, uargs, &n_args);
		/*
		 * Save probe arguments now so that we can retrieve them if
		 * the getargval method is called from further down the stack.
		 */
		curthread->t_dtrace_systrace_args = uargs;
	} else {
		if ((id = sa->callp->sy_return) == DTRACE_IDNONE)
			return;

		curthread->t_dtrace_systrace_args = NULL;
		/* Set arg0 and arg1 as the return value of this syscall. */
		uargs[0] = uargs[1] = retval;
	}

	/* Process the probe using the converted argments. */
	dtrace_probe(id, uargs[0], uargs[1], uargs[2], uargs[3], uargs[4]);
}
#endif /* NATIVE_ABI */
#endif /* __FreeBSD__ */

#ifdef __NetBSD__
static void
systrace_probe(uint32_t id, register_t sysnum, const struct sysent *se,
    const void *params, const register_t *ret, int error)
{
	size_t		n_args	= 0;
	uintptr_t	uargs[SYS_MAXSYSARGS + 3];

	memset(uargs, 0, sizeof(uargs));
	if (ret == NULL) {
		/* entry syscall, convert params */
		systrace_args(sysnum, params, uargs, &n_args);
	} else {
		/* return syscall, set values and params: */
		uargs[0] = ret[0];
		uargs[1] = ret[1];
		uargs[2] = error;
		systrace_args(sysnum, params, uargs + 3, &n_args);
	}
	/* Process the probe using the converted argments. */
	/* XXX: fix for more arguments! */
	dtrace_probe(id, uargs[0], uargs[1], uargs[2], uargs[3], uargs[4]);
}
#endif

static void
systrace_getargdesc(void *arg, dtrace_id_t id, void *parg, dtrace_argdesc_t *desc)
{
	int sysnum = SYSTRACE_SYSNUM((uintptr_t)parg);

	if (SYSTRACE_ISENTRY((uintptr_t)parg))
		systrace_entry_setargdesc(sysnum, desc->dtargd_ndx,
		    desc->dtargd_native, sizeof(desc->dtargd_native));
	else
		systrace_return_setargdesc(sysnum, desc->dtargd_ndx,
		    desc->dtargd_native, sizeof(desc->dtargd_native));

	if (desc->dtargd_native[0] == '\0')
		desc->dtargd_ndx = DTRACE_ARGNONE;
}

static uint64_t
systrace_getargval(void *arg, dtrace_id_t id, void *parg, int argno, int aframes)
{
	uint64_t *uargs;

	uargs = curthread->t_dtrace_systrace_args;
	if (uargs == NULL)
		/* This is a return probe. */
		return (0);
#ifdef __FreeBSD__
	if (argno >= nitems(((struct syscall_args *)NULL)->args))
		return (0);
#endif
#ifdef __NetBSD__
	if (argno >= SYS_MAXSYSARGS)
		return (0);
#endif

	return (uargs[argno]);
}

static void
systrace_provide(void *arg, dtrace_probedesc_t *desc)
{
	int i;

	if (desc != NULL)
		return;

	for (i = 0; i < MAXSYSCALL; i++) {
#ifdef __FreeBSD__
		if (dtrace_probe_lookup(systrace_id, MODNAME,
		    uglyhack.pp_syscallnames[i], "entry") != 0)
			continue;

		(void)dtrace_probe_create(systrace_id, MODNAME,
		    uglyhack.pp_syscallnames[i], "entry",
		    SYSTRACE_ARTIFICIAL_FRAMES,
		    (void *)((uintptr_t)SYSTRACE_ENTRY(i)));
		(void)dtrace_probe_create(systrace_id, MODNAME,
		    uglyhack.pp_syscallnames[i], "return",
		    SYSTRACE_ARTIFICIAL_FRAMES,
		    (void *)((uintptr_t)SYSTRACE_RETURN(i)));
#else
		const char *name = ALTSYSCALLNAMES[i] ? ALTSYSCALLNAMES[i] :
		    SYSCALLNAMES[i];
		if (dtrace_probe_lookup(systrace_id, NULL, __UNCONST(name), __UNCONST("entry")) != 0)
			continue;

		(void) dtrace_probe_create(systrace_id, NULL,
		    name, "entry", SYSTRACE_ARTIFICIAL_FRAMES,
		    (void *)(intptr_t)SYSTRACE_ENTRY(i));
		(void) dtrace_probe_create(systrace_id, NULL,
		    name, "return", SYSTRACE_ARTIFICIAL_FRAMES,
		    (void *)(intptr_t)SYSTRACE_RETURN(i));
#endif
	}
}

static void
systrace_destroy(void *arg, dtrace_id_t id, void *parg)
{
#ifdef DEBUG
	int sysnum = SYSTRACE_SYSNUM((uintptr_t)parg);

	/*
	 * There's nothing to do here but assert that we have actually been
	 * disabled.
	 */
	if (SYSTRACE_ISENTRY((uintptr_t)parg)) {
		ASSERT(sysent[sysnum].sy_entry == 0);
	} else {
		ASSERT(sysent[sysnum].sy_return == 0);
	}
#endif
}

static int
systrace_enable(void *arg, dtrace_id_t id, void *parg)
{
	int sysnum = SYSTRACE_SYSNUM((uintptr_t)parg);

#ifdef __FreeBSD__
	if (SYSENT[sysnum].sy_systrace_args_func == NULL)
		SYSENT[sysnum].sy_systrace_args_func = systrace_args;
#endif

	if (SYSTRACE_ISENTRY((uintptr_t)parg))
		SYSENT[sysnum].sy_entry = id;
	else
		SYSENT[sysnum].sy_return = id;

	return 0;
}

static void
systrace_disable(void *arg, dtrace_id_t id, void *parg)
{
	int sysnum = SYSTRACE_SYSNUM((uintptr_t)parg);

	SYSENT[sysnum].sy_entry = 0;
	SYSENT[sysnum].sy_return = 0;
}

static void
systrace_load(void *dummy)
{
	if (dtrace_register(PROVNAME, &systrace_attr, DTRACE_PRIV_USER, NULL,
	    &systrace_pops, NULL, &systrace_id) != 0)
		return;

#ifdef NATIVE_ABI
	systrace_probe_func = systrace_probe;
#endif
#ifdef __NetBSD__
	EMUL.e_dtrace_syscall = systrace_probe;
#endif
}


static int
systrace_unload()
{
	int error;

#ifdef NATIVE_ABI
	systrace_probe_func = NULL;
#endif
#ifdef __NetBSD__
	EMUL.e_dtrace_syscall = NULL;
#endif

	if ((error = dtrace_unregister(systrace_id)) != 0)
		return (error);

	return error;
}

#ifdef __FreeBSD__
static int
systrace_modevent(module_t mod __unused, int type, void *data __unused)
{
	int error;

	error = 0;
	switch (type) {
	case MOD_LOAD:
		break;

	case MOD_UNLOAD:
		break;

	case MOD_SHUTDOWN:
		break;

	default:
		error = EOPNOTSUPP;
		break;

	}
	return (error);
}

SYSINIT(systrace_load, SI_SUB_DTRACE_PROVIDER, SI_ORDER_ANY,
    systrace_load, NULL);
SYSUNINIT(systrace_unload, SI_SUB_DTRACE_PROVIDER, SI_ORDER_ANY,
    systrace_unload, NULL);

#ifdef LINUX_SYSTRACE
DEV_MODULE(systrace_linux, systrace_modevent, NULL);
MODULE_VERSION(systrace_linux, 1);
#ifdef __amd64__
MODULE_DEPEND(systrace_linux, linux64, 1, 1, 1);
#else
MODULE_DEPEND(systrace_linux, linux, 1, 1, 1);
#endif
MODULE_DEPEND(systrace_linux, dtrace, 1, 1, 1);
MODULE_DEPEND(systrace_linux, opensolaris, 1, 1, 1);
#elif defined(LINUX32_SYSTRACE)
DEV_MODULE(systrace_linux32, systrace_modevent, NULL);
MODULE_VERSION(systrace_linux32, 1);
MODULE_DEPEND(systrace_linux32, linux, 1, 1, 1);
MODULE_DEPEND(systrace_linux32, dtrace, 1, 1, 1);
MODULE_DEPEND(systrace_linux32, opensolaris, 1, 1, 1);
#elif defined(FREEBSD32_SYSTRACE)
DEV_MODULE(systrace_freebsd32, systrace_modevent, NULL);
MODULE_VERSION(systrace_freebsd32, 1);
MODULE_DEPEND(systrace_freebsd32, dtrace, 1, 1, 1);
MODULE_DEPEND(systrace_freebsd32, opensolaris, 1, 1, 1);
#else
DEV_MODULE(systrace, systrace_modevent, NULL);
MODULE_VERSION(systrace, 1);
MODULE_DEPEND(systrace, dtrace, 1, 1, 1);
MODULE_DEPEND(systrace, opensolaris, 1, 1, 1);
#endif
#endif /* __FreeBSD__ */

#ifdef __NetBSD__

static int
MODCMD(modcmd_t cmd, void *data)
{
	switch (cmd) {
	case MODULE_CMD_INIT:
		systrace_load(NULL);
		return 0;

	case MODULE_CMD_FINI:
		return systrace_unload();

	case MODULE_CMD_AUTOUNLOAD:
		return EBUSY;

	default:
		return ENOTTY;
	}
}

MODULE(MODULE_CLASS_MISC, MODNAME, MODDEP)

#endif /* __NetBSD__ */
