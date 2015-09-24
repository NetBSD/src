/*	$NetBSD: systrace.c,v 1.8 2015/09/24 14:26:44 christos Exp $	*/

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
 * $FreeBSD: src/sys/cddl/dev/systrace/systrace.c,v 1.2.2.1 2009/08/03 08:13:06 kensmith Exp $
 *
 */

/*
 * Copyright 2006 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/cpuvar.h>
#include <sys/fcntl.h>
#include <sys/filio.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/kthread.h>
#include <sys/limits.h>
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

#include "emultrace.h"

#define	CONCAT(x,y)	__CONCAT(x,y)
#define	STRING(s)	__STRING(s)

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
static void	systrace_provide(void *, const dtrace_probedesc_t *);
static void	systrace_destroy(void *, dtrace_id_t, void *);
static int	systrace_enable(void *, dtrace_id_t, void *);
static void	systrace_disable(void *, dtrace_id_t, void *);
static void	systrace_load(void *);

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
	NULL,
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

static void
systrace_provide(void *arg, const dtrace_probedesc_t *desc)
{
	int i;

	if (desc != NULL)
		return;

	for (i = 0; i < MAXSYSCALL; i++) {
		const char *name = ALTSYSCALLNAMES[i] ? ALTSYSCALLNAMES[i] :
		    SYSCALLNAMES[i];
		if (dtrace_probe_lookup(systrace_id, NULL, name, "entry") != 0)
			continue;

		(void) dtrace_probe_create(systrace_id, NULL,
		    name, "entry", SYSTRACE_ARTIFICIAL_FRAMES,
		    (void *)(intptr_t)SYSTRACE_ENTRY(i));
		(void) dtrace_probe_create(systrace_id, NULL,
		    name, "return", SYSTRACE_ARTIFICIAL_FRAMES,
		    (void *)(intptr_t)SYSTRACE_RETURN(i));
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
	if (dtrace_register(PROVNAME, &systrace_attr, DTRACE_PRIV_USER,
	    NULL, &systrace_pops, NULL, &systrace_id) != 0)
		return;

	EMUL.e_dtrace_syscall = systrace_probe;
}


static int
systrace_unload()
{
	int error;

	if ((error = dtrace_unregister(systrace_id)) != 0)
		return (error);

	EMUL.e_dtrace_syscall = NULL;

	return error;
}

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
