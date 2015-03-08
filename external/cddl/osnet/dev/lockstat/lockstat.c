/*	$NetBSD: lockstat.c,v 1.4 2015/03/08 04:13:46 christos Exp $	*/

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
 * Portions Copyright (c) 2008-2009 Stacey Son <sson@FreeBSD.org> 
 *
 * $FreeBSD: src/sys/cddl/dev/lockstat/lockstat.c,v 1.2.2.1 2009/08/03 08:13:06 kensmith Exp $
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
#include <sys/kernel.h>
#include <sys/limits.h>
#include <sys/lock.h>
#include <sys/linker.h>
#include <sys/module.h>
#include <sys/mutex.h>
#ifdef __NetBSD__
#include <sys/atomic.h>
#include <sys/xcall.h>
#endif

#include <sys/dtrace.h>
#ifdef __NetBSD__
#include <dev/lockstat.h>
#else
#include <sys/lockstat.h>
#endif

#ifdef __NetBSD__
#define	ASSERT	KASSERT
#endif

#if defined(__i386__) || defined(__amd64__) || defined(__arm__)
#define LOCKSTAT_AFRAMES 1
#else
#error "architecture not supported"
#endif

#if defined(__FreeBSD__)
static d_open_t lockstat_open;
#elif defined(__NetBSD__) && 0
static dev_type_open(lockstat_open);
#endif
static void	lockstat_provide(void *, const dtrace_probedesc_t *);
static void	lockstat_destroy(void *, dtrace_id_t, void *);
static int	lockstat_enable(void *, dtrace_id_t, void *);
static void	lockstat_disable(void *, dtrace_id_t, void *);
static void	lockstat_load(void *);
static int	lockstat_unload(void);


typedef struct lockstat_probe {
	const char	*lsp_func;
	const char	*lsp_name;
	int		lsp_probe;
	dtrace_id_t	lsp_id;
#ifdef __FreeBSD__
	int		lsp_frame;
#endif
} lockstat_probe_t;

