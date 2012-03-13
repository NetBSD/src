/*	$NetBSD: npf.c,v 1.10 2012/03/13 18:40:59 elad Exp $	*/

/*-
 * Copyright (c) 2009-2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This material is based upon work partially supported by The
 * NetBSD Foundation under a contract with Mindaugas Rasiukevicius.
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
 * NPF main: dynamic load/initialisation and unload routines.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: npf.c,v 1.10 2012/03/13 18:40:59 elad Exp $");

#include <sys/param.h>
#include <sys/types.h>

#include <sys/atomic.h>
#include <sys/conf.h>
#include <sys/kauth.h>
#include <sys/kmem.h>
#include <sys/lwp.h>
#include <sys/module.h>
#include <sys/percpu.h>
#include <sys/rwlock.h>
#include <sys/socketvar.h>
#include <sys/uio.h>

#include "npf_impl.h"

/*
 * Module and device structures.
 */
MODULE(MODULE_CLASS_DRIVER, npf, NULL);

void		npfattach(int);

static int	npf_fini(void);
static int	npf_dev_open(dev_t, int, int, lwp_t *);
static int	npf_dev_close(dev_t, int, int, lwp_t *);
static int	npf_dev_ioctl(dev_t, u_long, void *, int, lwp_t *);
static int	npf_dev_poll(dev_t, int, lwp_t *);
static int	npf_dev_read(dev_t, struct uio *, int);

typedef struct {
	npf_ruleset_t *		n_rules;
	npf_tableset_t *	n_tables;
	npf_ruleset_t *		n_nat_rules;
	prop_dictionary_t	n_dict;
	bool			n_default_pass;
} npf_core_t;

static void	npf_core_destroy(npf_core_t *);
static int	npfctl_stats(void *);

static krwlock_t		npf_lock		__cacheline_aligned;
static npf_core_t *		npf_core		__cacheline_aligned;
static percpu_t *		npf_stats_percpu	__read_mostly;

const struct cdevsw npf_cdevsw = {
	npf_dev_open, npf_dev_close, npf_dev_read, nowrite, npf_dev_ioctl,
	nostop, notty, npf_dev_poll, nommap, nokqfilter, D_OTHER | D_MPSAFE
};

static int
npf_init(void)
{
#ifdef _MODULE
	devmajor_t bmajor = NODEVMAJOR, cmajor = NODEVMAJOR;
#endif
	npf_ruleset_t *rset, *nset;
	npf_tableset_t *tset;
	prop_dictionary_t dict;
	int error = 0;

	rw_init(&npf_lock);
	npf_stats_percpu = percpu_alloc(NPF_STATS_SIZE);
	npf_tableset_sysinit();
	npf_session_sysinit();
	npf_nat_sysinit();
	npf_alg_sysinit();
	npflogattach(1);

	/* Load empty configuration. */
	dict = prop_dictionary_create();
	rset = npf_ruleset_create();
	tset = npf_tableset_create();
	nset = npf_ruleset_create();
	npf_reload(dict, rset, tset, nset, true);
	KASSERT(npf_core != NULL);

#ifdef _MODULE
	/* Attach /dev/npf device. */
	error = devsw_attach("npf", NULL, &bmajor, &npf_cdevsw, &cmajor);
	if (error) {
		/* It will call devsw_detach(), which is safe. */
		(void)npf_fini();
	}
#endif
	return error;
}

static int
npf_fini(void)
{

	/* At first, detach device and remove pfil hooks. */
#ifdef _MODULE
	devsw_detach(NULL, &npf_cdevsw);
#endif
	npflogdetach();
	npf_pfil_unregister();

	/* Flush all sessions, destroy configuration (ruleset, etc). */
	npf_session_tracking(false);
	npf_core_destroy(npf_core);

	/* Finally, safe to destroy the subsystems. */
	npf_alg_sysfini();
	npf_nat_sysfini();
	npf_session_sysfini();
	npf_tableset_sysfini();
	percpu_free(npf_stats_percpu, NPF_STATS_SIZE);
	rw_destroy(&npf_lock);

	return 0;
}

/*
 * Module interface.
 */
static int
npf_modcmd(modcmd_t cmd, void *arg)
{

	switch (cmd) {
	case MODULE_CMD_INIT:
		return npf_init();
	case MODULE_CMD_FINI:
		return npf_fini();
	default:
		return ENOTTY;
	}
	return 0;
}

void
npfattach(int nunits)
{

	/* Void. */
}

static int
npf_dev_open(dev_t dev, int flag, int mode, lwp_t *l)
{

	/* Available only for super-user. */
	if (kauth_authorize_network(l->l_cred, KAUTH_NETWORK_FIREWALL,
	    KAUTH_REQ_NETWORK_FIREWALL_FW, NULL, NULL, NULL)) {
		return EPERM;
	}
	return 0;
}

static int
npf_dev_close(dev_t dev, int flag, int mode, lwp_t *l)
{

	return 0;
}

