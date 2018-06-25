/*	$NetBSD: lockstat.c,v 1.8.14.1 2018/06/25 07:25:15 pgoyette Exp $	*/

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
 */
/*
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: lockstat.c,v 1.8.14.1 2018/06/25 07:25:15 pgoyette Exp $");

#include <sys/types.h>
#include <sys/proc.h>
#include <sys/param.h>
#include <sys/conf.h>
#include <sys/dtrace.h>
#include <sys/module.h>
#include <sys/mutex.h>
#include <sys/systm.h>
#include <sys/xcall.h>
#include <sys/atomic.h>

#define NLOCKSTAT 1
#include <dev/lockstat.h>

typedef struct lockstat_probe {
	const char	*lsp_func;
	const char	*lsp_name;
	int		lsp_probe;
	dtrace_id_t	lsp_id;
} lockstat_probe_t;

lockstat_probe_t lockstat_probes[] = {
	{ "mutex_spin", "spin",		LB_SPIN_MUTEX	| LB_SPIN,	0 },
	{ "mutex_adaptive", "sleep",	LB_ADAPTIVE_MUTEX | LB_SLEEP1,	0 },
	{ "mutex_adaptive", "spin",	LB_ADAPTIVE_MUTEX | LB_SPIN,	0 },
	{ "rwlock", "sleep_writer",	LB_RWLOCK	| LB_SLEEP1,	0 },
	{ "rwlock", "sleep_reader",	LB_RWLOCK	| LB_SLEEP2,	0 },
	{ "rwlock", "spin",		LB_RWLOCK	| LB_SPIN,	0 },
	{ "kernel", "spin",		LB_KERNEL_LOCK	| LB_SPIN,	0 },
	{ "lwp", "spin",		LB_NOPREEMPT	| LB_SPIN,	0 },
	{ NULL, NULL,			0,				0 },
};

static dtrace_provider_id_t lockstat_id;
static size_t lockstat_dtrace_count;

/*ARGSUSED*/
static int
lockstat_enable(void *arg, dtrace_id_t id, void *parg)
{
	lockstat_probe_t *probe = parg;

	ASSERT(!lockstat_probemap[LS_COMPRESS(probe->lsp_probe)]);

	lockstat_probemap[LS_COMPRESS(probe->lsp_probe)] = id;
	if (lockstat_dtrace_count++ == 0) {
		lockstat_dtrace_enabled = LB_DTRACE;
		LOCKSTAT_ENABLED_UPDATE();
	}

	return 0;
}

/*ARGSUSED*/
static void
lockstat_disable(void *arg, dtrace_id_t id __unused, void *parg)
{
	lockstat_probe_t *probe = parg;

	ASSERT(lockstat_probemap[LS_COMPRESS(probe->lsp_probe)]);

	if (--lockstat_dtrace_count == 0) {
		lockstat_dtrace_enabled = 0;
		LOCKSTAT_ENABLED_UPDATE();
	}

	lockstat_probemap[LS_COMPRESS(probe->lsp_probe)] = 0;
}

/*ARGSUSED*/
static void
lockstat_provide(void *arg, dtrace_probedesc_t *desc)
{
	int i = 0;

	for (i = 0; lockstat_probes[i].lsp_func != NULL; i++) {
		lockstat_probe_t *probe = &lockstat_probes[i];

		if (dtrace_probe_lookup(lockstat_id, __UNCONST("kernel"),
			__UNCONST(probe->lsp_func), __UNCONST(probe->lsp_name))
		    != 0)
			continue;

		ASSERT(!probe->lsp_id);
		probe->lsp_id = dtrace_probe_create(lockstat_id,
		    __UNCONST("kernel"), __UNCONST(probe->lsp_func),
		    __UNCONST(probe->lsp_name), 1, probe);
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
lockstat_barrier_xc(void *arg0 __unused, void *arg1 __unused)
{

	membar_consumer();
}

typedef void (*dtrace_probe_func_t)(dtrace_id_t, uintptr_t, uintptr_t,
    uintptr_t, uintptr_t, uintptr_t);

static bool
lockstat_cas_probe(dtrace_probe_func_t old, dtrace_probe_func_t new)
{

	ASSERT(kernconfig_is_held());

	if (lockstat_probe_func != old)
		return false;

	lockstat_probe_func = new;
	membar_producer();
	xc_wait(xc_broadcast(0, lockstat_barrier_xc, NULL, NULL));
	return true;
}

static int
lockstat_init(void)
{
	int error;
	bool ok;

	/* Install the probe function.  */
	ok = lockstat_cas_probe(lockstat_probe_stub, dtrace_probe);
	if (!ok) {
		printf("dtrace_lockstat: lockstat probe already installed\n");
		error = EEXIST;
		goto fail0;
	}

	/* Everything is in place.  Register a dtrace provider.  */
	ASSERT(lockstat_id == 0);
	error = dtrace_register("lockstat", &lockstat_attr, DTRACE_PRIV_USER,
	    NULL, &lockstat_pops, NULL, &lockstat_id);
	if (error) {
		printf("dtrace_lockstat: failed to register dtrace provider"
		    ": %d\n", error);
		goto fail1;
	}
	/* Success!  */
	return 0;

fail1:	ok = lockstat_cas_probe(dtrace_probe, lockstat_probe_stub);
	ASSERT(ok);
fail0:	ASSERT(error);
	return error;
}

static int
lockstat_fini(void)
{
	int error;
	bool ok __diagused;

	error = dtrace_unregister(lockstat_id);
	if (error) {
		printf("dtrace_lockstat: failed to unregister dtrace provider"
		    ": %d\n",
		    error);
		return error;
	}

	/* Unhook the probe.  */
	ok = lockstat_cas_probe(dtrace_probe, lockstat_probe_stub);
	ASSERT(ok);

	/* Success!  */
	return 0;
}

static int
dtrace_lockstat_modcmd(modcmd_t cmd, void *data)
{

	switch (cmd) {
	case MODULE_CMD_INIT:
		return lockstat_init();
	case MODULE_CMD_FINI:
		return lockstat_fini();
	default:
		return ENOTTY;
	}
}

MODULE(MODULE_CLASS_MISC, dtrace_lockstat, "dtrace");