#if defined(__FreeBSD__)
lockstat_probe_t lockstat_probes[] =
{
  /* Spin Locks */
  { LS_MTX_SPIN_LOCK,	LSS_ACQUIRE,	LS_MTX_SPIN_LOCK_ACQUIRE,
	  DTRACE_IDNONE, LOCKSTAT_AFRAMES },
  { LS_MTX_SPIN_LOCK, 	LSS_SPIN,	LS_MTX_SPIN_LOCK_SPIN,
	  DTRACE_IDNONE, LOCKSTAT_AFRAMES },
  { LS_MTX_SPIN_UNLOCK,	LSS_RELEASE,	LS_MTX_SPIN_UNLOCK_RELEASE,
	  DTRACE_IDNONE, LOCKSTAT_AFRAMES },
  /* Adaptive Locks */
  { LS_MTX_LOCK,	LSA_ACQUIRE,	LS_MTX_LOCK_ACQUIRE,
	  DTRACE_IDNONE, (LOCKSTAT_AFRAMES + 1) },
  { LS_MTX_LOCK,	LSA_BLOCK,	LS_MTX_LOCK_BLOCK,
	  DTRACE_IDNONE, (LOCKSTAT_AFRAMES + 1) },
  { LS_MTX_LOCK,	LSA_SPIN,	LS_MTX_LOCK_SPIN,
	  DTRACE_IDNONE, (LOCKSTAT_AFRAMES + 1) },
  { LS_MTX_UNLOCK,	LSA_RELEASE,	LS_MTX_UNLOCK_RELEASE,
	  DTRACE_IDNONE, LOCKSTAT_AFRAMES },
  { LS_MTX_TRYLOCK,	LSA_ACQUIRE,	LS_MTX_TRYLOCK_ACQUIRE,
	  DTRACE_IDNONE, LOCKSTAT_AFRAMES },
  /* Reader/Writer Locks */
  { LS_RW_RLOCK,	LSR_ACQUIRE,	LS_RW_RLOCK_ACQUIRE,
	  DTRACE_IDNONE, LOCKSTAT_AFRAMES },
  { LS_RW_RLOCK,	LSR_BLOCK,	LS_RW_RLOCK_BLOCK,
	  DTRACE_IDNONE, LOCKSTAT_AFRAMES },
  { LS_RW_RLOCK,	LSR_SPIN,	LS_RW_RLOCK_SPIN,
	  DTRACE_IDNONE, LOCKSTAT_AFRAMES },
  { LS_RW_RUNLOCK,	LSR_RELEASE,	LS_RW_RUNLOCK_RELEASE,
	  DTRACE_IDNONE, LOCKSTAT_AFRAMES },
  { LS_RW_WLOCK,	LSR_ACQUIRE,	LS_RW_WLOCK_ACQUIRE,
	  DTRACE_IDNONE, LOCKSTAT_AFRAMES },
  { LS_RW_WLOCK,	LSR_BLOCK,	LS_RW_WLOCK_BLOCK,
	  DTRACE_IDNONE, LOCKSTAT_AFRAMES },
  { LS_RW_WLOCK,	LSR_SPIN,	LS_RW_WLOCK_SPIN,
	  DTRACE_IDNONE, LOCKSTAT_AFRAMES },
  { LS_RW_WUNLOCK,	LSR_RELEASE,	LS_RW_WUNLOCK_RELEASE,
	  DTRACE_IDNONE, LOCKSTAT_AFRAMES },
  { LS_RW_TRYUPGRADE,	LSR_UPGRADE,   	LS_RW_TRYUPGRADE_UPGRADE,
	  DTRACE_IDNONE, LOCKSTAT_AFRAMES },
  { LS_RW_DOWNGRADE,	LSR_DOWNGRADE, 	LS_RW_DOWNGRADE_DOWNGRADE,
	  DTRACE_IDNONE, LOCKSTAT_AFRAMES },
  /* Shared/Exclusive Locks */
  { LS_SX_SLOCK,	LSX_ACQUIRE,	LS_SX_SLOCK_ACQUIRE,
	  DTRACE_IDNONE, LOCKSTAT_AFRAMES },
  { LS_SX_SLOCK,	LSX_BLOCK,	LS_SX_SLOCK_BLOCK,
	  DTRACE_IDNONE, LOCKSTAT_AFRAMES },
  { LS_SX_SLOCK,	LSX_SPIN,	LS_SX_SLOCK_SPIN,
	  DTRACE_IDNONE, LOCKSTAT_AFRAMES },
  { LS_SX_SUNLOCK,	LSX_RELEASE,	LS_SX_SUNLOCK_RELEASE,
	  DTRACE_IDNONE, LOCKSTAT_AFRAMES },
  { LS_SX_XLOCK,	LSX_ACQUIRE,	LS_SX_XLOCK_ACQUIRE,
	  DTRACE_IDNONE, LOCKSTAT_AFRAMES },
  { LS_SX_XLOCK,	LSX_BLOCK,	LS_SX_XLOCK_BLOCK,
	  DTRACE_IDNONE, LOCKSTAT_AFRAMES },
  { LS_SX_XLOCK,	LSX_SPIN,	LS_SX_XLOCK_SPIN,
	  DTRACE_IDNONE, LOCKSTAT_AFRAMES },
  { LS_SX_XUNLOCK,	LSX_RELEASE,	LS_SX_XUNLOCK_RELEASE,
	  DTRACE_IDNONE, LOCKSTAT_AFRAMES },
  { LS_SX_TRYUPGRADE,	LSX_UPGRADE,	LS_SX_TRYUPGRADE_UPGRADE,
	  DTRACE_IDNONE, LOCKSTAT_AFRAMES },
  { LS_SX_DOWNGRADE,	LSX_DOWNGRADE,	LS_SX_DOWNGRADE_DOWNGRADE,
	  DTRACE_IDNONE, LOCKSTAT_AFRAMES },
  /* Thread Locks */
  { LS_THREAD_LOCK,	LST_SPIN,	LS_THREAD_LOCK_SPIN,
	  DTRACE_IDNONE, LOCKSTAT_AFRAMES },
  { NULL }
};
#elif defined(__NetBSD__)
lockstat_probe_t lockstat_probes[] = {
	{ "mutex_spin", "spin",		LB_SPIN_MUTEX	| LB_SPIN,	0 },
	{ "mutex_adaptive", "sleep",	LB_SPIN_MUTEX	| LB_SLEEP1,	0 },
	{ "mutex_adaptive", "spin",	LB_SPIN_MUTEX	| LB_SPIN,	0 },
	{ "rwlock", "sleep_writer",	LB_RWLOCK	| LB_SLEEP1,	0 },
	{ "rwlock", "sleep_reader",	LB_RWLOCK	| LB_SLEEP2,	0 },
	{ "rwlock", "spin",		LB_RWLOCK	| LB_SPIN,	0 },
	{ "kernel", "spin",		LB_KERNEL_LOCK	| LB_SPIN,	0 },
	{ "lwp", "spin",		LB_NOPREEMPT	| LB_SPIN,	0 },
};
#else
#error "OS not supported"
#endif