static int
npf_dev_ioctl(dev_t dev, u_long cmd, void *data, int flag, lwp_t *l)
{
	int error;

	/* Available only for super-user. */
	if (kauth_authorize_network(l->l_cred, KAUTH_NETWORK_FIREWALL,
	    KAUTH_REQ_NETWORK_FIREWALL_FW, NULL, NULL, NULL)) {
		return EPERM;
	}

	switch (cmd) {
	case IOC_NPF_VERSION:
		*(int *)data = NPF_VERSION;
		error = 0;
		break;
	case IOC_NPF_SWITCH:
		error = npfctl_switch(data);
		break;
	case IOC_NPF_RELOAD:
		error = npfctl_reload(cmd, data);
		break;
	case IOC_NPF_GETCONF:
		error = npfctl_getconf(cmd, data);
		break;
	case IOC_NPF_TABLE:
		error = npfctl_table(data);
		break;
	case IOC_NPF_STATS:
		error = npfctl_stats(data);
		break;
	case IOC_NPF_SESSIONS_SAVE:
		error = npfctl_sessions_save(cmd, data);
		break;
	case IOC_NPF_SESSIONS_LOAD:
		error = npfctl_sessions_load(cmd, data);
		break;
	case IOC_NPF_UPDATE_RULE:
		error = npfctl_update_rule(cmd, data);
		break;
	default:
		error = ENOTTY;
		break;
	}
	return error;
}

static int
npf_dev_poll(dev_t dev, int events, lwp_t *l)
{

	return ENOTSUP;
}

static int
npf_dev_read(dev_t dev, struct uio *uio, int flag)
{

	return ENOTSUP;
}

/*
 * NPF core loading/reloading/unloading mechanism.
 */

static void
npf_core_destroy(npf_core_t *nc)
{

	prop_object_release(nc->n_dict);
	npf_ruleset_destroy(nc->n_rules);
	npf_ruleset_destroy(nc->n_nat_rules);
	npf_tableset_destroy(nc->n_tables);
	kmem_free(nc, sizeof(npf_core_t));
}

/*
 * npf_reload: atomically load new ruleset, tableset and NAT policies.
 * Then destroy old (unloaded) structures.
 */
void
npf_reload(prop_dictionary_t dict, npf_ruleset_t *rset,
    npf_tableset_t *tset, npf_ruleset_t *nset, bool flush)
{
	npf_core_t *nc, *onc;

	/* Setup a new core structure. */
	nc = kmem_zalloc(sizeof(npf_core_t), KM_SLEEP);
	nc->n_rules = rset;
	nc->n_tables = tset;
	nc->n_nat_rules = nset;
	nc->n_dict = dict;
	nc->n_default_pass = flush;

	/* Lock and load the core structure. */
	rw_enter(&npf_lock, RW_WRITER);
	onc = atomic_swap_ptr(&npf_core, nc);
	if (onc) {
		/* Reload only necessary NAT policies. */
		npf_ruleset_natreload(nset, onc->n_nat_rules);
	}
	/* Unlock.  Everything goes "live" now. */
	rw_exit(&npf_lock);

	if (onc) {
		/* Destroy unloaded structures. */
		npf_core_destroy(onc);
	}
}

void
npf_core_enter(void)
{
	rw_enter(&npf_lock, RW_READER);
}

npf_ruleset_t *
npf_core_ruleset(void)
{
	KASSERT(rw_lock_held(&npf_lock));
	return npf_core->n_rules;
}

npf_ruleset_t *
npf_core_natset(void)
{
	KASSERT(rw_lock_held(&npf_lock));
	return npf_core->n_nat_rules;
}

npf_tableset_t *
npf_core_tableset(void)
{
	KASSERT(rw_lock_held(&npf_lock));
	return npf_core->n_tables;
}

void
npf_core_exit(void)
{
	rw_exit(&npf_lock);
}

bool
npf_core_locked(void)
{
	return rw_lock_held(&npf_lock);
}

prop_dictionary_t
npf_core_dict(void)
{
	KASSERT(rw_lock_held(&npf_lock));
	return npf_core->n_dict;
}

bool
npf_default_pass(void)
{
	KASSERT(rw_lock_held(&npf_lock));
	return npf_core->n_default_pass;
}

/*
 * NPF statistics interface.
 */

void
npf_stats_inc(npf_stats_t st)
{
	uint64_t *stats = percpu_getref(npf_stats_percpu);
	stats[st]++;
	percpu_putref(npf_stats_percpu);
}

void
npf_stats_dec(npf_stats_t st)
{
	uint64_t *stats = percpu_getref(npf_stats_percpu);
	stats[st]--;
	percpu_putref(npf_stats_percpu);
}

static void
npf_stats_collect(void *mem, void *arg, struct cpu_info *ci)
{
	uint64_t *percpu_stats = mem, *full_stats = arg;
	int i;

	for (i = 0; i < NPF_STATS_COUNT; i++) {
		full_stats[i] += percpu_stats[i];
	}
}

/*
 * npfctl_stats: export collected statistics.
 */
static int
npfctl_stats(void *data)
{
	uint64_t *fullst, *uptr = *(uint64_t **)data;
	int error;

	fullst = kmem_zalloc(NPF_STATS_SIZE, KM_SLEEP);
	percpu_foreach(npf_stats_percpu, npf_stats_collect, fullst);
	error = copyout(fullst, uptr, NPF_STATS_SIZE);
	kmem_free(fullst, NPF_STATS_SIZE);
	return error;
}
