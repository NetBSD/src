/*-
 * Copyright (c) 2013-2020 The NetBSD Foundation, Inc.
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
 * NPF configuration loading mechanism.
 *
 * The main operations on the configuration are the following:
 * 1) Read access, primarily from the npf_packet_handler() function.
 * 2) Write access on a particular set, mainly rule or table updates.
 * 3) Deletion of the configuration after the reload operation.
 *
 * Synchronization
 *
 *	For the (1) case, EBR is used to allow concurrent access to
 *	the configuration set (ruleset, etc).  It guarantees that the
 *	configuration will not be destroyed while accessing it.
 *
 *	For the cases (2) and (3), mutual exclusion (npf_t::config_lock)
 *	is used with, when necessary, the writer-side barrier of EBR.
 */

#ifdef _KERNEL
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: npf_conf.c,v 1.18 2022/02/13 19:20:11 riastradh Exp $");

#include <sys/param.h>
#include <sys/types.h>

#include <sys/atomic.h>
#include <sys/kmem.h>
#include <sys/mutex.h>
#endif

#include "npf_impl.h"
#include "npf_conn.h"

void
npf_config_init(npf_t *npf)
{
	npf_config_t *nc;

	mutex_init(&npf->config_lock, MUTEX_DEFAULT, IPL_SOFTNET);
	nc = npf_config_create();

	/*
	 * Load an empty configuration.
	 */
	nc->ruleset = npf_ruleset_create(0);
	nc->nat_ruleset = npf_ruleset_create(0);
	nc->rule_procs = npf_rprocset_create();
	nc->tableset = npf_tableset_create(0);
	nc->default_pass = true;

	npf_config_load(npf, nc, NULL, true);
	KASSERT(npf->config != NULL);
}

npf_config_t *
npf_config_create(void)
{
	return kmem_zalloc(sizeof(npf_config_t), KM_SLEEP);
}

void
npf_config_destroy(npf_config_t *nc)
{
	/*
	 * Note: the rulesets must be destroyed first, in order to drop
	 * any references to the tableset.
	 */
	if (nc->ruleset) {
		npf_ruleset_destroy(nc->ruleset);
	}
	if (nc->nat_ruleset) {
		npf_ruleset_destroy(nc->nat_ruleset);
	}
	if (nc->rule_procs) {
		npf_rprocset_destroy(nc->rule_procs);
	}
	if (nc->tableset) {
		npf_tableset_destroy(nc->tableset);
	}
	kmem_free(nc, sizeof(npf_config_t));
}

void
npf_config_fini(npf_t *npf)
{
	npf_conndb_t *cd = npf_conndb_create();

	/* Flush the connections. */
	mutex_enter(&npf->config_lock);
	npf_conn_tracking(npf, false);
	npf_ebr_full_sync(npf->ebr);
	npf_conn_load(npf, cd, false);
	npf_ifmap_flush(npf);
	mutex_exit(&npf->config_lock);

	npf_config_destroy(npf->config);
	mutex_destroy(&npf->config_lock);
}

/*
 * npf_config_load: the main routine performing configuration load.
 * Performs the necessary synchronization and destroys the old config.
 */
void
npf_config_load(npf_t *npf, npf_config_t *nc, npf_conndb_t *conns, bool flush)
{
	const bool load = conns != NULL;
	npf_config_t *onc;

	nc->default_pass = flush;

	/*
	 * Acquire the lock and perform the first phase:
	 * - Scan and use existing dynamic tables, reload only static.
	 * - Scan and use matching NAT policies to preserve the connections.
	 */
	mutex_enter(&npf->config_lock);
	if ((onc = atomic_load_relaxed(&npf->config)) != NULL) {
		npf_ruleset_reload(npf, nc->ruleset, onc->ruleset, load);
		npf_tableset_reload(npf, nc->tableset, onc->tableset);
		npf_ruleset_reload(npf, nc->nat_ruleset, onc->nat_ruleset, load);
	}

	/*
	 * Set the new config and release the lock.
	 */
	atomic_store_release(&npf->config, nc);
	if (onc == NULL) {
		/* Initial load, done. */
		npf_ifmap_flush(npf);
		npf_conn_load(npf, conns, !flush);
		mutex_exit(&npf->config_lock);
		goto done;
	}

	/*
	 * If we are going to flush the connections or load the new ones,
	 * then disable the connection tracking for the grace period.
	 */
	if (flush || conns) {
		npf_conn_tracking(npf, false);
	}

	/* Synchronise: drain all references. */
	npf_ebr_full_sync(npf->ebr);
	if (flush) {
		npf_portmap_flush(npf->portmap);
		npf_ifmap_flush(npf);
	}

	/*
	 * G/C the existing connections and, if passed, load the new ones.
	 * If not flushing - enable the connection tracking.
	 */
	npf_conn_load(npf, conns, !flush);
	mutex_exit(&npf->config_lock);

	/* Finally, it is safe to destroy the old config. */
	npf_config_destroy(onc);
done:
	/* Sync all interface address tables (can be done asynchronously). */
	npf_ifaddr_syncall(npf);
}

/*
 * Writer-side exclusive locking.
 */

npf_config_t *
npf_config_enter(npf_t *npf)
{
	mutex_enter(&npf->config_lock);
	return npf->config;
}

void
npf_config_exit(npf_t *npf)
{
	mutex_exit(&npf->config_lock);
}

bool
npf_config_locked_p(npf_t *npf)
{
	return mutex_owned(&npf->config_lock);
}

void
npf_config_sync(npf_t *npf)
{
	KASSERT(npf_config_locked_p(npf));
	npf_ebr_full_sync(npf->ebr);
}

/*
 * Reader-side synchronization routines.
 */

int
npf_config_read_enter(npf_t *npf)
{
	/* Note: issues an acquire fence. */
	return npf_ebr_enter(npf->ebr);
}

void
npf_config_read_exit(npf_t *npf, int s)
{
	/* Note: issues a release fence. */
	npf_ebr_exit(npf->ebr, s);
}

/*
 * Accessors.
 */

npf_ruleset_t *
npf_config_ruleset(npf_t *npf)
{
	npf_config_t *config = atomic_load_consume(&npf->config);
	KASSERT(npf_config_locked_p(npf) || npf_ebr_incrit_p(npf->ebr));
	return config->ruleset;
}

npf_ruleset_t *
npf_config_natset(npf_t *npf)
{
	npf_config_t *config = atomic_load_consume(&npf->config);
	KASSERT(npf_config_locked_p(npf) || npf_ebr_incrit_p(npf->ebr));
	return config->nat_ruleset;
}

npf_tableset_t *
npf_config_tableset(npf_t *npf)
{
	npf_config_t *config = atomic_load_consume(&npf->config);
	KASSERT(npf_config_locked_p(npf) || npf_ebr_incrit_p(npf->ebr));
	return config->tableset;
}

bool
npf_default_pass(npf_t *npf)
{
	npf_config_t *config = atomic_load_consume(&npf->config);
	KASSERT(npf_config_locked_p(npf) || npf_ebr_incrit_p(npf->ebr));
	return config->default_pass;
}