#if defined(__FreeBSD__)
static struct cdevsw lockstat_cdevsw = {
	.d_version	= D_VERSION,
	.d_open		= lockstat_open,
	.d_name		= "lockstat",
};

static struct cdev		*lockstat_cdev; 
#elif defined(__NetBSD__) && 0
static struct cdevsw lockstat_cdevsw = {
	.d_open = lockstat_open,
	.d_close = noclose,
	.d_read = noread,
	.d_write = nowrite,
	.d_ioctl = noioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = nopoll,
	.d_mmap = nommap,
	.d_kqfilter = nokqfilter,
	.d_discard = nodiscard,
	.d_flag = D_OTHER,
};
#endif

static dtrace_provider_id_t 	lockstat_id;

/*ARGSUSED*/
static int
lockstat_enable(void *arg, dtrace_id_t id, void *parg)
{
	lockstat_probe_t *probe = parg;

	ASSERT(!lockstat_probemap[probe->lsp_probe]);
	if (lockstat_probe_func == lockstat_probe_stub) {
		lockstat_probe_func = dtrace_probe;
		membar_producer();
	} else {
		ASSERT(lockstat_probe_func == dtrace_probe);
	}
	lockstat_probemap[probe->lsp_probe] = id;

	return 0;
}

/*ARGSUSED*/
static void
lockstat_disable(void *arg, dtrace_id_t id, void *parg)
{
	lockstat_probe_t *probe = parg;
	int i;

	ASSERT(lockstat_probe_func == dtrace_probe);
	ASSERT(lockstat_probemap[probe->lsp_probe]);
	lockstat_probemap[probe->lsp_probe] = 0;
	membar_producer();

	/*
	 * See if we have any probes left enabled.
	 */
	for (i = 0; i < LS_NPROBES; i++) {
		if (lockstat_probemap[i]) {
			/*
			 * This probe is still enabled.  We don't need to deal
			 * with waiting for all threads to be out of the
			 * lockstat critical sections; just return.
			 */
			return;
		}
	}

	lockstat_probe_func = lockstat_probe_stub;

	/*
	 * Trigger some activity on all CPUs to make sure they're not
	 * in lockstat any more.
	 */
	xc_wait(xc_broadcast(0, (void *)nullop, NULL, NULL));
}

#if defined(__FreeBSD__)
/*ARGSUSED*/
static int
lockstat_open(struct cdev *dev __unused, int oflags __unused, 
	      int devtype __unused, struct thread *td __unused)
{
	return (0);
}
#elif defined(__NetBSD__) && 0
static int
lockstat_open(dev_t dev __unused, int flags __unused, int mode __unused,
    struct lwp *l __unused)
{

	return 0;
}
#endif

/*ARGSUSED*/
static void
lockstat_provide(void *arg, const dtrace_probedesc_t *desc)
{
	int i = 0;

	for (i = 0; lockstat_probes[i].lsp_func != NULL; i++) {
		lockstat_probe_t *probe = &lockstat_probes[i];

		if (dtrace_probe_lookup(lockstat_id, __UNCONST("kernel"),
			__UNCONST(probe->lsp_func), __UNCONST(probe->lsp_name))
		    != 0)
			continue;

		ASSERT(!probe->lsp_id);
#ifdef __FreeBSD__
		probe->lsp_id = dtrace_probe_create(lockstat_id,
		    "kernel", probe->lsp_func, probe->lsp_name,
		    probe->lsp_frame, probe);
#else
		probe->lsp_id = dtrace_probe_create(lockstat_id,
		    __UNCONST("kernel"), __UNCONST(probe->lsp_func),
		    __UNCONST(probe->lsp_name), LOCKSTAT_AFRAMES, probe);
#endif
	}
}

/*ARGSUSED*/
static void
lockstat_destroy(void *arg, dtrace_id_t id, void *parg)
{
	lockstat_probe_t *probe = parg;

	ASSERT(!lockstat_probemap[probe->lsp_probe]);
	probe->lsp_id = 0;
}

static dtrace_pattr_t lockstat_attr = {
{ DTRACE_STABILITY_EVOLVING, DTRACE_STABILITY_EVOLVING, DTRACE_CLASS_COMMON },
{ DTRACE_STABILITY_PRIVATE, DTRACE_STABILITY_PRIVATE, DTRACE_CLASS_UNKNOWN },
{ DTRACE_STABILITY_PRIVATE, DTRACE_STABILITY_PRIVATE, DTRACE_CLASS_UNKNOWN },
{ DTRACE_STABILITY_EVOLVING, DTRACE_STABILITY_EVOLVING, DTRACE_CLASS_COMMON },
{ DTRACE_STABILITY_EVOLVING, DTRACE_STABILITY_EVOLVING, DTRACE_CLASS_COMMON },
};

static dtrace_pops_t lockstat_pops = {
	lockstat_provide,
	NULL,
	lockstat_enable,
	lockstat_disable,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	lockstat_destroy
};

static void
lockstat_load(void *dummy)
{
#ifdef __FreeBSD__
	/* Create the /dev/dtrace/lockstat entry. */
	lockstat_cdev = make_dev(&lockstat_cdevsw, 0, UID_ROOT, GID_WHEEL, 0600,
	    "dtrace/lockstat");
#endif

	if (dtrace_register("lockstat", &lockstat_attr, DTRACE_PRIV_USER,
	    NULL, &lockstat_pops, NULL, &lockstat_id) != 0)
	        return;
}

static int
lockstat_unload(void)
{
	int error = 0;

	if ((error = dtrace_unregister(lockstat_id)) != 0)
	    return (error);

#ifdef __FreeBSD__
	destroy_dev(lockstat_cdev);
#endif

	return (error);
}

#if defined(__FreeBSD__)

/* ARGSUSED */
static int
lockstat_modevent(module_t mod __unused, int type, void *data __unused)
{
	int error = 0;

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

SYSINIT(lockstat_load, SI_SUB_DTRACE_PROVIDER, SI_ORDER_ANY, lockstat_load, NULL);
SYSUNINIT(lockstat_unload, SI_SUB_DTRACE_PROVIDER, SI_ORDER_ANY, lockstat_unload, NULL);

DEV_MODULE(lockstat, lockstat_modevent, NULL);
MODULE_VERSION(lockstat, 1);
MODULE_DEPEND(lockstat, dtrace, 1, 1, 1);
MODULE_DEPEND(lockstat, opensolaris, 1, 1, 1);

#elif defined(__NetBSD__)

static int
dtrace_lockstat_modcmd(modcmd_t cmd, void *data)
{
	switch (cmd) {
	case MODULE_CMD_INIT:
		lockstat_load(NULL);
		return 0;
	case MODULE_CMD_FINI:
		return lockstat_unload();
	case MODULE_CMD_AUTOUNLOAD:
		return EBUSY;
	default:
		return ENOTTY;
	}
}

MODULE(MODULE_CLASS_MISC, dtrace_lockstat, "dtrace");

#endif
